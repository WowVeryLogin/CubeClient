#include <stdlib.h>
#include <stdio.h>
#include "protocol.h"
#include "net.h"

#define error_check(a) \
	if (a) { \
		exit(2); \
	}

typedef struct
{
	RequestParams req;
	ConnectionParams conn;
} Args;

int parse_args(int num_args, char* args[], Args* req_params)
{
	int i = 0;

	if (num_args != 5) {
		printf("Not enough arguments\nUsage: cube hostname port token scope\n");
		return 1;
	}

	req_params->conn.host = args[1];
	req_params->conn.port = strtoul(args[2], NULL, 0);
	req_params->req.token = args[3];
	req_params->req.scope = args[4];

	return 0;
}

int main(int argc, char* argv[])
{
	Args req_args;
	error_check(parse_args(argc, argv, &req_args));

	int socket;
	error_check(connect_to_server(&req_args.conn, &socket));

	Response resp;
	error_check(make_request(socket, &req_args.req, &resp));
	close_connection(socket);
	char resp_str[512];
	resp_to_string(&resp, resp_str);
	printf("%s\n", resp_str);

	free_response(&resp);

	return 0;
}