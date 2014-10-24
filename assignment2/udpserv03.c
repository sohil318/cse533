#include	"unpifiplus.h"

int
main(int argc, char **argv)
{
	int					sockfd;
	const int			on = 1;
	pid_t				pid;
	struct ifi_info		*ifi, *ifihead;
	struct sockaddr_in	*sa, cliaddr, wildaddr;

	for (ifihead = ifi = Get_ifi_info_plus(AF_INET, 1);
		 ifi != NULL; ifi = ifi->ifi_next) {

			/*4bind unicast address */
		        sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

		        Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		        sa = (struct sockaddr_in *) ifi->ifi_addr;
		        sa->sin_family = AF_INET;
		        sa->sin_port = htons(SERV_PORT);
		        Bind(sockfd, (SA *) sa, sizeof(*sa));
		        printf("bound %s\n", Sock_ntop((SA *) sa, sizeof(*sa)));
	
	}
	return 0;
}

