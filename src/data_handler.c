#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
//#include <sys/types.h>
//#include <sys/stat.h>

#include "../include/global.h"
#include "../include/network_util.h"
#include "../include/author.h"
#include "../include/init.h"

#ifndef PACKET_USING_STRUCT
    #define CNTRL_data_CODE_OFFSET 0x04
    #define CNTRL_PAYLOAD_LEN_OFFSET 0x06
#endif

#define DATA_DESTINATION_ROUTER_IP_OFFSET 0
#define DATA_INIT_TTL_OFFSET 4
#define DATA_TRANSFER_ID_OFFSET 5
#define DATA_INIT_SEQ_NUMBER_OFFSET 6
#define DATA_PADDING_OFFSET 8
#define DATA_PAYLOAD_OFFSET 12
/* Linked List for active data connections */
struct dataConn
{
    int sockfd;
    LIST_ENTRY(dataConn) next;
}*connection, *conn_temp;
LIST_HEAD(dataConnsHead, dataConn) data_conn_list;

FILE *fp;
int create_new_fp;
char last_packet[DATA_PACKET_SIZE] = "";
char second_last_packet[DATA_PACKET_SIZE] ="";

int create_data_sock()
{
    int sock;
    struct sockaddr_in data_addr;
    socklen_t addrlen = sizeof(data_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&data_addr, sizeof(data_addr));

    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    data_addr.sin_port = router_list[this_router_index].data_port;

    if(bind(sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)
        ERROR("bind() failed");

    if(listen(sock, 5) < 0)
        ERROR("listen() failed");

    LIST_INIT(&data_conn_list);

    return sock;
}

int new_incoming_data_conn(int sock_index)
{
	
	fflush(stdout);
    int fdaccept, caddr_len;
    struct sockaddr_in remote_dataler_addr;

    caddr_len = sizeof(remote_dataler_addr);
    fdaccept = accept(sock_index, (struct sockaddr *)&remote_dataler_addr, &caddr_len);
    if(fdaccept < 0)
        ERROR("accept() failed");
	printf("[d]  data connection received on %d\n", fdaccept);
    /* Insert into list of active data connections */
    connection = malloc(sizeof(struct dataConn));
    connection->sockfd = fdaccept;
    LIST_INSERT_HEAD(&data_conn_list, connection, next);
	create_new_fp = 1;
    return fdaccept;
}

void remove_data_conn(int sock_index)
{
	printf("[remove_data_conn] closing %d\n",sock_index);
    LIST_FOREACH(connection, &data_conn_list, next) {
        if(connection->sockfd == sock_index) LIST_REMOVE(connection, next); // this may be unsafe?
        free(connection);
    }

    close(sock_index);
}

bool isData(int sock_index)
{
    LIST_FOREACH(connection, &data_conn_list, next)
        if(connection->sockfd == sock_index) return TRUE;

    return FALSE;
}

int new_outgoing_data_conn(uint32_t destination_ip)
{
	
	int sock;
    struct sockaddr_in data_addr;
    socklen_t addrlen = sizeof(data_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&data_addr, sizeof(data_addr));
	uint16_t destination_next_hop;
	for(int i = 0 ; i < num_router; i++)
	{
		if(router_list[i].ip == destination_ip)
		{
			//get id for next hop router
			destination_next_hop = router_list[i].next_hop;
			break;
		}
	}
	printf("[!] creating new output connection to %d\n",destination_next_hop);	
	int their_router_index = -1;
	for(int i = 0 ; i < num_router; i++)
	{
		if(router_list[i].id == destination_next_hop)
			their_router_index = i;
			
	}
    data_addr.sin_family = AF_INET;
    memcpy(&data_addr.sin_addr.s_addr, &router_list[their_router_index].ip, sizeof(router_list[their_router_index].ip));
    memcpy(&data_addr.sin_port, &router_list[their_router_index].data_port,sizeof(router_list[their_router_index].data_port));
	
	if (connect(sock, (struct sockaddr*)&data_addr, addrlen) == -1) 
	{
		close(sock);
		perror("client: connect");
	}
	return sock;
}

int close_outgoing_connection(int socket)
{
	return close(socket);
}



//int packets_received = 0;
int next_hop_socket;
int recv_data_packet(int sock_index)
{
	//printf("[r] Receiving packet %d\n", ++packets_received);
	char buffer[DATA_PACKET_SIZE];
	//receive packet 
	if(recvALL(sock_index, buffer, DATA_PACKET_SIZE) < 0)
	{
		remove_data_conn(sock_index);
		return -1;
	}
	//buffer in last packet and second last packet
	memcpy(second_last_packet, last_packet, DATA_PACKET_SIZE);
	memcpy(last_packet, buffer, DATA_PACKET_SIZE);
	uint8_t ttl;
	memcpy(&ttl, buffer + DATA_INIT_TTL_OFFSET, sizeof(ttl));
	//reduce ttl
	ttl--;
	
	memcpy(buffer + DATA_INIT_TTL_OFFSET, &ttl, sizeof(ttl));
	
	//if ttl > 0 store the packet if destination is this router else find destination and forward it
	if(ttl > 0)
	{		
		
		//forward packet to next_hop
		uint32_t destination_ip;
		memcpy(&destination_ip, buffer + DATA_DESTINATION_ROUTER_IP_OFFSET, sizeof(destination_ip));
		uint32_t padding = 0;
		memcpy(&destination_ip, buffer + DATA_DESTINATION_ROUTER_IP_OFFSET, sizeof(destination_ip));
		memcpy(&padding, buffer + DATA_PADDING_OFFSET, sizeof(padding));
		uint16_t next_hop_id;
		char *payload = malloc(DATA_PAYLOAD_SIZE*sizeof(char));
		if(destination_ip == router_list[this_router_index].ip)
		{	
			//create a new stats list entry
			struct sendfile_stat *stats = malloc(sizeof(struct sendfile_stat));
			stats->ttl = ttl;
			//memcpy(buffer + DATA_INIT_TTL_OFFSET, &ttl, sizeof(init_ttl));
			memcpy(&stats->transfer_id, buffer + DATA_TRANSFER_ID_OFFSET, sizeof(stats->transfer_id));
			memcpy(&stats->seq_number, buffer + DATA_INIT_SEQ_NUMBER_OFFSET, sizeof(stats->seq_number));
			LIST_INSERT_HEAD(&sendfile_stat_list, stats, sendfile_next);

			//store data in file
			//printf("[recv_data_packet] the file is for me\n");
			memset(payload, '\0', DATA_PAYLOAD_SIZE);
			memcpy(payload, buffer + DATA_PAYLOAD_OFFSET, DATA_PAYLOAD_SIZE);
			
//			struct sendfile_stat *stats = malloc (sizeof(struct sendfile_stat));
//			stats->transfer_id = transfer_id;
//			stats->ttl = ttl;
//			stats->seq_number = seq;
//			LIST_INSERT_HEAD(&sendfile_stat_list, stats, sendfile_next);

			//printf("packet received: %s",payload);			
			
			if(create_new_fp == 1)
			{
				char filename[9];
				memset(filename,'\0',9);
				sprintf(filename,"./file-%hu",stats->transfer_id);
				fp = fopen(filename,"ab");
				create_new_fp = 0;
			}
			
			int obj_written = 0;
			if(padding == 0)
				obj_written = fwrite(payload, DATA_PAYLOAD_SIZE, 1, fp);
			else
			{
				int size = 0;
				while(payload[size++]!= '\0');
				if(size < DATA_PAYLOAD_SIZE)
				{
					if(payload[size+1] == '\0')
					{	
						printf("last packet size: %d\n",size);
						obj_written = fwrite(payload, size-1, 1, fp);
					}
					else
						obj_written = fwrite(payload, DATA_PAYLOAD_SIZE, 1, fp);
				}
				else
					obj_written = fwrite(payload, DATA_PAYLOAD_SIZE, 1, fp);
				fclose(fp);
				
			}
			fflush(fp);
			if(obj_written < 0)
				perror("fwrite"); 
			//else
			//	printf("wrote %dB successfully",obj_written);
			free(payload);
			
		}
		else
		{
			//printf("[recv_data_packet] the file is for me\n");
			//create new data connection to the next_hop router 
			if(create_new_fp == 1)
			{
				next_hop_socket = new_outgoing_data_conn(destination_ip);
				create_new_fp = 0;
			}
			//forward packet
			
			sendALL(next_hop_socket, buffer, DATA_PACKET_SIZE);
			//close connections
			if(padding != 0)
				close_outgoing_connection(next_hop_socket);				
		}
	}
	else
	{
		printf("TTL = %d, packet dropped", ttl);
	}
	
	return 1;
}

int send_file(int their_socket, uint32_t dest_ip, uint8_t init_ttl, uint8_t transfer_id, uint16_t init_seq_number, char *filename)
{
	//printf("[send_file] sending data file %s\n", filename);
	long filelen;
	char *filepath = malloc(sizeof(char)*100);
	memset(filepath, '\0',sizeof(char)*100);
	sprintf(filepath,"./%s",filename);
	uint32_t padding = 0;
	FILE *fp = fopen(filepath, "rb");
	//struct stat file_stat;
	//if(stat(filepath, &file_stat) == -1)
	//	perror("stat");
	//filelen = stat.st_size;
	fseek(fp, 0, SEEK_END);
	filelen = ftell(fp);
	rewind(fp); 
	printf("[send_file] file len: %li\n", filelen);
	
	char *buf = (char *)malloc((filelen+1)*sizeof(char));
	fread(buf, filelen, 1, fp); 
	fclose(fp);
	buf[filelen] = '\0';
	char *data_packet = malloc((DATA_PACKET_SIZE)*sizeof(char));
	char *data_payload = malloc(DATA_PAYLOAD_SIZE*sizeof(char));
	uint16_t current_seq_number = ntohs(init_seq_number);

	memcpy(data_packet + DATA_DESTINATION_ROUTER_IP_OFFSET, &dest_ip, sizeof(dest_ip));
	memcpy(data_packet + DATA_INIT_TTL_OFFSET, &init_ttl, sizeof(init_ttl));
	memcpy(data_packet + DATA_TRANSFER_ID_OFFSET, &transfer_id, sizeof(transfer_id));
	memcpy(data_packet + DATA_PADDING_OFFSET, &padding, sizeof(padding));
	//printf("[!] buffer : %s\n",buf);
	int packets_sent = 0;
	for(long bytes_read = 0; bytes_read < filelen ; bytes_read += DATA_PAYLOAD_SIZE)
	{
		//printf("[send_file] packets sent: %d\n",++packets_sent);
		memset(data_payload, '\0', DATA_PAYLOAD_SIZE);
		if(filelen - bytes_read <= DATA_PAYLOAD_SIZE) // last packet
		{
			memcpy(data_payload, buf + bytes_read, filelen - bytes_read);
			printf("last packet size: %d", filelen - bytes_read);
			//data_payload[filelen - bytes_read] = '\0';
			//printf("[send_file] data_payload: %s",data_payload);
			padding = 0b10000000000000000000000000000000;
			memcpy(data_packet + DATA_PADDING_OFFSET, &padding, sizeof(padding));
		}
		else
		{
			memcpy(data_payload, buf + bytes_read, DATA_PAYLOAD_SIZE*sizeof(char));
			//printf("[send_file] data_payload: %s",data_payload);
		}
	
		uint16_t seq = htons(current_seq_number);
		current_seq_number++;
		
		//memset(data_packet, 0, 12*sizeof(char));
		memcpy(data_packet + DATA_INIT_SEQ_NUMBER_OFFSET, &seq, sizeof(init_seq_number));
		memcpy(data_packet + DATA_PAYLOAD_OFFSET, data_payload, DATA_PAYLOAD_SIZE);
		
		memset(second_last_packet, 0,12*sizeof(char));
		memcpy(second_last_packet, last_packet, DATA_PACKET_SIZE);
		memset(last_packet, 0,12*sizeof(char));
		memcpy(last_packet, data_packet, DATA_PACKET_SIZE );
		//printf("[send_file] sending: %s\n",data_packet+8);
		sendALL(their_socket, data_packet, (DATA_PACKET_SIZE));
		//printf("read: %s\n",str);
		
		//save statistics data
		struct sendfile_stat *stats = malloc (sizeof(struct sendfile_stat));
		stats->transfer_id = transfer_id;
		stats->ttl = init_ttl;
		stats->seq_number = seq;
		LIST_INSERT_HEAD(&sendfile_stat_list, stats, sendfile_next);
	}
	printf("[send_file] finished sending file\n");
	return 1;
}
