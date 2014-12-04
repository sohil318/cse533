#include        "utils.h"
#include 	"arp.h"
#include        "unp.h"

int main()
{
	int sockfd_raw, sockfd_stream, connfd, maxfdp;
	fd_set rset;

        sockfd_raw = Socket(PF_PACKET, SOCK_RAW, htons(IPPROTO_ID));
	sockfd_stream = Socket(AF_LOCAL, SOCK_STREAM, 0);
	

	FD_ZERO(&rset);

	while(1)
        	{

               		 FD_ZERO(&rset);
               		 FD_SET(sockfd_raw, &rset);
               		 FD_SET(sockfd_stream, &rset);
               		 if(sockfd_raw > sockfd_stream)
                       		maxfdp= sockfd_raw+1;
                	else
                	        maxfdp= sockfd_stream+1;



               	 select(maxfdp,&rset,NULL,NULL,NULL);
               	 if(FD_ISSET(sockfd_raw,&rset))
                	{
				
                //        	recv_process_pf_packet(sockfd);
                	}
               	 if(FD_ISSET(sockfd_stream,&rset))
                	{
		//		connfd = accept(sockfd_stream,(struct sockaddr *)&cliaddr, &clilen);
		//		if(errno==EINTR)
		//			continue;
		//		if(connfd<0)
		//		{
		//			perror("accept error:");
		//			continue;
		//		}	
                    //    	process_app_con(sockfd,connfd);
                	}
        	}
 
		return 0;
	
}
