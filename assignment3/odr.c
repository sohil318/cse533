#include        "unp.h"
#include	"odr.h"
#include        "utils.h"
#include	"hw_addrs.h"
#include        <sys/socket.h>
#include 	<sys/time.h>

char canonicalIP[IP_SIZE];
char hostname[STR_SIZE];
ifaceInfo *iface = NULL;
port_spath_map *portsunhead = NULL;
int max_port = 7000;
long staleness_parameter = 5;
rtabentry *routinghead = NULL;
int broadcast_id = 1000;
int special_delete = 0;
packetq *parkedqhead = NULL;

/* Add Packets to parked ODR Packet queue       */

void add_packing_queue(odrpacket * packet)
{
       packetq *node = (packetq *)malloc(sizeof(packetq));
        
       node->packet = packet;
       node->next = NULL;
       if (parkedqhead == NULL)
       {
//               printf("\nHead Created. New Node Parked.");
                parkedqhead = node;
       }
       else
       {
//               printf("\nNew Node Parked.");
               node->next = parkedqhead;
               parkedqhead = node;
       }
}

/* Lookup Packets parked in ODR Packet queue       */
odrpacket * lookup_packing_queue(char *srcip)
{
       packetq *node = parkedqhead;
       printf("\nStart Lookup in queue for ip : %s", srcip);
       while (node)
       {
//               printf("\nStart Lookup in queue for ip : %s", node->packet->dst_ip);
               if (strcmp(node->packet->dst_ip, srcip) == 0)
               {
//                       printf("\nFound node to be deleted.");
                       remove_node_link(srcip);
                       return node->packet;
               }
               node = node->next;
       }
       return NULL;
}

/* Remove parked from ODR Packet queue */
void remove_node_link(char *srcip)
{
       packetq *node = parkedqhead, *prev = NULL;

       while (node)
       {
               if (strcmp(node->packet->dst_ip, srcip) == 0)
               {
                       if (node == parkedqhead)
                               parkedqhead = node->next;
                       else
                               prev->next = node->next;
                       printf("\nRemoved Parked Node.");
                       return;
               }
               prev = node;
               node = node->next;
       }
}

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
                        memcpy(hptr, hwa->if_haddr, MAC_SIZE);
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

        struct timeval current_time;
        gettimeofday(&current_time, NULL);	 

        newentry->ts = current_time;
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
        printf("\n------------------------------------------------------");
        printf("\n--- PORTNUM --- | --- SUN_PATH ---| --- TIMESTAMP ---");
        printf("\n------------------------------------------------------");
        while (temp != NULL)
        {
                printf("\n      %4d      |  %3s     |   %ld ", temp->port, temp->sun_path, (long)temp->ts.tv_sec);
                temp = temp->next;
        }
        printf("\n----------------------------------");
        printf("\n");
        return;
}

/* Create a Unix Datagram Socket */
int createUXpacket(int family, int type, int protocol)
{
        int sockfd;
        if ((sockfd = socket(family, type, protocol)) < 0)
        {
                printf("\nError creating Unix DATAGRAM SOCKET\n");
                err_sys("socket error");
                perror("socket");
                return -1;
        }
        return sockfd;
}

/* Create a PF Packet Socket */
int createPFpacket(int family, int type, int protocol)
{
        int sockfd;
        if ((sockfd = socket(family, type, protocol)) < 0)
        {
                printf("\nError creating PF_PACKET SOCKET\n");
                err_sys("socket error");
                perror("socket");
                return -1;
        }
        return sockfd;
}


/* Module to handle client and server requests and responses via UNIX DATAGRAM SOCKET
 *
 * Handle ODR Packets through PF_PACKET
 */

void handleReqResp(int uxsockfd, int pfsockfd)
{
        int nready;
        fd_set rset, allset;
        int maxfd = max(uxsockfd, pfsockfd) + 1;

        FD_ZERO(&rset);
        FD_SET(uxsockfd, &rset);
        FD_SET(pfsockfd, &rset);
        allset = rset;
        for (;;)
        {
                rset = allset;
                if ( (nready = select(maxfd, &rset, NULL, NULL, NULL)) < 0 )
                {
                        if (errno == EINTR)
                                continue;
                        else
                                err_sys ("select error");
                }

                if ( FD_ISSET(uxsockfd, &rset))
                {
                        /* Check for client server sending messages to ODR layer */
//                        printf("\nHandling Client/Server Message at ODR.\n");
                        handleUnixSocketInfofromClientServer(uxsockfd, pfsockfd);
                }
                else if ( FD_ISSET(pfsockfd, &rset))
                {
                        /* Check for ODR sending messages to other VM's in ODR layer */
                        handlePFPacketSocketInfofromOtherODR(uxsockfd, pfsockfd);
                }
        }
}

/* Logic for handling Client/Server Message via Unix Domain Socket */

