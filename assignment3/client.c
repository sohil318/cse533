#include "unp.h"
#include "utils.h"
#include "odr.h"
#include  <setjmp.h>

char serverVM[255], clientVM[255];
static sigjmp_buf jmpbuf;

static void timeout_resend(int signo)
{
        printf("\n client at node %s timeout on response from %s", clientVM, serverVM);
        siglongjmp(jmpbuf, 1);
}

int main(int argc, char **argv)
{	
        int sockfd, fd, source_port, flag;
        char temp[100], ip_dest[IP_SIZE], ip_source[IP_SIZE];
        mrecv recvd_msg;
        char send_msg[MSG_STREAM_SIZE] = "Time requested from server.";
        struct sockaddr_un clientaddr, serveraddr;
        struct hostent *hp;

        Signal(SIGALRM, timeout_resend);

        /* Creating domain datagram socket */
        sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
        bzero(&clientaddr, sizeof(clientaddr));
        clientaddr.sun_family = AF_LOCAL;
        strcpy(temp,"/tmp/fileXXXXXX");
        fd = mkstemp(temp);
        if(fd<0){
                printf("\nSunpath cannot be created");
                return -1;
        }	
        close(fd);
        unlink(temp);
        strncpy(clientaddr.sun_path, temp, sizeof(clientaddr.sun_path) - 1);
        Bind(sockfd, (SA *) &clientaddr, SUN_LEN(&clientaddr));
        printf("\nClient bound at sun_path : %s", clientaddr.sun_path);	
        gethostname(clientVM, sizeof(clientVM));
        while(1) {
userinput:
                flag = 0;
                printf("\nSelect the VM server node : vm1, vm2, vm3, ..., vm10 \n");

                if ( scanf("%s",serverVM) != 1)
                        continue;

                hp = (struct hostent *)gethostbyname((const char *)serverVM);	

                sprintf(ip_dest, "%s", (char *)inet_ntoa( *(struct in_addr *)(hp->h_addr_list[0])));
                printf("\nServer canonical IP : %s", ip_dest);	
                printf("\nClient at node %s sending request to server at %s", clientVM, serverVM);
sending:
                msg_send(sockfd, ip_dest, SERV_PORT_NO, send_msg, flag);	//yet to be filled
                alarm(100);
                if(sigsetjmp(jmpbuf,1)!=0)
                {
                        if(flag == 0) {
                                flag = 1;     
                                goto sending;
                        }   	
                        else {
                                goto userinput;
                        }	
                }	
                //	        bzero(&recvd_msg, sizeof(mrecv));
                msg_recv(sockfd, &recvd_msg, &serveraddr);  
                //msg_recv(sockfd, recvd_msg, ip_source, source_port);	//yet to be filled
                printf("\nClient received time %s from server at %s\n", recvd_msg.msg, serverVM);

                alarm(0);
        }
        unlink(temp);

}

