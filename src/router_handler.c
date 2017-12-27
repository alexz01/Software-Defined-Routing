/**
 * @router_handler
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
 * Handler for the control plane.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include "../include/global.h"
#include "../include/network_util.h"
#include "../include/control_header_lib.h"
#include "../include/router_handler.h"


#ifndef PACKET_USING_STRUCT
    #define CNTRL_CONTROL_CODE_OFFSET 0x04
    #define CNTRL_PAYLOAD_LEN_OFFSET 0x06
#endif


#define NUM_UPDATE_FIELD_OFFSET 0
#define SOURCE_ROUTER_PORT_OFFSET 2
#define SOURCE_ROUTER_IP_OFFSET 4

#define N_ROUTER_IP_OFFSET 0
#define N_ROUTER_PORT_OFFSET 4
#define N_ROUTER_PADDING_OFFSET 6
#define N_ROUTER_ID_OFFSET 8
#define N_ROUTER_COST_OFFSET 10
#define N_VECTOR_START_OFFSET 8
#define N_VECTOR_NEXT_OFFSET 12

struct distance_vector
{
	uint16_t ip;
	uint16_t r_port;
	uint16_t id;
	uint16_t cost;	
};

struct distance_vector *dv;
uint16_t their_router_index;
int create_router_sock()
{
    int sock;
    struct sockaddr_in router_addr;
    socklen_t addrlen = sizeof(router_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&router_addr, sizeof(router_addr));

    router_addr.sin_family = AF_INET;
    router_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    router_addr.sin_port = router_list[this_router_index].router_port;

    if(bind(sock, (struct sockaddr *)&router_addr, sizeof(router_addr)) < 0)
        ERROR("bind() failed");

    //if(listen(sock, 5) < 0)
    //   ERROR("listen() failed");

    //LIST_INIT(&control_conn_list);

    return sock;
}

void recvVector(int sock_index)
{
	int MAXBUFLEN = 100;
	int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    addr_len = sizeof their_addr;
	memset(buf, '\0', MAXBUFLEN);
    if ((numbytes = recvfrom(sock_index, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
	//printf("buf = %s",buf);
	uint16_t num_update_fields, src_router_port;
	uint32_t src_router_ip;
	memcpy(&num_update_fields, &buf + NUM_UPDATE_FIELD_OFFSET, sizeof(num_update_fields));
	memcpy(&src_router_port, &buf + SOURCE_ROUTER_PORT_OFFSET, sizeof(src_router_port));
	memcpy(&src_router_ip, &buf + SOURCE_ROUTER_IP_OFFSET, sizeof(src_router_ip));
	
	num_update_fields = ntohs(num_update_fields);
	//printf("dv has %d routers\n", num_update_fields);
	dv = malloc(sizeof(struct distance_vector)* num_update_fields);
	uint16_t offset = N_VECTOR_START_OFFSET;
	
	//#debug code start
		socklen_t len;
		//struct sockaddr_storage addr;
		char ipstr[INET6_ADDRSTRLEN];
		len = sizeof their_addr;
		getpeername(sock_index, (struct sockaddr*)&their_addr, &len);
		struct sockaddr_in *s = (struct sockaddr_in *)&their_addr;
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
		//printf("vector received (from %s): \n",ipstr);
	//#debug code end
	for(uint16_t i = 0; i <num_update_fields; i++)
	{
		memcpy(&dv[i].ip, buf + offset + N_ROUTER_IP_OFFSET, sizeof(dv[i].ip));
		memcpy(&dv[i].r_port, buf + offset + N_ROUTER_PORT_OFFSET, sizeof(dv[i].r_port));
		memcpy(&dv[i].id, buf + offset + N_ROUTER_ID_OFFSET, sizeof(dv[i].id));
		memcpy(&dv[i].cost, buf + offset + N_ROUTER_COST_OFFSET, sizeof(dv[i].cost));
		
		if(ntohs(dv[i].cost) == 0)
			for(int counter = 0; counter < num_router; counter++)
				if(router_list[counter].id == dv[i].id)
					their_router_index = counter;
			
		offset += N_VECTOR_NEXT_OFFSET;
		//printf("id: %d, cost: %d\n", ntohs(dv[i].id), ntohs(dv[i].cost));
	}
	router_list[their_router_index].missed = 0;
	if(router_list[their_router_index].time = -1)
		router_list[their_router_index].time = current_time%update_interval+1;
	
	_updateTable(num_update_fields);
}

void _updateTable(uint16_t num_update_fields) //update routing table using DVRA
{
	uint16_t router_list_index_in_dv;
	for(int router_list_index = 0; router_list_index < num_router; router_list_index++)
	{	
		for(int i = 0; i < num_update_fields; i++)
			if(router_list[router_list_index].id == dv[i].id)
				router_list_index_in_dv = i;
		//printf("id :%d, prev: %d, new:%d\n", ntohs(router_list[router_list_index].id),ntohs(router_list[router_list_index].cost), (ntohs(router_list[their_router_index].cost) + ntohs(dv[router_list_index_in_dv].cost)));
		uint16_t prev_cost = ntohs(router_list[their_router_index].cost);
		uint16_t new_cost = ntohs(dv[router_list_index_in_dv].cost);
		new_cost += prev_cost;
		if(ntohs(router_list[router_list_index].cost) > (ntohs(router_list[their_router_index].cost) + ntohs(dv[router_list_index_in_dv].cost)))
		{
			printf("updated cost of %d from %d(next_hop = %d) to ",ntohs(router_list[router_list_index].id), ntohs(router_list[router_list_index].cost), ntohs(router_list[router_list_index].next_hop));
			router_list[router_list_index].cost = ntohs(router_list[their_router_index].cost) + ntohs(dv[router_list_index_in_dv].cost);
			router_list[router_list_index].cost = htons(router_list[router_list_index].cost);
			router_list[router_list_index].next_hop =  router_list[their_router_index].id;
			printf("%d(next_hop = %d)\n",ntohs(router_list[router_list_index].cost), ntohs(router_list[router_list_index].next_hop));
			printf("new routing table : \n");
			for(int i = 0; i < num_router; i++)
			{
				printf("%d : %d : %d\n",ntohs(router_list[i].id), ntohs(router_list[i].cost), ntohs(router_list[i].next_hop));
			}
		}
	}
	router_list[their_router_index].missed = 0;
	//printf("new routing table : \n");
	//for(int i = 0; i < num_router; i++)
	//{
	//	printf("%d : %d : %d\n",ntohs(router_list[i].id), ntohs(router_list[i].cost), ntohs(router_list[i].next_hop));
	//}
}

void sendVector(int sock_index)
{
	int payload_len = 8 + 12*num_router;
	int numbytes;
    struct sockaddr_in their_addr;
    char buf[payload_len];
    socklen_t addr_len;
    addr_len = sizeof their_addr;

	uint16_t num_update_fields, src_router_port;
	uint32_t src_router_ip;
	num_update_fields = htons(num_router);
	src_router_port = router_list[this_router_index].router_port;
	src_router_ip = router_list[this_router_index].ip;
	
	memcpy(&buf + NUM_UPDATE_FIELD_OFFSET, &num_update_fields, sizeof(num_update_fields));
	memcpy(&buf + SOURCE_ROUTER_PORT_OFFSET, &src_router_port, sizeof(src_router_port));
	memcpy(&buf + SOURCE_ROUTER_IP_OFFSET, &src_router_ip, sizeof(src_router_ip));
	
	num_update_fields = ntohs(num_update_fields);
	
	//dv = malloc(sizeof(struct distance_vector)* num_update_fields);
	uint16_t offset = N_VECTOR_START_OFFSET;
	uint16_t padding = 0x0000;
	for(uint16_t i = 0; i < num_update_fields; i++)
	{
		memcpy(buf + offset + N_ROUTER_IP_OFFSET, &router_list[i].ip, sizeof(router_list[i].ip));
		memcpy(buf + offset + N_ROUTER_PORT_OFFSET, &router_list[i].router_port, sizeof(router_list[i].router_port));
		memcpy(buf + offset + N_ROUTER_PADDING_OFFSET, &padding, sizeof(router_list[i].router_port));
		memcpy(buf + offset + N_ROUTER_ID_OFFSET, &router_list[i].id, sizeof(router_list[i].id));
		memcpy(buf + offset + N_ROUTER_COST_OFFSET, &router_list[i].cost, sizeof(router_list[i].cost));
		
		offset += N_VECTOR_NEXT_OFFSET;

	}
	
	their_addr.sin_family = AF_INET;
	memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);
	for(int i = 0; i < num_router; i++)
	{
		if(i == this_router_index) 
			continue;
		if(router_list[i].neighbour == TRUE)
		{
			//printf("sending dv to( cost to this: %d): %d\n",ntohs(router_list[i].cost) ,ntohs(router_list[i].id));
			their_addr.sin_port = router_list[i].router_port;
			their_addr.sin_addr.s_addr = router_list[i].ip;
			
			if ((numbytes = sendto(sock_index, buf, payload_len , 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
				perror("sendto");
				exit(1);
			}
		}
	}
}