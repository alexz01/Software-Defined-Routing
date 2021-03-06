#include <string.h>
#include <stdlib.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"

void crash_response(int sock_index)
{
	printf("inside crash response");
	uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

	//payload_len = sizeof(AUTHOR_STATEMENT)-1; // Discount the NULL chararcter
	//cntrl_response_payload = (char *) malloc(payload_len);
	//memcpy(cntrl_response_payload, AUTHOR_STATEMENT, payload_len);

	cntrl_response_header = create_response_header(sock_index, 4, 0, 0);

	response_len = CNTRL_RESP_HEADER_SIZE;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	//free(cntrl_response_header);
	/* Copy Payload */
	//memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	//free(cntrl_response_payload);

	sendALL(sock_index, cntrl_response, response_len);

	//free(cntrl_response);
	exit(EXIT_SUCCESS);
}