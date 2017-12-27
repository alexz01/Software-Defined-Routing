#include <string.h>
#include <arpa/inet.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"

#define TRANSFER_ID_OFFSET 0
#define TTL_OFFSET 1
#define PADDING_OFFSET 2
#define SEQ_NUMBER_OFFSET 2

void sendfile_stats_response(int sock_index, char* cntrl_payload, uint16_t cntrl_payload_len)
{
	uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;
	
	uint8_t transfer_id, ttl;
	memcpy(&transfer_id, cntrl_payload, cntrl_payload_len);
	uint16_t init_seq_number = UINT16_MAX;
	int packet_count = 0;
	struct sendfile_stat *stats;
	LIST_FOREACH(stats, &sendfile_stat_list, sendfile_next)
	{
		if(transfer_id == stats->transfer_id)
		{	
			packet_count++;
			uint16_t current_seq_number = ntohs(stats->seq_number);
			//assuming that all packets come to this router
			if(init_seq_number > current_seq_number)
			{
				init_seq_number = current_seq_number;
				ttl = stats->ttl;
			}		
		}
	}
		
	payload_len = packet_count*4;//1B for tx_id, 1B for ttl, 2B for seq_num
	cntrl_response_payload = (char *) malloc(payload_len);
	uint16_t padding = 0x0000;
	memcpy(cntrl_response_payload + TRANSFER_ID_OFFSET, &transfer_id, sizeof(transfer_id));
	memcpy(cntrl_response_payload + TTL_OFFSET, &ttl, sizeof(ttl));
	memcpy(cntrl_response_payload + PADDING_OFFSET, &padding, sizeof(padding));
	
	//if order is necessary
	int offset = 4;
	for(int i = init_seq_number; i < init_seq_number + packet_count; i++)
	{
		uint16_t seq = htons(i);
		memcpy(cntrl_response_payload + offset, &seq, sizeof(seq));
		offset += SEQ_NUMBER_OFFSET;
	}
	//if order is unncesessary
	//int offset = 4;
	//LIST_FOREACH(stats, &sendfile_stat_list, sendfile_next)
	//{
	//	if(transfer_id == stats->transfer_id)
	//	{
	//		memcpy(cntrl_response_payload + offset, &stats->seq_number, sizeof(seq));
	//		offset += SEQ_NUMBER_OFFSET;
	//	}
	//}
	cntrl_response_header = create_response_header(sock_index, 6, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	/* Copy Payload */
	memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	free(cntrl_response_payload);

	sendALL(sock_index, cntrl_response, response_len);

	free(cntrl_response);
}