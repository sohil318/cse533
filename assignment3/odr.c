#include        "unp.h"
#include	"odr.h"
#include	"hw_addrs.h"

char canonicalIP[STR_SIZE];
char hostname[STR_SIZE];
ifaceInfo *iface = NULL;
port_spath_map *portsunhead = NULL;

/* Pre defined functions given to read all interfaces and their IP and MAC addresses */
char* readInterfaces()
{
    struct hwa_info	*hwa, *hwahead;
    struct sockaddr	*sa;
    struct sockaddr_in  *tsockaddr = NULL;
    char   *ptr = NULL, *ipaddr, hptr[6];
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
        {
            ipaddr = Sock_ntop_host(sa, sizeof(*sa));
            printf("         IP addr = %s\n", ipaddr);
        }

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
            memcpy(hptr, hwa->if_haddr, 6);
            i = IF_HADDR;
            do {
                printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
            } while (--i > 0);
        }
        
        printf("\n         interface index = %d\n\n", hwa->if_index);
        if ((strcmp(hwa->if_name, "lo") != 0) && (strcmp(hwa->if_name, "eth0") != 0))
            addInterfaceList(hwa->if_index, hwa->if_name, ipaddr, hptr);
    }

    free_hwa_info(hwahead);
    return canonicalIP;
}


/* Create a linked list of all interfaces except lo and eth0 */
void addInterfaceList(int idx, char *name, char *ip_addr, char *haddr)
{
    ifaceInfo *temp = (ifaceInfo *)malloc(sizeof(ifaceInfo));
    
    temp->ifaceIdx = idx;
    strcpy(temp->ifaceName, name);
    strcpy(temp->ifaddr,ip_addr);
    memcpy(temp->haddr, haddr, 6);
    temp->next = iface;
    
    iface = temp;
}

/* Show sun_path vs port num table */
void print_interfaceInfo ()
{
    ifaceInfo *temp = iface;
    struct sockaddr *sa;
    int i;
    char *ptr = NULL;

    if (temp == NULL)
        return;
    printf("\n-------------------------------------------------------------------------------------------");
    printf("\n--- Interface Index --- | --- Interface Name --- | --- IP Address --- | --- MAC Address ---");
    printf("\n-------------------------------------------------------------------------------------------");
    while (temp != NULL)
    {
        printf("\n%15d         |  %12s          | %16s   | ", temp->ifaceIdx, temp->ifaceName, temp->ifaddr);
        ptr = temp->haddr;
        i = IF_HADDR;
        do {
            printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
        } while (--i > 0);

        temp = temp->next;
    }
    printf("\n-------------------------------------------------------------------------------------------");
    printf("\n");
    return;
}

/* Insert new node to sunpath vs port num table */
void add_sunpath_port_info( char *sunpath, int port)
{
    port_spath_map *newentry = (port_spath_map *)malloc(sizeof(port_spath_map));   
    
    newentry->port = port;
    strcpy(newentry->sun_path, sunpath);
    
    /* Get Current time stamp . Reference cplusplus.com */
    
    time_t rawtime;
    struct tm* timeinfo;
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    
    newentry->ts = timeinfo;
    newentry->next = NULL;

    /* Insert new entry to linked list */
    if (portsunhead == NULL)
        portsunhead = newentry;
    else
        {
            newentry->next = portsunhead;
            portsunhead = newentry;
        }
}

/* Show sun_path vs port num table */
void print_sunpath_port_map ()
{
    port_spath_map *temp = portsunhead;
    if (temp == NULL)
        return;
    printf("\n----------------------------------");
    printf("\n--- PORTNUM --- | --- SUN_PATH ---");
    printf("\n----------------------------------");
    while (temp != NULL)
    {
        printf("\n      %4d      |  %3s", temp->port, temp->sun_path);
        temp = temp->next;
    }
    printf("\n----------------------------------");
    printf("\n");
    return;
}

int main (int argc, char **argv)
{
    printf("\nCanonical IP : %s\n",readInterfaces());
    print_interfaceInfo ();
    gethostname(hostname, sizeof(hostname));
    printf("\nHostname : %s\n", hostname);
    add_sunpath_port_info(SERV_SUN_PATH, SERV_PORT);
    print_sunpath_port_map();
    return 0;
}
