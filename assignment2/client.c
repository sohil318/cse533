#include	 "utils.h"

#define LOOPBACK "127.0.0.1"

/* 
 * Function to check if client and server have same host network. 
 */

int checkLocal (struct clientStruct **cliInfo)
{
        int isLocal;  
        struct sockaddr_in sa;
        struct clientStruct *temp = *cliInfo;
        struct InterfaceInfo *head = temp->ifi_head;
        char src[128];

        inet_ntop(AF_INET, &temp->serv_addr.sin_addr, src, sizeof(src));
        if (strcmp(src, LOOPBACK) == 0)
        {
                printf ("\nServer IP is Loopback Address. Client IP = 127.0.0.1");
                temp->cli_addr = temp->serv_addr;
                return 1;
        }
        /*
        while (head)
        {
                        
        }
        */

        return 0;

}

int main(int argc, char **argv)
{	
	struct clientStruct *clientInfo = loadClientInfo();
        
        if (clientInfo)
                checkLocal(&clientInfo);
        
}

