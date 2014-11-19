#include        "utils.h"
#include 	"odr.h"
#include        "unp.h"

int msg_send(int sockfd, char *ip_dest, int dest_port, char *msg, int flag){
        msend send_p;
        struct sockaddr_un addr;

        bzero(&send_p, sizeof(msend));
        bzero(&addr, sizeof(struct sockaddr_un));
        addr.sun_family = AF_LOCAL;
        strcpy(addr.sun_path, SERV_SUN_PATH);
        send_p.sockfd = sockfd;
        strcpy(send_p.destIP, ip_dest);
        send_p.destportno = dest_port;
        send_p.rediscflag = flag;
        strcpy(send_p.msg, msg);
        return sendto(sockfd, (void *) &send_p, sizeof(send_p),0, (SA *) &addr, sizeof(addr));
}

int msg_recv(int sockfd, char* msg, char* ip_source, int source_port){
        mrecv recv_p;

        if (recvfrom(sockfd, (char *) &recv_p, sizeof(mrecv), 0, NULL, NULL) != sizeof(mrecv))
        {
                printf("error in reading data!");
                return -1;
        }
        strcpy(msg, recv_p.msg);
        strcpy(ip_source, recv_p.srcIP);
        source_port = recv_p.srcportno;
        return 0;
}    
