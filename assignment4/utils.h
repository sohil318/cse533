#ifndef __UTILS_h
#define __UTILS_h

#include    "unp.h"

#define     UNIX_PATH     "unix_path"

struct hwaddr{
	int sll_ifindex;
	unsigned short sll_hatype;
	unsigned char sll_halen;
	unsigned char sll_addr[8];
};
#endif  /* __UTILS_h */

