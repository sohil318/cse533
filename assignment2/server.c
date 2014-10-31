#include	 "utils.h"
#include	 "server.h"
#define LOOPBACK "127.0.0.1"

/*	Global	declarations	*/
struct existing_connections *existing_conn;

/* 
 * Function to check if client and server have same host network. 
 */

int checkLocal (struct sockaddr_in serverIP, struct sockaddr_in serverIPnmsk, struct sockaddr_in serverIPsubnet, struct sockaddr_in clientIP)	{
        int isLocal;  
        char src[128];

        inet_ntop(AF_INET, &serverIP.sin_addr, src, sizeof(src));
        
	if (strcmp(src, LOOPBACK) == 0)
		return 1;
	else if ((serverIPnmsk.sin_addr.s_addr & clientIP.sin_addr.s_addr) == serverIPsubnet.sin_addr.s_addr)
	    return 1;
	
	return 0;
}

/*
 * Initialize Sender Queue 
 */

void initSenderQueue(sendQ *queue, int winsize)	{
	queue->buffer		=   (sendWinElem *) calloc (winsize, sizeof(sendWinElem));
	queue->winsize		=   winsize;
	queue->cwinsize		=   5;				
	queue->slidwinstart	=   0;				
	queue->slidwinend	=   0;
}

/*
 * Add to Sender Queue
 */

void addToSenderQueue(sendQ *queue, sendWinElem elem)	{
	//printf("\nAdded To Sender Queue Seq Num : %d ", elem.seqnum);
	queue->buffer[elem.seqnum % queue->winsize] = elem;	

}

/*
 * Create a Sender Queue Element
 */
void createSenderElem (sendWinElem *elem, msg buf, int seqnum)	{
	//printf("Creating Element at seq num = %d", seqnum);
	elem->packet		=   buf;
	elem->seqnum		=   seqnum;					
	elem->retranx		=   0;					
	elem->isPresent		=   1;					
}

/* 
 * Write file contents over the connection socket.
 */

void sendFile(int sockfd, char filename[PAYLOAD_CHUNK_SIZE], struct sockaddr_in client_addr, sendQ *sendWin, int seqno, int adwin)	{
         char buf[PAYLOAD_CHUNK_SIZE];
         int fp, i;
	 int seqnum = seqno, nbytes, advwin = adwin, ts = 0, msgtype;
	 hdr header;
	 msg datapacket;
	 sendWinElem sendelem;
	
         fp = open(filename, O_RDONLY, S_IREAD);

	 for ( i = 0; i < sendWin->cwinsize; i++)	{
         
	    if (seqnum <= sendWin->slidwinend)
	    {	
		/* Need to resend the packet */
		sendelem = sendWin->buffer[ seqnum % sendWin->winsize ];
		sendelem.retranx++;
		datapacket = sendelem.packet;
		sendWin->buffer[ seqnum % sendWin->winsize ] = sendelem;
	    }
	    else
	    {
		nbytes = read(fp, buf, PAYLOAD_CHUNK_SIZE-1);
		buf[nbytes] = '\0';
//		printf("\nBuf : %s", buf);
		if (nbytes < PAYLOAD_CHUNK_SIZE - 1)
		    msgtype        = FIN;
		else
		    msgtype        = DATA_PAYLOAD;
	    
		createHeader(&header, msgtype, seqnum, advwin, ts);
		createMsgPacket(&datapacket, header, buf, nbytes+1);
		createSenderElem(&sendelem, datapacket, seqnum);
		addToSenderQueue(sendWin, sendelem);
		//printf("Sender Queue Elem seq Num : %d", sendWin->buffer[seqnum].packet.header.seq_num); 
		//printf("Sender Queue Elem data : %s", sendWin->buffer[seqnum].packet.payload); 
	    }
	    send(sockfd, &datapacket, sizeof(datapacket), 0);
	    seqnum++;

	    if (msgtype == FIN)
		break;
	    bzero(&buf, PAYLOAD_CHUNK_SIZE);
	    bzero(&datapacket, sizeof(datapacket));
         }
}

