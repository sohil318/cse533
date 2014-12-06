#include        "utils.h"
#include 	"arp.h"
#include        "unp.h"
#include 	<sys/socket.h>
#include 	<linux/if_ether.h>
#include 	<linux/if_packet.h>

int ifaceIdx;
struct hwa_info * info;
/* Get Host MAC Address */
struct hwa_info * getMacAddrst()
{
        struct hwa_info	*hwa, *hwahead;
        char   *ptr = NULL, *hptr;
        int    i, prflag;

        hptr = (void *)malloc(MAC_SIZE);

        for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {

                if (strcmp(hwa->if_name, "eth0") == 0)  
                {
                        return hwa;
                }
        }

        return NULL;
}

/*  Create ARP Request Packet */
arp_pack * create_areq_packet(struct writeArq * arq)
{

        char ip[IP_SIZE];
        arp_pack *packet = (arp_pack *)malloc(sizeof(arp_pack));
        packet->type  = htons(AREQ);
        packet->proto_id  =  htons(IPPROTO_ID);
        packet->hatype  =   htonl(arq->hw.sll_hatype);
        strcpy(packet->src_mac, info->if_haddr);
        struct sockaddr_in * ad = (struct sockaddr_in *)info->ip_addr;
        inet_ntop(AF_INET, &(ad->sin_addr), ip, 50);
        strcpy(packet->src_ip, ip);
        strcpy(packet->dest_ip, arq->ip_addr);
        return packet;
}

/*
typedef struct arp_packet{
int type;
int proto_id;
unsigned short hatype;
unsigned char src_mac[6];
char src_ip[IP_SIZE];
unsigned char dest_mac[6];
char dest_ip[IP_SIZE];
} arp_pack;
*/
/*  Flood ARP Request     */
/*void floodARPReq(int pfsockfd, char * ip_addr)
{
        arp_pack *packet;
        packet = createARPPkt();

	char dst_mac[6]  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	ifaceInfo *temp = iface;
	while (temp)    
        {
		if(temp->ifaceIdx != ifaceIdx)
		{
                        sendARPReq(pfsockfd, pack, temp->haddr, dst_mac, temp->ifaceIdx);
		}
                temp = temp->next;
	}

}
*/

/*  Send an ARP Request     */
void sendARPReq(int sockfd_raw, char * ip_addr)
{
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


        // PF_PACKET Socket
        sockfd_raw = Socket(PF_PACKET, SOCK_RAW, htons(IPPROTO_ID));
       
        // Unix Domain Socket
        sockfd_stream = Socket(AF_LOCAL, SOCK_STREAM, 0);

	bzero(&serveraddr,sizeof(serveraddr));
        serveraddr.sun_family= AF_LOCAL;
        strcpy(serveraddr.sun_path,UNIX_PATH);
	
        Bind(sockfd_stream,(struct sockaddr*)&serveraddr,sizeof(serveraddr));

	Listen(sockfd_stream,LISTENQ);
        
        struct writeArq * recvarq;
        cache* entry;

        // Print the address pairs found
        info = getMacAddrst();   
        struct sockaddr_in * ad = (struct sockaddr_in *)info->ip_addr; 
        printf("IP Address : %ld  |   HW Address : %s \n ", ad->sin_addr.s_addr ,info->if_haddr);
        
        int nready;
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
			
               		nready = select(maxfdp + 1, &rset, NULL, NULL, NULL);
                        if (nready < 0)
                        {
                                if (errno == EINTR)
                                {
                                        printf("EINTR error !\n");
                                        continue;
                                }
                                else
                                {
                                        perror("select error");
                                        exit (0);
                                }
                        }
               		
			// If AREP or AREQ comes on PFPACKET socket
			if(FD_ISSET(sockfd_raw,&rset))
                	{
	                        bzero(&saddr,sizeof(saddr));
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
                  //                      floodARPREQ(sockfd_raw, recvarq->ip_addr);
				}
			}
			// If something is recieved on the connfd
			if(connfd > -1 && FD_ISSET(connfd, &rset)){
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

