#ifndef ROUTER_HANDLER_H_
#define ROUTER_HANDLER_H_

int create_router_sock(); //UDP
//int new_router_conn(int sock_index);
//bool isRouter(int sock_index);
//bool router_recv_hook(int sock_index);
void recvVector(int sock_index);
void _updateTable(uint16_t num_update_fields);
void sendVector(int sock_index);
#endif