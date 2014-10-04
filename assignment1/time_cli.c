#include "unp.h"
#include <signal.h>
#define MAXBUF 4096
#define AFI AF_INET

int ppid;

void sigint_handler(int signo)
{
        if (signo == SIGINT)
        {
                printf ("received sigint");
	        if (writen(ppid, "DONE", 4) != 4)
		        err_sys("writen error");
                exit (0);
        }
}

void error_handler (char * errMsg)
{
        int len = strlen(errMsg);
        if (writen(ppid, errMsg, len) != len)
                err_sys("writen error");
        exit (0);
}

void start_timeClient(char *ipAddress, int portNum)
{
        int sockFD, len, nready, maxfdpl, n;
        char recvBuffer[MAXBUF + 1];
        struct sockaddr_in servAddr;
        struct timeval tv;
        fd_set  rset, allset;
        
        //printf("Time client child");
        if ( (sockFD = socket(AFI, SOCK_STREAM, 0)) < 0)
                error_handler("socket creation error");
        
        bzero(&servAddr, sizeof(servAddr));
        servAddr.sin_family = AFI;
        servAddr.sin_port = htons(portNum);
        if (inet_pton(AFI, ipAddress, &servAddr.sin_addr) <= 0)
                error_handler("inet_pton error ");
     
        if (connect(sockFD, (SA *) &servAddr, sizeof(servAddr)) < 0)
		error_handler("connect error");

        while ( (len = read(sockFD, recvBuffer, MAXLINE)) > 0) {
                recvBuffer[len] = '\0';
                if (fputs(recvBuffer, stdout) == EOF)
                        error_handler("fputs error");
        }

        if (len < 0)
                error_handler("read error");

}

int main(int argc, char **argv)
{
    if (argc < 2)
            err_quit("./time_cli <IPAddress> <pipe fd>");

    char *ipAddress = argv[1];
    ppid = atoi(argv[2]);
    int portNo = 5002;        
    signal (SIGINT, sigint_handler);
    
    start_timeClient(ipAddress, portNo);
    
    return 0;    
}

