#include    "unp.h"
#include    "utils.h"
#include    "odr.h"

int main(int argv, char **argc){
    
    char msg[MSG_STREAM_SIZE], serverVM[STR_SIZE], reply[MSG_STREAM_SIZE];
    int sockfd;
    struct sockaddr_un clientaddr, serveraddr;
    struct hostent *hp;
    mrecv recvmsg;

    time_t time_s;
    in_addr_t ip;

    bzero(&serveraddr, sizeof(serveraddr));
    bzero(&clientaddr, sizeof(clientaddr));
   
    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    
    unlink(SERV_SUN_PATH);
    serveraddr.sun_family = AF_LOCAL;
    strcpy(serveraddr.sun_path, SERV_SUN_PATH);
    
    Bind(sockfd, (SA*) &serveraddr, sizeof(serveraddr));
    printf("\nServer Bound at sun_path %s\n" , serveraddr.sun_path);
    gethostname(serverVM, sizeof(serverVM));
    while (1)
    {
        msg_recv(sockfd, &recvmsg, &clientaddr);
        ip = inet_addr(recvmsg.srcIP);  
        
        hp = gethostbyaddr((char*)&ip, sizeof(ip), AF_INET);
        
        printf("\nServer at node %s responding to request from %s", serverVM, hp->h_name);
        time_s = time(NULL);
        bzero(reply, sizeof(reply));
        snprintf(reply,sizeof(reply),"%.24s",(char *)ctime(&time_s));
        
        msg_send(sockfd, recvmsg.srcIP, recvmsg.srcportno, reply, 0);
    }
}