void handleUnixSocketInfofromClientServer(int uxsockfd, int pfsockfd)
{
        char msg_stream[DATA_SIZE];
        char src_mac[MAC_SIZE], dst_mac[MAC_SIZE], msg[DATA_SIZE], srcip[IP_SIZE], destip[IP_SIZE];
        int sport = 0, dport = 0, hop = 0, ifaceidx = 0, bid = 0, flag = 0, asent = 0;

        msend msgdata;
        odrpacket *packet, *dpacket;
        rtabentry *route;

        struct sockaddr_un saddr;
        int size = sizeof(saddr);
        port_spath_map *sunpathinfo;

//        bzero (packet, sizeof(odrpacket));
        bzero (&msgdata, sizeof(msend));
        
//        printf("Hello 1. \n");
        recvfrom(uxsockfd, msg_stream, DATA_SIZE, 0, (struct sockaddr *)&saddr, &size);
//        printf("Hello 2. \n");

        convertstreamtosendpacket(&msgdata, msg_stream);
//        printf("Hello 3. \n");

        if (strcmp(msgdata.destIP, canonicalIP) == 0)
        {
                printf("\nProcessing same node request.");
                client_server_same_vm(uxsockfd, pfsockfd, &msgdata, &saddr);
                return;
        }

        if (!strcmp(saddr.sun_path, SERV_SUN_PATH))
        {
                gethostname(hostname, sizeof(hostname));
                printf("\nTime packet from server at %s", hostname);
                sport = SERV_PORT_NO;
        }
        else
        {
                gethostname(hostname, sizeof(hostname));
                printf("\nTime Request packet from client at %s", hostname);
                sunpathinfo = sunpath_lookup(saddr.sun_path);
                if (sunpathinfo == NULL)
                {
                        printf("\nAdding new client info\n");        
                        sport = max_port;
                        add_sunpath_port_info(saddr.sun_path, max_port);
                        max_port++;
//                        print_sunpath_port_map();
                }
                else
                {
                        printf("\nExisting client found : %s\n", saddr.sun_path);        
                        sport = sunpathinfo->port;
                }
        }
        strcpy (srcip, canonicalIP);
        strcpy (destip,   msgdata.destIP);
        dport = msgdata.destportno;
        hop = 0;
        
        route = routing_table_lookup(destip, msgdata.rediscflag, broadcast_id);
        if (route == NULL)
        {
                /*      No Entry in the Routing table for matching destination or entry is stale. Create and send RREQ Packet */
                /*      Park packet in waiting queue    */
                strncpy(msg, msgdata.msg, DATA_SIZE);
                dpacket = createDataMessage (srcip, destip, sport, dport, hop, msg);
                add_packing_queue(dpacket);
                
//                printf("\nNo Routing Table Entry. Broadcasting RREQ's to all interfaces.\n");
                bid = broadcast_id++;   
                flag = msgdata.rediscflag;
                asent = 0;      /* TODO : Check for already sent flag */
                packet = createRREQMessage (srcip, destip, sport, dport, bid, hop, flag, asent);
                printf("\nBroadcast to all interfaces : src ip %s, destip %s, bid %d, route_disc = %d\n", srcip, destip, bid, flag); 
                ifaceidx = -1;
                /* TODO : Flood RREQ Logic */
                RREQ_broadcast(pfsockfd, packet, ifaceidx);
        }
        else
        {
                /*  Entry found in the Routing table for matching destination. Create and send DATA Packet. */
                printf("\nRouting Table Entry Found. Sending Data Packet.\n");
                strncpy(msg, msgdata.msg, DATA_SIZE);
                packet = createDataMessage (srcip, destip, sport, dport, hop, msg);
        //        ifaceidx = 3;
        //        char src_mac[6]  = {0x00, 0x0c, 0x29, 0xd9, 0x08, 0xF6};
	//        char dst_mac[6]  = {0x00, 0x0c, 0x29, 0x49, 0x3F, 0x65};
                ifaceidx = route->ifaceIdx;
                memcpy(src_mac, get_interface_mac(route->ifaceIdx), MAC_SIZE);
                memcpy(dst_mac, route->next_hop_MAC, MAC_SIZE);
                sendODR(pfsockfd, packet, src_mac, dst_mac, ifaceidx);
        }
}

char * get_interface_mac(int ifaceIdx)
{
        ifaceInfo *temp = iface;

        if (temp == NULL)
                return NULL;
        
        while (temp != NULL)
        {
                if (temp->ifaceIdx == ifaceIdx)
                {
                        return temp->haddr;
                }
                temp = temp->next;
        }
        return NULL;
}

/* Deleting an entry from linkedlist*/
void delete_entry(int port)
{
        port_spath_map *temp = portsunhead;
        if(portsunhead->port == port)
        { 
                portsunhead = portsunhead->next;
                return;
        }
        else 
        {
                while(temp)
                {
                        if(temp->next->port == port)
                        {
                                temp->next = temp->next->next;
                                return;
                        }
                        temp = temp->next;
                }
        }
}

/* returns 1 if the entry is stale else 0*/
int isStale(struct timeval ts)
{
        long staleness;
        struct timeval current_ts;
        gettimeofday(&current_ts, NULL);
        staleness = (current_ts.tv_sec - ts.tv_sec);
        //staleness += (current_ts.tv_usec - ts.tv_usec)/1000;

        if(staleness >= staleness_parameter)
                return 1;
        else
                return 0;
}


/* Lookup Sunpath Info from sunpath_portnum linked list */

port_spath_map * sunpath_lookup(char *sun_path)
{
        port_spath_map *temp = portsunhead;
        while (temp)
        {
                if (strcmp(temp->sun_path,sun_path) == 0)
                {
                                return temp;
                }	
                temp = temp->next;
        }
        return temp;
}


/* Lookup Sunpath Info from sunpath_portnum linked list */

port_spath_map * port_lookup(int port)
{
        port_spath_map *temp = portsunhead;

        while (temp)
        {
                if(temp->port == port)
                {
                                return temp;
                }
                temp = temp->next;

        }	
        return temp;
}

/* Broadcast RREQ to the world */
void RREQ_broadcast(int sockfd, odrpacket *pack, int ifaceIdx)
{
	char dst_mac[6]  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	ifaceInfo *temp = iface;
	while (temp)    
        {
		if(temp->ifaceIdx != ifaceIdx)
		{
                        sendODR(sockfd, pack, temp->haddr, dst_mac, temp->ifaceIdx);
		}
                temp = temp->next;
	}
}

