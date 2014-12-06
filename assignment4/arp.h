#ifndef __ARP_h
#define __ARP_h

#include    "unp.h"
#include    "utils.h"
#include    "tour.h"
#include	"hw_addrs.h"

#define		IPPROTO_ID		0x1554	 
#define		AREQ			0
#define		AREP			1
#define 	ETH_LEN  		1514
#define         SERV_SUN_PATH           "serv_path" 


typedef struct arp_packet{
        int type;
        int proto_id;
        unsigned short hatype;
        unsigned char src_mac[6];
        char src_ip[IP_SIZE];
        unsigned char dest_mac[6];
        char dest_ip[IP_SIZE];
} arp_pack;

struct hwa_info * getMacAddr();
arp_pack * create_areq_packet(struct writeArq * arq);
void sendARPReq(int sockfd, arp_pack *packet, char *src_mac, char *dst_mac, int ifaceidx);
void floodARPReq(int pfsockfd, struct writeArq* arq);
void printARPPacket(arp_pack *packet);

#endif  /* __ARP_h */

