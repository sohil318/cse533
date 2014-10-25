#include        "utils.h"
#include	"unpifiplus.h"

interfaceInfo* get_interfaces_client()
{
	interfaceInfo		*head = NULL, *temp;
	struct ifi_info		*ifi, *ifihead;
	struct sockaddr_in	*sa, *netmask, *subnet;
	int sockfd;
	char src[128], dst[128];

	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1);
		 ifi != NULL; ifi = ifi->ifi_next) {
		
		temp = (interfaceInfo *)malloc(sizeof(interfaceInfo));

		if ( (sa = (struct sockaddr_in *)ifi->ifi_addr) != NULL)
		{
		    //printf("Copied");
		    memcpy(&temp->ifi_addr, sa, sizeof(struct sockaddr_in));
		}

		if ( (netmask = (struct sockaddr_in *)ifi->ifi_ntmaddr) != NULL)
		{
		    //printf("Copied");
		    memcpy(&temp->ifi_ntmaddr, netmask, sizeof(struct sockaddr_in));
		}

                subnet = sa;
		subnet->sin_addr.s_addr = (sa->sin_addr.s_addr & netmask->sin_addr.s_addr);

		memcpy(&temp->ifi_subnetaddr, subnet, sizeof(struct sockaddr_in));

		temp->ifi_next = head;
		head = temp;

		printf("\n\n%s: \n", ifi->ifi_name);
		inet_ntop(AF_INET, &temp->ifi_addr.sin_addr, src, sizeof(src));
		printf("  IP addr: %s\n",	src);
		inet_ntop(AF_INET, &temp->ifi_ntmaddr.sin_addr, src, sizeof(src));
		printf("  Subnet Mask: %s\n",	src);
		inet_ntop(AF_INET, &temp->ifi_subnetaddr.sin_addr, src, sizeof(src));
		printf("  Subnet Addr: %s\n",	src);
		
	}
	free_ifi_info_plus(ifihead);
	return head;

}

interfaceInfo* get_interfaces_server(int portno)
{
	interfaceInfo		*head = NULL, *temp;
	struct ifi_info		*ifi, *ifihead;
	struct sockaddr_in	*sa, *netmask;
	u_char			*ptr;
	int sockfd;


	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1);
		 ifi != NULL; ifi = ifi->ifi_next) {
		
		temp = (interfaceInfo *)malloc(sizeof(interfaceInfo));

		printf("%s: ", ifi->ifi_name);
		if (ifi->ifi_index != 0)
			printf("(%d) ", ifi->ifi_index);

		printf("\n");
		
		if ( (sa = (struct sockaddr_in *)ifi->ifi_addr) != NULL)
		    memcpy(&temp->ifi_addr,sa,sizeof(struct sockaddr_in));

		if ( (netmask = (struct sockaddr_in *)ifi->ifi_ntmaddr) != NULL)
		    memcpy(&temp->ifi_ntmaddr, netmask,sizeof(struct sockaddr_in));

/*=============================================================================*/

/*
		sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
		Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		sa = (struct sockaddr_in *) ifi->ifi_addr;
		sa->sin_family = AF_INET;
		sa->sin_port = htons(SERV_PORT);
		Bind(sockfd, (SA *) sa, sizeof(*sa));
*/		
		temp->ifi_next = head;
		head = temp;
	}
	free_ifi_info_plus(ifihead);
	exit(0);
}

int
main(int argc, char **argv)
{
	interfaceInfo		*head = NULL, *temp;
        int count = 0;
        head = get_interfaces_client();
	while (head)
	{
	    count++;
	    head = head->ifi_next;
	}
	printf ("Count = %d \n", count);
        return 0;
}