/* Handle Client and Server Communication on same node */

void client_server_same_vm(int uxsockfd, int pfsockfd, msend *msgdata, struct sockaddr_un *saddr)
{
        mrecv recvp;
        struct sockaddr_un clientaddr;
        port_spath_map *sunpathinfo;
        char msg_stream[MSG_STREAM_SIZE];

        bzero(&clientaddr, sizeof(struct sockaddr_un));

        if (strcmp(saddr->sun_path, SERV_SUN_PATH) == 0)
        {
                gethostname(hostname, sizeof(hostname));
                printf("\nTime packet from server at %s", hostname);
                recvp.srcportno = SERV_PORT_NO;
                sunpathinfo = port_lookup(msgdata->destportno);
                if (sunpathinfo == NULL)
                {
                        printf("\nStaleness limit reached. Packets Dropped.\n");
                        return;
                }
                strcpy(clientaddr.sun_path, sunpathinfo->sun_path);
                clientaddr.sun_family = AF_LOCAL;
        }
        else
        {
                gethostname(hostname, sizeof(hostname));
                printf("\nTime Request packet from client at %s", hostname);
                sunpathinfo = sunpath_lookup(saddr->sun_path);
                if (sunpathinfo == NULL)
                {
//                        printf("\nAdding new client info sunpath = %s\n", saddr->sun_path);        
                        recvp.srcportno = max_port;
                        add_sunpath_port_info(saddr->sun_path, recvp.srcportno);
                        max_port++;
//                        print_sunpath_port_map();
                }
                else
                {
//                        printf("\nExisting client info sunpath = %s\n", saddr->sun_path);        
                        recvp.srcportno = sunpathinfo->port;
                }
                strcpy(clientaddr.sun_path, SERV_SUN_PATH);
                clientaddr.sun_family = AF_LOCAL;
        }

        strcpy(recvp.srcIP, msgdata->destIP);
        strcpy(recvp.msg, msgdata->msg);

        sprintf(msg_stream, "%s;%d;%s", recvp.srcIP, recvp.srcportno, recvp.msg);

        printf("\nSending Stream : %s   to sun_path = %s", msg_stream, clientaddr.sun_path);
        sendto(uxsockfd, msg_stream, sizeof(msg_stream), 0, (struct sockaddr *)&clientaddr, (socklen_t)sizeof(clientaddr));
        return;
}

/* Sending PF Packets across ODR layer */

void handlePFPacketSocketInfofromOtherODR(int uxsockfd, int pfsockfd)
{
        //printf("\nTODO");
        struct sockaddr_ll saddr;
        char src_mac[MAC_SIZE], dst_mac[MAC_SIZE], ether_frame [ETHR_FRAME_LEN], *ptr;
        int len, ifaceidx, i;
        len = sizeof(saddr);
        odrpacket *packet;
        rtabentry *route;

        recvfrom(pfsockfd, ether_frame, ETHR_FRAME_LEN, 0, (SA *)&saddr, &len);
        
        packet = getODRPacketfromEthernetPacket(ether_frame);
        memcpy(dst_mac, ether_frame, MAC_SIZE);
        ptr = ether_frame;
        printf ("\ndst Mac : ");
        i = IF_HADDR;
        do {
                printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
        } while (--i > 0);
        memcpy(src_mac, ether_frame + MAC_SIZE, MAC_SIZE);
        
        ptr = src_mac;
        printf ("\nsrc Mac : ");
        i = IF_HADDR;
        do {
                printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
        } while (--i > 0);
        
        ifaceidx = saddr.sll_ifindex;
        
        if (packet->packet_type == 1)
        {
                gethostname(hostname, sizeof(hostname));
                printf("\nIncoming RREQ on vm = %s", hostname);
                handleRREQPacket(pfsockfd, src_mac, dst_mac, packet, ifaceidx);
        }
        else if (packet->packet_type == 2)
        {
                gethostname(hostname, sizeof(hostname));
                printf("\nIncoming RREP on vm = %s", hostname);
                handleRREPPacket(pfsockfd, src_mac, dst_mac, packet, ifaceidx);
        }
        else if (packet->packet_type == DATA)
        {
                gethostname(hostname, sizeof(hostname));
                printf("\nIncoming DATA on vm = %s", hostname);
                handleDATAPacket(uxsockfd, pfsockfd, src_mac, dst_mac, packet, ifaceidx);
        }
}

/* Extract ODR Packet from Ethernet Frame . Convert it back from network to host long address.  */

odrpacket * getODRPacketfromEthernetPacket(char *ether_frame)
{
//        printf("\nGet Packet from Ethernet Frame");
        odrpacket * packet;
        
        unsigned char * odrpackethead = ether_frame + 14;
        packet = (odrpacket *)odrpackethead;
        

        /* Reverse back to Host Long Order */
        packet->packet_type       =       ntohl(packet->packet_type);
        packet->src_port          =       ntohl(packet->src_port);
        packet->dest_port         =       ntohl(packet->dest_port);
        packet->hopcount          =       ntohl(packet->hopcount);

        if (packet->packet_type == RREQ)
        {
        
        packet->broadcastid       =       ntohl(packet->broadcastid);
        packet->route_discovery   =       ntohl(packet->route_discovery);
        packet->rep_already_sent  =       ntohl(packet->rep_already_sent);
        
        }

        else if (packet->packet_type == RREP)
        {
        
        packet->route_discovery   =       ntohl(packet->route_discovery);
        
        }
        
        return packet;
}

