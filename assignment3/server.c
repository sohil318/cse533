#include "unp.h"
#include "odr.h"

int main(int argv, char **argc){
	int sockfd;
	char msg[255];
	char ip_source[100];
	int source_port;
	struct sockaddr_un clientaddr, serveraddr;
	struct hostent *hp;
	time_t time_s;
	char serverVM[100];
	in_addr_t ip;
	char reply[100];
	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	bzero(&serveraddr, sizeof(serveraddr));	
	unlink(SERV_SUN_PATH);
	serveraddr.sun_family = AF_LOCAL;
	strcpy(serveraddr.sun_path, SERV_SUN_PATH);
	Bind(sockfd, (SA*) &serveraddr, sizeof(serveraddr));
	gethostname(serverVM, sizeof(serverVM));
	while(1){
		msg_recv(sockfd, msg, ip_source, source_port);
		ip = inet_addr(ip_source);  
                hp = gethostbyaddr((char*)&ip, sizeof(ip), AF_INET);
		printf("server at node %s responding to request from %s", serverVM, hp->h_name);
		time_s = time(NULL);
		bzero(reply, sizeof(reply));
		snprintf(reply,sizeof(reply),"%.24s\r\n",(char *)ctime(&time_s));
		msg_send(sockfd, ip_source, source_port, reply, 0);
	}
}
