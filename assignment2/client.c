#include	 "utils.h"
#include	 "unprtt.h"
#include	 "client.h"
#include	 <setjmp.h>

#define LOOPBACK "127.0.0.1"

/*
 * Function to get subnet mask bits
 */

static sigjmp_buf jmpbuf;

int getSubnetCount(unsigned long netmsk)
{
    int count = 0;
    while (netmsk)
    {
	netmsk = netmsk & (netmsk - 1);
	count++;
    }
    return count;
}

/* 
 * Signal Handler for SIGALARM
 */

static void alarm_handler (int signo)
{
    siglongjmp(jmpbuf, 1);
}


/* 
 * Function to check if client and server have same host network. 
 */

int checkLocal (struct clientStruct **cliInfo)
{
        int isLocal;  
        struct sockaddr_in sa, *subnet;
        struct clientStruct *temp = *cliInfo;
        struct InterfaceInfo *head = temp->ifi_head;
        char src[128];
	int maxlcs = -1, lcs;

        inet_ntop(AF_INET, &temp->serv_addr.sin_addr, src, sizeof(src));
        if (strcmp(src, LOOPBACK) == 0)
        {
                //printf ("\nServer IP is Loopback Address. Client IP = 127.0.0.1\n");
                temp->cli_addr = temp->serv_addr;
                cliInfo = &temp;
		return 1;
        }
        
        while (head)
        {
		if ((temp->serv_addr.sin_addr.s_addr & head->ifi_ntmaddr.sin_addr.s_addr) == head->ifi_subnetaddr.sin_addr.s_addr)
                {
			lcs = getSubnetCount(head->ifi_ntmaddr.sin_addr.s_addr);
			if (lcs > maxlcs)
			{
			    maxlcs = lcs;
			    temp->cli_addr = head->ifi_addr;  
			}
		}
		head = head->ifi_next;
        }
	cliInfo = &temp;
	if (maxlcs == -1)
	    return 0;
	else
	    return 1;
	
}

/*
 * Assign Non Local Client IP
 */

void assignCliIPnonLocal(struct clientStruct **cliInfo)
{
        struct clientStruct *temp = *cliInfo;
        struct InterfaceInfo *head = temp->ifi_head;
        char src[128];
        
        inet_ntop(AF_INET, &head->ifi_addr.sin_addr, src, sizeof(src));
        if (strcmp(src, LOOPBACK) == 0)
        {
                if (head->ifi_next)
                {
                        temp->cli_addr = head->ifi_next->ifi_addr;
                }
        }
        else
                temp->cli_addr = head->ifi_addr;

        cliInfo = &temp;
	return;
}

/*
 * Create Initial Connection
 */

int createInitialConn(struct clientStruct **cliInfo, int isLocal)
{
        int sockfd, optval = -1;
        struct sockaddr_in clientIP, serverIP, addr;
        struct clientStruct *temp = *cliInfo;
	int len;
	char src[128];
        
        if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                err_sys("\nsocket creation error\n");

        if(isLocal)
                if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
                {
                        printf("\nerror : socket option error.\n");
                        close(sockfd);
                        exit(2);
                }
        
        /* Bind socket to client IP */
        bzero(&clientIP, sizeof(clientIP));
        clientIP.sin_family =   AF_INET;
        clientIP.sin_port   =   htons(0);
        clientIP.sin_addr   =   temp->cli_addr.sin_addr;
       
	if (bind(sockfd, (SA *) &clientIP, sizeof(clientIP)) < 0)
                err_sys("\nbind error\n");
        
	len = sizeof(struct sockaddr);
        bzero(&addr, sizeof(struct sockaddr_in));  
        getsockname(sockfd, (struct sockaddr *)&addr, &len);
	inet_ntop(AF_INET, &addr.sin_addr, src, sizeof(src)); 
        printf("\nClient IP Address = %s \t Port No : %d\n ", src, ntohs(addr.sin_port)); 
        temp->cli_portNum = ntohs(addr.sin_port);

        /* Connect socket to Server IP */
        bzero(&serverIP, sizeof(serverIP));
        serverIP.sin_family =   AF_INET;
        serverIP.sin_port   =   htons(temp->serv_portNum);
        serverIP.sin_addr   =   temp->serv_addr.sin_addr;
       
	if (connect(sockfd, (SA *) &serverIP, sizeof(serverIP)) < 0)
                err_sys("\nconnect error\n");
        
        bzero(&addr, sizeof(struct sockaddr_in));  
        getpeername(sockfd, (struct sockaddr *)&addr, &len);
	inet_ntop(AF_INET, &addr.sin_addr, src, sizeof(src)); 
	printf("\nServer IP Address = %s \t Port No : %d\n ", src, ntohs(addr.sin_port)); 
        
        cliInfo = &temp;
        return sockfd;

}

