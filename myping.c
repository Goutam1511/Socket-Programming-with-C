#include <stdio.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h> 
#include <netinet/ip_icmp.h> 
#include <time.h> 
#include <fcntl.h> 
#include <signal.h> 
#include <time.h> 
#include <string.h>
#include <netinet/if_ether.h> 
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>


#define PING_PKT_S 64 
#define PORT_NO 0 
#define PING_SLEEP_RATE 10000 
#define RECV_TIMEOUT 1 

int pingloop=1; 

/*The ICMP Packet Structure*/
struct ping_pkt {
    struct ethhdr eth; //Ethernet header
    struct ip ip;	//IP header 
    struct icmphdr hdr; //ICMP header
    char msg[PING_PKT_S-sizeof(struct icmphdr)]; //Junk payload 
}g_pckt; 

/*Dummy structure to hold the essential parameters of the packet*/
typedef struct ping_ctx{
    int tos;
    int ttl;
    char srcIP[200];
    char dstIP[200];
    int r_sock;
}ping_ctx;

/*The checksum function used both for IPv4 and ICMP headers*/
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b; 
    unsigned int sum=0; 
    unsigned short result; 

    for ( sum = 0; len > 1; len -= 2 ) 
        sum += *buf++; 
    if ( len == 1 ) 
        sum += *(unsigned char*)buf; 
    sum = (sum >> 16) + (sum & 0xFFFF); 
    sum += (sum >> 16); 
    result = ~sum; 
    return result; 
}

/*Interrupt handler*/
void intHandler(int dummy) { 
	pingloop=0; 
} 

/*Function to fill IP header, takes the exact address of the IP header in the packet*/
void fill_ip_h(struct ip *ip, ping_ctx* ctx){
        ip->ip_src.s_addr = inet_addr(ctx->srcIP);
        ip->ip_dst.s_addr = inet_addr(ctx->dstIP);
        ip->ip_v = 4;
        ip->ip_hl = sizeof*ip >> 2; //Header length - 20 
        ip->ip_tos = (unsigned char)(ctx->tos);
        ip->ip_len = htons(sizeof(g_pckt)-sizeof(struct ethhdr)); //Total length
        ip->ip_id = htons(4321);
        ip->ip_off = htons(0);
        ip->ip_ttl = (unsigned char)(ctx->ttl);
        ip->ip_p = 1;
        ip->ip_sum = checksum(ip,(sizeof(g_pckt)-sizeof(struct ethhdr))); /* We have to provide this for AF_PACKET */
}

/*Function to fill ICMP header, takes the exact address of the ICMP header in the packet*/
void fill_icmp_h(struct icmphdr* hdr,int *msg_count){
        hdr->type = ICMP_ECHO;
        hdr->un.echo.id = 1; 
        hdr->un.echo.sequence = (*msg_count)++; 
}

/*Function to fill payload with Junk data*/
void fill_data(unsigned char * data){
    memset(data, 'J', PING_PKT_S-sizeof(struct icmphdr));
}

/*
Function to fill the Ethernet Header
As of now the Source and Destination MAC address is hardcoded.
These are to be replaced later.
*/
void fill_eth(struct ethhdr* eth){
    eth->h_source[0] = 0x78;
    eth->h_source[1] = 0xe7;
    eth->h_source[2] = 0xd1;
    eth->h_source[3] = 0xe2;
    eth->h_source[4] = 0xc3;
    eth->h_source[5] = 0x48;
    eth->h_dest[0] = 0xc0;
    eth->h_dest[1] = 0x42;
    eth->h_dest[2] = 0xd0;
    eth->h_dest[3] = 0x6e;
    eth->h_dest[4] = 0xe1;
    eth->h_dest[5] = 0xd9;
    eth->h_proto = htons(ETH_P_IP); //Denotes that upper layer must be IP
}

