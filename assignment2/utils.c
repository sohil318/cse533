#include        "utils.h"
#include	"unpifiplus.h"

interfaceInfo* get_interfaces_client()
{
	interfaceInfo		*head = NULL, *temp;
	struct ifi_info		*ifi, *ifihead;
	struct sockaddr_in	*sa, *netmask, *subnet;
	int sockfd;
	char src[128], dst[128];

	printf("\n ========================== CLIENT INTERFACES INFO =============================");
	printf("\n ======= Interface_Name    IP_Address    Subnet_Mask    Subnet_Address   =======");
	printf("\n ===============================================================================");
	printf("\n ||                                                                           ||");
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

                printf("\n ||        %s     ", ifi->ifi_name);
                inet_ntop(AF_INET, &temp->ifi_addr.sin_addr, src, sizeof(src));
                printf("\t %s",	src);
                inet_ntop(AF_INET, &temp->ifi_ntmaddr.sin_addr, src, sizeof(src));
                printf("\t%s",	src);
                inet_ntop(AF_INET, &temp->ifi_subnetaddr.sin_addr, src, sizeof(src));
                printf("\t %s\t      ||",	src);
		
	}
	printf("\n ||                                                                           ||");
	printf("\n ===============================================================================");

	free_ifi_info_plus(ifihead);
	return head;

}

interfaceInfo* get_interfaces_server(int portno)
{
	interfaceInfo		*head = NULL, *temp;
	struct ifi_info		*ifi, *ifihead;
	struct sockaddr_in	*sa, *netmask, *subnet, *sosa;
        const int               on = 1;
	int sockfd;
	char src[128], dst[128];

	printf("\n ================================= SERVER INTERFACES INFO ===============================");
	printf("\n ======= Interface_Name    IP_Address    Subnet_Mask    Subnet_Address     SockFD =======");
	printf("\n ========================================================================================");
	printf("\n ||                                                                                    ||");
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

		if (sa != NULL && netmask != NULL)
		{
		    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
		    Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		    sosa = (struct sockaddr_in *) ifi->ifi_addr;
		    sosa->sin_family = AF_INET;
		    sosa->sin_port = htons(portno);
		    if (bind(sockfd, (struct sockaddr *) sosa, sizeof(struct sockaddr_in)) < 0)
			err_sys("bind error");

		    temp->sockfd    = sockfd;
                
		    subnet = sa;
		    subnet->sin_addr.s_addr = (sa->sin_addr.s_addr & netmask->sin_addr.s_addr);
		    memcpy(&temp->ifi_subnetaddr, subnet, sizeof(struct sockaddr_in));

		    temp->ifi_next  = head;
		    head = temp;
		    
		    printf("\n ||         %s     ", ifi->ifi_name);
		    inet_ntop(AF_INET, &temp->ifi_addr.sin_addr, src, sizeof(src));
		    printf("\t %s",	src);
		    inet_ntop(AF_INET, &temp->ifi_ntmaddr.sin_addr, src, sizeof(src));
		    printf("\t %s",	src);
		    inet_ntop(AF_INET, &temp->ifi_subnetaddr.sin_addr, src, sizeof(src));
		    printf("\t %s",	src);
		    
                    printf("\t     %d\t       ||",   temp->sockfd);
		}
	}
	printf("\n ||                                                                                    ||");
	printf("\n ========================================================================================\n");


	free_ifi_info_plus(ifihead);
	return head;


}
/*
int
main(int argc, char **argv)
{
	interfaceInfo		*head = NULL, *temp;
        int count = 0;
        head = loadClientInfo();
	while (head)
	{
	    count++;
	    head = head->ifi_next;
	}
	//printf ("Count = %d \n", count);
        count = 0;
	head = loadServerInfo();
	while (head)
	{
	    count++;
	    head = head->ifi_next;
	}
	//printf ("Count = %d \n", count);
	//loadContents(1);
        return 0;
}
*/
interfaceInfo * loadServerInfo()
{
        FILE *input;
        char temp[50];
        input = fopen("server.in","r");
        servInfo = (servStruct *)calloc(1, sizeof(servStruct));
        if (input == NULL){
                printf("Error in reading the file server.in");
                return;
        }
        fscanf(input, "%s", temp);
        servInfo->serv_portNum = atoi(temp);

        fscanf(input, "%s", temp);
        servInfo->send_Window = atoi(temp);

        printf("\n ============================================================== ");
        printf("\n                      SERVER PARAMETERS                         ");
        printf("\n ============================================================== ");
        printf("\n | server port                        :       %ld            |", servInfo->serv_portNum);
        printf("\n | max sending sliding window size    :       %d              |", servInfo->send_Window);
        printf("\n ============================================================== \n");
        servInfo->ifi_head = get_interfaces_server(servInfo->serv_portNum);

        return servInfo->ifi_head;
}


interfaceInfo * loadClientInfo()
{
	FILE *input;
	char temp[50];
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

        printf("\n ============================================================== ");
        printf("\n                      CLIENT PARAMETERS                         ");
        printf("\n ============================================================== ");
        printf("\n |    server ip                       :       %ld     |", clientInfo->serv_addr.sin_addr.s_addr);
        printf("\n |    server port                     :       %d            |", clientInfo->serv_portNum);
        printf("\n |    filename                        :       %s      |", clientInfo->fileName);
        printf("\n |    recieving sliding window size   :       %d              |", clientInfo->rec_Window);
        printf("\n |    seed value                      :       %d               |", clientInfo->seed);
        printf("\n |    datagram loss probability       :       %f       |", clientInfo->dg_lossProb);
        printf("\n |    mean receive rate (ms)          :       %d             |", clientInfo->recv_rate);
        printf("\n ============================================================== \n");
        
        clientInfo->ifi_head = get_interfaces_client();
        
        return clientInfo->ifi_head;
}
