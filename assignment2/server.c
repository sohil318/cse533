#include	 "utils.h"
#include	 "server.h"
#include         <setjmp.h>

#define LOOPBACK "127.0.0.1"

/*	Global	declarations	*/
struct existing_connections *existing_conn;
static sigjmp_buf jmpbuf;

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
 * SIGNAL ALARM Handler
 */

static void alarm_handler (int signo)
{
    //    printf("in child alarm handler for 2 hs\n");
    siglongjmp(jmpbuf, 1);
}

/*
 * Initialize Sender Queue 
 */

void initSenderQueue(sendQ *queue, int winsize, int slidwinstart)	{
    queue->buffer		=   (sendWinElem *) calloc (winsize, sizeof(sendWinElem));
    queue->winsize		=   winsize;
    queue->cwinsize		=   3;				
    queue->slidwinstart		=   slidwinstart;				
    queue->slidwinend		=   -1;  /// slidwinstart + queue->cwinsize;
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
 * Set the Present bit in sending Window
 */

void setPresentFlag(sendQ *sendWin, int i)
{
    sendWin->buffer[ i % sendWin->winsize ].isPresent = 1;
}

/*
 * Reset the Present bit in Sending Window
 */

void resetPresentFlag(sendQ *sendWin, int i)
{
    sendWin->buffer[ i % sendWin->winsize ].isPresent = 0;
}

/*
 * Minimum of 3 numbers
 */

int minWin(int x, int y, int z)
{
    if ( x <= y && x <= z )
	return x;
    else if ( y <= z && y <= x )
	return y;

    return z;
}

/*
 * Print the Sending Window
 */

void printSendingBuffer(sendQ *sendWin)	{
    int i;
    for (i = 0; i < sendWin->winsize ; i++)
    {
	if (sendWin->buffer[i].isPresent)
	    printf("%8d", sendWin->buffer[i].seqnum);
	else
	    printf("    XXXX");
    }	
	    printf("\n");
}

/* 
 * Write file contents over the connection socket.
 */

void sendFile(int sockfd, char filename[PAYLOAD_CHUNK_SIZE], struct sockaddr_in client_addr, sendQ *sendWin, int seqno, int adwin)	{
    char buf[PAYLOAD_CHUNK_SIZE];
    int fp, i;
    int seqnum = seqno, nbytes, advwin = adwin, ts = 0, msgtype, cwindow;
    hdr header;
    msg datapacket, ack;
    sendWinElem sendelem;

    fp = open(filename, O_RDONLY, S_IREAD);
    
ackRcvdresendNewPacketsCongWin:
    
    cwindow = minWin(adwin, sendWin->cwinsize, sendWin->winsize);
    printf("\n\nSending from %d packet,  %d number of packets.\n", seqnum, cwindow);

    for ( i = 0; i < cwindow; i++)	{

	if (seqnum <= sendWin->slidwinend || sendWin->buffer[i].isPresent == 1)
	{	
	    /* Packet is already in transit */
	    printf("\nSkipping Packet : %d", seqnum);

//	    sendelem = sendWin->buffer[ seqnum % sendWin->winsize ];
//	    sendelem.retranx++;
//	    datapacket = sendelem.packet;
//	    sendWin->buffer[ seqnum % sendWin->winsize ] = sendelem;
	}
	else
	{
	    if (seqnum > sendWin->slidwinend)
		sendWin->slidwinend = seqnum;

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
	    printf("\nSending Seq # %d", datapacket.header.seq_num);
	    send(sockfd, &datapacket, sizeof(datapacket), 0);
	}
	printf("\n\tSender Window State : ");
	printSendingBuffer(sendWin);
	//printf("\n");

	seqnum++;

	if (msgtype == FIN)
	    break;
	
	bzero(&buf, PAYLOAD_CHUNK_SIZE);
	bzero(&datapacket, sizeof(datapacket));

    }

//    recv(sockfd, &ack, sizeof(ack), 0);
/*    if (ack.header.seq_num == sendWin->slidwinstart + 1)
    {
	resetPresentFlag(sendWin, ack.header.seq_num - 1);
//	sendWin->buffer[ack.header.seq_num % sendWin->winsize].isPresent = 0;
	sendWin->slidwinstart++;
    }
    if (ack.header.msg_type == FIN_ACK)
	return;
*/
    if (msgtype == FIN)
    {
	while (1)
	{
	    recv(sockfd, &ack, sizeof(ack), 0);
	    
	    printf("\nAck # %d received. Adv Win : %d", ack.header.seq_num - 1, ack.header.adv_window);
	    if (ack.header.seq_num == sendWin->slidwinstart + 1)
	    {
		resetPresentFlag(sendWin, ack.header.seq_num - 1);
//		sendWin->buffer[ack.header.seq_num % sendWin->winsize].isPresent = 0;
		sendWin->slidwinstart++;
	    }
	    printf("\n\tSender Window State : ");
	    printSendingBuffer(sendWin);

	    if (ack.header.msg_type == FIN_ACK)
		break;
	    
	}
    }
    else
    {
	//printf("\nReceiving Ack #");
	recv(sockfd, &ack, sizeof(ack), 0);
	if (ack.header.seq_num == sendWin->slidwinstart + 1)
	{
	    resetPresentFlag(sendWin, ack.header.seq_num - 1);
//	    sendWin->buffer[ack.header.seq_num % sendWin->winsize].isPresent = 0;
	    sendWin->slidwinstart++;
	}
	sendWin->cwinsize += 1;
	seqnum = ack.header.seq_num;    //sendWin->slidwinend;
	adwin = ack.header.adv_window;
	//sendWin->slidwinend += 2;
	printf("\nAck # %d. Adv. Window # %d .  Please start sending : %d packets from seqnum : %d", ack.header.seq_num - 1, ack.header.adv_window, sendWin->cwinsize, ack.header.seq_num);
	printf("\n\tSender Window State : ");
	printSendingBuffer(sendWin);

	goto ackRcvdresendNewPacketsCongWin;
    }
    
}

/* 
 * Setup connection with using new port 
 */

int createConn(int isLocal, struct sockaddr_in client_addr, struct sockaddr_in *clientaddr, struct sockaddr_in *servaddr, struct sockaddr_in *addr, struct InterfaceInfo *head)
{
    int sockfd, optval = -1;
    socklen_t len;
    char src[128];

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	err_sys("\nsocket creation error\n");

    printf("socket completed");
    if(isLocal)
	if (setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, &optval, sizeof(optval)) < 0)
	{
	    printf("\nerror : socket option error.\n");
	    close(sockfd);
	    exit(2);
	}
    printf("socket completed");

    /* binding to the serv ip */
    bzero(&servaddr, sizeof(servaddr));
    servaddr->sin_family     	 = AF_INET;
    servaddr->sin_addr.s_addr 	 = htonl(head->ifi_addr.sin_addr.s_addr);				
    servaddr->sin_port        	 = htons(0);
    if(bind(sockfd, (SA *) servaddr, sizeof(servaddr))) 
    {
	close(sockfd);
	err_sys("error in bind\n");
    }
    printf("bind completed");
    /* determining the port number */
    len = sizeof(struct sockaddr);
    bzero(&addr, sizeof(struct sockaddr_in));  
    getsockname(sockfd, (struct sockaddr *)addr, &len);
    inet_ntop(AF_INET, &addr->sin_addr, src, sizeof(src)); 
    printf("\nClient is connected to Server at IP Address = %s \t Port No : %d\n ", src, ntohs(addr->sin_port)); 

    /* connect to client ip */
    bzero(&clientaddr, sizeof(clientaddr));
    clientaddr->sin_family              = AF_INET;
    clientaddr->sin_addr.s_addr         = htonl(client_addr.sin_addr.s_addr);
    clientaddr->sin_port                = addr->sin_port;
    if(connect(sockfd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr))!=0) 
    {
	printf("init_connection_socket: failed to connect to client\n");
    }
    return sockfd;
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
    char src[128], buf[PAYLOAD_CHUNK_SIZE];

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

    /* Send new connection socket to client in 2 Handshake*/
    msg pack_2HS;
    hdr header2;
    sprintf(buf, "%d", newport);
    createHeader(&header2, ACK_HS2, 2, 0, 0);
    createMsgPacket(&pack_2HS, header2, buf, sizeof(buf));
    int retransmit_attempt = 0;