void handleRREQPacket(int pfsockfd, char * src_mac, char * dst_mac, odrpacket * packet, int ifaceidx)
{
        printf("\nHandling RREQ sip %s, dip %s\n", packet->src_ip, packet->dst_ip);
        int hop, redisc_flag, asent;
        rtabentry * route;
	odrpacket * req_packet, *rep_packet;
//        int dup_rreq = checkDupPacket(packet->src_ip, packet->broadcastid); /* TODO : Check for duplicate RREQ. Stop Flooding */
        if (strcmp(packet->src_ip, canonicalIP) == 0)
        {
                printf("\nReturning from RREQ\n");
                return;
        }
        int entry = add_routing_entry(RREQ, packet->src_ip, src_mac, ifaceidx, packet->hopcount + 1, packet->broadcastid, 0);
//        printf("\nRouting Entry in RREQ inserted.\n");
         
        print_routingtable();
//        printf("\nPrinting Routing Entry in RREQ inserted.\n");
	
        if (strcmp(packet->dst_ip, canonicalIP) == 0)
	{	
                /* RREQ reached  dest ip. Now create and send the response RREP back. */
		//route = routing_table_lookup(packet->src_ip, 0, packet->broadcastid);
		hop             =       0;
                redisc_flag     =       packet->route_discovery;

                /* Create a response RREP packet and send as ODR Frame . TODO : Add condition for asent Flag and Duplicate RReq */
                if ((packet->rep_already_sent == 0) && (entry == 1))
                {
                         
                        rep_packet = createRREPMessage(packet->dst_ip, packet->src_ip, packet->dest_port, packet->src_port, hop, redisc_flag);
		        sendODR(pfsockfd, rep_packet, get_interface_mac(ifaceidx), src_mac , ifaceidx); //using get_interface_mac fresh entry
                        printf("\nReached Destination. Sending Response.");
                }
                else
                        printf("\nReached Destination. Reply not sent");
        }
        else
        {	
                /* RREQ forwarded when dest ip is not matched with Canonical IP of current VM */
                route = routing_table_lookup(packet->dst_ip, packet->route_discovery, packet->broadcastid);	/*assuming there is an entry in rtable TODO otherwise */
                if ((route != NULL) && (packet->route_discovery == 1))
                {
                        printf("\nStop Flooding of RREQ's with Route Discovery Flag.");
                        return;
                }
                /* No route found to destination or rediscovery flag = 1. Continue Flooding */
                if ((route == NULL) && (entry == 1))
                {
                        printf("\nNo entry in the Routing Table found for destination. ");
                        req_packet = createRREQMessage (packet->src_ip, packet->dst_ip, packet->src_port, packet->dest_port, packet->broadcastid, packet->hopcount + 1, packet->route_discovery, packet->rep_already_sent);
                        RREQ_broadcast(pfsockfd, req_packet, ifaceidx);                               
                }
                else if ((route != NULL) && (entry == 1))
                {       /* Route to destination exists in Routing Table, Check for new src_ip. If yes, set asent = 1 and continue flooding. */
                        printf("\nEntry found in the Routing Table. Broadcast with asent flag = 1. ");
                        
                        if (packet->rep_already_sent == 0)
                        {
                                rep_packet = createRREPMessage(packet->dst_ip, packet->src_ip, packet->dest_port, packet->src_port, route->hopcount + 1, packet->route_discovery);
                                sendODR(pfsockfd, rep_packet, get_interface_mac(route->ifaceIdx), route->next_hop_MAC , route->ifaceIdx); //using get_interface_mac fresh entry
                        }

                        asent = 1;      /* Stop new replies coming back */
                        req_packet = createRREQMessage (packet->src_ip, packet->dst_ip, packet->src_port, packet->dest_port, packet->broadcastid, packet->hopcount + 1, packet->route_discovery, asent);
                        RREQ_broadcast(pfsockfd, req_packet, ifaceidx);                               
                }
                else if ((route != NULL) && (entry == 0) && (packet->rep_already_sent == 0))
                {
                        printf("\nEntry found in the Routing Table. Broadcast with asent flag = 1. ");

                        rep_packet = createRREPMessage(packet->dst_ip, packet->src_ip, packet->dest_port, packet->src_port, route->hopcount + 1, packet->route_discovery);
                        sendODR(pfsockfd, rep_packet, get_interface_mac(route->ifaceIdx), route->next_hop_MAC , route->ifaceIdx); //using get_interface_mac fresh entry
                }
        }
}

/* Handle incoming responses through PF_PACKET socket. */

