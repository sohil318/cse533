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

typedef struct arp_cache
{
    char ip_addr[IP_SIZE];
    unsigned char hw_addr[6];
    int ifindex;
    unsigned short hatype;
    int connfd;
    int incomplete;
    struct arp_cache * next;
}cache;

typedef struct arp_packet{
int type;
int proto_id;
unsigned short hatype;
unsigned char src_mac[6];
char src_ip[IP_SIZE];
unsigned char dest_mac[6];
char dest_ip[IP_SIZE];
} arp_pack;

struct hwa_info * getMacAddrst();

#endif  /* __ARP_h */

