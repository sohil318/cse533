#include        "utils.h"
#include	"unpifiplus.h"

interfaceInfo* get_interfaces_client()
{
	interfaceInfo		*head = NULL, *temp;
	struct ifi_info		*ifi, *ifihead;
	struct sockaddr_in	*sa, *netmask;
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
//		    printf("  IP addr: %s\n", Sock_ntop_host(sa, sizeof(*sa)));


		if ( (netmask = (struct sockaddr_in *)ifi->ifi_ntmaddr) != NULL)
		    memcpy(&temp->ifi_ntmaddr, netmask,sizeof(struct sockaddr_in));
//			printf("  network mask: %s\n", Sock_ntop_host(sa, sizeof(*sa)));

		temp->ifi_next = head;
		head = temp;
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
/*	int					sockfd;
	const int			on = 1;
	pid_t				pid;
	struct ifi_info		*ifi, *ifihead;
	struct sockaddr_in	*sa, cliaddr, wildaddr;

	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1);
		 ifi != NULL; ifi = ifi->ifi_next) {
*/
			/*4bind unicast address */
/*		        sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

		        Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		        sa = (struct sockaddr_in *) ifi->ifi_addr;
		        sa->sin_family = AF_INET;
		        sa->sin_port = htons(SERV_PORT);
		        Bind(sockfd, (SA *) sa, sizeof(*sa));
		        printf("bound %s\n", Sock_ntop((SA *) sa, sizeof(*sa)));
	
	}
*/
        get_interfaces_client();
        return 0;
}

