#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/data_handler.h"


#define DESTINATION_ROUTER_IP_OFFSET 0
#define INIT_TTL_OFFSET 4
#define TRANSFER_ID_OFFSET 5
#define INIT_SEQ_NUMBER_OFFSET 6
#define FILENAME_OFFSET 8

void sendfile_response(int sock_index,char *cntrl_payload, uint16_t cntrl_payload_len)
{
	uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;
	uint32_t dest_ip;
	uint8_t init_ttl, transfer_id; 
	uint16_t init_seq_number, filename_len;
	printf("[sendfile_response] control payload len: %d\n",cntrl_payload_len);
	filename_len = cntrl_payload_len - 8; 
	printf("[sendfile_response] filename len: %d\n",filename_len);
	char* filename = malloc(filename_len + 1);
	memset(filename, '\0',filename_len+1);
	memcpy(&dest_ip, cntrl_payload + DESTINATION_ROUTER_IP_OFFSET, sizeof(dest_ip));
	memcpy(&init_ttl, cntrl_payload + INIT_TTL_OFFSET, sizeof(init_ttl));
	memcpy(&transfer_id, cntrl_payload + TRANSFER_ID_OFFSET, sizeof(transfer_id));
	memcpy(&init_seq_number, cntrl_payload + INIT_SEQ_NUMBER_OFFSET, sizeof(init_seq_number));
	memcpy(filename, cntrl_payload + FILENAME_OFFSET, filename_len);
	//filename[filename_len] = '\0';
	printf("[sendfile_response] init_ttl %d, transfer_id : %d, init_seq_number : %d\n ",init_ttl, transfer_id, ntohs(init_seq_number));
	printf("[sendfile_response] filename : %s\n", filename);
	//printf("[sendfile_response] cntrl_payload : %x\n", cntrl_payload);
	
	int their_socket = new_outgoing_data_conn(dest_ip);
	send_file(their_socket, dest_ip, init_ttl, transfer_id, init_seq_number, filename);
	close(their_socket);
	
	cntrl_response_header = create_response_header(sock_index, 6, 0, 0);
	response_len = CNTRL_RESP_HEADER_SIZE;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	/* Copy Payload */
	//no payload
	sendALL(sock_index, cntrl_response, response_len);
	free(cntrl_response);
}
