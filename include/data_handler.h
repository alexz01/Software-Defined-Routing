#ifndef DATA_HANDLER_H_
#define DATA_HANDLER_H_

int create_data_sock();
int new_incoming_data_conn(int sock_index);
int new_outgoing_data_conn(uint32_t destination_ip);
bool isData(int sock_index);
bool data_recv_hook(int sock_index);
int send_file(int their_socket, uint32_t dest_ip, uint8_t init_ttl, uint8_t transfer_id, uint16_t init_seq_number, char *filename);
int recv_data_packet(int sock_index);
int send_data_packet(int sock_index);
#endif