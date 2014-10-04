#include "unp.h"
#define AFI AF_INET

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
                strcpy(ip, "\0");
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
    struct hostent *hp;
    struct in_addr ipv4addr;
    char buf[MAXLINE], temp[10];
    int n;
    int pfd[2];
    pid_t pid;

    if (argc != 2)
            err_quit("./client <IPAddress>");

    char ipAddress[100], *hostname = argv[1];
    
    //inet_pton(AFI, ipAddress, &ipv4addr);
    //hp = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AFI);
    
    int ret = hostname_to_ip(hostname , ipAddress); 
    
    if (ret == -1)
        printf("gethostbyname failed");

    //printf("Return = %d, %s", ret, ipAddress);
    if (strcmp(ipAddress, hostname) == 0)
    {
        inet_pton(AFI, ipAddress, &ipv4addr);
        hp = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AFI);
        printf("The server host is %s.\n", hp->h_name);
    }
    
    while (1)
    {
            printf("\n\n1. Echo Client. \n2. Date-Time Client. \n3. Exit Gracefully. \nEnter your choice. (1 - 3) : \t");
            if (!scanf("%d", &n))
                    printf("\nInvalid Input type.");
            switch (n) {
                case 1:
                        if (pipe(pfd) == -1)
                        {
                                perror("pipe failed");
                                exit (1);
                        }
                        if ((pid = fork()) < 0)
                                perror("fork error");
                        else if (pid == 0) {
                                close(pfd[0]);
                                sprintf(temp, "%d", pfd[1]);
                                execlp("xterm", "xterm", "-e", "./echo_cli", ipAddress, temp, (char *)0);
                                close(pfd[1]);
                        }
                        else
                        {
                                //printf("Enter parent client");
                                close(pfd[1]);
                                n = read(pfd[0], buf, 1024);
		                if (n == -1)
                                        err_sys("read error");
                                else {
                                        buf[n] = '\0';
                                        if ( n == 0 )
                                        {
                                                printf ("\nEcho Child Terminated !!!\n");
                                                wait(NULL);
                                        }
                                        else if (strcmp(buf, "DONE") == 0)
                                        {
                                                printf ("\nEcho Forked Child Terminated !!!\n");
                                                wait(NULL);
                                        }
                                        else
                                                printf ("\nPipe Message from Echo Client : %s\n", buf);
                                }
                                close(pfd[0]);
                        }
                        break;
                case 2:
                        if (pipe(pfd) == -1)
                        {
                                perror("pipe failed");
                                exit (1);
                        }
                        if ((pid = fork()) < 0)
                                perror("fork error");
                        else if (pid == 0) {
                                close(pfd[0]);
                                sprintf(temp, "%d", pfd[1]);
                                execlp("xterm", "xterm", "-e", "./time_cli", ipAddress, temp, (char *)0);
                                close(pfd[1]);
                        }
                        else
                        {
                                //printf("Enter parent client");
                                close(pfd[1]);
                                n = read(pfd[0], buf, 1024);
                                if (n == -1)
                                        err_sys("read error");
                                else {
                                        buf[n] = '\0';
                                        if ( n == 0 )
                                        {
                                                printf ("\nTime Child Terminated !!!\n");
                                                wait(NULL);
                                        }
                                        else if (strcmp(buf, "DONE") == 0)
                                        {
                                                printf ("\nTime Forked Child Terminated !!!\n");
                                                wait(NULL);
                                        }
                                        else
                                                printf ("\nPipe Message from Time Client : %s\n", buf);
                                }
                                close(pfd[0]);
                        }
                        break;
                case 3:
                        exit(0);
                        break;
                default:
                        printf("\nPlease enter valid choice.\n");
                        break;
            }
    }
    return 0;    
}

