#include "unp.h"
#include <time.h>

#define MAXBUF 4096
#define AFI AF_INET

void
str_echoSrv(int sockFD)
{
	int  nbytes;
	char buff[MAXBUF];

again:
	while ( (nbytes = read(sockFD, buff, MAXBUF)) > 0)
        {
                printf ("\nServer Data : %s", buff);
	        if (writen(sockFD, buff, nbytes) != nbytes)
		        err_sys("writen error");
        }
	if (nbytes < 0 && errno == EINTR)
		goto again;
	else if (nbytes < 0)
		err_sys("str_echo: read error");
}


void start_echoServer(int portNum)
{
        int listenSockFD, connFD, backlog;
        char	*ptr;
        pid_t childpid;
        struct sockaddr_in servAddr;

        if ( (listenSockFD = socket(AFI, SOCK_STREAM, 0)) < 0)
                err_sys("socket creation error");

        bzero(&servAddr, sizeof(servAddr));
        servAddr.sin_family = AFI;
        servAddr.sin_port = htons(portNum);
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(listenSockFD, (SA *) &servAddr, sizeof(servAddr)) < 0)
                err_sys("bind error");

        backlog = LISTENQ;

        /*4can override 2nd argument with environment variable */
        if ( (ptr = getenv("LISTENQ")) != NULL)
                backlog = atoi(ptr);

        if (listen(listenSockFD, backlog) < 0)
                err_sys("listen error");

        while (1) {
again:
                if ( (connFD = accept(listenSockFD, (SA *) NULL, NULL)) < 0) {
#ifdef	EPROTO
                        if (errno == EPROTO || errno == ECONNABORTED)
#else
                                if (errno == ECONNABORTED)
#endif
                                        goto again;
                                else
                                        err_sys("accept error");
                }
                if ( (childpid = fork()) == -1)
	                err_sys("fork error");
                else if (childpid == 0)
                {
                        if (close(listenSockFD) == -1)
                                err_sys("close error");
                        str_echoSrv(connFD);       
                        exit (0);
                }
                if (close(connFD) == -1)
                        err_sys("close error");
        }
}

int main(int argc, char **argv)
{
        //printf("\nHello Sohil");

        if (argc != 1)
                err_quit("./echo_srv");

        int portNo = 5001;           //atoi(argv[1]);
        start_echoServer(portNo);
        return 0;    
}

