#ifndef __ODR_h
#define __ODR_h

#include    "unp.h"
#include    <net/if.h>
#define     STR_SIZE        100
#define     SERV_SUN_PATH   "serv_sun_path" 
#define     SERV_PORT_NO    5000
#define     CLI_PORT        6000
#define     MY_PROTOCOL     0x5892

/* Interface Information structure to hold all interfaces of a machine */
typedef struct InterfaceInfo {
    int ifaceIdx;
    char ifaceName[STR_SIZE];
    char ifaddr[STR_SIZE];
    char haddr[6];
    struct InterfaceInfo *next;
}ifaceInfo;

/* Structure to store map of sunpath and port number. */
typedef struct port_sunpath_dict {
    int port;
    char sun_path[STR_SIZE];
    struct tm *ts;
    struct port_sunpath_dict *next;
}port_spath_map;

char* readInterfaces();
void addInterfaceList(int idx, char *name, char *ip_addr, char *haddr);
void print_interfaceInfo ();
void add_sunpath_port_info( char *sunpath, int port);
void print_sunpath_port_map ();
int createUXpacket(int family, int type, int protocol);
int createPFpacket(int family, int type, int protocol);


#endif  /* __odr_h */