/*
 * Initialize Receiver Queue 
 */

void initRecvQueue(recvQ *queue, int winsize, int advwinstart)	{
    queue->buffer		=   (recvWinElem *) calloc (winsize, sizeof(recvWinElem));
    queue->winsize		=   winsize;
    queue->advwinsize		=   winsize;				
    queue->advwinstart		=   advwinstart;				
    queue->readpacketidx    	=   -1;
}

/*
 * Create a Receiver Queue Element
 */

void createRecvElem (recvWinElem *elem, msg datapacket, int seqnum)	{
    //printf("Creating Element at seq num = %d", seqnum);
    elem->packet		=   datapacket;
    elem->seqnum		=   seqnum;					
    elem->retranx		=   0;					
    elem->isValid   		=   1;					
}

/*
 * Add to Receiver Queue
 */

int addToReceiverQueue(recvQ *queue, recvWinElem elem)	{
    printf("\nAdded To Receiver Queue Seq Num : %d ", elem.seqnum);
    int idx = elem.seqnum % queue->winsize;
    if (queue->buffer[idx].isValid == 0)
    {
	if (queue->readpacketidx == -1)
	    queue->readpacketidx = elem.seqnum;
    
	queue->buffer[idx]	    =	elem;
	queue->buffer[idx].isValid  =	1;
	if (queue->advwinstart == elem.seqnum)
	{
	    queue->advwinsize--;
	    if (queue->advwinsize == 0)
	    {
		queue->advwinstart++;
		printf("\nReciever Buffer is full. Waiting for consumer thread to read data.");
		return 1;
	    }
	    while (queue->buffer[ ++queue->advwinstart % queue->winsize ].isValid)
	    {
		queue->advwinsize--;
	    }
	}
    }
    else
	return 0;

    return 1;
}

/*
 * Print the Receiving Window
 */

void printReceivingBuffer(recvQ *recvWin)	{
    int i;
    for (i = 0; i < recvWin->winsize ; i++)
    {
	if (recvWin->buffer[i].isValid)
	    printf("%8d", recvWin->buffer[i].seqnum);
	else
	    printf("    XXXX");
    }	
	    printf("\n");

}

/* 
 * Read file contents over the network sent by the Reliable UDP Server.
 */

void recvFile(int sockfd, recvQ *queue, struct sockaddr_in serverInfo, int awin)
{
	msg m, ack;
	recvWinElem elem;
	int msgtype, advwin = awin, ts = 0, status;
	printf("\nData Received on Client : \n");
	while (1)
	{
	    //printf("Printinf Seq Num\n");
	    recv(sockfd, &m, sizeof(m), 0);
	    createRecvElem (&elem, m, m.header.seq_num);
	    status = addToReceiverQueue(queue, elem);

	    //printf("Printinf Seq Num %d", m.header.seq_num);
	    //printf("Printinf Payload %s", m.payload);
	    //printf("%s", m.payload);
	    printf("Received Packet : %d", m.header.seq_num);
	    printf("\nReceived Queue State \t"); 
	    printReceivingBuffer(queue);
	    printf("\n");
	    if (m.header.msg_type == FIN)
		msgtype = FIN_ACK;
	    else
		msgtype = DATA_ACK;
	    
	    createAckPacket(&ack, msgtype, queue->advwinstart, queue->advwinsize, ts);
	    send(sockfd, &ack, sizeof(ack), 0);
	    
	    if (m.header.msg_type == FIN)
		break;
		//msgtype = FIN_ACK;
	    //else
		//msgtype = ACK;

	}
}

