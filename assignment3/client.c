#include "unp.h"
#include  <setjmp.h>
char serverVM[255], clientVM[255];
static sigjmp_buf jmpbuf;

static void timeout_resend(int signo)
{
        printf("client at node %s timeout on response from %s", clientVM, serverVM);
        siglongjmp(jmpbuf, 1);
}

int main(int argc, char **argv)
{	
	int sockfd, fd, source_port;
	char temp[100], recvd_msg[255];
	char send_msg[255] = "Time requested from server\n";
	struct sockaddr_un clientaddr, serveraddr;
	struct hostent *hp;
	char ip_dest[100];
	char ip_source[100];
	int flag;

	Signal(SIGALRM, timeout_resend);

	/* Creating domain datagram socket */
	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	bzero(&clientaddr, sizeof(clientaddr));
	clientaddr.sun_family = AF_LOCAL;
	strcpy(temp,"/tmp/fileXXXXXX");
	fd = mkstemp(temp);
    	if(fd<0){
		printf("\nSunpath cannot be created");
		return -1;
	}	
	close(fd);
    	unlink(temp);
	strncpy(clientaddr.sun_path, temp, sizeof(clientaddr.sun_path) - 1);
	Bind(sockfd, (SA *) &clientaddr, SUN_LEN(&clientaddr));
	
	gethostname(clientVM, sizeof(clientVM));
	while(1) {
	   userinput:
		flag = 0;
		printf("Select the VM server node : vm1, vm2, vm3, ..., vm10 \n");
		scanf("%s",&serverVM);
		hp = (struct hostent*)gethostbyname((const char *)serverVM);	
		inet_ntop(hp->h_addrtype,hp->h_addr_list,ip_dest,100);
		printf("client at node %s sending request to server at %s", clientVM, serverVM);
	   sending:
		msg_send(sockfd, ip_dest, 5000, send_msg, flag);	//yet to be filled
		alarm(50);
		if(sigsetjmp(jmpbuf,1)!=0)
        	{
			if(flag = 0) {
				flag = 1;     
				goto sending;
			}   	
			else {
				goto userinput;
			}	
		}	
		msg_recv(sockfd, recvd_msg, ip_source, source_port);	//yet to be filled
		alarm(0);
		}
}

