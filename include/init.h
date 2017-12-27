#ifndef INIT_H_
#define INIT_H_

#define ROUTER_OFFSET 4
#define ROUTER_OFFSET_INCREMENT 12
#define ROUTER_PORT_OFFSET 2
#define ROUTER_DATA_PORT_OFFSET 4
#define ROUTER_COST_OFFSET 6
#define ROUTER_IP_OFFSET 8

#include "../include/router_handler.h"
#include "../include/data_handler.h"

void init_response(int sock_index,char* cntrl_payload, uint16_t payload_len);
void init_router_list(int size, char *cntrl_payload, uint16_t cntrl_payload_len);
void init_router_table(int dim_xy, char *cntrl_payload, uint16_t cntrl_payload_len);



#endif