#include "unp.h"
#define MAXBUF 4096
#define AFI AF_INET

void str_echoCli(FILE *fp, int sockfd)
{
        char	sendline[MAXBUF], recvline[MAXBUF];
        char	*ptr;
        int     len, n;

        while ( (ptr = fgets(sendline, MAXBUF, fp)) != NULL) {

                len = strlen(sendline);
                if (writen(sockfd, sendline, len) != len)
                        err_sys("writen error");

                if ((n = readline(sockfd, recvline, MAXBUF)) == 0)
                        err_quit("str_cli: server terminated prematurely");
                else if (n < 0)
                        err_sys("readline error");

                if (fputs(recvline, stdout) == EOF)
                        err_sys("fputs error");

        }
        if (ptr == NULL && ferror(fp))
                err_sys("fgets error");

}

void start_echoClient(char *ipAddress, int portNum)
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

        /* Check in you have to write an own version of this function . */
        str_cli(stdin, sockFD);
}

int main(int argc, char **argv)
{
        struct hostent *hp;
        struct in_addr ipv4addr;

        if (argc != 2)
                err_quit("./echo_cli <IPAddress>");

        char *ipAddress = argv[1];
        int portNo = 5001;       //atoi(argv[2]);

        start_echoClient(ipAddress, portNo);

        return 0;    
}

