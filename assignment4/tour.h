#ifndef __TOUR_h
#define __TOUR_h

#include    "unp.h"

#define         IP_SIZE         16
#define         HOST_SIZE       30
#define         MCAST_IP        "234.245.210.123"
#define         MCAST_PORT      9850
#define         RT_PROTO        197         
#define         MAX_TOUR_SIZE   90                              // Store maximum 95 IP addresses in the tour
#define         IP_PACK_SIZE    1500

typedef struct TourPayload
{
        int next_ip_idx;
        int tour_size;
        int multi_port;        
        char multi_ip[IP_SIZE];
        char ip_addrs[MAX_TOUR_SIZE * IP_SIZE];
}tpayload;

int createSocket(int *pfsockfd, int *rtsockfd, int *pgsockfd, int *multisockfd);
void converthostnametoIP(int argc, char *argv[], char *ipAddrs);
void createPayload(tpayload *packet, int idx, int tour_size, char *ipaddrs);
void printTourPacket(tpayload packet);
char * getIPaddrbyIdx(tpayload *packet, int idx, char *ip);
unsigned short csum(unsigned short *buf, int nwords);
void send_tour_packet(int rtsockfd, tpayload *packet, int len);
void addtomulticastgroup(int sockfd);

#endif  /* __TOUR_h */

