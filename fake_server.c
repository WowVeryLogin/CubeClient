#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "protocol.h"

void send_error_response(int comm_fd)
{
	ErrorResponseBody errorResponse;
	errorResponse.error_string = "bad scope";
	Response resp;
	resp.return_code = CUBE_OAUTH2_ERR_BAD_SCOPE;
	resp.data = &errorResponse;
	Buffer response_serialized = response_serialize(&resp);

	Header resp_header;
	resp_header.svc_id = SVC_ID;
	resp_header.request_id = 1;
	resp_header.body_length = strlen(errorResponse.error_string) + 1 + sizeof(resp.return_code);
	Buffer header_serialized = header_serialize(&resp_header);
	Buffer msg = concat_buffers(&header_serialized, &response_serialized);

	if (write(comm_fd, msg.data, msg.len) != msg.len) {
		printf("ERROR: Unable to send response\n");
	}

	free(msg.data);
}

void send_ok_response(int comm_fd)
{
	OkResponseBody okResponse;
	okResponse.client_id = "test_client_id";
	okResponse.client_type = 2002;
	okResponse.expires_in = 3600;
	okResponse.user_id = 101010;
	okResponse.username = "testuser@mail.ru";
	Response resp;
	resp.return_code = CUBE_OAUTH2_ERR_OK;
	resp.data = &okResponse;
	Buffer response_serialized = response_serialize(&resp);

	Header resp_header;
	resp_header.svc_id = SVC_ID;
	resp_header.request_id = 1;
	resp_header.body_length = strlen(okResponse.username) + 1 + strlen(okResponse.client_id) + 1 + sizeof(okResponse.client_type) +
		sizeof(okResponse.expires_in) + sizeof(okResponse.user_id) + sizeof(resp.return_code);
	Buffer header_serialized = header_serialize(&resp_header);
	Buffer msg = concat_buffers(&header_serialized, &response_serialized);

	if (write(comm_fd, msg.data, msg.len) != msg.len) {
		printf("ERROR: Unable to send response\n");
	}

	free(msg.data);
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		printf("Not enough arguments\n");
		exit(1);
	}

	char buf[512];
	int listen_fd, comm_fd;
	unsigned int port = strtoul(argv[1], NULL, 0);

	struct sockaddr_in servaddr;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	int enable = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	bzero( &servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if (bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr))) {
		printf("Error: Unable to bind port\n");
	}

	if (listen(listen_fd, 10)) {
		printf("Error: Unable to lister on port\n");
	}

	comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);

	int bytes_readed = 0;
	while (bytes_readed < HEADER_SIZE) {
		int rec = read(comm_fd, buf + bytes_readed, HEADER_SIZE - bytes_readed);
		if (rec < 0) {
			perror("Unable to read request header");
			return 1;
		}
		bytes_readed += rec;
	}

	Header header;
	header_deserialize(&header, buf);
	bytes_readed = 0;
	while (bytes_readed < header.body_length) {
		int rec = read(comm_fd, buf + bytes_readed, header.body_length - bytes_readed);
		if (rec < 0) {
			perror("Unable to read request body");
			return 1;
		}
		bytes_readed += rec;
	}

	RequestParams request;

	if (request_deserialize(&request, buf, header.body_length)) {
		printf("Couldn't parse request\n");
	} else {
		if (!strcmp(argv[2], "send_good")) {
			send_ok_response(comm_fd);
		} else if (!strcmp(argv[2], "send_bad")) {
			send_error_response(comm_fd);
		}
		free(request.scope);
		free(request.token);
	}

	close(comm_fd);
	close(listen_fd);
	return 0;
}