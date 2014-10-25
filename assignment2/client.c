#include	 "utils.h"

#define LOOPBACK "127.0.0.1"

/*
 * Function to get subnet mask bits
 */

int getSubnetCount(unsigned long netmsk)
{
    int count = 0;
    while (netmsk)
    {
	netmsk = netmsk & (netmsk - 1);
	count++;
    }
    return count;
}

/* 
 * Function to check if client and server have same host network. 
 */

int checkLocal (struct clientStruct **cliInfo)
{
        int isLocal;  
        struct sockaddr_in sa, *subnet;
        struct clientStruct *temp = *cliInfo;
        struct InterfaceInfo *head = temp->ifi_head;
        char src[128];
	int maxlcs = -1, lcs;

        inet_ntop(AF_INET, &temp->serv_addr.sin_addr, src, sizeof(src));
        if (strcmp(src, LOOPBACK) == 0)
        {
                printf ("\nServer IP is Loopback Address. Client IP = 127.0.0.1\n");
                temp->cli_addr = temp->serv_addr;
                cliInfo = &temp;
		return 1;
        }
        
        while (head)
        {
		if ((temp->serv_addr.sin_addr.s_addr & head->ifi_ntmaddr.sin_addr.s_addr) == head->ifi_subnetaddr.sin_addr.s_addr)
                {
//			printf("\nFound match\n");
			lcs = getSubnetCount(head->ifi_ntmaddr.sin_addr.s_addr);
			if (lcs > maxlcs)
			{
			    maxlcs = lcs;
			    temp->cli_addr = head->ifi_addr;  
			}
		}
		head = head->ifi_next;
        }
	cliInfo = &temp;
	if (maxlcs == -1)
	    return 0;
	else
	    return 1;
	
	//inet_ntop(AF_INET, &temp->serv_addr.sin_addr, src, sizeof(src));
        //printf("\nClient Address : %s",	src);

        return 0;

}

int main(int argc, char **argv)
{	
	struct clientStruct *clientInfo = loadClientInfo();
        int isLocal;
	char src[128];

        if (clientInfo)
                isLocal = checkLocal(&clientInfo);
        
	if (isLocal)
	    printf("\nIP Address is local\n");
	else
	    printf("\nServer IP is non local.\n");
        
	inet_ntop(AF_INET, &clientInfo->cli_addr.sin_addr, src, sizeof(src));
        printf("\nClient Address : %s",	src);

}

