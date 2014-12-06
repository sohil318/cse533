#include        "utils.h"
#include 	"tour.h"
#include	"hw_addrs.h"
#include        <stdlib.h>
#include        <linux/if_ether.h>
#include        <netinet/ip.h>
#include        <netinet/ip_icmp.h>
#include        <sys/socket.h>
#include        <linux/if_packet.h>
#include        <stdio.h>
#include        <time.h>

int visited = 0;
int ifaceIdx = 2;
int pinged_vm[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

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

/* Get VM number */
int get_vm_index(uint32_t ip)
{
        int idx;
        struct in_addr taddr;
        taddr.s_addr = ip;
        const char *str = inet_ntoa(taddr);
        idx = str[13] - (int)'0';
        return idx;
}

/* Print Mac Address */
void printmac(char *mac)
{
        char *ptr = mac;
        int i = IF_HADDR;
        do {
                printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
        } while (--i > 0);

}

/* Get Source IP Address */
uint32_t getsrcipaddr()
{
        struct hostent *hp;
        char hostname[HOST_SIZE];

        gethostname(hostname, sizeof(hostname));
        hp = (struct hostent *)gethostbyname((const char *)hostname);	

        return ((struct in_addr *)hp->h_addr_list[0])->s_addr;
}

/* Get Host MAC Address */
char* getSrcMacAddr()
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

void getDestMac(uint32_t dip, struct hwaddr *HWaddr){
        struct sockaddr_in *IPaddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        IPaddr->sin_addr.s_addr = dip;
        int result;
        HWaddr->sll_ifindex = ifaceIdx;
        HWaddr->sll_hatype = ARPHRD_ETHER;
        HWaddr->sll_halen = ETH_ALEN;
        result = areq ((SA *)IPaddr, sizeof(struct sockaddr_in), HWaddr);
}

/* Send echo request */
int send_ping_req(int pfsockfd, int pgsockfd, uint32_t dip)
{
        printf("Create and Send Ping Request\n");
        uint32_t sip; 
        int datalen = 4;
        char datac[4] = {'a', 'b', 'c', 'd'};
        char src_mac[MAC_SIZE], dst_mac[MAC_SIZE];
        struct icmp *icmppkt;
        struct ip *ip;
        struct hwaddr *HWaddr;

        memcpy(src_mac, getSrcMacAddr(), MAC_SIZE);                                             /*      Get Source MAC Address                          */
        printf("Step 1. \n");
        printf("Source Mac : "); 
        printf("Step 2. \n");
        printmac(src_mac);
        printf("Step 3. \n");
        printf("\n");
        
        getDestMac(dip, HWaddr);                                                                /*      TODO : Get Dest MAC Address                     */
        
        memcpy(dst_mac, HWaddr->sll_addr, MAC_SIZE);
        sip = getsrcipaddr();
        send_v4(icmppkt);

        int send_result = 0;
        struct sockaddr_ll socket_address;                                                      /*      target address                                  */
        void* buffer = (void*)malloc(ETHR_FRAME_LEN);                                           /*      buffer for ethernet frame                       */
        memset(buffer, 0, ETHR_FRAME_LEN);

        unsigned char* etherhead = buffer;                                                      /*      pointer to ethenet header                       */
        unsigned char* ipheader  = buffer + 14;                                                 /*      ip header in ethernet frame                     */  
        unsigned char* icmphead  = buffer + 14 + sizeof(struct ip);                             /*      userdata in ethernet frame                      */  
        unsigned char* data      = buffer + 14 + sizeof(struct ip) + sizeof(struct icmp);       /*      userdata in ethernet frame                      */  

        struct ethhdr *eh = (struct ethhdr *)etherhead;                                         /*      another pointer to ethernet header              */

        /*      Prepare sockaddr_ll     */
        socket_address.sll_family   =   PF_PACKET;                                              /*      RAW communication                               */  
        socket_address.sll_protocol =   htons(ETH_P_IP);                                        /*      We don't use a protocoll above ethernet layer just use anything here.  */
        socket_address.sll_ifindex  =   ifaceIdx;                                               /*      Interface Index of the network device in function parameter     */

        
        socket_address.sll_hatype   =   ARPHRD_ETHER;                                           /*      ARP hardware identifier is ethernet             */  
        
        socket_address.sll_pkttype  =   PACKET_OTHERHOST;                                       /*      Target is another host.                         */  

        socket_address.sll_halen    =   MAC_SIZE;                                               /*      Address length                                  */

        /*      MAC - begin     */
        socket_address.sll_addr[0]  =   dst_mac[0];             
        socket_address.sll_addr[1]  =   dst_mac[1];             
        socket_address.sll_addr[2]  =   dst_mac[2];  
        socket_address.sll_addr[3]  =   dst_mac[3];  
        socket_address.sll_addr[4]  =   dst_mac[4];
        socket_address.sll_addr[5]  =   dst_mac[5];
        /*      MAC - end       */

        socket_address.sll_addr[6]  =   0x00;                                                   /*      not used                                        */
        socket_address.sll_addr[7]  =   0x00;                                                   /*      not used                                        */      

        memcpy((void*)buffer, (void*)dst_mac, MAC_SIZE);                                        /*      Set Dest Mac in the ethernet frame header       */
        memcpy((void*)(buffer + MAC_SIZE), (void*)src_mac, MAC_SIZE);                           /*      Set Src Mac in the ethernet frame header        */  
        eh->h_proto = htons(ETH_P_IP);

        /* Fill Contents of IP Packet */
	ip = (struct ip *) ipheader;

	/* 4ip_output() calcuates & stores IP header checksum */
	ip->ip_src.s_addr = sip;
	ip->ip_dst.s_addr = dip;
	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(struct ip) >> 2;
	ip->ip_tos = 0;
        
        int len = sizeof(struct ip) + sizeof(struct icmp) + datalen;

	ip->ip_len = htons(len);	/* network byte order */
	ip->ip_id  = htons(IP_IDENT);			/* let IP set this */
	ip->ip_off = 0;			/* frag offset, MF and DF flags */
	ip->ip_ttl = 1;
        ip->ip_p   = IPPROTO_ICMP; 
        ip->ip_sum = htons(0);
        
        memcpy((void *)icmphead, (void *)icmppkt, sizeof(struct icmp));

        //ip->ip_sum = htons(csum((unsigned short *)buf, userlen));

        memcpy((void *)data, (void *)datac, datalen);                                           /*      Fill the frame with ODR Packet                  */
        
        /*send the packet*/
        send_result = sendto(pfsockfd, buffer, ETHR_FRAME_LEN, 0, (struct sockaddr *) &socket_address, sizeof(socket_address));

}

void proc_v4(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv)
{
	int				hlen1, icmplen;
	double			rtt;
	struct ip		*ip;
	struct icmp		*icmp;
	struct timeval	*tvsend;
        struct in_addr  iaddr;
        struct hostent *he;

	ip = (struct ip *) ptr;		/* start of IP header */
	hlen1 = ip->ip_hl << 2;		/* length of IP header */
	if (ip->ip_p != IPPROTO_ICMP)
        {
                printf("Rejecting ICMP Reply. Not Mine. \n");
		return;				/* not ICMP */
        }

	icmp = (struct icmp *) (ptr + hlen1);	/* start of ICMP header */
	if ( (icmplen = len - hlen1) < 8)
		return;				/* malformed packet */

	if (icmp->icmp_type == ICMP_ECHOREPLY) {
		if (icmp->icmp_id != IP_IDENT)
			return;			/* not a response to our ECHO_REQUEST */
//		if (icmplen < 16)
//			return;			/* not enough data to use */

		tvsend = (struct timeval *) icmp->icmp_data;
		tv_sub(tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

                he = (struct hostent *)malloc(sizeof(struct hostent));

                iaddr.s_addr = ip->ip_src.s_addr;
                he = gethostbyaddr(&iaddr, sizeof(iaddr), AF_INET);
                
		printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
				icmplen, he->h_name,
				icmp->icmp_seq, ip->ip_ttl, rtt);
	}
}


/* Receive Ping/Echo replies */
void recv_ping_rep(int pgsockfd)
{
        struct sockaddr_in recvaddr;
        struct msghdr msg;
        struct timeval tvrecv;
        int len = sizeof(recvaddr);
        char recvbuf[ETHR_FRAME_LEN];
        
        bzero(&recvaddr, sizeof(struct sockaddr_in));
        
        Recvfrom(pgsockfd, (void *)recvbuf, ETHR_FRAME_LEN, 0, (SA *)&recvaddr, &len);
        
        Gettimeofday(&tvrecv, NULL);

        proc_v4(recvbuf + 14, sizeof(recvbuf) - 14, &msg, &tvrecv);
        
}


/* Handle 5 echo request reply sequences for the last packet in tour */
void handle_final_pings(int pfsockfd, int pgsockfd, uint32_t dip)
{
        printf("Handle Last 5 Pings.\n");
        int     maxfdpl, nready, i;
        fd_set  rset, allset;
        struct timeval tv;

        FD_ZERO(&allset);
        FD_ZERO(&rset);
	
        FD_SET(pgsockfd, &allset);
        
        maxfdpl = pgsockfd;

	for (i = 0 ; i < 5; i++ ) 
        {
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                rset = allset;

                nready = select(maxfdpl + 1, &rset, NULL, NULL, &tv); 
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
                if (nready == 0)                {
                        printf("1 Second completed. Send Next Ping.\n");
                        //send_all_ping_req(pfsockfd, pgsockfd);
                }
		if (FD_ISSET(pgsockfd, &rset))  {	/* socket is readable */
                        recv_ping_rep(pgsockfd);
                        //send_ping_req(pfsockfd, pgsockfd, dip);
		}
        }
        
}


/* Handle Incoming Tour IP RAW Packet */
void handletourpacket(int rtsockfd, int multisockfd, int pgsockfd, int pfsockfd)
{
        int idx, datalen = 56;
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

                /* Pinging Sender Node */
                idx = get_vm_index(packhead->ip_src.s_addr);
                printf("Current Index : %d\n", idx);

                if (pinged_vm[idx] == 0)
                {
                        struct in_addr taddr;
                        taddr.s_addr = packhead->ip_src.s_addr;
                        const char *str = inet_ntoa(taddr);

	                printf("PING %s: %d data bytes\n", str, datalen);
                        pinged_vm[idx] = 1;
                        //send_ping_req(pfsockfd, pgsockfd, packhead->ip_src.s_addr);
                }
                
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
        else
        {
                /* Pinging Sender Node */
                idx = get_vm_index(packhead->ip_src.s_addr);
                printf("Current Index : %d\n", idx);

                if (pinged_vm[idx] == 0)
                {
                        struct in_addr taddr;
                        taddr.s_addr = packhead->ip_src.s_addr;
                        const char *str = inet_ntoa(taddr);

	                printf("PING %s: %d data bytes\n", str, datalen);
                        pinged_vm[idx] = 1;
                        //send_ping_req(pfsockfd, pgsockfd, packhead->ip_src.s_addr);
                }

        }

        if (data->next_ip_idx + 1 == data->tour_size)
        {
                struct in_addr taddr;
                taddr.s_addr = packhead->ip_src.s_addr;
                const char *str = inet_ntoa(taddr);

                printf("PING %s: %d data bytes\n", str, datalen);
                handle_final_pings(pfsockfd, pgsockfd, packhead->ip_src.s_addr);
                
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

        int     maxfdpl, nready, i;
        fd_set  rset, allset;
        struct timeval tv;

        FD_ZERO(&allset);
        FD_ZERO(&rset);
	
        FD_SET(multisockfd, &allset);
        
        maxfdpl = multisockfd;

        for (;;)
        {
                tv.tv_sec = 5;
                tv.tv_usec = 0;
                rset = allset;

                nready = select(maxfdpl + 1, &rset, NULL, NULL, &tv); 
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
                if (nready == 0)                {
                        printf("Exiting Tour Application..\n");
                        exit (0);
                }
                if (FD_ISSET(multisockfd, &rset))  {	/* socket is readable */
                        Recvfrom(multisockfd, (void *)buff, IP_PACK_SIZE, 0, (SA *)&mcast_addr, &len);
                        gethostname(vm_name, sizeof(vm_name));
                        printf("Node %s. Received  %s\n", vm_name, buff);
                }
        }
}

void send_v4(struct icmp *icmp)
{
	int			len;
//	struct icmp	*icmp;

//	icmp = (struct icmp *) malloc (sizeof(struct icmp));
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = IP_IDENT;
	icmp->icmp_seq = 0;
	//memset(icmp->icmp_data, 0xa5, datalen);	/* fill with pattern */
	//Gettimeofday((struct timeval *) icmp->icmp_data, NULL);

	//len = 8 + datalen;		/* checksum ICMP header and data */
	//icmp->icmp_cksum = 0;
	//icmp->icmp_cksum = in_cksum((u_short *) icmp, len);
        //sendechoreq();
	//Sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen);
}

/* Send ping requests every second */
void send_all_ping_req(int pfsockfd, int pgsockfd)
{
        int i = 0, datalen = 56;
        char hostname[HOST_SIZE], *str;
        uint32_t ipaddr;
        struct in_addr taddr;
        struct hostent *hp;

        for (i = 0; i < 10; i++)
        {
                if (pinged_vm[i] == 1)
                {
                        if (i == 0)
                                sprintf(hostname, "vm%d", 10);
                        else
                                sprintf(hostname, "vm%d", i);
                }
                
                hp = (struct hostent *)gethostbyname((const char *)hostname);	
                ipaddr = ((struct in_addr *)hp->h_addr_list[0])->s_addr;
                taddr.s_addr = ipaddr;
                strcpy(str, inet_ntoa(taddr));

                printf("PING %s: %d data bytes\n", str, datalen);
                //send_ping_req(pfsockfd, pgsockfd, ipaddr);
        }
}

/* Handle Ping Repies */
void handlepingreply(int pgsockfd)
{
        printf("Handling Echo Replies \n");
        recv_ping_rep(pgsockfd);
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

                addtomulticastgroup(multisockfd, MCAST_IP);

                /* Send Tour packet thru Tour IP RAW Socket */
                send_tour_packet(rtsockfd, &packet, sizeof(tpayload)); 

        }

        int     maxfdpl, nready;
        fd_set  rset, allset;
        struct timeval tv;

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
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                rset = allset;

                //nready = select(maxfdpl + 1, &rset, NULL, NULL, &tv); 
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
                if (nready == 0)                {
                        printf("1 Second completed. Send Next Ping.\n");
                        //send_all_ping_req(pfsockfd, pgsockfd);
                }
		if (FD_ISSET(rtsockfd, &rset))  {	/* socket is readable */
                        handletourpacket(rtsockfd, multisockfd, pgsockfd, pfsockfd);
		}
		if (FD_ISSET(pgsockfd, &rset))  {	/* socket is readable */
                        handlepingreply(pgsockfd);
		}
		if (FD_ISSET(multisockfd, &rset)) {	/* socket is readable */
                        handlemultipacket(multisockfd);
		}

        }
        return 0;
}
