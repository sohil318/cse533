#include        "utils.h"
#include 	"tour.h"
#include        "unp.h"
#include        <stdlib.h>
#include        <linux/if_ether.h>
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
        packet->curr_ip_idx     =       idx;
        packet->tour_size       =       tour_size;
        packet->multi_port      =       MCAST_PORT;       
        memcpy(packet->multi_ip, MCAST_IP, sizeof(MCAST_IP));       
        strcpy(packet->ip_addrs, ipaddrs);
}

/* Print tour packet Payload. */
void printTourPacket(tpayload packet)  
{
        printf("Curr IP index : %d, Tour Size : %d, Multicast port : %d, Multicast IP : %s, List of IP's : %s\n", packet.curr_ip_idx, packet.tour_size, packet.multi_port, packet.multi_ip, packet.ip_addrs);
}


int main(int argc, char *argv[])
{
//        printf("\nTour Main");
        tpayload packet;
        char ipAddrs[MAX_TOUR_SIZE * IP_SIZE];

        int multisockfd, pfsockfd, rtsockfd, pgsockfd;
        
        if((createSocket(&pfsockfd, &rtsockfd, &pgsockfd, &multisockfd)) == -1)
                return 0;
        
        if (argc > 1)
        {
                converthostnametoIP(argc, argv, ipAddrs);
//                printf("List of IP Addresses main : %s \n", ipAddrs);
                createPayload(&packet, 1, argc - 1, ipAddrs);
                printTourPacket(packet);
        }
        return 0;
}
