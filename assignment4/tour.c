#include        "utils.h"
#include 	"tour.h"
#include        "unp.h"
#include        <stdlib.h>
#include        <linux/if_ether.h>
#include        <netinet/ip.h>
#include        <stdio.h>


/* Create 4 Sockets for the tour application */
int createSocket(int *pfsockfd, int *rtsockfd, int *pgsockfd, int *multisockfd)
{
        const int optval = 1;

        /* Create an PF_PACKET RAW Socket for Echo Requests */
        if ((*pfsockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) == -1)
        {
                printf("Error creating PF packet socket\n");
                perror("socket");
                return -1;
        }

        /* Create an IP RAW Socket for tour */
        if ((*rtsockfd = socket(AF_INET, SOCK_RAW, RT_PROTO)) == -1)
        {
                printf("Error creating tour IP RAW packet socket\n");
                perror("socket");
                return -1;
        }

        /* Set Socket option for rtsockfd to IP_HDRINCL */
        Setsockopt(*rtsockfd, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(optval));

        /* Create an IP RAW Socket for Echo replies : PING SOCKET */
        if ((*pgsockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
        {
                printf("Error creating ping IP RAW packet socket\n");
                perror("socket");
                return -1;
        }

        /* Create an UDP IP Socket for Multicast : Multicast SOCKET */
        if ((*multisockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
                printf("Error creating multicast UDP socket\n");
                perror("socket");
                return -1;
        }
        return 0;
}

/* Create IP String for payload , add source IP */
void converthostnametoIP(int argc, char *argv[], char *ipAddrs)
{
        int i;
        struct hostent *hp;
        char hostname[HOST_SIZE], ip_dest[IP_SIZE];
        gethostname(hostname, sizeof(hostname));
        printf("Local Machine Hostname is :  %s\n", hostname);

        hp = (struct hostent *)gethostbyname((const char *)hostname);	
        sprintf(ip_dest, "%s", (char *)inet_ntoa( *(struct in_addr *)(hp->h_addr_list[0])));
        
        strcpy(ipAddrs, ip_dest);
        strcat(ipAddrs, ";");
        
        for (i = 1; i < argc; i++)
        {
                if (strcmp(argv[i], argv[i-1]) == 0)
                {
                        printf("Invalid tour sequence. Same consecutive machine names : %s. \n", argv[i]);
                        exit (0);
                }

                hp = (struct hostent *)gethostbyname((const char *)argv[i]);	
                sprintf(ip_dest, "%s", (char *)inet_ntoa( *(struct in_addr *)(hp->h_addr_list[0])));

                strcat(ipAddrs, ip_dest);
                strcat(ipAddrs, ";");
        }        
//        printf("List of IP Addresses Final: %s \n", ipAddrs);
}

/* Create Payload Struct with the tour packet information */
void createPayload(tpayload *packet, int idx, int tour_size, char *ipaddrs)  
{
        packet->next_ip_idx     =       idx;
        packet->tour_size       =       tour_size;
        packet->multi_port      =       MCAST_PORT;       
        memcpy(packet->multi_ip, MCAST_IP, sizeof(MCAST_IP));       
        strcpy(packet->ip_addrs, ipaddrs);
}

/* Print tour packet Payload. */
void printTourPacket(tpayload packet)  
{
        printf("Curr IP index : %d, Tour Size : %d, Multicast port : %d, Multicast IP : %s, List of IP's : %s\n", packet.next_ip_idx, packet.tour_size, packet.multi_port, packet.multi_ip, packet.ip_addrs);
}

/* Get IP Address by Index from tour packet */
char * getIPaddrbyIdx(tpayload *packet, int idx, char *ip)
{
        char buff[MAX_TOUR_SIZE * IP_SIZE], *tempip;
        strcpy(buff, packet->ip_addrs);
        //ip = strtok(packet->ip_addrs, ";");
        tempip = strtok(buff, ";");
        if (idx == 0)
        {
//              printf("IP Address : %s\n", tempip);
                strcpy(ip, tempip);
                return;
        }

        while (idx > 0)
        {
                tempip = strtok(NULL, ";");
                idx--;
        }
        strcpy(ip, tempip);
//      printf("IP Address : %s\n", ip);
}

/* Calculate Checksum */
unsigned short csum(unsigned short *buf, int nwords)
{       //
        unsigned long sum;
        for(sum=0; nwords>0; nwords--)
                sum += *buf++;
        sum = (sum >> 16) + (sum &0xffff);
        sum += (sum >> 16);
        return (unsigned short)(~sum);
}

/* Send tour Packet , and update ip index */
void send_tour_packet(int rtsockfd, tpayload *packet, int userlen)
{
        char src_ip[IP_SIZE], dst_ip[IP_SIZE], buf[IP_PACK_SIZE];
        struct sockaddr_in local, dest;
	struct ip       *ip;

        bzero(&local, sizeof(local));
        bzero(&dest, sizeof(dest));
//        addr.sin_family         =       AF_INET;
//        addr.sin_port           =       htons(MCAST_PORT);      /* TODO : Confirm about htons */
        
        getIPaddrbyIdx(packet, packet->next_ip_idx, src_ip);            /* Get Source IP address */
        printf("Src IP : %s\n", src_ip); 
        inet_pton(AF_INET, src_ip, &local.sin_addr);
        
        packet->next_ip_idx++;                                          /* Increment IP Index of packet payload */  
        
        getIPaddrbyIdx(packet, packet->next_ip_idx, dst_ip);            /* Get Destination IP address */
        printf("Dest IP : %s\n", dst_ip); 
        inet_pton(AF_INET, dst_ip, &dest.sin_addr);


        memset(buf, 0, IP_PACK_SIZE);

        char *tourpacket = buf + 20;

	/* 4fill in and checksum UDP header */
	ip = (struct ip *) buf;
	
        userlen += sizeof(struct ip);

	/* 4ip_output() calcuates & stores IP header checksum */
	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(struct ip) >> 2;
	ip->ip_tos = 0;
#if defined(linux) || defined(__OpenBSD__)
	ip->ip_len = htons(userlen);	/* network byte order */
#else
	ip->ip_len = userlen;			/* host byte order */
#endif
	ip->ip_id  = htons(0);			/* let IP set this */
	ip->ip_off = 0;			/* frag offset, MF and DF flags */
	ip->ip_ttl = 1;
        ip->ip_p   = htons(RT_PROTO); 
        memcpy((void *)tourpacket, (void *)packet, sizeof(tpayload));
        ip->ip_sum = csum((unsigned short *)buf, userlen);

	Sendto(rtsockfd, buf, userlen, 0, (SA *)&dest, sizeof(dest));

}

/* Join the multicast group using mcast_join function */
void addtomulticastgroup(int sockfd)
{
        struct sockaddr_in addr;

        bzero(&addr, sizeof(addr));
        addr.sin_family         =       AF_INET;
        addr.sin_port           =       htons(MCAST_PORT);      /* TODO : Confirm about htons */
        inet_pton(AF_INET, MCAST_IP, &addr.sin_addr);

        Mcast_join(sockfd, (const SA *)&addr, sizeof(addr), NULL, 0);

}


int main(int argc, char *argv[])
{
//      printf("\nTour Main");
        struct sockaddr_in mcast_addr;
        tpayload packet;
        char ipAddrs[MAX_TOUR_SIZE * IP_SIZE];

        int multisockfd, pfsockfd, rtsockfd, pgsockfd;
        
        if((createSocket(&pfsockfd, &rtsockfd, &pgsockfd, &multisockfd)) == -1)
                return 0;
        
        if (argc > 1)
        {
                converthostnametoIP(argc, argv, ipAddrs);
//              printf("List of IP Addresses main : %s \n", ipAddrs);
                createPayload(&packet, 0, argc - 1, ipAddrs);
                printTourPacket(packet);
                
                bzero(&mcast_addr, sizeof(struct sockaddr_in));
                mcast_addr.sin_family           =       AF_INET;
                mcast_addr.sin_port             =       htons(MCAST_PORT);
                mcast_addr.sin_addr.s_addr      =       htonl(INADDR_ANY);

                Bind(multisockfd, (SA *)&mcast_addr, sizeof(mcast_addr));

                /* TODO : Add to Multicast Group */
                addtomulticastgroup(multisockfd);

                /* Send Tour packet thru Tour IP RAW Socket */
                send_tour_packet(rtsockfd, &packet, sizeof(tpayload)); 

        }
        return 0;
}
