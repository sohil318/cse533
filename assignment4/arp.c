#include        "utils.h"
#include 	"arp.h"
#include        "unp.h"
#include 	<sys/socket.h>
#include 	<linux/if_ether.h>
#include 	<linux/if_packet.h>
#include	"hw_addrs.h"

int ifaceIdx;
/* Get Host MAC Address */
char* getMacAddr()
{
        struct hwa_info	*hwa, *hwahead;
        char   *ptr = NULL, *hptr;
        int    i, prflag;

        hptr = (void *)malloc(MAC_SIZE);

        for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {

                if (strcmp(hwa->if_name, "eth0") == 0)  
                {
                        ifaceIdx = hwa->if_index;
                        prflag = 0;
                        i = 0;
                        do {
                                if (hwa->if_haddr[i] != '\0') {
                                        prflag = 1;
                                        break;
                                }
                        } while (++i < IF_HADDR);

                        if (prflag) {
                                printf("         HW addr = ");
                                ptr = hwa->if_haddr;
                                memcpy(hptr, hwa->if_haddr, MAC_SIZE);
                        }
                        return hptr;
                }
        }

        return NULL;
}


int main()
{
	int sockfd_raw, sockfd_stream, maxfdp, clientlen;
	fd_set rset;
	int len, connfd = -1;
	struct sockaddr_un serveraddr,clientaddr;
	struct sockaddr_ll saddr;
	void* buff = (void*)malloc(ETH_LEN);
	arp_pack* arp_p = (arp_pack*) malloc(sizeof(arp_pack));      
 	len = sizeof(saddr);
	char recvBuff[1024];
	clientlen = sizeof(clientaddr);

	bzero(&serveraddr,sizeof(serveraddr));
        serveraddr.sun_family= AF_LOCAL;
        strcpy(serveraddr.sun_path,UNIX_PATH);

        sockfd_raw = Socket(PF_PACKET, SOCK_RAW, htons(IPPROTO_ID));
        sockfd_stream = Socket(AF_LOCAL, SOCK_STREAM, 0);

	Bind(sockfd_stream,(struct sockaddr*)&serveraddr,sizeof(serveraddr));

	Listen(sockfd_stream,LISTENQ);
        
        struct writeArq * recvarq;
        cache* entry;

	FD_ZERO(&rset);

	while(1)
        	{

               		 FD_ZERO(&rset);
               		 FD_SET(sockfd_raw, &rset);
               		 FD_SET(sockfd_stream, &rset);
			 
			 maxfdp= max(sockfd_raw, sockfd_stream);
			 
			if(connfd!=-1)
			{
				FD_SET(connfd, &rset);	
			}

			maxfdp = max(maxfdp, connfd);
			
               		select(maxfdp,&rset,NULL,NULL,NULL);
               		
			// If AREP or AREQ comes on PFPACKET socket
			if(FD_ISSET(sockfd_raw,&rset))
                	{
				recvfrom(sockfd_raw, buff, ETH_LEN, 0, (SA *)&saddr, &len);
                                memcpy(arp_p, buff + 14, sizeof(arp_pack));
			}

			// If request comes from areq() API
               		 if(FD_ISSET(sockfd_stream,&rset))
                	{
 				connfd = accept(sockfd_stream,(struct sockaddr *)&clientaddr, &clientlen);
				if(errno==EINTR)
					continue;
                		read(connfd, recvBuff, sizeof(recvBuff));
				recvarq = (struct writeArq *)recvBuff;
				entry = (cache *)find_in_cache(recvarq->ip_addr);
				
				// If an entry is found in the chache
				if(entry != NULL)
				{
					write(connfd, entry->hw_addr, 6);
					Close(connfd);
					connfd = -1;
				}
				// If the entry does not exist in the cache 
				else
				{
					 printf("No entry in cache for %s. Creating an incomplete entry.\n", recvarq->ip_addr);
					// Add a new incomplete entry
				        add_entry(recvarq , connfd); 
                                        // Prepare a req header
					// Send ARP REQ
				}
			}
			// If something is recieved on the connfd
			if(connfd>-1 && FD_ISSET(connfd, &rset)){
				// delete incomplete entry on timeout
		                printf("Recieved something in connfd.\n");
                                int bytes_read = Read(connfd, recvBuff, sizeof(recvBuff));
                                if (bytes_read == 0){
                                        printf("Timeout detected. Connection closed by tour client.\n");        
                                        delete_cache_entry(connfd);
                                        Close(connfd);
                                        connfd = -1;
                                }
                        }
        	}
 
		return 0;
	
}
