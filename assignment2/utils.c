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
	struct sockaddr_in	*sa, *netmask, *subnet;
        const int               on = 1;
	int sockfd;
	char src[128], dst[128];

	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1);
		 ifi != NULL; ifi = ifi->ifi_next) {
		
		temp = (interfaceInfo *)malloc(sizeof(interfaceInfo));

		if ( (sa = (struct sockaddr_in *)ifi->ifi_addr) != NULL)        {
		    //printf("Copied");
		    memcpy(&temp->ifi_addr, sa, sizeof(struct sockaddr_in));
		}

		if ( (netmask = (struct sockaddr_in *)ifi->ifi_ntmaddr) != NULL)        {
		    //printf("Copied");
		    memcpy(&temp->ifi_ntmaddr, netmask, sizeof(struct sockaddr_in));
		}

                subnet = sa;
		subnet->sin_addr.s_addr = (sa->sin_addr.s_addr & netmask->sin_addr.s_addr);

		memcpy(&temp->ifi_subnetaddr, subnet, sizeof(struct sockaddr_in));

		sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
		Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		sa = (struct sockaddr_in *) ifi->ifi_addr;
		sa->sin_family = AF_INET;
		sa->sin_port = htons(portno);
		Bind(sockfd, (SA *) sa, sizeof(*sa));
                
                temp->sockfd    = sockfd;
		temp->ifi_next  = head;
		head = temp;

		printf("\n\n%s: \n", ifi->ifi_name);
		printf("  Socket FD: %d\n",   temp->sockfd);
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
        count = 0;
        head = get_interfaces_server(9876);
	while (head)
	{
	    count++;
	    head = head->ifi_next;
	}
	printf ("Count = %d \n", count);
	//loadContents(1);
        return 0;
}

void loadContents(int type){
	FILE *input;
	char temp[50];
	if(type==1){
		input = fopen("server.in","r");
		servInfo = (servStruct*)calloc(1,sizeof(servStruct));
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
		clientInfo = (clientStruct*)calloc(1,sizeof(clientStruct));
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
