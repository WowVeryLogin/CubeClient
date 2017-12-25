#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "protocol.h"

//Previous buffers will be invalid
Buffer concat_buffers(Buffer* buf_1, Buffer* buf_2)
{
	char* new_buf = malloc(buf_1->len + buf_2->len);
	memcpy(new_buf, buf_1->data, buf_1->len);
	memcpy(new_buf + buf_1->len, buf_2->data, buf_2->len);
	free(buf_1->data);
	free(buf_2->data);

	Buffer res = {new_buf, buf_1->len + buf_2->len};
	return res;
}

void resp_to_string(const Response* const response, char* buf)
{
	if (response->return_code == CUBE_OAUTH2_ERR_OK) {
		OkResponseBody* resp = (OkResponseBody*) response->data;
		sprintf(buf, "client_id: %s\nclient_type: %d\nexpires_in: %d\nuser_id: %d\nusername: %s",
			resp->client_id,
			resp->client_type,
			resp->expires_in,
			resp->user_id,
			resp->username);
	} else {
		ErrorResponseBody* resp = (ErrorResponseBody*) response->data;
		sprintf(buf, "error: %s\nmessage: %s", ResponseErrorCodes[response->return_code], resp->error_string);
	}
}

void free_response(Response* resp)
{
	if (resp->return_code == CUBE_OAUTH2_ERR_OK) {
		free(((OkResponseBody*)resp->data)->username);
		free(((OkResponseBody*)resp->data)->client_id);
		free(resp->data);
	} else {
		free(((ErrorResponseBody*)resp->data)->error_string);
		free(resp->data);
	}
}

Buffer header_serialize(const Header* const header)
{
	char* buf = malloc(HEADER_SIZE);
	char* pos = buf;
	memcpy(pos, &header->svc_id, sizeof(header->svc_id));
	pos += sizeof(header->svc_id);
	memcpy(pos, &header->body_length, sizeof(header->body_length));
	pos += sizeof(header->body_length);
	memcpy(pos, &header->request_id, sizeof(header->request_id));
	pos += sizeof(header->request_id);

	Buffer res = {buf, pos - buf};
	return res;
}

void header_deserialize(Header* header, char* buf)
{
	char* pos = buf;
	memcpy(&header->svc_id, pos, sizeof(header->svc_id));
	pos += sizeof(header->svc_id);
	memcpy(&header->body_length, pos, sizeof(header->body_length));
	pos += sizeof(header->body_length);
	memcpy(&header->request_id, pos, sizeof(header->request_id));
}

//Should manage returned char* buffer yourself
Buffer request_serialize(RequestParams* const params)
{
	params->svc_msg = SVC_MSG;
	int token_len = strlen(params->token) + 1, scope_len = strlen(params->scope) + 1;

	char* buf = malloc(sizeof(params->svc_msg) + token_len + scope_len);
	char* pos = buf;
	memcpy(pos, &params->svc_msg, sizeof(params->svc_msg));
	pos += sizeof(params->svc_msg);
	memcpy(pos, params->token, token_len);
	pos += token_len;
	memcpy(pos, params->scope, scope_len);
	pos += scope_len;

	Buffer res = {buf, pos - buf};
	return res;
}

int request_deserialize(RequestParams* params, char* buf, const int expected_size)
{
	char* pos = buf;
	RequestParams resp;

	memcpy(&params->svc_msg, pos, sizeof(params->svc_msg));
	pos += sizeof(params->svc_msg);

	int max_token_len = expected_size - sizeof(params->svc_msg);
	int token_len = strnlen(pos, max_token_len) + 1;
	params->token = malloc(token_len);
	memcpy(params->token, pos, token_len);
	pos += token_len;

	int max_scope_len = max_token_len - token_len;
	int scope_len = strnlen(pos, max_scope_len) + 1;
	params->scope = malloc(scope_len);
	memcpy(params->scope, pos, scope_len);

	return sizeof(params->svc_msg) + token_len + scope_len != expected_size;
}

