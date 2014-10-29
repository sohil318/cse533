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

servStruct * loadServerInfo()
{
        servStruct *servInfo;
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

        return servInfo;
}

clientStruct * loadClientInfo()
{
	clientStruct *clientInfo;
	char src[128];
        FILE *input;
	char temp[50];
        input = fopen("client.in","r");
        if (input == NULL){
                printf("Error in reading the file client.in");
                return;
        }
        
        clientInfo = (clientStruct*)calloc(1,sizeof(clientStruct));
        fscanf(input, "%s", temp);
        inet_pton(AF_INET, temp, &clientInfo->serv_addr.sin_addr);

        fscanf(input, "%s", temp);
        clientInfo->serv_portNum = atoi(temp);	

        fscanf(input, "%s", temp);
//        printf("Input str : %s", temp);
        temp[sizeof(temp)] = '\0';
	strncpy(clientInfo->fileName, temp, sizeof(temp));

        fscanf(input, "%s", temp);
        clientInfo->rec_Window = atoi(temp);

        fscanf(input, "%s", temp);
        clientInfo->seed = atoi(temp);

        fscanf(input, "%s", temp);
        clientInfo->dg_lossProb = atof(temp);

        fscanf(input, "%s", temp);
        clientInfo->recv_rate = atoi(temp);

        inet_ntop(AF_INET, &clientInfo->serv_addr.sin_addr, src, sizeof(src));

        printf("\n ============================================================== ");
        printf("\n                      CLIENT PARAMETERS                         ");
        printf("\n ============================================================== ");
        printf("\n |    server ip                       :       %s     |", src);
        printf("\n |    server port                     :       %d            |", clientInfo->serv_portNum);
        printf("\n |    filename                        :       %s      |", clientInfo->fileName);
        printf("\n |    recieving sliding window size   :       %d              |", clientInfo->rec_Window);
        printf("\n |    seed value                      :       %d               |", clientInfo->seed);
        printf("\n |    datagram loss probability       :       %f       |", clientInfo->dg_lossProb);
        printf("\n |    mean receive rate (ms)          :       %d             |", clientInfo->recv_rate);
        printf("\n ============================================================== \n");
        
        clientInfo->ifi_head = get_interfaces_client();
        
        return clientInfo;
}

hdr* createHeader(int msgtype, int seqnum, int advwin, int ts)
{
        hdr *header;
	header = (hdr *)malloc(sizeof(hdr));
        header->msg_type        = msgtype;
        header->seq_num         = seqnum;
        header->adv_window      = advwin;
        header->timestamp       = ts;
        return header;
}

struct msghdr* createDataPacket(hdr *dataheader, char *data, int datalen)
{
        struct msghdr *datamsg;
	datamsg = (struct msghdr *) malloc (sizeof(struct msghdr));
        struct iovec dataiovec[2];

//	printf("\nCreated Packet \n"); //iovec[1].iov_base);
        bzero(datamsg, sizeof(struct msghdr));
        datamsg->msg_name       = NULL;
        datamsg->msg_namelen    = 0;
        datamsg->msg_iov        = dataiovec;
        datamsg->msg_iovlen     = 2;

        dataiovec[0].iov_base   = (void *)dataheader;
        dataiovec[0].iov_len    = sizeof(hdr);
        dataiovec[1].iov_base   = data;
	printf("\nCreated Packet %s\n", dataiovec[1].iov_base);
        dataiovec[1].iov_len    = datalen;
	printf("\nCreated Packet %d\n", dataiovec[1].iov_len);

        return datamsg;
}

struct msghdr createAckPacket(hdr ackheader)
{
        struct msghdr ackmsg;
        struct iovec ackiovec[1];

        bzero(&ackmsg, sizeof(struct msghdr));
        ackmsg.msg_name       = NULL;
        ackmsg.msg_namelen    = 0;
        ackmsg.msg_iov        = ackiovec;
        ackmsg.msg_iovlen     = 1;

        ackiovec[0].iov_base   = (void *)&ackheader;
        ackiovec[0].iov_len    = sizeof(ackheader);

        return ackmsg;
}

hdr* readHeader(struct msghdr *messageHeader)
{
	hdr *header;
	struct iovec headeriovec[1];

	if(messageHeader->msg_iov[0].iov_base != NULL) {
	    headeriovec[0] = messageHeader->msg_iov[0];
	    header = (hdr *)headeriovec[0].iov_base;
	}
	else {
	    printf("\n readHeader : Header is NULL\n");
	    return;
	}
	return header;
}

void* readData(struct msghdr *messageHeader)
{
	struct iovec dataiovec[1];
	if(messageHeader->msg_iov[1].iov_base){
	    dataiovec[1] = messageHeader->msg_iov[1];
	    return dataiovec[1].iov_base;
	} 
	else{
	    printf("\n readData : payloadNULL in msghdr \n");
	    return;
	}
}
