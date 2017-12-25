#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "net.h"

int write_with_timeout(int fd, char* buf, int size)
{
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = REQUEST_TIMEOUT_USEC;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(fd, &set);

	int rc = select(fd + 1, NULL, &set, NULL, &timeout);
	if (rc > 0) {
		return write(fd, buf, size);
	} else {
		return -1;
	}
}

int read_with_timeout(int fd, char* buf, int size)
{
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = REQUEST_TIMEOUT_USEC;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(fd, &set);

	int rc = select(fd + 1, &set, NULL, NULL, &timeout);
	if (rc > 0) {
		return read(fd, buf, size);
	} else {
		return -1;
	}
}

int send_request(int socket, RequestParams* params)
{
	Header header;
	header.svc_id = SVC_ID;
	header.request_id = 0;
	header.body_length = strlen(params->token) + 1 + strlen(params->scope) + 1 + sizeof(params->svc_msg);

	Buffer header_ser = header_serialize(&header);
	Buffer req_ser = request_serialize(params);
	Buffer msg = concat_buffers(&header_ser, &req_ser);

	int res = write_with_timeout(socket, msg.data, msg.len);
	free(msg.data);
	if (res != msg.len) {
		printf("Unable to send request\n");
		return 1;
	}
	return 0;
}

int receive_response(int socket, Response* response)
{
	char header_buf[HEADER_SIZE];
	int bytes_readed = 0;
	while (bytes_readed < HEADER_SIZE) {
		int rec = read_with_timeout(socket, header_buf + bytes_readed, HEADER_SIZE - bytes_readed);
		if ((rec < 0) || (!rec && bytes_readed < HEADER_SIZE)) {
			printf("Unable to read response header\n");
			return 1;
		}
		bytes_readed += rec;
	}

	Header header;
	header_deserialize(&header, header_buf);

	char* buf = malloc(header.body_length);
	bytes_readed = 0;
	while(bytes_readed < header.body_length) {
		int rec = read_with_timeout(socket, buf + bytes_readed, header.body_length - bytes_readed);
		if ((rec < 0) || (!rec && bytes_readed < header.body_length)) {
			free(buf);
			printf("Unable to read response body\n");
			return 1;
		}
		bytes_readed += rec;
	}

	int rc = response_deserialize(response, buf, header.body_length);
	free(buf);
	return rc;
}

int make_request(int socket, RequestParams* const params, Response* response)
{
	if (send_request(socket, params)) {
		return 1;
	}
	if (receive_response(socket, response)) {
		return 1;
	}
	return 0;
}

int connect_to_server(const ConnectionParams* const con_params, int* sockfd)
{
	int n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char* hostname = con_params->host;
	uint16_t portno = con_params->port;
	*sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (*sockfd < 0) {
		printf("Couldn't open socket\n");
		return 1;
	}
		
	server = gethostbyname(hostname);

	if (!server) {
		printf("No such host\n");
		return 1;
	}

	memset((char *) &serv_addr, sizeof(serv_addr), 0);
	serv_addr.sin_family = AF_INET;
	memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	/* Connect to the server */
	if (connect(*sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("Connection error\n");
		return 1;
	}

	return 0;
}

void close_connection(int socket)
{
	close(socket);
}