void handleRREPPacket(int pfsockfd, char * src_mac, char * dst_mac, odrpacket * packet, int ifaceidx)
{
//        printf("\nRREP");
        rtabentry * route;
	odrpacket * datapacket = NULL, *rep_packet, *req_packet;
        if (strcmp(packet->src_ip, canonicalIP) == 0)
                return;
        int entry = add_routing_entry(RREP, packet->src_ip, src_mac, ifaceidx, packet->hopcount + 1, 0, 0);
        print_routingtable();

	if (strcmp(packet->dst_ip, canonicalIP) == 0)
	{	
                /* RREP reached  dest ip. Now send the data request from server. TODO : Read from parked queue looking for src_ip. */
		printf("\nRREP Packet reached home to sender. Prepare DATA packet and resend.");
                //route = routing_table_lookup(packet->src_ip, 0);
                
		char msg[MSG_STREAM_SIZE] = "Time request";
                datapacket = lookup_packing_queue(packet->src_ip);
                 
		//datapacket = createDataMessage (packet->dst_ip, packet->src_ip, packet->dest_port, packet->src_port, 0, msg);	
                /*if (route == NULL)
                {
                        //dpacket = createDataMessage (srcip, destip, sport, dport, hop, msg);
                        printf("\nShould never come here. Route back to incoming source not available scenario. NOT POSSIBLE !!!!!"
                        add_packing_queue(datapacket);
                        req_packet = createRREQMessage (packet->dst_ip, packet->src_ip, packet->dest_port, packet->src_port, packet->broadcastid, 0, packet->route_discovery, packet->rep_already_sent);
                        RREQ_broadcast(pfsockfd, req_packet, ifaceidx);                               
                }
                else
                {*/
                if (datapacket)
                {
                        printf("\nSending DATA request packet on receiving RREP.\n");
		        //sendODR(pfsockfd, datapacket, get_interface_mac(route->ifaceIdx), route->next_hop_MAC , route->ifaceIdx); //using get_interface_mac fresh entry
		        sendODR(pfsockfd, datapacket, get_interface_mac(ifaceidx), src_mac , ifaceidx); //using get_interface_mac fresh entry
                 }
                //}
	}
	else
	{	
		printf("\nRREP Packet reached intermediate node. Prepare RREP packet and resend.");
                /* RREP forwarded when dest ip is not matched with Canonical IP of current VM */
		route = routing_table_lookup(packet->dst_ip, packet->route_discovery, 0);	/*assuming there is an entry in rtable TODO otherwise */
                /* Packet updates HOP Count and converts to htonl format */
                rep_packet = createRREPMessage(packet->src_ip, packet->dst_ip, packet->src_port, packet->dest_port, packet->hopcount + 1, packet->route_discovery);
                if (route == NULL)
                {
                        //dpacket = createDataMessage (srcip, destip, sport, dport, hop, msg);
                        printf("\nRREP reached. No Routing Table entry. Parking RREP and broadcasting RREQ.");
                        add_packing_queue(rep_packet);
                        req_packet = createRREQMessage (canonicalIP, packet->dst_ip, packet->dest_port, packet->src_port, broadcast_id, 0, packet->route_discovery, packet->rep_already_sent);
                        RREQ_broadcast(pfsockfd, req_packet, ifaceidx);                               
                }
                else
                {
                        printf("\nForwarding RREP packet using Routing Table Entry.");
		        sendODR(pfsockfd, rep_packet, get_interface_mac(route->ifaceIdx), route->next_hop_MAC, route->ifaceIdx);
	        }
        }
}

void handleDATAPacket(int uxsockfd, int pfsockfd, char * src_mac, char * dst_mac, odrpacket * packet, int ifaceidx)
{
//        printf("\nDATA");
        mrecv *rec = (mrecv *)malloc(sizeof(mrecv));
        port_spath_map *port_sun;
        struct sockaddr_un clientaddr, saddr;
        rtabentry *route;
        odrpacket * datapacket, *req_packet;
        msend msgdata;
        char msg_stream[MSG_STREAM_SIZE], msg_str[MSEND_SIZE];
        
        /* TODO : Handle DATA Packets with no Routing table entries. Park entries and perform route discovery by Broadcasting  */
        if (strcmp(packet->src_ip, canonicalIP) == 0)
                return;
        int entry = add_routing_entry(DATA, packet->src_ip, src_mac, ifaceidx, packet->hopcount + 1, 0, 0);
//        printf("\nRouting Entry in DATA inserted.\n");
         
        print_routingtable();
//        printf("\nPrinting Routing Entry in DATA inserted.\n");

        bzero(&clientaddr, sizeof(struct sockaddr_un));
        bzero(&saddr, sizeof(struct sockaddr_un));

        if(strcmp(packet->dst_ip, canonicalIP)==0)
        {
                printf("\nData Packet Reached Destination.");
                strcpy(rec->srcIP, packet->src_ip);
                rec->srcportno = packet->src_port;
                strcpy(rec->msg, packet->datamsg);
                
                clientaddr.sun_family = AF_LOCAL;
//                printf("\nLooking Up Port No : %d", packet->dest_port);
                port_sun = port_lookup(packet->dest_port);
                if (port_sun == NULL)
                {
//                        print_sunpath_port_map ();
//                        printf("\nNo Client - Server exists at port number : %d", packet->dest_port); 
                        return;
                }
                strcpy(clientaddr.sun_path, port_sun->sun_path);	

                printf("Writing stream on IP %s, port num : %d, msg = %s", rec->srcIP, rec->srcportno, rec->msg);
                sprintf(msg_stream, "%s;%d;%s", rec->srcIP, rec->srcportno, rec->msg);
                //print_sunpath_port_map ();

                sendto(uxsockfd, msg_stream, strlen(msg_stream), 0, (struct sockaddr *)&clientaddr, (socklen_t)sizeof(clientaddr));

                int size = sizeof(saddr);
                if(packet->dest_port == SERV_PORT_NO)
                {
                        printf("\nSending Time Request to Server. Waiting for response. ");
                        bzero (&msgdata, sizeof(msend));

                        recvfrom(uxsockfd, msg_str, MSEND_SIZE, 0, (struct sockaddr *)&saddr, &size);

                        convertstreamtosendpacket(&msgdata, msg_str);
                        printf("\nReceived Time from Server. Sending ODR Data Packet with time = %s at dest_ip = %s and port no = %d\n", msgdata.msg, msgdata.destIP, msgdata.destportno);
                        //route = routing_table_lookup(msgdata.destIP, 0);
                        datapacket = createDataMessage (canonicalIP, msgdata.destIP, SERV_PORT_NO, msgdata.destportno, 0, msgdata.msg);
                        sendODR(pfsockfd, datapacket,get_interface_mac(ifaceidx), src_mac, ifaceidx);
                        //sendODR(pfsockfd, datapacket,get_interface_mac(route->ifaceIdx), route->next_hop_MAC, route->ifaceIdx);
                }
                else
                {
                        printf("\nReceived final time. Communcation Ended !!! YAYYYYY !!!. \n");
                        return;
                }
        }
        else
        {
                printf("\nData Packet Being Forwarded to Destination.");
                route = routing_table_lookup(packet->dst_ip, 0, 0);
                datapacket = createDataMessage (packet->src_ip, packet->dst_ip, packet->src_port, packet->dest_port, packet->hopcount + 1, packet->datamsg);	
                if (route == NULL)
                {
                        //dpacket = createDataMessage (srcip, destip, sport, dport, hop, msg);
                        printf("\nDATA reached. No Routing Table entry. Parking RREP and broadcasting RREQ.");
                        add_packing_queue(datapacket);
                        req_packet = createRREQMessage (canonicalIP, packet->dst_ip, packet->src_port, packet->dest_port, broadcast_id, 0, packet->route_discovery, packet->rep_already_sent);
                        RREQ_broadcast(pfsockfd, req_packet, ifaceidx);                               
                }
                else
                {
                        printf("\nForwarding RREP packet using Routing Table Entry.");
                        sendODR(pfsockfd, datapacket, get_interface_mac(route->ifaceIdx), route->next_hop_MAC, route->ifaceIdx);
	        }
                
        }
}

