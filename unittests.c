#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

int RequestCmp(RequestParams* req_1, RequestParams* req_2)
{
	return strcmp(req_1->scope, req_2->scope) | 
		strcmp(req_1->token, req_2->token) | 
		(req_1->svc_msg ^ req_2->svc_msg);
}

int ResponseCmp(Response* resp_1, Response* resp_2)
{
	if (resp_1->return_code != resp_2->return_code) {
		return 1;
	}

	if (resp_1->return_code == CUBE_OAUTH2_ERR_OK) {
		OkResponseBody* ok_resp_1 = (OkResponseBody*) resp_1->data;
		OkResponseBody* ok_resp_2 = (OkResponseBody*) resp_2->data;

		return strcmp(ok_resp_1->client_id, ok_resp_2->client_id) | 
			strcmp(ok_resp_1->username, ok_resp_2->username) | 
			(ok_resp_1->client_type ^ ok_resp_2->client_type) |
			(ok_resp_1->expires_in ^ ok_resp_2->expires_in) |
			(ok_resp_1->user_id ^ ok_resp_2->user_id);
	} else {
		ErrorResponseBody* error_resp_1 = (ErrorResponseBody*) resp_1->data;
		ErrorResponseBody* error_resp_2 = (ErrorResponseBody*) resp_2->data;
		return strcmp(error_resp_1->error_string, error_resp_2->error_string);
	}
}

void HeaderSerializeDeserializeInv()
{
	Header header;
	header.body_length = 30;
	header.request_id = 2;
	header.svc_id = SVC_ID;

	Buffer ser = header_serialize(&header);
	Header res;
	header_deserialize(&res, ser.data);

	if (memcmp(&res, &header, sizeof(Header))) {
		printf("ERROR - Header test failed\n");
	} else {
		printf("OK - Header test passed\n");
	}

	free(ser.data);
}

void RequestSerializeDeserializeInv()
{
	RequestParams req;
	req.scope = "TestingScope";
	req.token = "TestingToken";
	Buffer ser = request_serialize(&req);

	int request_size = strlen(req.scope) + strlen(req.token) + sizeof(req.svc_msg);
	RequestParams res;
	request_deserialize(&res, ser.data, request_size);

	if (RequestCmp(&req, &res)) {
		printf("ERROR - Request test failed\n");
	} else {
		printf("OK - Request test passed\n");
	}

	free(ser.data);
	free(res.scope);
	free(res.token);
}

void OkResponseSerializeDeserializeInv()
{
	OkResponseBody ok_resp;
	ok_resp.client_id = "TestClientId";
	ok_resp.client_type = 22;
	ok_resp.expires_in = 33;
	ok_resp.user_id = 66;
	ok_resp.username = "TestClientUserName";

	Response resp;
	resp.return_code = CUBE_OAUTH2_ERR_OK;
	resp.data = (void *) &ok_resp;

	Buffer ser = response_serialize(&resp);

	int response_size = strlen(ok_resp.client_id) + 1 + strlen(ok_resp.username) + 1 +
		sizeof(ok_resp.client_type) + sizeof(ok_resp.expires_in) + sizeof(ok_resp.user_id) +
		RETURN_CODE_SIZE;
	Response res;
	response_deserialize(&res, ser.data, response_size);

	if (ResponseCmp(&resp, &res)) {
		printf("ERROR - OkResponse test failed\n");
	} else {
		printf("OK - OkResponse test passed\n");
	}

	free(ser.data);
	free_response(&res);
}

void ErrorResponseSerializeDeserializeInv()
{
	ErrorResponseBody error_resp;
	error_resp.error_string = "bad packet";

	Response resp;
	resp.return_code = CUBE_OAUTH2_ERR_BAD_PACKET;
	resp.data = (void *) &error_resp;

	Buffer ser = response_serialize(&resp);

	int response_size = strlen(error_resp.error_string) + 1 + RETURN_CODE_SIZE;
	Response res;
	response_deserialize(&res, ser.data, response_size);

	if (ResponseCmp(&resp, &res)) {
		printf("ERROR - ErrorResponse test failed\n");
	} else {
		printf("OK - ErrorResponse test passed\n");
	}

	free(ser.data);
	free_response(&res);
}

int main() {
	HeaderSerializeDeserializeInv();
	RequestSerializeDeserializeInv();
	OkResponseSerializeDeserializeInv();
	ErrorResponseSerializeDeserializeInv();
	return 0;
}