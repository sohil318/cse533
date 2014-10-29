#include	 "utils.h"
#define LOOPBACK "127.0.0.1"

/*	Global	declarations	*/
struct existing_connections *existing_conn;

/* 
 * Function to check if client and server have same host network. 
 */

int checkLocal (struct sockaddr_in serverIP, struct sockaddr_in serverIPnmsk, struct sockaddr_in serverIPsubnet, struct sockaddr_in clientIP)
{
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
 * Write file contents over the connection socket.
 */

void sendFile(int sockfd, char filename[496])
{
         char buf[PAYLOAD_CHUNK_SIZE];
         int fp;
	 int seqnum = 3, nbytes;
	 hdr header;
	 struct msghdr datamsg;
	 struct iovec dataiovec[2];
	
         fp = open(filename, O_RDONLY, S_IREAD);
         while ((nbytes = read(fp, buf, PAYLOAD_CHUNK_SIZE-1)) <= PAYLOAD_CHUNK_SIZE-1)
	 { 
	    buf[nbytes] = '\0';
	    //printf("\nBuf : %s", buf);
	    if (nbytes < PAYLOAD_CHUNK_SIZE - 1)
		header.msg_type        = FIN;
	    else
		header.msg_type        = DATA_PAYLOAD;
	    header.seq_num         = seqnum;
	    header.adv_window      = 0;
	    header.timestamp       = 0;
	    
	    datamsg.msg_name       = NULL;
	    datamsg.msg_namelen    = 0;
	    datamsg.msg_iov        = dataiovec;
	    datamsg.msg_iovlen     = 2;

	    dataiovec[0].iov_base   = (void *)&header;
	    dataiovec[0].iov_len    = sizeof(hdr);
	    dataiovec[1].iov_base   = (void *)buf;
	    dataiovec[1].iov_len    = PAYLOAD_CHUNK_SIZE;
	    
	    sendmsg(sockfd, &datamsg, 0);
	    seqnum++;
	    if (header.msg_type == FIN)
		break;
	    bzero(&buf, PAYLOAD_CHUNK_SIZE);                                                                                                                                  
	    bzero(&header, sizeof(hdr));
	    bzero(&datamsg, sizeof(datamsg));
         }
	 //printf("%s", buf);
}


/* Setup connection with using new port */
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

/* Forked server child for service requesting client */ 
int childRequestHandler(int sock, struct InterfaceInfo *head, struct sockaddr_in client_addr, char filename[496])
{
	printf ("Handling forked Child");
	int sockfd, optval = -1, isLocal;
	socklen_t len;
	struct sockaddr_in	servaddr, clientaddr, addr;
	char src[128], buf[512];
	
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
			printf("\nClient is connected to Server at IP Address = %s \t Port No : %d\n ", src, ntohs(addr.sin_port)); 

			/* connect to client ip */
			//bzero(&clientaddr, sizeof(clientaddr));
			//clientaddr.sin_family              = AF_INET;
			//clientaddr.sin_addr.s_addr         = htonl(client_addr.sin_addr.s_addr);
			//clientaddr.sin_port                = addr.sin_port;
			if(connect(sockfd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr))!=0) 
			{
			    printf("init_connection_socket: failed to connect to client\n");
			}
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
	sprintf(buf, "%d", addr.sin_port);
	sendto(sock, buf, sizeof(buf), 0, (SA *)&client_addr, sizeof(client_addr));
	read(sockfd, buf, sizeof(buf));
	if (close(sock) == -1)
		err_sys("close error");
        printf("\nReceived 3rd Hand Shake : %s", buf);
        
        sendFile(sockfd, filename);
	//return ntohs(addr.sin_port);
}

int existing_connection(struct sockaddr_in *client_addr){
	struct existing_connections *conn_list = existing_conn;
	while(conn_list!=NULL){
		if(((conn_list->client_portNum == client_addr->sin_port) && (conn_list->client_addr.sin_addr.s_addr == client_addr->sin_addr.s_addr))){
			return 1;
		}	
		conn_list = conn_list->next_connection;
	}	
	return 0;
}

/* Executed when server-child process dies */
static void
exitChild_handler (int signo)
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

void listenInterfaces(struct servStruct *servInfo)
{
	fd_set rset, allset;
	socklen_t len;
	
	struct msghdr recvmsg;
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
				recvfrom(head->sockfd, msg, sizeof(msg), 0, (struct sockaddr *)&clientInfo, &len);
				//recvfrom(head->sockfd, (void *)&recvmsg, sizeof(struct msghdr), 0, (struct sockaddr *)&clientInfo, &len);
				inet_ntop(AF_INET, &clientInfo.sin_addr, src, sizeof(src));
				printf("\nClient Address  %s & port number %d ", src, clientInfo.sin_port);
                                printf("\nFilename %s: \n", msg);
				

                                if( existing_connection(&clientInfo) == 1 ) { 
					printf("Duplicate connection request!");
				}
				else {
					if ((pid = fork()) == 0)
                                        {
                                                printf("\nClient Request Handler forked .");
						childRequestHandler(head->sockfd, interfaceList, clientInfo, msg);
						exit(0);
					}
					else
					{
						struct existing_connections *new_conn;
						new_conn = (struct existing_connections *)malloc(sizeof(struct existing_connections));
						new_conn->client_addr.sin_addr.s_addr = clientInfo.sin_addr.s_addr;
						new_conn->client_portNum = clientInfo.sin_port;
						new_conn->child_pid = pid;
						new_conn->serv_addr = head->ifi_addr;
						new_conn->next_connection = existing_conn;
						existing_conn = new_conn;
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
	existing_connections *existing_conn = NULL;
	
	struct servStruct *servInfo = loadServerInfo();

        listenInterfaces (servInfo);
	signal(SIGCHLD, exitChild_handler);
}