/*Function to Receive reply (RX)*/
int recv_ping(int ping_sockfd){ 
    int addr_len=0; //The length of receiving sockaddr structure
	int msg_received_count=0; 
    struct sockaddr_ll r_addr; //Link Layer Sockaddr_ll structure to hold the receiving packet info
	char* buffer = (char*) malloc (65536); //Buffer to hold the reply
	struct ethhdr *eth = NULL;
	unsigned short iphdrlen;
	struct iphdr *ip = NULL;

    addr_len=sizeof(r_addr);
	memset(buffer,0,65536);

    if ( recvfrom(ping_sockfd, buffer, 65536, 0, 
                    (struct sockaddr*)&r_addr, &addr_len) <= 0 ) { //API to receive reply
            printf("\nPacket receive failed!\n"); 
    } 

    else{ //If reply received parse the reply packet (Incomplete Parse) 
		eth = (struct ethhdr*) (buffer);
        ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
		if((unsigned int)ip->protocol==1&&(unsigned int)ip->version==4){//Check if packet received is ICMP 
			/*
			// Uncomment to parse the Packet and print the contents
			printf("\n Ethernet Header \n");
			printf("\t|Source Address : %.2X - %.2X - %.2X - %.2X - %.2X - %.2X",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
			printf("\t|Destination Address : %.2X - %.2X - %.2X - %.2X - %.2X - %.2X",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);

			printf("\n Internet header \n");
			printf("\t|-Version : %d\n",(unsigned int)ip->version);
			printf("\t|-Internet Header Length : %d DWORDS or %d Bytes\n",(unsigned int)ip->ihl,((unsigned int)(ip->ihl))*4);
			printf("\t|-Type Of Service : %d\n",(unsigned int)ip->tos);
			printf("\t|-Total Length : %d Bytes\n",ntohs(ip->tot_len));
			printf("\t|-Identification : %d\n",ntohs(ip->id));
			printf("\t|-Time To Live : %d\n",(unsigned int)ip->ttl);
			printf("\t|-Protocol : %d\n",(unsigned int)ip->protocol);
			printf("\t|-Header Checksum : %d\n",ntohs(ip->check));
			printf("\t|-Source IP : %d\n", ip->saddr);
			printf("\t|-Destination IP : %d\n",ip->daddr);
			*/
            return 1;
		}
        else{
            return 0;
        }
    }	 
} 

/*Function to Send request (TX)*/
int send_ping(ping_ctx* ctx){ 
    int msg_count=0;
	int addr_len=0;
	int pkt_sent=1; //Initialise the flag to check successful packet transfer
    int msg_received_count=0;
	int ret =0; //return code of sendto api
	int on =1; 
    struct sockaddr_ll remote_addr;	//Sockaddr_ll structure to bind to socket 
    struct ip *ip = NULL;
    struct icmphdr* icmph = NULL;
    unsigned  char* data = NULL;
    struct ethhdr* eth = NULL;
	int ifindex = 0; //variable to hold the interface number
    

    /*Set params*/ 
    char *to = ctx->dstIP;
    char *from = ctx->srcIP;
    int   ping_sockfd = ctx->r_sock;
    int ttl = ctx->ttl;
    int tos =ctx->tos;

	/*The Interface name is hardcoded here but if required should be made dynamic using IOCTL*/
    ifindex = if_nametoindex("enp2s0f0"); 
    //printf("The interface number is : %d \n",ifindex);
	
	/*The physical layer address for the binding sockaddr structure should be the next hop MAC address.
	The address is again hardcoded here*/
    remote_addr.sll_ifindex = ifindex; //interface index
    remote_addr.sll_halen = ETH_ALEN;  //Size of physical layer address
    remote_addr.sll_family = AF_PACKET; //Always AF_PACKET for Link Layer
    remote_addr.sll_addr[0] = 0xc0;
    remote_addr.sll_addr[1] = 0x42;
    remote_addr.sll_addr[2] = 0xd0;
    remote_addr.sll_addr[3] = 0x6e;
    remote_addr.sll_addr[4] = 0xe1;
    remote_addr.sll_addr[5] = 0xd9;
 
    pkt_sent=1; 
    memset(&g_pckt, 0, sizeof(g_pckt));

    /*ETHERNET Header*/
    eth = (struct ethhdr *)&g_pckt;
    fill_eth(eth);
        
    /*IP Header*/        
    ip = (struct ip *)(eth + 1); 
    fill_ip_h(ip, ctx);
      
    /*ICMP Header*/
    icmph = (struct icmphdr*)(ip + 1);
    fill_icmp_h(icmph, &msg_count); 

    /*Data*/
    data = (unsigned char *)(icmph + 1);
    fill_data(data);

    /*ICMP Checksum*/
    icmph->checksum = checksum(icmph, PING_PKT_S);

    /*TX*/ 
        //clock_gettime(CLOCK_MONOTONIC, &time_start); 
    ret = sendto(ping_sockfd, &g_pckt, sizeof(g_pckt), 0,(struct sockaddr*) &remote_addr, sizeof(remote_addr));
    if ( ret <= 0) { 
        printf("\nPacket Sending Failed!\n"); 
        pkt_sent=0; 
		return 0;
    } 
    else{ 
        //clock_gettime(CLOCK_MONOTONIC, &time_end); 
		return 1;
    }	 
 } 
 
