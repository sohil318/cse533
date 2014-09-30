#include "unp.h"
#define MAXBUF 4096
#define AFI AF_INET

void start_timeClient(char *ipAddress, int portNum)
{
        int sockFD, len;
        char recvBuffer[MAXBUF + 1];
        struct sockaddr_in servAddr;
        if ( (sockFD = socket(AFI, SOCK_STREAM, 0)) < 0)
                err_sys("socket creation error");
        
        bzero(&servAddr, sizeof(servAddr));
        servAddr.sin_family = AFI;
        servAddr.sin_port = htons(portNum);
        if (inet_pton(AFI, ipAddress, &servAddr.sin_addr) <= 0)
		err_quit("inet_pton error for %s", ipAddress);

        if (connect(sockFD, (SA *) &servAddr, sizeof(servAddr)) < 0)
		err_sys("connect error");

	while ( (len = read(sockFD, recvBuffer, MAXLINE)) > 0) {
		recvBuffer[len] = '\0';
		if (fputs(recvBuffer, stdout) == EOF)
			err_sys("fputs error");
	}

	if (len < 0)
		err_sys("read error");
        
}

/*
   Get ip from domain name
 */

int hostname_to_ip(char *hostname , char* ip)
{
        struct hostent *he;
        struct in_addr **addr_list;
        int i;

        if ( (he = gethostbyname( hostname ) ) == NULL) 
        {
                // get the host info
                //herror("gethostbyname");
                return -1;
        }

        addr_list = (struct in_addr **) he->h_addr_list;

        for(i = 0; addr_list[i] != NULL; i++) 
        {
                //Return the first one;
                strcpy(ip , inet_ntoa(*addr_list[i]) );
                return 0;
        }

        return 1;
}

int main(int argc, char **argv)
{
    //printf("\nHello Sohil");
    struct hostent *hp;
    struct in_addr ipv4addr;
    
    if (argc != 2)
            err_quit("./time_cli <IPAddress>");

    char ipAddress[100], *hostname = argv[1];
    int portNo = 5000;          //atoi(argv[2]);
    
    inet_pton(AFI, ipAddress, &ipv4addr);
    hp = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AFI);

    int ret = hostname_to_ip(hostname , ipAddress);
    //printf("Return = %d, %s", ret, ipAddress);
    if (strcmp(ipAddress, hostname) == 0)
    {
        inet_pton(AFI, ipAddress, &ipv4addr);
        hp = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AFI);
        printf("The server host is %s.\n", hp->h_name);
    }

    
    start_timeClient(ipAddress, portNo);
    
    return 0;    
}

