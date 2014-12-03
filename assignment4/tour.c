#include        "utils.h"
#include 	"tour.h"
#include        "unp.h"
#include        <stdlib.h>
#include        <linux/if_ether.h>
#include        <stdio.h>

int main()
{
        printf("\nTour Main");

        int multisockfd, pfsockfd, rtsockfd, pgsockfd;
        const int optval = 1;
        

        /* Create an PF_PACKET RAW Socket for Echo Requests */
        if ((pfsockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) == -1)
        {
                printf("Error creating PF packet socket\n");
                perror("socket");
                return 0;
        }

        /* Create an IP RAW Socket for tour */
        if ((rtsockfd = socket(AF_INET, SOCK_RAW, RT_PROTO)) == -1)
        {
                printf("Error creating tour IP RAW packet socket\n");
                perror("socket");
                return 0;
        }
        
        /* Set Socket option for rtsockfd to IP_HDRINCL */
        Setsockopt(rtsockfd, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(optval));
        
        /* Create an IP RAW Socket for Echo replies : PING SOCKET */
        if ((pgsockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
        {
                printf("Error creating ping IP RAW packet socket\n");
                perror("socket");
                return 0;
        }

        /* Create an UDP IP Socket for Multicast : Multicast SOCKET */
        if ((multisockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
                printf("Error creating multicast UDP socket\n");
                perror("socket");
                return 0;
        }

        



        return 0;
}
