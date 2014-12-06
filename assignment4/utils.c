#include        "utils.h"
#include        "unp.h"
#include 	"arp.h"

cache * cache_head = NULL;

int areq (struct sockaddr *IPaddr, socklen_t sockaddrlen, struct hwaddr *HWaddr)
{
	int len, sockfd, ret, num;
	struct sockaddr_un serveraddr;
	struct sockaddr_in *ip;
	fd_set rset;
	struct timeval tv;
	struct writeArq sendarq;
	char mac[6];

	sockfd = Socket(AF_LOCAL, SOCK_STREAM, 0);
	ip = (struct sockaddr_in *)IPaddr;

	sendarq.hw.sll_ifindex = HWaddr->sll_ifindex;
	sendarq.hw.sll_hatype = HWaddr->sll_hatype;
	sendarq.hw.sll_halen = HWaddr->sll_halen;
	inet_ntop(AF_INET, &(ip->sin_addr), sendarq.ip_addr, IP_SIZE);

	printf("HW address being sought for %s \n", inet_ntoa(ip->sin_addr));

	bzero(&serveraddr, sizeof(struct sockaddr_un));
	serveraddr.sun_family = AF_LOCAL;
	strcpy(serveraddr.sun_path, UNIX_PATH);
	
	Connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	len = write(sockfd, &sendarq, sizeof(sendarq));
	
	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);

//	Timeout on read

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	
	select(sockfd+1, &rset, NULL, NULL, &tv);	
	
	if(FD_ISSET(sockfd, &rset)){
		read(sockfd, mac, sizeof(mac));
	        memcpy(HWaddr->sll_addr, mac, 6);
        for(num=0; num<6; num++)
		{
			printf("%.2x:", mac[num]);			
		}
		printf("\n");
	}
	else{
		printf("areq time out! \n");	
		ret = -1;
	}
	return ret;
}

int add_entry(struct writeArq arq , int connfd)
{
	cache * new = malloc(sizeof(cache));
	memset(new,0,sizeof(cache));


	if(arq.hw.sll_addr == NULL)
		new->incomplete=1;

	strcpy(new->ip_addr, arq.ip_addr);
	new->ifindex = arq.hw.sll_ifindex;
	new->hatype = arq.hw.sll_hatype;
	new->connfd= connfd;
        new->incomplete = 1;
        new->next = NULL;

	if(arq.hw.sll_addr)
	{
	        new->incomplete=0;
                memcpy(new->hw_addr,arq.hw.sll_addr,6);
	}
        if (cache_head != NULL)
	        new->next= cache_head;
	cache_head= new;
	
	printf("added ip: %s\n",new->ip_addr);
	return 0;

}

void delete_cache_entry(int connfd)
{
              cache *temp = cache_head;
              if(cache_head->connfd == connfd)
              {
                         cache_head = cache_head->next;
                         return;
              }
              else
              {
                     while(temp)
                     {
                                if(cache_head->connfd == connfd)
                                {
                                         temp->next = temp->next->next;        
                                         return;
                                }
                                temp = temp->next;
                     }
              }                     
}

int update_cache(struct writeArq arq, int connfd){
        delete_cache_entry(connfd);
        add_entry(arq,connfd);
}

cache * find_in_cache(char *ip_addr)
{
	cache *temp = cache_head;

    	while (temp)
    	{
        	if(strcmp(temp->ip_addr, ip_addr)==0)
        	{
                        return temp;
        	}
        temp = temp->next;
     	}

    return temp;
}

//arp_pack* create_arp_req(int type, int proto_id, unsigned short hatype, ){}
//arp_pack* create_arp_rep(){}
