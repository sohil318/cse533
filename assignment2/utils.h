#ifndef	__utils_h
#define	__utils_h

#include	"unp.h"
#include	<net/if.h>

/* Linklist of Interfaces */
typedef struct InterfaceInfo {
	int			sockfd;                 /* socket file descriptor       */
        int                     mask;                   /* Subnet mask bits             */
        struct sockaddr_in      ifi_addr;	        /* primary address              */
        struct sockaddr_in      ifi_ntmaddr;		/* netmask address              */
        struct sockaddr_in      ifi_subnetAddr;		/* subnet address               */
        struct InterfacesInfo   *ifi_next;	        /* next of these structures     */
}interfaceInfo;

/* function prototypes */
interfaceInfo* get_interfaces_client();
interfaceInfo* get_interfaces_server(int portno);
void   loadContents(int type);

#endif	/* __utils_h */

