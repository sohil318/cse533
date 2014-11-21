#ifndef __UTILS_h
#define __UTILS_h

#include    "unp.h"
#define     IP_SIZE     16
#define     TS_SIZE     1440

typedef struct msg_send_packet
{
    char destIP[IP_SIZE];
    int destportno;
    int rediscflag;
    char msg[TS_SIZE];
}msend;

typedef struct msg_recv_packet
{
    char srcIP[IP_SIZE];
    int srcportno;
    char msg[TS_SIZE];
}mrecv;

int msg_send(int sockfd, char *ip_dest, int dest_port, char *msg, int flag);
int msg_recv(int sockfd, mrecv * recv_p, struct sockaddr_un *addr);  
void convertstreamtosendpacket(msend *msgdata, char* msg_stream);


#endif  /* __UTILS_h */

