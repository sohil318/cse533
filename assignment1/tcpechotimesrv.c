#include	"unpthread.h"
#include <time.h>
#include <pthread.h>
#include "unp.h"
#define MAXBUF 4096
#define AFI AF_INET

void * timeSrv(void *);		/* each thread executes this function */
void * echoSrv(void *);		/* each thread executes this function */

void * timeSrv(void *arg)
{
        int     connFD, len, maxfdpl, n, nready;
        time_t  ticks;
        char    recvBuffer[MAXBUF + 1];
        struct timeval tv;
        fd_set  rset, allset;

	connFD = *((int *) arg);
	free(arg);
//        printf("Connfd = %d", connFD);
        if ( (n = pthread_detach(pthread_self())) != 0) {
        	errno = n;
	        err_sys("pthread_detach error");
        }
        //while (1);
        maxfdpl = connFD;
        FD_ZERO(&allset);
        FD_SET(connFD, &allset);
        tv.tv_sec  = 0;
        tv.tv_usec = 0;

        while (1)
        {
                rset = allset;
                nready = select(maxfdpl + 1, &rset, NULL, NULL, &tv); 
	        if (nready < 0)
		        err_sys("select error");
                
                if (nready > 0)      {
		        n = read(connFD, recvBuffer, MAXBUF);
                        if ( n == -1 )
                		err_sys("read error");
                        else if (n == 0) {
           //                     printf("\nHere\n");
                                if (close(connFD) == -1)
		                        err_sys("close error");
				FD_CLR(connFD, &allset);
                                pthread_exit(arg);
                                exit (0);

                        }
                 }
                 else {
                                 ticks = time(NULL);
                                 snprintf(recvBuffer, sizeof(recvBuffer), "%.24s\r\n", ctime(&ticks));
                                 len = strlen(recvBuffer);
                                 if (write(connFD, recvBuffer, len) != len)
                                         err_sys("write error");
                 }
                tv.tv_sec = 5;
                tv.tv_usec = 0;

        }

        if (close(connFD) == -1)
                err_sys("close error");

	//Close(connfd);			/* done with connected socket */
	return(NULL);
}

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

void * echoSrv(void *arg)
{
        int     connFD, n;

	connFD = *((int *) arg);
	free(arg);
//        printf("Connfd = %d", connFD);
        if ( (n = pthread_detach(pthread_self())) != 0) {
        	errno = n;
                err_sys("pthread_detach error");
        }
        
        str_echo(connFD);       
        if (close(connFD) == -1)
                err_sys("close error");

	//Close(connfd);			/* done with connected socket */
	return(NULL);
}


int serverListen(int portNum)
{

        int     listenSockFD, connFD, backlog;
        char	*ptr;
        struct  sockaddr_in servAddr;
        int     option = 1;

        if ( (listenSockFD = socket(AFI, SOCK_STREAM, 0)) < 0)
                err_sys("socket creation error");

        bzero(&servAddr, sizeof(servAddr));
        servAddr.sin_family = AFI;
        servAddr.sin_port = htons(portNum);
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(listenSockFD, (SA *) &servAddr, sizeof(servAddr)) < 0)
                err_sys("bind error");

        if(setsockopt(listenSockFD, SOL_SOCKET, SO_REUSEADDR,(char*)&option, sizeof(option)) < 0)
        {
                printf("setsockopt failed\n");
                close(listenSockFD);
                exit(2);
        }


        backlog = LISTENQ;

        /*4can override 2nd argument with environment variable */
        if ( (ptr = getenv("LISTENQ")) != NULL)
                backlog = atoi(ptr);

        if (listen(listenSockFD, backlog) < 0)
                err_sys("listen error");

        return listenSockFD;
}

int main(int argc, char **argv)
{
	int	        *connFD, listenfdT, listenfdE, *iptr;
        int             maxfdpl, n, nready;
	pthread_t	tid;
	socklen_t	addrlen;
	struct sockaddr	*cliaddr;
        int portNumT = 5002, portNumE = 5003;
        fd_set  rset, allset;

        if (argc != 1)
                err_quit("./server");

	listenfdT = serverListen(portNumT);
	listenfdE = serverListen(portNumE);

        maxfdpl = max(listenfdT, listenfdE);
        FD_ZERO(&allset);
        FD_SET(listenfdT, &allset);
        FD_SET(listenfdE, &allset);

        for(;;)
        {
                rset = allset;
                nready = select(maxfdpl + 1, &rset, NULL, NULL, NULL); 
	        if (nready < 0)
		        err_sys("select error");
                if (FD_ISSET(listenfdT, &rset))      {
	                if ( (connFD = (int *)malloc(sizeof(int))) == NULL)
		                err_sys("malloc error");
again:
                        if ( (*connFD = accept(listenfdT, (SA *) NULL, NULL)) < 0) {
#ifdef	EPROTO
                                if (errno == EPROTO || errno == ECONNABORTED)
#else   
                                        if (errno == ECONNABORTED)
#endif  
                                                goto again;
                                        else
                                                err_sys("accept error");
                        }
//                        printf("time thread creation %d", *connFD);
	                pthread_create(&tid, NULL, &timeSrv, connFD);
                }
                if (FD_ISSET(listenfdE, &rset))      {
                        if ((connFD = (int *)malloc(sizeof(int))) == NULL)
                                err_sys("malloc error");
again1:
                        if ( (*connFD = accept(listenfdE, (SA *) NULL, NULL)) < 0) {
#ifdef	EPROTO
                                if (errno == EPROTO || errno == ECONNABORTED)
#else   
                                        if (errno == ECONNABORTED)
#endif  
                                                goto again1;
                                        else
                                                err_sys("accept error");
                        }
	                pthread_create(&tid, NULL, &echoSrv, connFD);
                }
        }
}

