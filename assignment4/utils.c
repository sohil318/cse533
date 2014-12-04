#include        "utils.h"
#include        "unp.h"

int areq (struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr)
{
	int len, sockfd, ret, num;
	struct sockaddr_un serveraddr;
	struct sockaddr_in *ip;
	fd_set rset;
	struct timeval tv;

	sockfd = Socket(AF_LOCAL, SOCK_STREAM, 0);
	ip = (struct sockaddr_in *)IPaddr;
	printf("HW address being sought for %s \n", inet_ntoa(ip->sin_addr));
	bzero(&serveraddr, sizeof(struct sockaddr_un));
	serveraddr.sun_family = AF_LOCAL;
	strcpy(serveraddr.sun_path, UNIX_PATH);
	
	Connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	len = write(sockfd, &ip->sin_addr, 4);
	
	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);

//	Timeout on read

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	
	select(sockfd+1, &rset, NULL, NULL, &tv);	
	
	if(FD_ISSET(sockfd, &rset)){
		read(sockfd, HWaddr, sizeof(HWaddr));
		for(num=0; num<6; num++)
		{
			printf("%.2x:", HWaddr->sll_addr[num]);			
		}
		printf("\n");
	}
	else{
		printf("areq time out! \n");	
		ret = -1;
	}
	return ret;
}
