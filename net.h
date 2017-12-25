#ifndef NET_H
#define NET_H

#include "protocol.h"

#define REQUEST_TIMEOUT_USEC 10000

typedef struct
{
	char* host;
	uint16_t port;
} ConnectionParams;

int connect_to_server(const ConnectionParams* const con_params, int* sockfd);
int make_request(int socket, RequestParams* const params, Response* response);
void close_connection(int socket);
#endif