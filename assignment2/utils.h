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
        struct sockaddr_in      ifi_subnetaddr;		/* subnet address               */
        struct InterfaceInfo	*ifi_next;	        /* next of these structures     */
}interfaceInfo;

typedef struct servStruct {
	int 			 serv_portNum;		/* Well know server port number */
	int 			 send_Window;		/* MaxSending sliding win size  */
	struct InterfaceInfo	 *ifi_head;
} servStruct;

typedef struct clientStruct {
	struct sockaddr_in 	serv_addr;		/* IP address of server		*/
	int 			serv_portNum;		/* Server port number		*/
	char 			*fileName; 		/* Filename to be transferred	*/
	int 			rec_Window;		/* Recievingsliding window size */
	int 			seed;			/* Random generator seed value 	*/
	float 			dg_lossProb; 		/* Datagram loss probability    */
	int 			recv_rate; 		/* mean rate in ms				*/
	struct InterfaceInfo 	*ifi_head;		/* head of interface linklist   */	 
} clientStruct;

clientStruct *clientInfo;
servStruct *servInfo;

/* function prototypes */
interfaceInfo* get_interfaces_client();
interfaceInfo* get_interfaces_server(int portno);
void   loadContents(int type);

#endif	/* __utils_h */