resend_HS2:
    sendto(sock, (void *)&pack_2HS, sizeof(pack_2HS), 0, (SA *)&client_addr, sizeof(client_addr));
    retransmit_attempt++;

    if(retransmit_attempt>1){
	//printf("retransmitting from the other port also. attempt num : %d\n", retransmit_attempt);
	sendto(sockfd, (void *)&pack_2HS, sizeof(pack_2HS), 0, (SA *)&client_addr, sizeof(client_addr)); 
    }

    if (sigsetjmp(jmpbuf, 1) != 0) {
	if (retransmit_attempt > 12) {
	    printf("No response from the client for 2HS after max retransmission attempts. Giving up.\n");
	    exit(0);
	}
	printf("Request timed out. Retransmitting 2HS from both the ports now ...: attempt number: %d\n", retransmit_attempt);
	goto resend_HS2;
    }                                                                                                                                                                                                   
    alarm(3);

    /* Recieving 3 Handshake */
    hdr header3;
    do{
	//  printf("Waiting for 3 handshake\n");	
	read(sockfd, buf, sizeof(buf)); 
	msg *pack_3HS = (msg *)buf;
	header3 = pack_3HS->header;
	//printf("received msg type num: %d\n", header3.msg_type);
    }while(header3.msg_type != SYN_ACK_HS3);

    alarm(0);

    /* Closing the listening socket after third handshake is recieved*/
    if (close(sock) == -1)
	err_sys("close error");

    if(header3.msg_type == SYN_ACK_HS3){	
	printf("\nReceived 3rd Hand Shake Successfully");
    }

    printf("\n Start file transfer Seq number = %d, Initial Advertising window = %d\n", header3.seq_num, header3.adv_window);
    sendFile(sockfd, filename, client_addr, sendWin, header3.seq_num, header3.adv_window);

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

