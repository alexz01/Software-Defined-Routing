#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/queue.h>

typedef enum {FALSE, TRUE} bool;

struct router
{
	//all fields in network representation format 
	uint16_t id;
	uint32_t ip; 
	uint16_t router_port;
	uint16_t data_port;	
	uint16_t cost;
	uint16_t next_hop;
	bool neighbour;
	int time;
	uint16_t missed;
	//LIST_ENTRY(router_distance_vector) router_next;
}; //*router_list, *router_temp;



struct __attribute__((__packed__)) sendfile_stat 
{
	uint8_t transfer_id;
	uint8_t ttl;
	uint16_t seq_number;
	//uint16_t last_seq_number;
	LIST_ENTRY(sendfile_stat) sendfile_next;
};
LIST_HEAD(sendfile_stat_head, sendfile_stat) sendfile_stat_list;


#define ERROR(err_msg) {perror(err_msg); exit(EXIT_FAILURE);}

/* https://scaryreasoner.wordpress.com/2009/02/28/checking-sizeof-at-compile-time/ */
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)])) // Interesting stuff to read if you are interested to know how this works

#define DATA_PACKET_SIZE 1036
#define DATA_PAYLOAD_SIZE 1024


extern uint16_t CONTROL_PORT;
extern int router_socket, data_socket;
extern uint16_t num_router, update_interval;
extern struct timeval SELECT_TIMEOUT;
extern struct router *router_list;
//extern struct distance_vector *dist_vector;
extern uint16_t this_router_index;
extern bool isInit;
extern fd_set master_list;
extern int head_fd;
extern char last_packet[DATA_PACKET_SIZE];
extern char second_last_packet[DATA_PACKET_SIZE];
extern uint16_t current_time;
#endif