/*  Complete ODR Frame  REF ==> http://aschauf.landshut.org/fh/linux/udp_vs_raw/ch01s03.html   */

void sendODR(int sockfd, odrpacket *packet, char *src_mac, char *dst_mac, int ifaceidx)
{
        int send_result = 0;

        struct sockaddr_ll socket_address;                              /*      target address                                  */
        void* buffer = (void*)malloc(ETHR_FRAME_LEN);                   /*      buffer for ethernet frame                       */
        unsigned char* etherhead = buffer;                              /*      pointer to ethenet header                       */
        unsigned char* data = buffer + 14;                              /*      userdata in ethernet frame                      */  

        struct ethhdr *eh = (struct ethhdr *)etherhead;                 /*      another pointer to ethernet header              */

        /*      Prepare sockaddr_ll     */
        socket_address.sll_family   =   PF_PACKET;                      /*      RAW communication                               */  
        socket_address.sll_protocol =   htons(MY_PROTOCOL);                /*      We don't use a protocoll above ethernet layer just use anything here.  */
        socket_address.sll_ifindex  =   ifaceidx;                       /*      Interface Index of the network device in function parameter     */

        
        socket_address.sll_hatype   =   ARPHRD_ETHER;                   /*      ARP hardware identifier is ethernet             */  
        
        socket_address.sll_pkttype  =   PACKET_OTHERHOST;               /*      Target is another host.                         */  

        socket_address.sll_halen    =   MAC_SIZE;                       /*      Address length                                  */

        /*      MAC - begin     */
        socket_address.sll_addr[0]  =   dst_mac[0];             
        socket_address.sll_addr[1]  =   dst_mac[1];             
        socket_address.sll_addr[2]  =   dst_mac[2];  
        socket_address.sll_addr[3]  =   dst_mac[3];  
        socket_address.sll_addr[4]  =   dst_mac[4];
        socket_address.sll_addr[5]  =   dst_mac[5];
        /*      MAC - end       */

        socket_address.sll_addr[6]  =   0x00;                           /*      not used                                        */
        socket_address.sll_addr[7]  =   0x00;                           /*      not used                                        */      

        memcpy((void*)buffer, (void*)dst_mac, MAC_SIZE);                /*      Set Dest Mac in the ethernet frame header       */
        memcpy((void*)(buffer + MAC_SIZE), (void*)src_mac, MAC_SIZE);   /*      Set Src Mac in the ethernet frame header        */  
        eh->h_proto = htons(MY_PROTOCOL);

        memcpy((void *)data, (void *)packet, sizeof(odrpacket));        /*      Fill the frame with ODR Packet                  */
        
        /*send the packet*/
        send_result = sendto(sockfd, buffer, ETHR_FRAME_LEN, 0, (struct sockaddr *) &socket_address, sizeof(socket_address));

        if (send_result == -1) 
        {
                printf("\nError in Sending ODR");
                perror("sendto");
        }

        struct hostent *he, *he1;
        struct in_addr ipv4addr, ipadr;
        he = (struct hostent *)malloc(sizeof(struct hostent));
        he1 = (struct hostent *)malloc(sizeof(struct hostent));
       
        int i;
        char *ptr;

        inet_pton(AF_INET, packet->src_ip, &ipv4addr);
        he = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
        
        
        //printf("Host name: %s\n", he->h_name);
        gethostname(hostname, sizeof(hostname));

        printf("\nODR at %s : ", hostname);
        ptr = dst_mac;
        printf ("sending frame dst mac addr: ");
        i = IF_HADDR;
        do {
                printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
        } while (--i > 0);
        
        ptr = src_mac;
        printf ("\t from src Mac : ");
        i = IF_HADDR;
        do {
                printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
        } while (--i > 0);

        printf("\n\t  src %s ip : %s ", he->h_name, packet->src_ip);
        inet_pton(AF_INET, packet->dst_ip, &ipadr);
        he1 = gethostbyaddr(&ipadr, sizeof(ipadr), AF_INET);
        printf(", dst %s ip %s, msg : %s , msg_type : %d\n",  he1->h_name, packet->dst_ip, packet->datamsg, ntohl(packet->packet_type));
        
}

