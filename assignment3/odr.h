#ifndef __ODR_h
#define __ODR_h

#include    "unp.h"
#include    "utils.h"
#include    <sys/socket.h>
#include    <linux/if_ether.h>
#include    <linux/if_arp.h>


#define     SERV_PORT_NO        5000
#define     CLI_PORT            6000
#define     MY_PROTOCOL         0x5892
#define     UNIX_DGRAM_PATH     "unix_path"
#define     SERV_SUN_PATH       "serv_path" 

#define     RREQ                1
#define     RREP                2
#define     DATA                3

#define     MAC_SIZE            6
#define     IP_SIZE             16
#define     STR_SIZE            100
#define     DATA_SIZE           1440
#define     MSG_STREAM_SIZE     1440

#define     ETHR_FRAME_LEN       1514


/***********************************************************************/
/* Interface Information structure to hold all interfaces of a machine */
typedef struct InterfaceInfo {
    int ifaceIdx;
    char ifaceName[STR_SIZE];
    char ifaddr[IP_SIZE];
    char haddr[MAC_SIZE];
    struct InterfaceInfo *next;
}ifaceInfo;
/***********************************************************************/

/***********************************************************************/
/* Structure to store map of sunpath and port number. */
typedef struct port_sunpath_dict {
    int port;
    char sun_path[STR_SIZE];
    struct timeval ts;
    struct port_sunpath_dict *next;
}port_spath_map;
/***********************************************************************/

/***********************************************************************/
/* Structure with routing table entry. */
typedef struct routing_table_entry  {
    char destIP[IP_SIZE];
    char next_hop_MAC[MAC_SIZE];
    int ifaceIdx;
    int hopcount;
    int broadcastId;
    struct timeval ts;
    struct routing_table_entry *next;
} rtabentry;
/***********************************************************************/

/***********************************************************************/
/* ODR Frame of different types RREQ, RREP, DATA */
typedef struct ODRpacket    {
    int packet_type;
    char src_ip[IP_SIZE];
    int src_port;
    char dst_ip[IP_SIZE];
    int dest_port;
    int broadcastid;
    int hopcount;
    int rep_already_sent;
    int route_discovery;
    char datamsg[DATA_SIZE];
}odrpacket;
/***********************************************************************/



/************************************************************************************************************************************************/

char* readInterfaces();
void addInterfaceList(int idx, char *name, char *ip_addr, char *haddr);
void print_interfaceInfo ();
void add_sunpath_port_info( char *sunpath, int port);
void print_sunpath_port_map ();
int createUXpacket(int family, int type, int protocol);
int createPFpacket(int family, int type, int protocol);

int BindUnixSocket(struct sockaddr_un *servAddr, char *sun_path, int uxsockfd);
void handleReqResp(int uxsockfd, int pfsockfd);
void handleUnixSocketInfofromClientServer(int uxsockfd, int pfsockfd);
void handlePFPacketSocketInfofromOtherODR(int uxsockfd, int pfsockfd);
port_spath_map * sunpath_lookup(char *sun_path);
port_spath_map * port_lookup(int port);
void client_server_same_vm(int uxsockfd, int pfsockfd, msend *msgdata, struct sockaddr_un *saddr);

void sendODR(int sockfd, odrpacket *packet, char *src_mac, char *dst_mac, int ifaceidx);
odrpacket * createRREQMessage (char *srcIP, char *destIP, int sport, int dport, int bid, int hop, int flag, int asent);
odrpacket * createRREPMessage (char *srcIP, char *destIP, int sport, int dport, int hop, int flag);
odrpacket * createDataMessage (char *srcIP, char *destIP, int sport, int dport, int hop, char *msg);

void delete_routing_entry(char *destIP);
int add_routing_entry(int packet_type, char *destIP, char *next_hop_MAC, int ifaceIdx, int hopcount, int broadcastId, int rdiscflag);
rtabentry * routing_table_lookup(char *destIP, int disc_flag);
char * get_interface_mac(int ifaceIdx);
void RREQ_broadcast(int sockfd, odrpacket *pack, int ifaceIdx);
odrpacket * getODRPacketfromEthernetPacket(char *ether_frame);
void handleRREQPacket(int pfsockfd, char * src_mac, char * dst_mac, odrpacket * packet, int ifaceidx);
void handleRREPPacket(int pfsockfd, char * src_mac, char * dst_mac, odrpacket * packet, int ifaceidx);
void handleDATAPacket(int uxsockfd, char * src_mac, char * dst_mac, odrpacket * packet, int ifaceidx);

/************************************************************************************************************************************************/

#endif  /* __odr_h */