Buffer response_serialize(const Response* const response)
{
	Buffer res;

	if (response->return_code == CUBE_OAUTH2_ERR_OK) {
		OkResponseBody* ok_resp = (OkResponseBody*) response->data;
		int client_id_len = strlen(ok_resp->client_id) + 1;
		int username_len = strlen(ok_resp->username) + 1;
		char* buf = malloc(
			RETURN_CODE_SIZE +
			client_id_len +
			username_len +
			sizeof(ok_resp->client_type) +
			sizeof(ok_resp->expires_in) +
			sizeof(ok_resp->user_id)
		);
		char* pos = buf;

		memcpy(pos, &response->return_code, RETURN_CODE_SIZE);
		pos += RETURN_CODE_SIZE;

		memcpy(pos, ok_resp->client_id, client_id_len);
		pos += client_id_len;

		memcpy(pos, &ok_resp->client_type, sizeof(ok_resp->client_type));
		pos += sizeof(ok_resp->client_type);

		memcpy(pos, ok_resp->username, username_len);
		pos += username_len;

		memcpy(pos, &ok_resp->expires_in, sizeof(ok_resp->expires_in));
		pos += sizeof(ok_resp->expires_in);

		memcpy(pos, &ok_resp->user_id, sizeof(ok_resp->user_id));
		pos += sizeof(ok_resp->user_id);

		res.data = buf;
		res.len = pos - buf;
	} else {
		ErrorResponseBody* error_resp = (ErrorResponseBody*) response->data;
		int error_str_len = strlen(error_resp->error_string) + 1;
		char* buf = malloc(RETURN_CODE_SIZE + error_str_len);
		char* pos = buf;

		memcpy(pos, &response->return_code, RETURN_CODE_SIZE);
		pos += RETURN_CODE_SIZE;

		memcpy(pos, error_resp->error_string, error_str_len);
		pos += error_str_len;

		res.data = buf;
		res.len = pos - buf;
	}

	return res;
}

int response_deserialize(Response* response, char* buf, const int expected_size)
{
	char* pos = buf;

	ReturnCode return_code;
	memcpy(&return_code, buf, RETURN_CODE_SIZE);
	pos += RETURN_CODE_SIZE;
	response->return_code = return_code;

	if (return_code == CUBE_OAUTH2_ERR_OK) {
		OkResponseBody resp;

		int max_client_id_len = 
			expected_size - sizeof(resp.client_type) - 
			sizeof(resp.user_id) - sizeof(resp.expires_in) - RETURN_CODE_SIZE;

		int client_id_len = strnlen(pos, max_client_id_len) + 1;
		resp.client_id = malloc(client_id_len);
		memcpy(resp.client_id, pos, client_id_len);
		pos += client_id_len;

		memcpy(&resp.client_type, pos, sizeof(resp.client_type));
		pos += sizeof(resp.client_type);

		int max_username_len = max_client_id_len - client_id_len;
		int username_len = strnlen(pos, max_username_len) + 1;
		resp.username = malloc(username_len);
		memcpy(resp.username, pos, username_len);
		pos += username_len;

		memcpy(&resp.expires_in, pos, sizeof(resp.expires_in));
		pos += sizeof(resp.expires_in);

		memcpy(&resp.user_id, pos, sizeof(resp.user_id));

		response->data = malloc(sizeof(OkResponseBody));
		memcpy(response->data, &resp, sizeof(OkResponseBody));

		int parsed_size = client_id_len + username_len + sizeof(resp.client_type) + 
			sizeof(resp.user_id) + sizeof(resp.expires_in) + RETURN_CODE_SIZE;

		return parsed_size != expected_size;

	} else {
		ErrorResponseBody resp;

		int max_error_str_len = expected_size - RETURN_CODE_SIZE;
		int error_str_len = strnlen(pos, max_error_str_len) + 1;
		resp.error_string = malloc(error_str_len);
		memcpy(resp.error_string, pos, error_str_len);
		response->data = malloc(sizeof(ErrorResponseBody));
		memcpy(response->data, &resp, sizeof(ErrorResponseBody));

		return error_str_len + RETURN_CODE_SIZE != expected_size;
	}
}