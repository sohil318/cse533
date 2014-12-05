#include        "utils.h"
#include 	"tour.h"
#include        <stdlib.h>
#include        <linux/if_ether.h>
#include        <netinet/ip.h>
#include        <stdio.h>
#include        <time.h>

int visited = 0;

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
void converthostnametoIP(int argc, char *argv[], uint32_t *ipAddrs)
{
        int i;
        struct hostent *hp;

        char hostname[HOST_SIZE], ip_dest[IP_SIZE];
        gethostname(hostname, sizeof(hostname));
        printf("Local Machine Hostname is :  %s\n", hostname);

        hp = (struct hostent *)gethostbyname((const char *)hostname);	
        ipAddrs[0] = ((struct in_addr *)hp->h_addr_list[0])->s_addr;
        
        
        for (i = 1; i < argc; i++)
        {
                if (strcmp(argv[i], argv[i-1]) == 0)
                {
                        printf("Invalid tour sequence. Same consecutive machine names : %s. \n", argv[i]);
                        exit (0);
                }

                hp = (struct hostent *)gethostbyname((const char *)argv[i]);	
                ipAddrs[i] = ((struct in_addr *)hp->h_addr_list[0])->s_addr;

        }        
//        printf("List of IP Addresses Final: %s \n", ipAddrs);
}

/* Create Payload Struct with the tour packet information */
void createPayload(tpayload *packet, int idx, int tour_size, uint32_t *ipaddrs)  
{
        packet->next_ip_idx     =       idx;
        packet->tour_size       =       tour_size;
        packet->multi_port      =       MCAST_PORT;       
        memcpy(packet->multi_ip, MCAST_IP, sizeof(MCAST_IP));       
        memcpy(packet->ip_addrs, ipaddrs, sizeof(uint32_t) * tour_size);
}

/* Print tour packet Payload. */
void printTourPacket(tpayload packet)  
{
        int i = 0;
        struct in_addr addr;
        char *ip;
        printf("Tour Packet Contents <<<< Curr IP index : %d, Tour Size : %d, Multicast port : %d, Multicast IP : %s, List of IP's : ", packet.next_ip_idx, packet.tour_size, packet.multi_port, packet.multi_ip);
        for ( i = 0; i < packet.tour_size; i++)
        {
                addr.s_addr = packet.ip_addrs[i];
                ip = inet_ntoa(addr);
                printf("%s;",ip); 
        }
        printf(">>>> \n");
}

/* Get IP Address by Index from tour packet */
uint32_t getIPaddrbyIdx(tpayload *packet, int idx)
{
        return packet->ip_addrs[idx];
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
        char *src_ip, *dst_ip, buf[IP_PACK_SIZE];
        uint32_t sip, dip;
        struct in_addr addr;
        struct sockaddr_in local, dest;
	struct ip       *ip;

        bzero(&dest, sizeof(dest));
        src_ip = (char *)malloc(IP_SIZE);
        dst_ip = (char *)malloc(IP_SIZE);
        
        sip = getIPaddrbyIdx(packet, packet->next_ip_idx);            /* Get Source IP address */
        addr.s_addr = sip;
        strcpy(src_ip, inet_ntoa(addr));
        //printf("Src IP : %s\n", src_ip); 
        
        packet->next_ip_idx++;                                          /* Increment IP Index of packet payload */  
        
        dip = getIPaddrbyIdx(packet, packet->next_ip_idx);            /* Get Destination IP address */
        addr.s_addr = dip;
        strcpy(dst_ip, inet_ntoa(addr));
        //printf("Dest IP : %s\n", dst_ip); 

        userlen += sizeof(struct ip);
        memset(buf, 0, userlen);

        char *tourpacket = buf + sizeof(struct ip);

	/* 4fill in and checksum UDP header */
	ip = (struct ip *) buf;
	

	/* 4ip_output() calcuates & stores IP header checksum */
	ip->ip_src.s_addr = sip;
	ip->ip_dst.s_addr = dip;
	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(struct ip) >> 2;
	ip->ip_tos = 0;
#if defined(linux) || defined(__OpenBSD__)
	ip->ip_len = htons(userlen);	/* network byte order */
#else
	ip->ip_len = userlen;			/* host byte order */
#endif
	ip->ip_id  = htons(IP_IDENT);			/* let IP set this */
	ip->ip_off = 0;			/* frag offset, MF and DF flags */
	ip->ip_ttl = 1;
        ip->ip_p   = RT_PROTO; 
        memcpy((void *)tourpacket, (void *)packet, sizeof(tpayload));
        ip->ip_sum = htons(csum((unsigned short *)buf, userlen));
        
        dest.sin_addr.s_addr = dip;
        dest.sin_family      = AF_INET;

	Sendto(rtsockfd, buf, userlen, 0, (SA *)&dest, sizeof(dest));
        printf("Sending IP Packet from %s to %s\n", src_ip, dst_ip);

}