void ping(ping_ctx* ctx,int r_sock){
	int msg_count=0;
	int msg_received_count=0;
	int pkt_sent=1;
	int pkt_recv=1;
	double packetloss = 0;
	struct timespec time_start, time_end, tfs, tfe;
	double timeElapsed = 0;
	long double rtt_msec=0, total_msec=0; 
	
	clock_gettime(CLOCK_MONOTONIC, &tfs);
	
	while(pingloop) {
		pkt_sent=1;
        usleep(PING_SLEEP_RATE);
		pkt_sent = send_ping(ctx);
		if(pkt_sent){
			msg_count++;
			pkt_recv = recv_ping(r_sock);
			if(pkt_recv){
				msg_received_count++;
			}
		}
	}
	
	clock_gettime(CLOCK_MONOTONIC, &tfe);
	
	timeElapsed = ((double)(tfe.tv_nsec - tfs.tv_nsec))/1000000.0; 
    total_msec = (tfe.tv_sec-tfs.tv_sec)*1000.0+ timeElapsed ;
	
	packetloss = ((double)(msg_count - msg_received_count)/(double)msg_count) * 100.0;
	printf("\n %d Packets transmitted, %d Packets received, %lf Packet loss, time %Lf \n", msg_count, msg_received_count, packetloss, total_msec );
}
   

int main(int argc, char *argv[]) { //Command line arguments should be Destination IP and Source IP
    ping_ctx ctx = {0}; 
	int recv_sock = 0;

	if(argc!=3) 
	{ 
		printf("sudo ./myping 10.117.159.251 10.39.51.117\n"); 
		return 0; 
	} 

	signal(SIGINT, intHandler); //Interrupt handler registration
	ctx.r_sock = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW); //Link layer Raw Socket IPPROTO_RAW is send-only
	recv_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP)); //Link Layer Raw Socket to sniff all types of packet.
	if(recv_sock <0) 
	{ 
		printf("\nSocket file descriptor not received\n"); 
		return 0; 
	} 
	if(ctx.r_sock <0) 
	{ 
        printf("%d",ctx.r_sock);
		printf("\nSocket file descriptor not received\n"); 
		return 0; 
	} 

    ctx.tos = 0;
    ctx.ttl = 64;
    strncpy(ctx.dstIP, argv[1],strlen(argv[1]));
    strncpy(ctx.srcIP, argv[2],strlen(argv[2]));

	ping(&ctx,recv_sock); 
	
	return 0; 
} 
