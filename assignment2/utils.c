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

void loadContents(int type){
	FILE *input;
	int line = 1;
	char temp[50];
	if(type==1){
		input = fopen("server.in","r");
		bzero(servInfo,sizeof(servStruct));
		if (input == NULL){
			printf("Error in reading the file server.in");
			return;
		}
		fscanf(input, "%s", temp);
               	servInfo->serv_portNum = atoi(temp);
			
		fscanf(input, "%s", temp);
		servInfo->send_Window = atoi(temp);

		servInfo->ifi_head = get_interfaces_server(servInfo->serv_portNum);
		printf("SERVER PARAMETERS:\n");
                printf("server port: %ld\n", servInfo->serv_portNum);
                printf("max sending sliding window size: %d\n", servInfo->send_Window);
	}
	if(type==2){
		input = fopen("client.in","r");
		if (input == NULL){
                        printf("Error in reading the file client.in");
                        return;
                }
		bzero(clientInfo, sizeof(clientStruct));
                fscanf(input, "%s", temp);
		clientInfo->serv_addr.sin_addr.s_addr = atof(temp) ;

		fscanf(input, "%s", temp);
		clientInfo->serv_portNum = atoi(temp);	

		fscanf(input, "%s", temp);
                clientInfo->fileName = temp;

		fscanf(input, "%s", temp);
                clientInfo->rec_Window = atoi(temp);
	
		fscanf(input, "%s", temp);
                clientInfo->seed = atoi(temp);
	
		fscanf(input, "%s", temp);
                clientInfo->dg_lossProb = atof(temp);

                fscanf(input, "%s", temp);
                clientInfo->recv_rate = atoi(temp);

		clientInfo->ifi_head = get_interfaces_client();

		printf("CLIENT PARAMETERS:\n");	
    		printf("server ip: %ld\n", clientInfo->serv_addr.sin_addr.s_addr);
    		printf("server port: %d\n", clientInfo->serv_portNum);
    		printf("filename: %s\n", clientInfo->fileName);
    		printf("recieving sliding window size: %d\n", clientInfo->rec_Window);
    		printf("seed value: %d\n", clientInfo->seed);
    		printf("datagram loss probability: %f\n", clientInfo->dg_lossProb);
    		printf("mean receive rate (ms): %d\n", clientInfo->recv_rate);
	}
}