int existing_connection(struct sockaddr_in *client_addr){
    struct existing_connections *conn_list = existing_conn;
    while(conn_list!=NULL){
	if(((conn_list->client_portNum == client_addr->sin_port) && (conn_list->client_addr.sin_addr.s_addr == client_addr->sin_addr.s_addr)))	{
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
    char msg[PAYLOAD_CHUNK_SIZE];
    char src[128];

    int maxfdpl = -1, nready, pid;
    sigset_t signal_set;

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
		hdr header1 = packet_1HS.header;
		if(header1.msg_type == SYN_HS1)	{
		    printf("\n1 Handshake recieved from client \n");
		}
		if( existing_connection(&clientInfo) == 1 ) { 
		    printf("Duplicate connection request!");
		}
		else {
		    if ((pid = fork()) == 0)    {
			printf("Filename requested for transfer by the client: %s \n", packet_1HS.payload);
			//                                              printf("\nClient Request Handler forked .");
			childRequestHandler(head->sockfd, interfaceList, clientInfo, packet_1HS.payload, sendWin);
			exit(0);
		    }
		    else
		    {
			sigemptyset(&signal_set);
			sigaddset(&signal_set, SIGCHLD);
			sigprocmask(SIG_BLOCK, &signal_set, NULL);
			addNewClienttoExistingConnections(clientInfo, pid, head->ifi_addr);
			//						printf("\nelse to be done");
			sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
		    }
		}
	    }
	    head = head->ifi_next;
	}
    }
}

int main(int argc, char **argv)
{	

    Signal(SIGCHLD, exitChild_handler);
    Signal(SIGALRM, alarm_handler);

    sendQ sendWin;
    struct servStruct *servInfo = loadServerInfo();
    existing_connections *existing_conn = NULL;

    initSenderQueue(&sendWin, servInfo->send_Window, 3);
    listenInterfaces (servInfo, &sendWin);
}
