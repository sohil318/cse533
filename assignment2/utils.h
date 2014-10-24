#ifndef	__utils_h
#define	__utils_h

#include	"unp.h"
#include	<net/if.h>

/* Linklist of Interfaces */
struct interfaceInfo {
	int			sockfd;                 /* socket file descriptor       */
        int                     mask;                   /* Subnet mask bits             */
        struct sockaddr         *ifi_addr;	        /* primary address              */
        struct sockaddr         *ifi_ntmaddr;           /* netmask address              */
        struct sockaddr         *ifi_subnetAddr;        /* subnet address               */
        struct interfacesInfo   *ifi_next;	        /* next of these structures     */
};

/* function prototypes */
struct interfaceInfo* loadInterfaces();
void   loadContents(int type);

#endif	/* __utils_h */

