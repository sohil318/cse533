#include        "utils.h"
#include 	"odr.h"
#include        "unp.h"

/* Message Send API */
int msg_send(int sockfd, char *ip_dest, int dest_port, char *msg, int flag) 
{
    struct sockaddr_un addr;
    char sendcstream[MSG_STREAM_SIZE];

    sprintf(sendcstream, "%s;%d;%d;%s", ip_dest, dest_port, flag, msg);
    printf("\nSending Stream : %s", sendcstream);

    bzero(&addr, sizeof(struct sockaddr_un));
    addr.sun_family = AF_LOCAL;
    strcpy(addr.sun_path, UNIX_DGRAM_PATH);

    return sendto(sockfd, sendcstream, strlen(sendcstream), 0, (SA *) &addr, sizeof(addr));
}

/* Message Receive API */
int msg_recv(int sockfd, mrecv * recv_p, struct sockaddr_un *addr)  
{

    char msg_stream[MSG_STREAM_SIZE];
    int len = sizeof(struct sockaddr_un);
    if (recvfrom(sockfd, msg_stream, MSG_STREAM_SIZE, 0, (SA *)addr, &len) < 0)
    {
        printf("error in reading data!");
        return -1;
    }
    
    strcpy(recv_p->srcIP, strtok(msg_stream, ";"));
    recv_p->srcportno   =   atoi(strtok(NULL, ";"));
    strcpy(recv_p->msg, strtok(NULL, ";"));

    return 0;
}    


/* Convert Character Stream to Message Send Struct */
void convertstreamtosendpacket(msend *msgdata, char* msg_stream)
{
    strcpy(msgdata->destIP, strtok(msg_stream, ";"));
    msgdata->destportno     =       atoi(strtok(NULL, ";"));
    msgdata->rediscflag     =       atoi(strtok(NULL, ";"));   
    strcpy(msgdata->msg, strtok(NULL, ";"));
}


