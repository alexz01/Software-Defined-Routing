#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"

#define NEXT_OFFSET 8
#define ID_OFFSET 0
#define PADDING_OFFSET 2
#define NEXT_HOP_OFFSET 4
#define COST_OFFSET 6

void routing_table_response(int sock_index)
{
	time_t time_v;
	time(&time_v);
	char *str = malloc(sizeof(char)*1000);
	sprintf(str,"Sending at %s", asctime(localtime(&time_v)));
	sprintf(str,"%s%d's List start at: %x \n",str, ntohs(router_list[this_router_index].id), router_list);
	uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

	payload_len = (sizeof(uint16_t)*4)*num_router;
	printf("payload len: %d\n", payload_len);
	sprintf(str, "%spayload len: %d\n",str, payload_len);
	cntrl_response_payload = (char *) malloc(payload_len);
	uint16_t offset = 0, router_list_index, id;
	uint16_t padding = 0x0000;
	uint16_t next_hop = 0xffff, cost = 0xffff;
	for(router_list_index = 0; router_list_index < num_router; router_list_index++)
	{
		id = router_list[router_list_index].id;
		memcpy(cntrl_response_payload + offset + ID_OFFSET, &id, sizeof(uint16_t));
		memcpy(cntrl_response_payload + offset + PADDING_OFFSET, &padding, sizeof(uint16_t));
		next_hop = router_list[router_list_index].next_hop;
		memcpy(cntrl_response_payload + offset + NEXT_HOP_OFFSET, &next_hop, sizeof(uint16_t));
		cost = router_list[router_list_index].cost;
		memcpy(cntrl_response_payload + offset + COST_OFFSET, &cost, sizeof(uint16_t));
		sprintf(str,"%s%d, %x %x %x; \n",str, ntohs(id), padding, &router_list[router_list_index].next_hop, &router_list[router_list_index].cost);
		offset += NEXT_OFFSET;
	}
	
	
	cntrl_response_header = create_response_header(sock_index, 2, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	/* Copy Payload */
	memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	free(cntrl_response_payload);
	printf("sending: ");
	sprintf(str, "%ssending: ",str);
	for(int i = 0; i < payload_len+CNTRL_RESP_HEADER_SIZE; i++){
		if(i%16==0)
		{	sprintf(str, "%s  ",str);
			printf("  "); 
		}
		if(i%8==0)
		{	sprintf(str, "%s\n",str);
			printf("\n"); 
		}
		sprintf(str, "%s%02x ", str, cntrl_response[i]);
		printf("%02x ", cntrl_response[i]);
		
	}
	sprintf(str, "%s\n",str);
	printf("\n");
	int sentb = sendALL(sock_index, cntrl_response, response_len);
	printf("bytes sent: %d",sentb);
	
	FILE *fp;
	char *filepath = malloc(sizeof(char)*100);
sprintf(filepath,"%s%d","/home/csgrad/aumale/MNC/pa3/rt_resp_output",ntohs(router_list[this_router_index].id));
	fp = fopen(filepath,"a");
	fprintf(fp,"%s\n",str);
	fflush(fp);

	free(cntrl_response);
}