/* 
 * Forked server child for service requesting client  
 */ 

int childRequestHandler(int sock, struct InterfaceInfo *head, struct sockaddr_in client_addr, char filename[PAYLOAD_CHUNK_SIZE], sendQ* sendWin)
{
	//printf ("Handling forked Child");
	int sockfd, optval = -1, isLocal, newport;
	socklen_t len;
	struct sockaddr_in	servaddr, clientaddr, addr;
	char src[128], buf[PACKET_SIZE];
	
	// Close other sockets except for the one 
	while(head != NULL)
	{
		if(head->sockfd == sock)
		{
			isLocal = checkLocal (head-> ifi_addr, head->ifi_ntmaddr, head->ifi_subnetaddr, client_addr);
        
			if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			    err_sys("\nsocket creation error\n");

			if(isLocal)
			    if (setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, &optval, sizeof(optval)) < 0)
			    {
				printf("\nerror : socket option error.\n");
				close(sockfd);
				exit(2);
			    }

			/* binding to the serv ip */
			bzero(&servaddr, sizeof(servaddr));
			servaddr.sin_family     	 = AF_INET;
			servaddr.sin_addr.s_addr 	 = htonl(head->ifi_addr.sin_addr.s_addr);				
			servaddr.sin_port        	 = htons(0);
			if(bind(sockfd, (SA *) &servaddr, sizeof(servaddr))) 
			{
			    close(sockfd);
			    err_sys("error in bind\n");
			}
			/* determining the port number */
			len = sizeof(struct sockaddr);
			bzero(&addr, sizeof(struct sockaddr_in));  
			getsockname(sockfd, (struct sockaddr *)&addr, &len);
			inet_ntop(AF_INET, &addr.sin_addr, src, sizeof(src)); 
			newport = addr.sin_port;
			printf("\n\nClient : IP Address = %s \t Port No : %d ", src, ntohs(addr.sin_port)); 
			

			/* connect to client ip */
			if(connect(sockfd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr))!=0) 
			{
			    printf("init_connection_socket: failed to connect to client\n");
			}
			bzero(&addr, sizeof(struct sockaddr_in));  
			getpeername(sockfd, (struct sockaddr *)&addr, &len);
			inet_ntop(AF_INET, &addr.sin_addr, src, sizeof(src)); 
			printf("\nServer : IP Address = %s \t Port No : %d\n ", src, ntohs(addr.sin_port)); 
                        //sockfd = createConn(isLocal, client_addr, &clientaddr, &servaddr, &addr, head);		 	
		}
		else
		{	
			/* Closing the other sockets*/
	                if (close(head->sockfd) == -1)
		                err_sys("close error");
		}
		head = head->ifi_next;
	}

	/* Send new connection socket to client */
	sprintf(buf, "%d", newport);
	sendto(sock, buf, sizeof(buf), 0, (SA *)&client_addr, sizeof(client_addr));
	read(sockfd, buf, sizeof(buf));
	if (close(sock) == -1)
		err_sys("close error");
        printf("\nReceived 3rd Hand Shake : %s", buf);
        
        sendFile(sockfd, filename, client_addr, sendWin, 3, 10);

}

/*
 *	Add Client Connect Request to the List of Existing Connections
 */

void addNewClienttoExistingConnections(struct sockaddr_in clientInfo, int pid, struct sockaddr_in headaddr)	{   
	struct existing_connections *new_conn;
	new_conn = (struct existing_connections *)malloc(sizeof(struct existing_connections));
	new_conn->client_addr.sin_addr.s_addr = clientInfo.sin_addr.s_addr;
	new_conn->client_portNum = clientInfo.sin_port;
	new_conn->child_pid = pid;
	new_conn->serv_addr = headaddr;
	new_conn->next_connection = existing_conn;
	existing_conn = new_conn;
}

