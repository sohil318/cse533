#ifndef __UTILS_h
#define __UTILS_h

#include    "unp.h"
#include    "tour.h"

#define     UNIX_PATH     "unix_path"

struct hwaddr{
	int sll_ifindex;
	unsigned short sll_hatype;
	unsigned char sll_halen;
	unsigned char sll_addr[8];
};

struct writeArq{
	struct hwaddr hw;
	char ip_addr[IP_SIZE];
};

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

cache * find_in_cache(char *ip_addr);

#endif  /* __UTILS_h */

