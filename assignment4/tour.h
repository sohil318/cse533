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
#define         IP_IDENT        5892
#define         MSG_SIZE        200

typedef struct TourPayload
{
        int next_ip_idx;
        int tour_size;
        int multi_port;        
        char multi_ip[IP_SIZE];
        uint32_t ip_addrs[MAX_TOUR_SIZE];
}tpayload;

int createSocket(int *pfsockfd, int *rtsockfd, int *pgsockfd, int *multisockfd);
void converthostnametoIP(int argc, char *argv[], uint32_t *ipAddrs);
void createPayload(tpayload *packet, int idx, int tour_size, uint32_t *ipaddrs);
void printTourPacket(tpayload packet);
uint32_t getIPaddrbyIdx(tpayload *packet, int idx);
unsigned short csum(unsigned short *buf, int nwords);
void send_tour_packet(int rtsockfd, tpayload *packet, int userlen);
void addtomulticastgroup(int sockfd, char *ip);
void handletourpacket(int rtsockfd, int multisockfd, int pgsockfd);

#endif  /* __TOUR_h */

