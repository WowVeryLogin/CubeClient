#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdint.h>

#define SVC_ID 0x00000002
#define SVC_MSG 0x00000001

typedef struct
{
	char* data;
	size_t len;
} Buffer;

//use only precise types for fields of this struct
//because of using sizeof for protocol impl
typedef struct
{
	int32_t svc_msg;
	char* token;
	char* scope;
} RequestParams;

typedef enum
{
	CUBE_OAUTH2_ERR_OK,
	CUBE_OAUTH2_ERR_TOKEN_NOT_FOUND,
	CUBE_OAUTH2_ERR_DB_ERROR,
	CUBE_OAUTH2_ERR_UNKNOWN_MSG,
	CUBE_OAUTH2_ERR_BAD_PACKET,
	CUBE_OAUTH2_ERR_BAD_CLIENT,
	CUBE_OAUTH2_ERR_BAD_SCOPE
} ReturnCode;

//ReturnCode is enum so should use fixed size
#define RETURN_CODE_SIZE sizeof(int32_t)

static
const char* const ResponseErrorCodes[] = {
	"CUBE_OAUTH2_ERR_OK",
	"CUBE_OAUTH2_ERR_TOKEN_NOT_FOUND",
	"CUBE_OAUTH2_ERR_DB_ERROR",
	"CUBE_OAUTH2_ERR_UNKNOWN_MSG",
	"CUBE_OAUTH2_ERR_BAD_PACKET",
	"CUBE_OAUTH2_ERR_BAD_CLIENT",
	"CUBE_OAUTH2_ERR_BAD_SCOPE"
};

//use only precise types for fields of this struct
//because of using sizeof for protocol impl
typedef struct
{
	int32_t svc_id;
	int32_t body_length;
	int32_t request_id;
} Header;

#define FAKE_HEADER ((Header*)0)
#define HEADER_SIZE (sizeof(FAKE_HEADER->svc_id) + sizeof(FAKE_HEADER->body_length) + sizeof(FAKE_HEADER->request_id))

typedef struct
{
	void* data;
	ReturnCode return_code;
} Response;

//use only precise types for fields of this struct
//because of using sizeof for protocol impl
typedef struct
{
	char* client_id;
	int32_t client_type;
	char* username;
	int32_t expires_in;
	int64_t user_id;
} OkResponseBody;

//use only precise types for fields of this struct
//because of using sizeof for protocol impl
typedef struct
{
	char* error_string;
} ErrorResponseBody;

void free_response(Response* resp);

Buffer concat_buffers(Buffer* buf_1, Buffer* buf_2);

Buffer header_serialize(const Header* const header);
void header_deserialize(Header* header, char* buf);

Buffer request_serialize(RequestParams* const params);
int request_deserialize(RequestParams* params, char* buf, const int expected_size);

Buffer response_serialize(const Response* const response);
int response_deserialize(Response* response, char* buf, const int expected_size);

void resp_to_string(const Response* const response, char* buf);
#endif