/* Join the multicast group using mcast_join function */
void addtomulticastgroup(int sockfd, char *ip)
{
        struct sockaddr_in addr;

        bzero(&addr, sizeof(addr));
        addr.sin_family         =       AF_INET;
        addr.sin_port           =       htons(MCAST_PORT);      /* TODO : Confirm about htons */
        inet_pton(AF_INET, ip, &addr.sin_addr);

        Mcast_join(sockfd, (const SA *)&addr, sizeof(addr), NULL, 0);

}

/* Send Multicast Message */
void sendMulticastPacket(int multisockfd, char * mcast_msg, int mport, char *mip)
{
        char multi_ip[IP_SIZE], vm_name[HOST_SIZE];
        struct sockaddr_in dest;
        
        strcpy(multi_ip, mip);
        dest.sin_family      =  AF_INET;
        dest.sin_port        =  htons(mport);
        inet_pton(AF_INET, multi_ip, &dest.sin_addr);
        
        gethostname(vm_name, sizeof(vm_name));
        printf("Node %s. Sending   %s\n", vm_name, mcast_msg);
         
	Sendto(multisockfd, mcast_msg, MSG_SIZE, 0, (SA *)&dest, sizeof(dest));
}


/* Handle Incoming Tour IP RAW Packet */
void handletourpacket(int rtsockfd, int multisockfd, int pgsockfd)
{
        char buff[IP_PACK_SIZE], src_ip[IP_SIZE], vm_name[HOST_SIZE], mcast_msg[MSG_SIZE];
        struct sockaddr_in addr, mcast_addr;
        struct in_addr  iaddr;
        struct ip *packhead;
        struct hostent *he;
       
        tpayload *data;
        int len = sizeof(addr);
        time_t ticks;

        Recvfrom(rtsockfd, (void *)buff, IP_PACK_SIZE, 0, (SA *)&addr, &len);
        
        packhead = (struct ip *)buff;
        data = (tpayload *)(buff + 20);
       
        if (ntohs(packhead->ip_id) != IP_IDENT)
        {
                printf("Invalid Packet identifier Received.\n");
                return;
        }

        ticks = time(NULL);
        he = (struct hostent *)malloc(sizeof(struct hostent));

        iaddr.s_addr = packhead->ip_src.s_addr;
//        strcpy(src_ip, inet_ntoa(iaddr));
//        printf("Src IP : %s\n", src_ip); 
        he = gethostbyaddr(&iaddr, sizeof(iaddr), AF_INET);
         
        printf("%.24s: Received source routing packet from %s\n", (char *)ctime(&ticks), he->h_name);
        printTourPacket(*data);
        gethostname(vm_name, sizeof(vm_name));


        if (visited == 0)
        {
                visited = 1;

                /* Add to Multicast Group */
                bzero(&mcast_addr, sizeof(struct sockaddr_in));
                mcast_addr.sin_family           =       AF_INET;
                mcast_addr.sin_port             =       htons(data->multi_port);
                mcast_addr.sin_addr.s_addr      =       htonl(INADDR_ANY);

                Bind(multisockfd, (SA *)&mcast_addr, sizeof(mcast_addr));

                /* TODO : Add to Multicast Group */
                printf("Adding %s to multicast group.\n", vm_name);
                addtomulticastgroup(multisockfd, data->multi_ip);
                
        }

        if (data->next_ip_idx + 1 == data->tour_size)
        {
                data->next_ip_idx++;
                sprintf(mcast_msg, "<<<<<<<<<<<<<< This is node %s. Tour has ended. Group Members please identify yourselves. >>>>>>>>>>", vm_name);
                printf("%s\n", mcast_msg); 
                sendMulticastPacket(multisockfd, mcast_msg, data->multi_port, data->multi_ip);
        }                
        else
        {
                send_tour_packet(rtsockfd, data, sizeof(tpayload)); 
        }
}

