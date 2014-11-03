#ifndef	__utils_h
#define	__utils_h

#include	"unp.h"
#include	<net/if.h>

#define SYN_HS1		    1
#define ACK_HS2		    2
#define SYN_ACK_HS3	    3

#define WIN_CHECK	    4
#define WIN_UPDATE	    5

#define DATA_PAYLOAD	    11
#define DATA_ACK	    12

#define FIN		    13
#define FIN_ACK		    14
#define FIN_ACK_ACK	    15

#define PAYLOAD_CHUNK_SIZE  512
#define PACKET_SIZE	    528

/* struct for a Frame/Message Header */
typedef struct header   {
        int msg_type;		                        /* Determine type of message			       */
        int seq_num;		                        /* sequence no/next expected seq no depending on msg_type */
        int adv_window;                                 /* Advertising Window				       */
        uint32_t timestamp;                             /* Time Stamp					       */
}hdr;

/* struct for message / frame */
typedef struct message  {
        hdr     header;					/* Structure to store header information    */
        char    payload[PAYLOAD_CHUNK_SIZE];            /* Actual Data bytes to be sent.	    */
	int	len;					/* Size of payload			    */
}msg;

/* struct for an Interface */
typedef struct InterfaceInfo {
	int			sockfd;                 /* socket file descriptor       */
        int                     mask;                   /* Subnet mask bits             */
        struct sockaddr_in      ifi_addr;	        /* primary address              */
        struct sockaddr_in      ifi_ntmaddr;		/* netmask address              */
        struct sockaddr_in      ifi_subnetaddr;		/* subnet address               */
        struct InterfaceInfo	*ifi_next;	        /* next of these structures     */
}interfaceInfo;

/* struct for server Info */
typedef struct servStruct {
	struct InterfaceInfo	*ifi_head;
	int 			serv_portNum;		/* Well know server port number */
	int 			send_Window;		/* MaxSending sliding win size  */
} servStruct;

/* struct for client Info */
typedef struct clientStruct {
	struct sockaddr_in 	serv_addr;		/* IP address of server		 */
	struct sockaddr_in 	cli_addr;		/* IP address of client		 */
	int 			serv_portNum;		/* Server port number		 */
	int                     cli_portNum;            /* Client port number            */
        char 			fileName[100]; 		/* Filename to be transferred	 */
	int 			rec_Window;		/* Recieving sliding window size */
	int 			seed;			/* Random generator seed value 	 */
	float 			dg_lossProb; 		/* Datagram loss probability     */
	int 			recv_rate; 		/* mean rate in ms		 */
	struct InterfaceInfo 	*ifi_head;		/* head of interface linklist    */	 
} clientStruct;

typedef struct existing_connections {
	struct sockaddr_in		serv_addr;		/* IP address of server		 */
	struct sockaddr_in		client_addr;		/* IP address of client		 */
	int				client_portNum;		/* Client port number		 */
	int				serv_portNum;		/* Server port number		 */
	int				child_pid;		/* child pid			 */
	struct existing_connections	*next_connection;	/* next of these structures	 */
}existing_connections;

/* function prototypes */
interfaceInfo * get_interfaces_client();
interfaceInfo * get_interfaces_server(int portno);
servStruct * loadServerInfo();
clientStruct * loadClientInfo();

void createHeader(hdr *header, int msg_type, int seqnum, int advwin, int ts);
void createMsgPacket(msg *datapack, hdr header, char *buf, int len);
void createAckPacket(msg *ackpack, int msgtype, int ackno, int advwin, int ts);

#endif	/* __utils_h */

