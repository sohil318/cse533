#include	 "utils.h"

int main(int argc, char **argv)
{	
	struct InterfaceInfo *interface_list = loadClientInfo();
	struct InterfaceInfo *head = interface_list;

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
		/*
		while (head!=NULL) {
			if(FD_ISSET(head->sockfd, &allset)) {
				recvfrom(head->sockfd, msg, MAXLINE, 0, (struct sockaddr*)&client_addr, &len);
				printf("\nClient Address %s: \n", client_addr );
			}
			head = head->ifi_next;
		}*/

	}



}

int existing_connection(){
	return 0;
}

