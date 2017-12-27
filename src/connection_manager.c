/**
 * @connection_manager
 * @author  Swetank Kumar Saha <swetankk@buffalo.edu>
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
 * Connection Manager listens for incoming connections/messages from the
 * controller and other routers and calls the desginated handlers.
 */


#include<arpa/inet.h>
#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/data_handler.h"
#include "../include/router_handler.h"

fd_set watch_list;
fd_set master_list;
int head_fd;
uint16_t current_time;
struct timeval SELECT_TIMEOUT;

void main_loop()
{	
	SELECT_TIMEOUT.tv_sec = 100000;
	int counter = 0;
    int selret, sock_index, fdaccept;
	
    while(TRUE){
		//printf("timeout: %d.%d\n",SELECT_TIMEOUT.tv_sec,SELECT_TIMEOUT.tv_usec);
        watch_list = master_list;
        selret = select(head_fd+1, &watch_list, NULL, NULL, &SELECT_TIMEOUT);
		bool fd_isset = FALSE;
        if(selret < 0)
		{
            ERROR("select failed.");
		}
        /* Loop through file descriptors to check which ones are ready */
        else if(selret)
			for(sock_index=0; sock_index<=head_fd; sock_index+=1){

				if(FD_ISSET(sock_index, &watch_list)){ // select invoked by a file_discripter TODO: write code for timeout interrupt
					fd_isset = TRUE;
					/* control_socket */
					if(sock_index == control_socket){
						fdaccept = new_control_conn(sock_index);
	
						/* Add to watched socket list */
						FD_SET(fdaccept, &master_list);
						if(fdaccept > head_fd) head_fd = fdaccept;
					}
	
					/* router_socket */
					else if(sock_index == router_socket){
						//call handler that will call recvfrom() .....
						recvVector(sock_index);
					}
	
					/* data_socket */
					else if(sock_index == data_socket){
						fdaccept = new_incoming_data_conn(sock_index);
						FD_SET(fdaccept, &master_list);
						if(fdaccept > head_fd) head_fd = fdaccept;
					}
	
					/* Existing connection */
					else{
						if(isControl(sock_index))
						{
							if(!control_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);
						}
						else if (isData(sock_index))
						{
							if(recv_data_packet(sock_index) < 0 ) FD_CLR(sock_index, &master_list);
						}
						else {printf("socket causing err: %d",sock_index); ERROR("Unknown socket index");}
					}
				}
			}
		else // select returned due to timeout
		{
			current_time++;// += update_interval - SELECT_TIMEOUT.tv_sec;
			//printf("counter = %d \n",counter++);
			SELECT_TIMEOUT.tv_sec = 1;
			//SELECT_TIMEOUT.tv_sec = update_interval - SELECT_TIMEOUT.tv_sec;
			SELECT_TIMEOUT.tv_usec = 0;
			//send distance vector to all routers with cost != UINT16_MAX
			if(current_time%update_interval==0)
				sendVector(router_socket);
			for(int i = 0; i < num_router; i++)
			{
				if(i == this_router_index) continue;
				if(current_time%update_interval == router_list[i].time)
					router_list[i].missed++;
				if(router_list[i].missed ==3 )
				{	
					printf("%d has probably stopped!!!!!", ntohs(router_list[i].id));
					router_list[i].neighbour = FALSE;
					router_list[i].cost = UINT16_MAX;
					//router_list[i].next_hop = UINT16_MAX;
					printf("new routing table : \n");
					for(int i = 0; i < num_router; i++)
					{
						printf("%d : %d : %d\n",ntohs(router_list[i].id), ntohs(router_list[i].cost), ntohs(router_list[i].next_hop));
					}
				}	
			}
		}
    }
}

void init()
{
	printf("Initiailizing\n");
    control_socket = create_control_sock();

    //router_socket and data_socket will be initialized after INIT from controller

    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the control socket */
    FD_SET(control_socket, &master_list);
    head_fd = control_socket;
	printf("Initialization complete\n");
	
    main_loop();
}