/* Handle Incoming Multicast packet */
void handlemultipacket(int multisockfd)
{
        char buff[IP_PACK_SIZE], src_ip[IP_SIZE], vm_name[HOST_SIZE], mcast_msg[MSG_SIZE];
        struct sockaddr_in mcast_addr;
        int len = sizeof(mcast_addr);
       
        Recvfrom(multisockfd, (void *)buff, IP_PACK_SIZE, 0, (SA *)&mcast_addr, &len);
        
        gethostname(vm_name, sizeof(vm_name));
        printf("Node %s. Received  %s\n", vm_name, buff);

        sprintf(mcast_msg, "<<<<< Node %s. I am member of group. >>>>>", vm_name);
        sendMulticastPacket(multisockfd, mcast_msg, MCAST_PORT, MCAST_IP);

        for (;;)
        {
                Recvfrom(multisockfd, (void *)buff, IP_PACK_SIZE, 0, (SA *)&mcast_addr, &len);
                gethostname(vm_name, sizeof(vm_name));
                printf("Node %s. Received  %s\n", vm_name, buff);
        }
}

int main(int argc, char *argv[])
{
//      printf("\nTour Main");
        struct sockaddr_in mcast_addr;
        tpayload packet;
        uint32_t *ipAddrs;

        ipAddrs = (uint32_t *)malloc(argc * sizeof(uint32_t));
        
        int multisockfd, pfsockfd, rtsockfd, pgsockfd;
        
        if((createSocket(&pfsockfd, &rtsockfd, &pgsockfd, &multisockfd)) == -1)
                return 0;
        
//        printf("Multisockfd %d, pfsockfd %d, rtsockfd %d, pgsockfd %d\n", multisockfd, pfsockfd, rtsockfd, pgsockfd);

        if (argc > 1)
        {
                converthostnametoIP(argc, argv, ipAddrs);
//              printf("List of IP Addresses main : %s \n", ipAddrs);
                createPayload(&packet, 0, argc, ipAddrs);
                printTourPacket(packet);
//                send_tour_packet(rtsockfd, &packet, sizeof(tpayload)); 
                
                bzero(&mcast_addr, sizeof(struct sockaddr_in));
                mcast_addr.sin_family           =       AF_INET;
                mcast_addr.sin_port             =       htons(MCAST_PORT);
                mcast_addr.sin_addr.s_addr      =       htonl(INADDR_ANY);

                Bind(multisockfd, (SA *)&mcast_addr, sizeof(mcast_addr));

                /* TODO : Add to Multicast Group */
                addtomulticastgroup(multisockfd, MCAST_IP);

                /* Send Tour packet thru Tour IP RAW Socket */
                send_tour_packet(rtsockfd, &packet, sizeof(tpayload)); 

        }

        int     maxfdpl, nready;
        fd_set  rset, allset;

        FD_ZERO(&allset);
        FD_ZERO(&rset);
	
        FD_SET(rtsockfd, &allset);
        FD_SET(pgsockfd, &allset);
        FD_SET(multisockfd, &allset);
        
        if ((rtsockfd > pgsockfd) && (rtsockfd > multisockfd))
                maxfdpl = rtsockfd;
        else if ((pgsockfd > rtsockfd) && (pgsockfd > multisockfd))
                maxfdpl = pgsockfd;
        else
                maxfdpl = multisockfd;

	for ( ; ; ) 
        {
		rset = allset;
                nready = select(maxfdpl + 1, &rset, NULL, NULL, NULL); 
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
		if (FD_ISSET(rtsockfd, &rset)) {	/* socket is readable */
                //        printf("Received Tour packet.\n");
                        handletourpacket(rtsockfd, multisockfd, pgsockfd);
		}
		if (FD_ISSET(pgsockfd, &rset)) {	/* socket is readable */
	        //        n = readline(sockFD, recvline, MAXBUF);

		}
		if (FD_ISSET(multisockfd, &rset)) {	/* socket is readable */
	        //        n = readline(sockFD, recvline, MAXBUF);
                        handlemultipacket(multisockfd);
		}

        }
        return 0;
}
