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
        {
		return 1;
        }
        
	else if ((serverIPnmsk.sin_addr.s_addr & clientIP.sin_addr.s_addr) == serverIPsubnet.sin_addr.s_addr)
        {
	    return 1;
	}
	
	return 0;
	
}


/* Forked server child for service requesting client */ 
void childRequestHandler(int sock, struct InterfaceInfo *head, struct sockaddr_in client_addr)
{
	int sockfd, isLocal, optval = -1;
	socklen_t len;
	struct sockaddr_in	servaddr, clientaddr, addr;
	char src[128];
	
	// Close other sockets except for the one 
	while(head != NULL)
	{
		if(head->sockfd == sock)
		{
			isLocal = checkLocal (head-> ifi_addr, head->ifi_ntmaddr, head->ifi_subnetaddr, client_addr);
		 	
			if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
			    err_sys("\nsocket creation error\n");

			if(isLocal)
			    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
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
			bzero(&clientaddr, sizeof(clientaddr));
                        clientaddr.sin_family              = AF_INET;
                        clientaddr.sin_addr.s_addr         = htonl(client_addr.sin_addr.s_addr);
                        clientaddr.sin_port                = addr.sin_port;
			if(connect(sockfd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr))!=0) 
			{
        			printf("init_connection_socket: failed to connect to client\n");
    			}
		}
		else
		{	
			/* Closing the other sockets*/
			close(head->sockfd);
		}
		head = head->ifi_next;
	}
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

void listenInterfaces(struct servStruct *servInfo)
{
	fd_set rset, allset;
	socklen_t len;
	
        char msg[MAXLINE];
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
				recvfrom(head->sockfd, msg, MAXLINE, 0, (struct sockaddr *)&clientInfo, &len);
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
						childRequestHandler(head->sockfd, interfaceList, clientInfo);
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
						printf("\nelse to be done");
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
}
