#include	 "utils.h"
#include	 "server.h"
#include         <setjmp.h>
#include	 "unprtt.h"
#define		 LOOPBACK   "127.0.0.1"

/*	Global	declarations	*/
struct existing_connections *existing_conn;
static sigjmp_buf jmpbuf1, jmpbuf2;
static struct rtt_info rttinfo;

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

static void alarm_handler1 (int signo)
{
    //    printf("in child alarm handler for 2 hs\n");
    siglongjmp(jmpbuf1, 1);
}

/*
 * SIGNAL ALARM Handler
 */

static void alarm_handler2 (int signo)
{
    //    printf("in child alarm handler for 2 hs\n");
    siglongjmp(jmpbuf2, 1);
}

/*
 * Initialize Sender Queue 
 */

void initSenderQueue(sendQ *queue, int winsize, int slidwinstart)	{
    queue->buffer		=   (sendWinElem *) calloc (winsize, sizeof(sendWinElem));
    queue->winsize		=   winsize;
    queue->cwinsize		=   1;				
    queue->slidwinstart		=   slidwinstart;				
    queue->slidwinend		=   -1;  /// slidwinstart + queue->cwinsize;
    queue->ssthresh		=   winsize;
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
    elem->isRetransmit		=   0;
}

/*
 * Update the Retransmit count in sending Window
 */
void updateRetransmissionCount(sendQ *sendWin, int i)
{
    sendWin->buffer[ i % sendWin->winsize ].retranx++;
}

/*
 * Set the Retransmit bit in sending Window
 */

void setRetransmitFlag(sendQ *sendWin, int i)
{
    sendWin->buffer[ i % sendWin->winsize ].isRetransmit = 1;
}

/*
 * Reset the Present bit in Sending Window
 */

