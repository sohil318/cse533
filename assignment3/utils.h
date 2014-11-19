#ifndef __UTILS_h
#define __UTILS_h

#include    "unp.h"
#include    <net/if.h>
#define     IP_SIZE     16
#define     TS_SIZE     100

typedef struct msg_send_packet
{
    int sockfd;
    char destIP[IP_SIZE];
    int destportno;
    int rediscflag;
    char msg[TS_SIZE];
}msend;

typedef struct msg_recv_packet
{
    int sockfd;
    char srcIP[IP_SIZE];
    int srcportno;
    char msg[TS_SIZE];
}mrecv;

#endif  /* __UTILS_h */