int main(int argc, char **argv)
{	
	struct clientStruct  *clientInfo        =       loadClientInfo();
        int isLocal, sockfd, advwin = 0, ts = 0;
	char src[128];
	char recvBuff[1024];	
	struct sockaddr_in servIP;	
	int retrans_times = 0;

	Signal(SIGALRM, alarm_handler);
        
	if (clientInfo)
                isLocal = checkLocal(&clientInfo);
        
	if (isLocal)
	    printf("\nServer IP Address is local\n");
	else
	{
                assignCliIPnonLocal(&clientInfo);
                printf("\nServer IP is non local.\n");
        }

	memset(recvBuff, '0',sizeof(recvBuff));

	sockfd = createInitialConn(&clientInfo, isLocal);  

	struct rtt_info rttinfo;
	//rtt_init(&rttinfo);
	//rtt_newpack(&rttinfo);
	msg pack_1HS;
        hdr header;
	advwin = clientInfo->rec_Window;
	createHeader(&header, SYN_HS1, 1, advwin, ts);
        createMsgPacket(&pack_1HS, header, clientInfo->fileName, sizeof(clientInfo->fileName));
	
send_1HS_again:

	write(sockfd, &pack_1HS, sizeof(pack_1HS));
	retrans_times++;

    if (sigsetjmp(jmpbuf, 1) != 0) {
            if (retrans_times > 12) {
	                printf("No response from the server for 1HS after max retransmission attempts. Giving up.\n");
			exit(0);
	   }
	   printf("Request timed out. Retransmitting 1HS ...");
	   goto send_1HS_again;
    }

	alarm(3);

	hdr header2;
	char newPort[PAYLOAD_CHUNK_SIZE]; 
	
	do{	
	    read(sockfd, recvBuff, sizeof(recvBuff));
	    msg *pack_2HS = (msg *)recvBuff;
	    header2 = pack_2HS->header;
	    memcpy(newPort, pack_2HS->payload, PAYLOAD_CHUNK_SIZE);
	}while(header2.msg_type != ACK_HS2);

	alarm(0);

	if(header2.msg_type == ACK_HS2){
	    printf(" 2 HS recvd : New port number recieved from Server : %d \n", ntohs(atoi(newPort)));
	}
	clientInfo->serv_portNum = htons(atoi(newPort));

	/* Connect to the server on new port */
	bzero(&servIP, sizeof(servIP));
        servIP.sin_family =   AF_INET;
        servIP.sin_port   =   htons(clientInfo->serv_portNum);
        servIP.sin_addr   =   clientInfo->serv_addr.sin_addr;
	if (connect(sockfd, (SA *) &servIP, sizeof(servIP)) < 0)
                err_sys("\nconnect error\n");

	/* Sending the 3-hand shake */
	sleep(4);
	
	msg pack_3HS;
	hdr header3;
	createHeader(&header3, SYN_ACK_HS3, 3, advwin, ts);
	createMsgPacket(&pack_3HS, header3, NULL, 0);
	
	write(sockfd, &pack_3HS, sizeof(pack_3HS));    
	//printf("Sent third handshake, waiting for data from server\n");

	//printf("Sent third handshake, waiting for data from server  now \n");
	recvQ queue;
	
	initRecvQueue(&queue, clientInfo->rec_Window, 3);
	
	recvFile(sockfd, &queue, servIP, clientInfo->rec_Window);
    
}