/* Create RREQ Message          */

odrpacket * createRREQMessage (char *srcIP, char *destIP, int sport, int dport, int bid, int hop, int flag, int asent)
{
        odrpacket *packet = (odrpacket *)malloc(sizeof(odrpacket));

        packet->packet_type          =     htonl(RREQ);
        packet->src_port             =     htonl(sport);
        packet->dest_port            =     htonl(dport);
        packet->hopcount             =     htonl(hop);
        packet->broadcastid          =     htonl(bid);
        packet->route_discovery      =     htonl(flag);
        packet->rep_already_sent     =     htonl(asent);
        strcpy(packet->src_ip, srcIP);
        strcpy(packet->dst_ip, destIP);

        return packet;
}

/* Create RREP Message          */
odrpacket * createRREPMessage (char *srcIP, char *destIP, int sport, int dport, int hop, int flag)
{
        odrpacket *packet = (odrpacket *)malloc(sizeof(odrpacket));

        packet->packet_type     =     htonl(RREP);
        packet->src_port        =     htonl(sport);
        packet->dest_port       =     htonl(dport);
        packet->hopcount        =     htonl(hop);
        packet->route_discovery =     htonl(flag);
        strcpy(packet->src_ip, srcIP);
        strcpy(packet->dst_ip, destIP);

        return packet;
}

/* Create DATA Message          */
odrpacket * createDataMessage (char *srcIP, char *destIP, int sport, int dport, int hop, char *msg)
{
        odrpacket *packet = (odrpacket *)malloc(sizeof(odrpacket));

        packet->packet_type     =     htonl(DATA);
        packet->src_port        =     htonl(sport);
        packet->dest_port       =     htonl(dport);
        packet->hopcount        =     htonl(hop);
        strcpy(packet->src_ip, srcIP);
        strcpy(packet->dst_ip, destIP);
        strcpy(packet->datamsg, msg);

        return packet;
}

/* Deleting an entry from routing table linkedlist*/
void delete_routing_entry(char *destIP)
{
        rtabentry *temp = routinghead;
        if((strcmp(temp->destIP, destIP)==0))
        {
                routinghead = routinghead->next;
                return;
        }
        else
        {
                while(temp)
                {
                        if((strcmp(temp->next->destIP, destIP)==0))
                        {
                                temp->next = temp->next->next;
                                return;
                        }
                        temp = temp->next;
                }
        }
}


/* Check for duplicate pair <broadcastid, destip> . Helps in preventing flooding */
int isDuplicate(int broadcastid, char *destip)
{
        rtabentry *temp = routinghead;
        while (temp)
        {
                if ((temp->broadcastId == broadcastid) && (strcmp(destip, temp->destIP) == 0))
                        return 1;
                temp = temp->next;
        }
        return 0;
}


/* Insert new entry to routing table */

/* CHECK : Handle all types of addtion/updates to routing table. Return 1 for success and 0 otherwise. */
int add_routing_entry(int packet_type, char *destIP, char *next_hop_MAC, int ifaceIdx, int hopcount, int broadcastId, int rdisc)
{
        printf("\nHandling Routing Table Entry Addition. Packet Type : %d, destIP : %s, ifaceIdx : %d, hopcount : %d, broadcastid : %d, rdisc : %d\n", packet_type, destIP, ifaceIdx, hopcount, broadcastId, rdisc);
        int checkdup = 0;
        struct timeval current_time;
        rtabentry *newentry = (rtabentry *)malloc(sizeof(rtabentry));
        rtabentry *temp = routinghead, *entry;
        
        if (packet_type == RREQ)
                checkdup = isDuplicate(broadcastId, destIP);
        
        entry = routing_table_lookup(destIP, rdisc, broadcastId);
//        printf("\nLookup Routing Table Performed : check dup %d\n", checkdup);
        if ( entry == NULL)
        {
                /* Create New Entry in Routing Table */
                printf("\nNo Routing Table Found. Adding New Entry.\n");
                newentry->ifaceIdx = ifaceIdx;
                memcpy(newentry->destIP, destIP, IP_SIZE);
                memcpy(newentry->next_hop_MAC, next_hop_MAC, MAC_SIZE);
                newentry->hopcount = hopcount;
                newentry->broadcastId = broadcastId;

                /* Get Current time stamp */
                gettimeofday(&current_time, NULL);

                newentry->ts = current_time;
                newentry->next = NULL;

                /* Insert new entry to linked list */
                if (routinghead == NULL)
                        routinghead = newentry;
                else
                {
                        newentry->next = routinghead;
                        routinghead = newentry;
                }
//                printf("Added New Entry.\n");
                //packet = newentry;
                //if (special_delete == 0)
                        return 1;
        }
        else //if ((special_delete == 1) || (entry != NULL))
        {
               if ((packet_type == RREQ) && (rdisc == 0))
               {
                       if (entry->broadcastId < broadcastId)
                       {
                               entry->broadcastId = broadcastId;
                               memcpy(entry->next_hop_MAC, next_hop_MAC, MAC_SIZE);
                               entry->hopcount = hopcount;
                               entry->ifaceIdx = ifaceIdx;
                               gettimeofday(&current_time, NULL);
                               entry->ts = current_time;
                               printf("Updated existing Entry. New Broadcast ID. For RREQ. \n");
                               return 1;
                       }
                       if (checkdup)
                       {
                               if (hopcount <= entry->hopcount)
                               {
                                        entry->broadcastId = broadcastId;
                                        memcpy(entry->next_hop_MAC, next_hop_MAC, MAC_SIZE);
                                        entry->hopcount = hopcount;
                                        entry->ifaceIdx = ifaceIdx;
                                        gettimeofday(&current_time, NULL);
                                        entry->ts = current_time;
                                        printf("Updated existing Entry. Same Broadcast ID. hop <= rt->hopcount. For RREQ. \n");
                                        return 1;
                               }
                               else
                               {
                                       printf("No Update. Same Broadcast ID. hop > rt->hopcount. For RREQ. \n");
                                       return 0;
                               }
                       }
               }
               if ((rdisc == 1) && (packet_type == RREQ))
               {        
                       if (entry->broadcastId < broadcastId)
                       {
                               entry->broadcastId = broadcastId;
                               memcpy(entry->next_hop_MAC, next_hop_MAC, MAC_SIZE);
                               entry->hopcount = hopcount;
                               entry->ifaceIdx = ifaceIdx;
                               gettimeofday(&current_time, NULL);
                               entry->ts = current_time;
                               printf("Updated all existing Entries. rdisc = 1. For RREQ. \n");
                               return 1;
                       }
                       else
                               return 0;

               }
               else if ((entry->hopcount >= hopcount) && (packet_type != RREQ))
               {
                       memcpy(entry->next_hop_MAC, next_hop_MAC, MAC_SIZE);
                       entry->broadcastId = broadcastId;
                       entry->hopcount = hopcount;
                       entry->ifaceIdx = ifaceIdx;
                       gettimeofday(&current_time, NULL);
                       entry->ts = current_time;
                       printf("Updated existing Entry. rdisc != 1. For not RR/EQ. \n");
                       return 1;
               }
               
        }
        printf("Returning without any updates.\n");
        return 0;
}

