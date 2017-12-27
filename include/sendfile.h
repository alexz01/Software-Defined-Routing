#ifndef SENDFILE_H_
#define SENDFILE_H_

int sendfile_response(int sock_index,char *cntrl_payload, uint16_t cntrl_payload_len);

#endif