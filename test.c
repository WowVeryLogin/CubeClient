#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_TESTS 6

typedef struct
{
	char* test_name;
	char* input;
	char* fake_server_params;
	char* expected_result;
} Test;

static
const Test tests[NUM_TESTS] = {
	{
		"OkResponse",
		"localhost 1234 abracadabra test",
		"1234 send_good",
		"client_id: test_client_id\nclient_type: 2002\nexpires_in: 3600\nuser_id: 101010\nusername: testuser@mail.ru\n"
	},
	{
		"ErrorResponse",
		"localhost 1234 abracadabra xxx",
		"1234 send_bad",
		"error: CUBE_OAUTH2_ERR_BAD_SCOPE\nmessage: bad scope\n"
	},
	{
		"Server stop answering",
		"localhost 1234 abracadabra test",
		"1234 die",
		"Unable to read response header\n"
	},
	{
		"Not enough arguments",
		"localhost 1234 abracadabra",
		NULL,
		"Not enough arguments\nUsage: cube hostname port token scope\n"
	},
	{
		"Unknown host",
		"unknownhost 1234 abracadabra test",
		NULL,
		"No such host\n"
	},
	{
		"Server unavailable",
		"localhost 1234 abracadabra test",
		NULL,
		"Connection error\n"
	}
};

int main() {
	FILE* fp;
	FILE* sfp;
	char buf[1024];
	int i;

	for (int i = 0; i < NUM_TESTS; ++i) {
		char start_buf[100];

		if (tests[i].fake_server_params) {
			sprintf(start_buf, "./fake_server %s", tests[i].fake_server_params);
			sfp = popen(start_buf, "w");
			if (!sfp) {
				printf("Failed to run tests\n");
				exit(1);
			}
			//To be sure that server have started and ready to accept connections
			sleep(1);
		}

		sprintf(start_buf, "./cube %s", tests[i].input);
		fp = popen(start_buf, "r");
		if (!fp) {
			printf("Failed to run tests\n");
			exit(1);
		}

		char* pos = buf;
		int readed;
		while (fgets(pos, sizeof(buf)-1, fp)) {
			pos += strlen(pos);
		}

		if (!strcmp(buf, tests[i].expected_result)) {
			printf("OK - TEST: %s - SUCCEEDED\n", tests[i].test_name);
		} else {
			printf("ERROR - TEST: %s - failed\n", tests[i].test_name);
			printf("\tExpected: %s\n", tests[i].expected_result);
			printf("\tGot: %s\n", buf);
		}

		pclose(fp);
		if (tests[i].fake_server_params) {
			pclose(sfp);
		}
	}

	return 0;
}