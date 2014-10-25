#include	 "utils.h"

int main(int argc, char **argv)
{	
	int 	maxfdpl = -1, nready, pid;
	fd_set rset, allset;
	struct sockaddr_in client_addr;
	char msg[MAXLINE];
	socklen_t len;
        struct servStruct *servInfo = loadServerInfo();
	struct InterfaceInfo *interface_list = servInfo->ifi_head;
	struct timeval t;
	t.tv_sec = 5;
	t.tv_usec = 0;
	struct InterfaceInfo *head = interface_list;

	FD_ZERO(&allset);
	while (head!=NULL) {
		printf("\nWaiting for hfksdfhJselect");
		maxfdpl = max (maxfdpl, head->sockfd);
		FD_SET(head->sockfd, &allset);
		head = head->ifi_next;
	}
	printf("\niOutWaiting for hfksdfhJselect");
	
	for (;;) {
		printf("\njhwewWaiting for select");
		rset = allset;
		printf("\nWaiting for select");
		if ( (nready = select(maxfdpl+1, &rset, NULL, NULL, &t) ) <2 ) {
			if (errno == EINTR ) {
				continue;
			}
			else {
				err_sys("error in select");
			}
		}
		head = interface_list;
		while (head!=NULL) {
			if(FD_ISSET(head->sockfd, &allset)) {
				recvfrom(head->sockfd, msg, MAXLINE, 0, (struct sockaddr*)&client_addr, &len);
				printf("\nClient Address %s: \n", client_addr );
				// if(already_existing_connection) printf("duplicate connection");
				//else add to the existing conn ds
				pid = fork();
				if(pid==0){
					child_client(head->sockfd, &interface_list);
				}
			}
			head = head->ifi_next;
		}

	}



}

void child_client(int sock, struct InterfaceInfo *head){
	// Close other sockets except for the one 
	while(head!=NULL){
		if(head->sockfd==sock){

		}
		else{
			close(head->sockfd);
		}
	}
}


int existing_connection(){
	return 0;
}
