#include "unp.h"
#define MAXBUF 4096
#define AFI AF_INET

int ppid;

int sigint_handler(int signo)
{
        if (signo == SIGINT)
        {
                printf ("received sigint");
	        if (writen(ppid, "DONE", 4) != 4)
		        err_sys("writen error");
                exit (0);
        }
}

void str_echoCli(FILE *fp, int sockFD)
{
        char    recvline[MAXBUF];
        char	*ptr;
        int     len, n, maxfdpl, stdineof = 0, nready;
        fd_set  rset;

        FD_ZERO(&rset);
	for ( ; ; ) {
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockFD, &rset);
		maxfdpl = max(fileno(fp), sockFD);
		//Select(maxfdp1, &rset, NULL, NULL, NULL);
                nready = select(maxfdpl + 1, &rset, NULL, NULL, NULL); 
	        if (nready < 0)
		        err_sys("select error");

		if (FD_ISSET(sockFD, &rset)) {	/* socket is readable */
	                n = readline(sockFD, recvline, MAXBUF);
	                if (n == -1)
		                err_sys("read error");
			else if (n == 0) {
				if (stdineof == 1)
					return;		/* normal termination */
				else
				        err_quit("str_cli: server terminated prematurely");
			}
	                if (write(fileno(stdout), recvline, n) != n)
		                err_sys("write error");
		}

		if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable */
	                n = readline(fileno(fp), recvline, MAXBUF);
	                if (n == -1)
		                err_sys("read error");
			else if (n == 0) { 
				stdineof = 1;
				/* send FIN */
	                        if (shutdown(sockFD, SHUT_WR) < 0)
		                        err_sys("shutdown error");
				FD_CLR(fileno(fp), &rset);
				continue;
			}

	                if (writen(sockFD, recvline, n) != n)
		                err_sys("writen error");
			//Writen(sockfd, buf, n);
		}
	}
/*
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
*/
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
        str_echoCli(stdin, sockFD);
}

int main(int argc, char **argv)
{
        struct hostent *hp;
        struct in_addr ipv4addr;

        if (argc < 2)
                err_quit("./time_cli <IPAddress>");

        char *ipAddress = argv[1];
        ppid = atoi(argv[2]);
        int portNo = 5001;          //atoi(argv[2]);
        signal (SIGINT, sigint_handler);

        start_echoClient(ipAddress, portNo);

        return 0;    
}

