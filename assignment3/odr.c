#include        "unp.h"
#include	"odr.h"
#include	"hw_addrs.h"

char canonicalIP[50];
ifaceInfo *iface = NULL;

void addInterfaceList(int idx, char *name, struct sockaddr_in *ip_addr, char *haddr)
{
    ifaceInfo *temp = (ifaceInfo *)malloc(sizeof(ifaceInfo));

    temp->ifaceIdx = idx;
    strcpy(temp->ifaceName, name);
    temp->ifaddr = ip_addr;
    strcpy(temp->haddr, haddr);
    temp->next = iface;
    
    iface = temp;
}

char* readInterfaces()
{
    struct hwa_info	*hwa, *hwahead;
    struct sockaddr	*sa;
    struct sockaddr_in  *tsockaddr = NULL;
    char   *ptr = NULL;
    int    i, prflag;

    printf("\n");

    for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {

        printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
        
        if (strcmp(hwa->if_name, "eth0") == 0)  
        {
            tsockaddr = (struct sockaddr_in *)hwa->ip_addr;
            inet_ntop(AF_INET, &(tsockaddr->sin_addr), canonicalIP, 50);
        }

        if ((sa = hwa->ip_addr) != NULL)
            printf("         IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));

        prflag = 0;
        i = 0;
        do {
            if (hwa->if_haddr[i] != '\0') {
                prflag = 1;
                break;
            }
        } while (++i < IF_HADDR);

        if (prflag) {
            printf("         HW addr = ");
            ptr = hwa->if_haddr;
            i = IF_HADDR;
            do {
                printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
            } while (--i > 0);
        }

        printf("\n         interface index = %d\n\n", hwa->if_index);
        if ((strcmp(hwa->if_name, "lo") != 0) && (strcmp(hwa->if_name, "lo") != 0))
            addInterfaceList(hwa->if_index, hwa->if_name, (struct sockaddr_in *)hwa->ip_addr, ptr);
    }

    free_hwa_info(hwahead);
    return canonicalIP;
}


int main (int argc, char **argv)
{
    printf("\nCanonical IP : %s\n",readInterfaces());
    return 0;
}
