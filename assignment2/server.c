#include	 "utils.h"

/*	Global	declarations	*/
struct existing_connections *existing_conn;

void child_requestHandler(int sock, struct InterfaceInfo *head, struct sockaddr_in client_addr)
{
	int connfd;
	socklen_t len;
	struct sockaddr_in	servaddr, clientaddr, socket_addr;
	// Close other sockets except for the one 
	while(head!=NULL)
	{
		if(head->sockfd==sock)
		{
		 	connfd = socket(AF_INET, SOCK_DGRAM, 0);
    			if (connfd < 0) 
			{
        			printf("failed to create socket (%d)\n", errno);
        			return;
    			}

			/* binding to the serv ip*/
			bzero(&servaddr, sizeof(servaddr));
		        servaddr.sin_family     	 = AF_INET;
			servaddr.sin_addr.s_addr 	 = htonl(head->ifi_addr.sin_addr.s_addr);				
    			servaddr.sin_port        	 = htons(0);
			if(bind(connfd, (SA *) &servaddr, sizeof(servaddr))) 
			{
        			close(connfd);
        			err_sys("error in bind\n");
    			}
			
			/* determining the port number */
		        len = sizeof(struct sockaddr);
			getsockname(connfd, (struct sockaddr *)&socket_addr, &len);
			// need to print the port num.
			
			/* connect to client ip*/
			bzero(&clientaddr, sizeof(clientaddr));
                        clientaddr.sin_family              = AF_INET;
                        clientaddr.sin_addr.s_addr         = htonl(client_addr.sin_addr.s_addr);
                        clientaddr.sin_port                = socket_addr.sin_port;
			if(connect(connfd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr))!=0) 
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

void listenInterfaces(struct servStruct **serv)
{
	fd_set rset, allset;
	socklen_t len;
	
        char msg[MAXLINE];
	char src[128];
	
        int 	maxfdpl = -1, nready, pid;
	
        struct sockaddr_in clientInfo;
	struct servStruct *servInfo = *serv;
        struct InterfaceInfo *head  = servInfo->ifi_head;
        struct InterfaceInfo *interfaceList  = servInfo->ifi_head;
        	
        FD_ZERO(&allset);
	
        /* Setup select all interface sockfd's to listen for incoming client */
        while (head != NULL) 
        {
		printf("\nWaiting for hfksdfhJselect");
		maxfdpl = max (maxfdpl, head->sockfd);
		FD_SET(head->sockfd, &allset);
		head = head->ifi_next;
	}

        /* Server waits on select. When client comes, forks a child server to handle client */

	for (;;) {
		rset = allset;
		printf("\nWaiting for select");
		if ((nready = select(maxfdpl+1, &rset, NULL, NULL, NULL) ) < 0) {
			if (errno == EINTR ) {
				continue;
			}
			else {
				err_sys("error in select");
			}
		}
	
                head = interfaceList;
		while (head != NULL) {
			if(FD_ISSET(head->sockfd, &allset)) {
				recvfrom(head->sockfd, msg, MAXLINE, 0, (struct sockaddr*)&clientInfo, &len);
				inet_ntop(AF_INET, &clientInfo.sin_addr, src, sizeof(src));
				printf("\nClient Address %s: \n", src );
                                printf("\nFilename %s: \n", msg);
				
                                if( existing_connection(&clientInfo) == 1 ) { 
					printf("Duplicate connection request!");
				}
				else {
					pid = fork();
					if ((pid = fork()) == 0)
                                        {
                                                printf("\nClient Request Handler forked .");
						//child_requestHandler(head->sockfd, interface_list, client_addr);
					}
					else 
                                        {
						struct existing_connections *new_conn;
						new_conn->client_addr.sin_addr.s_addr = clientInfo.sin_addr.s_addr;
						new_conn->client_portNum = clientInfo.sin_port;
						new_conn->child_pid = pid;
						new_conn->next_connection = existing_conn;
						existing_conn = new_conn;
					}
				}
			}
			head = head->ifi_next;
		}
	}
}

int main(int argc, char **argv)
{	
	struct servStruct *servInfo = loadServerInfo();

        listenInterfaces (&servInfo);
}
