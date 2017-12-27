#include <string.h>
#include <arpa/inet.h>
#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"

#define COST_OFFSET 2

void update_response(int sock_index,char *cntrl_payload, uint16_t cntrl_payload_len)
{
	uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;
	uint16_t router_id, router_cost;
	//payload_len = sizeof(AUTHOR_STATEMENT)-1; // Discount the NULL chararcter
	//cntrl_response_payload = (char *) malloc(sizeof(router_id)+sizeof(router_cost));
	
	memcpy(&router_id, cntrl_payload , sizeof(router_id));
	memcpy(&router_cost, cntrl_payload + COST_OFFSET , sizeof(router_cost));
	
	for(uint16_t router_list_index = 0; router_list_index < num_router; router_list_index++)
		if(router_list[router_list_index].id == router_id)
			router_list[router_list_index].cost = router_cost;	
	
	//memcpy(cntrl_response_payload, AUTHOR_STATEMENT, payload_len);

	cntrl_response_header = create_response_header(sock_index, 3, 0, 0);

	response_len = CNTRL_RESP_HEADER_SIZE;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	/* Copy Payload */
	//memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	//free(cntrl_response_payload);
	printf("new routing table : \n");
	for(int i = 0; i < num_router; i++)
	{
		printf("%d : %d : %d\n",ntohs(router_list[i].id), ntohs(router_list[i].cost), ntohs(router_list[i].next_hop));
	}
	sendALL(sock_index, cntrl_response, response_len);

	free(cntrl_response);
}