void resetRetransmitFlag(sendQ *sendWin, int i)
{
    sendWin->buffer[ i % sendWin->winsize ].isRetransmit = 0;
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
 * Get Message Type in Sending Window element
 */

int getMsgType(sendQ *queue, int seqnum)
{
    return queue->buffer[seqnum % queue->winsize].packet.header.msg_type;
}

/*
 * Get Retransmission Count in Sending Window element
 */

int getRetranxCount(sendQ *queue, int seqnum)
{
    return queue->buffer[seqnum % queue->winsize].retranx;
}

/*
 * Set Timestamp in Sending Window element
 */

void setTimeStamp(sendQ *queue, int seqnum, int ts)
{
    queue->buffer[seqnum % queue->winsize].packet.header.timestamp = ts;
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

void sendFile(int sockfd, char filename[PAYLOAD_CHUNK_SIZE], struct sockaddr_in client_addr, sendQ *sendWin, int seqno, int adwin)	
{
    char buf[PAYLOAD_CHUNK_SIZE];
    int fp, i, finseq = 65536;
    int seqnum = seqno, nbytes, advwin = adwin, ts = 0, msgtype, cwindow, isRetransmit = 0;
    int prevAck = -1, dupAckCount = 0, sendPack = 0, addi = 0, addiseqnum = -1;
    uint32_t timestamp;
    hdr header;
    msg datapacket, ack_ack, ack, checkWin, checkWinAck;
    sendWinElem sendelem;

    fp = open(filename, O_RDONLY, S_IREAD);

ackRcvdresendNewPacketsCongWin:

    if (adwin == 0)
    {
	while (1)
	{
	    sleep(1);
	    createCheckWinPacket(&checkWin, WIN_CHECK, seqnum, adwin, ts);
	    send(sockfd, &checkWin, sizeof(checkWin), 0);
	    recv(sockfd, &checkWinAck, sizeof(checkWinAck), 0);
	    if(checkWinAck.header.seq_num == seqnum && checkWinAck.header.msg_type == WIN_UPDATE)
	    {
		adwin = checkWinAck.header.adv_window;
		if (adwin > 0)
		    break;
	    }
	}
    }

    
    if (sigsetjmp(jmpbuf2, 1) != 0) {
	/* Update SS_THRESH */	
	sendWin->ssthresh   =	sendWin->cwinsize / 2;
	sendWin->cwinsize   =	1;
	isRetransmit = 1;
	addi = 0;
	if (rtt_timeout(&rttinfo, getRetranxCount(sendWin, seqnum)) == -1) {
	    printf("\nNo response from the client for packet # %d after 12 retransmission attempts. Giving up.\n", sendWin->slidwinstart);
	    exit(0);
	}
	printf("\n ============== REQUEST TIMED OUT !!! Retransmitting Packet Seq # %d, Retransmission attempt number: %d =============\n", sendWin->slidwinstart, getRetranxCount(sendWin, sendWin->slidwinstart));
	printf("\n ==================================== SLOW-START STARTED !!!. New CWIN : %d, New SS_THRESH %d ===========================\n", seqnum, sendWin->cwinsize, sendWin->ssthresh);
	goto timeOutReSend;
    }

timeOutReSend:
    timestamp = rtt_ts(&rttinfo); 
    rtt_set_timer(rtt_start(&rttinfo));
    
    cwindow = minWin(adwin, sendWin->cwinsize, sendWin->winsize);
    seqnum = sendWin->slidwinstart;
    printf("\n------------------------------------------------------------------------------------------------------------------------------------");
    printf("\nSending from Seq # %d, min(CWIN, Advertising Window) : %d , CWIN : %d , Advertising Window : %d, SS_THRESH %d.", seqnum, cwindow, sendWin->cwinsize, adwin, sendWin->ssthresh);
    printf("\n------------------------------------------------------------------------------------------------------------------------------------\n");

    for ( i = 0; i < cwindow && seqnum <= finseq; i++)	{

	if (sendWin->cwinsize >= sendWin->ssthresh)
	{
	    if (addi == 0)
	    {
		addiseqnum = seqnum + sendWin->ssthresh;
		printf("\n ============ CWIN threshold reached. ADDITIVE INCREASE started at %d . Next CWIN update @ seqnum %d, Current CWIN %d ============\n", seqnum, addiseqnum, sendWin->cwinsize);
	    }
	    addi = 1;
//	    printf("\nADDI starting at %d . Wait for seqnum %d, cwin %d", seqnum, addiseqnum, sendWin->cwinsize);
	}
	else 
	{
	    addi = 0;
	}

	/* Check Retransx Flag */
	if (isRetransmit == 1)
	{
	    datapacket = sendWin->buffer[seqnum % sendWin->winsize].packet;
	    printf("\nRe-Sending Packet Seq # %d", datapacket.header.seq_num);
	    sendPack = 1;
	}
	else if (seqnum <= sendWin->slidwinend || sendWin->buffer[seqnum % sendWin->winsize].isPresent == 1) 
	{	
	    /* Packet is already in transit */
	    printf("\nSkipping Packet Seq # %d", seqnum);
	    sendPack = 0;
	}
	else
	{
	    /* Build New Packet to send */
	    if (seqnum > sendWin->slidwinend)
		sendWin->slidwinend = seqnum;

	    nbytes = read(fp, buf, PAYLOAD_CHUNK_SIZE-1);
	    buf[nbytes] = '\0';
	    //printf("\nBuf : %s", buf);
	    if (nbytes < PAYLOAD_CHUNK_SIZE - 1)
	    {
		msgtype		= FIN;
		finseq		= seqnum;
	    }
	    else
		msgtype        = DATA_PAYLOAD;

	    createHeader(&header, msgtype, seqnum, advwin, ts);
	    createMsgPacket(&datapacket, header, buf, nbytes+1);
	    createSenderElem(&sendelem, datapacket, seqnum);
	    addToSenderQueue(sendWin, sendelem);
	    printf("\nSending  Packet Seq # %d", datapacket.header.seq_num);
	    sendPack = 1;
	}

	if (sendPack ==  1)
	{
	    send(sockfd, &datapacket, sizeof(datapacket), 0);
	    updateRetransmissionCount(sendWin, datapacket.header.seq_num); 
	}

	msgtype = getMsgType(sendWin, seqnum);

	setTimeStamp(sendWin, seqnum, timestamp);
	
	if (msgtype == FIN)
	    break;

	seqnum = seqnum + 1;

	bzero(&buf, PAYLOAD_CHUNK_SIZE);
	bzero(&datapacket, sizeof(datapacket));

    }
    printf("\nSender Window State after all send  : ");
    printSendingBuffer(sendWin);

    while (1)
    {
	recv(sockfd, &ack, sizeof(ack), 0);

	if (sendWin->slidwinstart == ack.header.seq_num)
	{
	    dupAckCount++;
	    if (dupAckCount == 3)
	    {
		isRetransmit = 1;
		sendWin->cwinsize /= 2;
		sendWin->ssthresh = sendWin->cwinsize;
		printf("\n ================================================   FAST RETRANSMIT !!!  ========================================================");
		printf("\n =========================== 3 Duplicate ACK's with ACK packet seq # %d, New CWIN # %d, New Threshold # %d =========================\n", sendWin->slidwinstart, sendWin->cwinsize, sendWin->ssthresh);
		break;
	    }
	}
	else
	{
	    while (ack.header.seq_num > sendWin->slidwinstart)
	    {
		resetPresentFlag(sendWin, sendWin->slidwinstart);
		sendWin->slidwinstart++;
		if (addi != 1)
		    sendWin->cwinsize += 1;
	    }
	    if (addi)
	    {
		if (ack.header.seq_num >= addiseqnum)
		{
		    sendWin->cwinsize += 1;
		    addiseqnum = ack.header.seq_num + sendWin->cwinsize;
		}
	    }
	    
	    dupAckCount = 0;
	    isRetransmit = 0;
	    rtt_set_timer(0);
	    rtt_stop(&rttinfo, timestamp);
	    break;
	}
    }


    seqnum = ack.header.seq_num;    
    adwin = ack.header.adv_window;

    printf("\nAck Received # %d. Advertising Window : %d.", ack.header.seq_num, ack.header.adv_window);
    //printf("\nAck Received # %d. Adv. Window # %d .  Please start sending : %d packets from seqnum : %d, type : %d", ack.header.seq_num, ack.header.adv_window, sendWin->cwinsize, ack.header.seq_num, ack.header.msg_type);
    printf("\nSender Window State on Ack received : ");
    printSendingBuffer(sendWin);

    if (ack.header.msg_type != FIN_ACK)
	goto ackRcvdresendNewPacketsCongWin;
    
    
    createAckPacket(&ack_ack, FIN_ACK_ACK, 0, 0, 0);
    send(sockfd, &ack_ack, sizeof(ack_ack), 0);

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
	    {
		printf("\nClient IP Address is local.\n");
		if (setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, &optval, sizeof(optval)) < 0)
		{
		    printf("\nerror : socket option error.\n");
		    close(sockfd);
		    exit(2);
	    	}
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
	    printf("\nClient : IP Address = %s \t New Ephemeral Port No : %d ", src, ntohs(addr.sin_port)); 


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
    rtt_init(&rttinfo);   /*To be done just once*/
    int retransmit_attempt = 0;
    uint32_t timestamp;

resend_HS2:
    sendto(sock, (void *)&pack_2HS, sizeof(pack_2HS), 0, (SA *)&client_addr, sizeof(client_addr));
    timestamp = rtt_ts(&rttinfo); 
    retransmit_attempt++;

    if(retransmit_attempt>1){
	//printf("retransmitting from the other port also. attempt num : %d\n", retransmit_attempt);
	sendto(sockfd, (void *)&pack_2HS, sizeof(pack_2HS), 0, (SA *)&client_addr, sizeof(client_addr)); 
    }


    if (sigsetjmp(jmpbuf1, 1) != 0) {
	if (rtt_timeout(&rttinfo, retransmit_attempt == -1)) {
	    printf("No response from the client for 2HS after max retransmission attempts. Giving up.\n");
	    exit(0);
	}
	printf("Request timed out. Retransmitting 2HS from both the ports now ...: attempt number: %d\n", retransmit_attempt);
	goto resend_HS2;
    }
    
    // alarm(3);
    rtt_set_timer(rtt_start(&rttinfo));

    /* Recieving 3 Handshake */
    hdr header3;
    do{
	//  printf("Waiting for 3 handshake\n");	
	read(sockfd, buf, sizeof(buf)); 
	msg *pack_3HS = (msg *)buf;
	header3 = pack_3HS->header;
	//printf("received msg type num: %d\n", header3.msg_type);
    }	while ( header3.msg_type != SYN_ACK_HS3 );

    //alarm(0);
    rtt_set_timer(0);
    rtt_stop(&rttinfo, timestamp);
    /* Closing the listening socket after third handshake is recieved*/
    if (close(sock) == -1)
	err_sys("close error");

    if(header3.msg_type == SYN_ACK_HS3){	
	printf("\nHandshake 3 recieved from client.\n");
	//printf("\nReceived 3rd Hand Shake Successfully");
    }
    
    Signal(SIGALRM, alarm_handler2);
    printf("\nStart file transfer Seq number = %d, Initial Advertising window = %d\n", header3.seq_num, header3.adv_window);
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
		    printf("\nHandshake 1 recieved from client. \n");
		}
		if( existing_connection(&clientInfo) == 1 ) { 
		    printf("Duplicate connection request!");
		}
		else {
		    if ((pid = fork()) == 0)    {
			printf("File to be send to client: %s \n", packet_1HS.payload);
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
    Signal(SIGALRM, alarm_handler1);

    sendQ sendWin;
    struct servStruct *servInfo = loadServerInfo();
    existing_connections *existing_conn = NULL;

    initSenderQueue(&sendWin, servInfo->send_Window, 3);
    listenInterfaces (servInfo, &sendWin);
}
