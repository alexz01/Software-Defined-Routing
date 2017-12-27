/**
 * @author
 * @author  Alexander Umale <aumale@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * INIT [Control Code: 0x01]
 */

#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/init.h"

//externs in global.h
struct router *router_list;
int router_socket, data_socket;
uint16_t num_router, update_interval;

uint16_t this_router_index;
bool isInit;


void init_response(int sock_index,char *cntrl_payload, uint16_t cntrl_payload_len)
{
	isInit = TRUE;
	/* TODO ALL*/
	uint16_t payload_len, response_len;
	
	uint16_t router_offset;
	//cntrl_payload_len = strlen(cntrl_payload);
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;
	
	printf("%s\n",&cntrl_payload);
	
	memcpy(&num_router, cntrl_payload + router_offset, sizeof(num_router));
	num_router = ntohs(num_router);
	router_offset += sizeof(num_router);
	memcpy(&update_interval,cntrl_payload + router_offset, sizeof(update_interval));
	update_interval = ntohs(update_interval);
	router_offset += sizeof(update_interval);
	
	printf("%d, %d, %d	\n",num_router, update_interval, cntrl_payload_len);
	
	// initialize router list
	init_router_list(num_router, cntrl_payload, cntrl_payload_len);
	//inialize router distance vector
	//init_router_table(num_router, cntrl_payload, cntrl_payload_len);	
	
	//TODO:
	//start listening on router and data port
	//1> create router connection UDP ()
	router_socket = create_router_sock();
	FD_SET(router_socket, &master_list);
	if(head_fd < router_socket)
		head_fd = router_socket;
	//2> create data connection TCP 
	data_socket = create_data_sock();
	FD_SET(data_socket, &master_list);
	if(head_fd < data_socket)
		head_fd = data_socket;
	
	//3> update timer  --- TO BE DONE
	SELECT_TIMEOUT.tv_sec = update_interval;
	SELECT_TIMEOUT.tv_usec = 0;
	
	//4> send response to controller with routing table
	payload_len = 0; // Discount the NULL chararcter
	//cntrl_response_payload = (char *) malloc(payload_len);
	//memcpy(cntrl_response_payload, AUTHOR_STATEMENT, payload_len);

	LIST_INIT(&sendfile_stat_list);
	
	cntrl_response_header = create_response_header(sock_index, 1, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE + payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	/* Copy Payload */
	//memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	//free(cntrl_response_payload);

	sendALL(sock_index, cntrl_response, response_len);
	
	//free(cntrl_response); throwing error for some reason
	
}


void init_router_list(int size, char* cntrl_payload, uint16_t cntrl_payload_len)
{
	printf("size %d %d\n",size, sizeof(struct router)*size);
	router_list = (struct router*) malloc(sizeof(struct router)*size);
	//TODO : ADD THE LIST TABLE
	uint16_t router_offset;
	uint16_t id, router_port, data_port, cost;
	uint32_t ip;
	char *str = malloc(sizeof(char)*1000);
	int router_list_index = 0;
	for(router_offset = ROUTER_OFFSET; router_offset < cntrl_payload_len; router_offset += ROUTER_OFFSET_INCREMENT)
	{
		//printf("inside loop\n");
		memcpy(&id, cntrl_payload + router_offset, sizeof(id));
		router_list[router_list_index].id = id;
		//printf("id: %d %x %d\n",id, id, htons(ntohs(id)));
		//sprintf(str, "id: %d %x %d\n",id, id, htons(ntohs(id)));
		//id = ntohs(id);
		uint16_t temp16;
		uint32_t temp32;
		//printf("id: %d\n",id);
		
		memcpy( &temp16, cntrl_payload + router_offset + ROUTER_PORT_OFFSET, sizeof(router_port));
		printf("%d\n",temp16);
		router_list[router_list_index].router_port = temp16;
		router_port = ntohs(router_list[router_list_index].router_port);
		
		memcpy(&temp16, cntrl_payload + router_offset + ROUTER_DATA_PORT_OFFSET, sizeof(data_port));
		router_list[router_list_index].data_port = temp16;
		data_port = ntohs(router_list[router_list_index].data_port);
		
		memcpy(&cost, cntrl_payload + router_offset + ROUTER_COST_OFFSET, sizeof(cost));
		router_list[router_list_index].cost = cost;
		cost = ntohs(router_list[router_list_index].cost);

		if(cost != UINT16_MAX)
		{
			router_list[router_list_index].next_hop = router_list[router_list_index].id;
			router_list[router_list_index].neighbour = TRUE;
		}
		else
		{
			router_list[router_list_index].next_hop = router_list[router_list_index].cost;
			router_list[router_list_index].neighbour = FALSE;
			router_list[router_list_index].missed = 3;
			router_list[router_list_index].time = -1;
		}
		if (cost == 0)
		{
			this_router_index = router_list_index;
			router_list[router_list_index].neighbour = FALSE;
		}		
		memcpy(&temp32, cntrl_payload + router_offset + ROUTER_IP_OFFSET, sizeof(ip));
		router_list[router_list_index].ip = temp32;	
		//ip = ntohs(ip);
		struct in_addr inaddr;
		inaddr.s_addr = router_list[router_list_index].ip;
		char* ip_str = inet_ntoa(inaddr);
		printf("ip: %s, r_port:%d, d_port:%d, cost:%d, next_hop:%d\n",ip_str, router_port, data_port, cost, ntohs(router_list[router_list_index].next_hop));
		sprintf(str, "%sid: %d, ip: %s, r_port:%d, d_port:%d, cost:%d, next_hop:%d\n",str, id, ip_str, router_port, data_port, cost, ntohs(router_list[router_list_index].next_hop));
		//uint32_t ip_ = router_list[router_list_index].ip;
		//uint16_t rport_ = router_list[router_list_index].router_port;
		//uint16_t dport_ = router_list[router_list_index].data_port;
		//uint16_t cost_ = router_list[router_list_index].cost;
		//uint16_t next_hop_ = router_list[router_list_index].next_hop;
		//printf("r_port_n:%02x, d_port_n:%02x, cost_n:%02x, next_hop: %02x\n", router_list[router_list_index].router_port, router_list[router_list_index].data_port, router_list[router_list_index].cost,router_list[router_list_index].next_hop );
		//sprintf(str, "id:%d, r_port_n:%02x, d_port_n:%02x, cost_n:%02x, next_hop: %02x\n", id, router_list[router_list_index].router_port, router_list[router_list_index].data_port, router_list[router_list_index].cost,router_list[router_list_index].next_hop );
		router_list[router_list_index].missed = 0;
		router_list_index++;
		
	}
	sprintf(str,"%s%d's router list init'ed at %x\n", str, ntohs(router_list[this_router_index].id), router_list);
	//printf("completed loop\n");
	//for(int i=0; i <5; i++)
	//{
	//	printf("id: %d, next_hop:%d, cost: %d\n",i+1,ntohs(router_list[i].next_hop),ntohs(router_list[i].cost));
	//	printf("id: %x, next_hop:%x, cost: %x\n",i+1,htons(ntohs(router_list[i].next_hop)),htons(ntohs(router_list[i].cost)));
	//}
	FILE *fp1;
	char *filepath = malloc(sizeof(char)*100);
	sprintf(filepath,"%s%d","/home/csgrad/aumale/MNC/pa3/rt_resp_output",ntohs(router_list[this_router_index].id));
	fp1 = fopen(filepath,"a");
	fprintf(fp1,"%s\n",str);
	fflush(fp1);
}
//NOTE: not in use
//oid init_router_table(int dim_xy, char *cntrl_payload, uint16_t cntrl_payload_len )
//
//	int i,j;
//	uint16_t **table;
//	uint16_t router_offset;
//	uint16_t id, cost;
//	table = (uint16_t **) malloc(sizeof(uint16_t *) * dim_xy);
//	for (i = 0; i < dim_xy; i++) 
//	{
//		table[i] = (uint16_t *) malloc(sizeof(uint16_t) * dim_xy);
//	}
//	
//	for (i = 0; i < dim_xy; i++)
//	{
//		for (j = 0; j < dim_xy; j++)
//		{
//			table[i][j] = UINT16_MAX;
//		}
//	}
//	for(router_offset = ROUTER_OFFSET; router_offset < cntrl_payload_len; router_offset += ROUTER_OFFSET_INCREMENT)
//	{
//		//printf("inside loop\n");
//		memcpy(&id, cntrl_payload + router_offset, sizeof(id));
//		id = ntohs(id);
//		memcpy(&cost, cntrl_payload + router_offset + ROUTER_COST_OFFSET, sizeof(cost));
//		cost = ntohs(cost);
//		table[this_router_id][id] = cost;
//	}
//	router_table = table;
//