/*
 *	Check if Client Connection Request is in the List of Existing Connections
 */

int existing_connection(struct sockaddr_in *client_addr)    {
	struct existing_connections *conn_list = existing_conn;
	while(conn_list!=NULL){
		if(((conn_list->client_portNum == client_addr->sin_port) && (conn_list->client_addr.sin_addr.s_addr == client_addr->sin_addr.s_addr))){
			return 1;
		}	
		conn_list = conn_list->next_connection;
	}	
	return 0;
}

/* 
 * Executed when server-child process dies 
 */

static void exitChild_handler (int signo)
{
	int l;
    	pid_t pid, del_pid = 0;
	struct existing_connections *prev = existing_conn; 
	struct existing_connections *next = existing_conn;	

    	while ((pid = waitpid(-1, &l, WNOHANG)) > 0) {
        	printf("\n Child %d terminated (%d)\n", 
               	(int)pid, l);
    	}
	
	/* Delete the entry from existing connection list*/
	while(next!=NULL){
		if(next->child_pid == pid){
			if(next->child_pid==existing_conn->child_pid){
				existing_conn = next->next_connection;
			}
			else {
			prev->next_connection = next->next_connection;		
			}
		}
		prev=next;
		next= next->next_connection;
	}	
}	

void listenInterfaces(struct servStruct *servInfo, sendQ *sendWin)
{
	fd_set rset, allset;
	socklen_t len;
	
	msg packet_1HS;
        char msg[512];
	char src[128];
	
        int 	maxfdpl = -1, nready, pid;
	
        struct sockaddr_in clientInfo;
        struct InterfaceInfo *head  = servInfo->ifi_head;
        struct InterfaceInfo *interfaceList  = servInfo->ifi_head;
        	
        FD_ZERO(&allset);
	
        /* Setup select all interface sockfd's to listen for incoming client */
        while (head != NULL) 
        {
		maxfdpl = max (maxfdpl, head->sockfd);
		FD_SET(head->sockfd, &allset);
		head = head->ifi_next;
	}
        
	
	/* Server waits on select. When client comes, forks a child server to handle client */
	for (;;) {
		rset = allset;
		if ((nready = select(maxfdpl+1, &rset, NULL, NULL, NULL) ) < 0) {
			if (errno == EINTR ) {
				continue;
			}
			else {
				err_sys("error in select");
			}
		}
                head = interfaceList;
		while (head) {
			if(FD_ISSET(head->sockfd, &rset)) {
				len = sizeof(clientInfo);
				recvfrom(head->sockfd, &packet_1HS, sizeof(packet_1HS), 0, (struct sockaddr *)&clientInfo, &len);
				inet_ntop(AF_INET, &clientInfo.sin_addr, src, sizeof(src));
//				printf("\nClient Address  %s & port number %d ", src, clientInfo.sin_port);
                                printf("\nFilename requested for transfer : %s \n", packet_1HS.payload);

                                if( existing_connection(&clientInfo) == 1 ) { 
					printf("Duplicate connection request!");
				}
				else {
					if ((pid = fork()) == 0)    {
//                                              printf("\nClient Request Handler forked .");
						childRequestHandler(head->sockfd, interfaceList, clientInfo, packet_1HS.payload, sendWin);
						exit(0);
					}
					else	{
						addNewClienttoExistingConnections(clientInfo, pid, head->ifi_addr);
//						printf("\nelse to be done");
					}
				}
			}
			head = head->ifi_next;
		}
	}
}

int main(int argc, char **argv)
{	
	sendQ sendWin;
	existing_connections *existing_conn = NULL;
	struct servStruct *servInfo = loadServerInfo();
	initSenderQueue(&sendWin, servInfo->send_Window);
	
        listenInterfaces (servInfo, &sendWin);
	signal(SIGCHLD, exitChild_handler);
}