/* Updating the timestamp of the routing table entry */
void update_routing_entry_ts(rtabentry *update)
{
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    update->ts = current_time;
}


/* Lookup in routing table */
rtabentry * routing_table_lookup(char *destIP, int disc_flag, int bid)
{
    rtabentry *temp = routinghead;

    while (temp)
    {
        if(strcmp(temp->destIP, destIP)==0)
        {
                if ((isStale(temp->ts)) || ((disc_flag == 1) && (bid > temp->broadcastId)) || ((disc_flag == 1) && (bid == 0)))
                {
                        special_delete = 1;
                        delete_routing_entry(temp->destIP);
                        return NULL;
                }
                else
                {
                        return temp;
                }
        }
        temp = temp->next;
     }
    return temp;
}

void print_routingtable()
{	
        char destIP[IP_SIZE];
        char next_hop_MAC[MAC_SIZE];
        int ifaceIdx, hopcount, broadcastId, i;
        char *ptr;
        time_t nowtime;
        struct tm *nowtm;
        char tmbuf[64], buf[64];
        struct timeval ts;
        printf("\n|---------------------------------------------------------------------------------------------------------------  |\n");
        printf("\n| %15s | %15s  | %10s | %10s | %10s | %30s |\n", "Dest IP", "Next Hop Mac", "Ifaceidx", "HopCount", "BrdcastID", "TimeStamp");
        printf("\n|---------------------------------------------------------------------------------------------------------------  |\n");
        rtabentry *temp = routinghead;
        while(temp)
        {
                printf("\n| %15s | ", temp->destIP);
                ptr = temp->next_hop_MAC;
                i = IF_HADDR;
                do {
                        printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
                } while (--i > 0);
                
                nowtime = temp->ts.tv_sec;
                nowtm   = localtime(&nowtime);

                strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);
                snprintf(buf, sizeof(buf), "%s.%06d", tmbuf,temp->ts.tv_usec);


                printf(" | %10d | %10d | %10d | %30s |\n", temp->ifaceIdx, temp->hopcount, temp->broadcastId, buf);
                temp = temp->next;
        }
}

int main (int argc, char **argv)
{
        int pfsockfd, uxsockfd, optval = -1, len;
        struct sockaddr_un servAddr, checkAddr;
        if(argc > 1)
        {
                staleness_parameter = atoi(argv[1]);
        }
        else
        {
                printf("\nPlease enter a staleness parameter! \n");
                exit(0);
        }
        printf("\nCanonical IP : %s\n",readInterfaces());
        print_interfaceInfo ();
         gethostname(hostname, sizeof(hostname));
        printf("\nHostname : %s\n", hostname);

        /* Create Unix Datagram Socket to bind to well-known server sunpath. */
        if ((uxsockfd = createUXpacket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
                return 1;

        //setsockopt(uxsockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        unlink(UNIX_DGRAM_PATH);
        bzero(&servAddr, sizeof(struct sockaddr_un));
        servAddr.sun_family = AF_LOCAL;
        strcpy(servAddr.sun_path, UNIX_DGRAM_PATH);
        Bind(uxsockfd, (SA *)&servAddr, SUN_LEN(&servAddr));

        len = sizeof(servAddr);
        Getsockname(uxsockfd, (SA *) &checkAddr, &len);

        printf("\nUnix Datagram socket for server created and bound name = %s, len = %d.\n", checkAddr.sun_path, len);

        add_sunpath_port_info(SERV_SUN_PATH, SERV_PORT_NO);
        print_sunpath_port_map();

        if ((pfsockfd = createPFpacket(PF_PACKET, SOCK_RAW, htons(MY_PROTOCOL))) < 0)
                return 1;

        handleReqResp(uxsockfd, pfsockfd);    

        return 0;
}
