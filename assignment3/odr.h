#ifndef __ODR_h
#define __ODR_h

#include    "unp.h"
#include    <net/if.h>

/* Interface Information structure to hold all interfaces of a machine */
typedef struct InterfaceInfo {
    int ifaceIdx;
    char ifaceName[100];
    struct sockaddr_in *ifaddr;
    char haddr[6];
    struct InterfaceInfo *next;
}ifaceInfo;


/* Structure to store map of sunpath and port number. */
typedef struct port_sunpath_dict {
    int port;
    char *sun_path;
    struct timeval ts;
    struct port_sunpath_dict *next;
}port_spath_map;

#endif  /* __odr_h */
