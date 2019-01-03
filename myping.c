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
#define PING_SLEEP_RATE 1000000 
#define RECV_TIMEOUT 1 

int pingloop=1; 

struct ping_pkt 
{
    struct ethhdr eth; 
    struct ip ip;	
    struct icmphdr hdr; 
    char msg[PING_PKT_S-sizeof(struct icmphdr)]; 
}g_pckt; 


typedef struct ping_ctx{
    int tos;
    int ttl;
    char srcIP[200];
    char dstIP[200];
    int r_sock;
}ping_ctx;

unsigned short checksum(void *b, int len) 
{
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

void intHandler(int dummy) 
{ 
	pingloop=0; 
} 

void fill_ip_h(struct ip *ip, ping_ctx* ctx)
{
        ip->ip_src.s_addr = inet_addr(ctx->srcIP);
        ip->ip_dst.s_addr = inet_addr(ctx->dstIP);
        ip->ip_v = 4;
        ip->ip_hl = sizeof*ip >> 2;
        ip->ip_tos = (unsigned char)(ctx->tos);
        ip->ip_len = htons(sizeof(g_pckt)-sizeof(struct ethhdr));
        ip->ip_id = htons(4321);
        ip->ip_off = htons(0);
        ip->ip_ttl = (unsigned char)(ctx->ttl);
        ip->ip_p = 1;
        ip->ip_sum = 0xba81; /* Let kernel fills in */
}

void fill_icmp_h(struct icmphdr* hdr,int *msg_count)
{
        hdr->type = ICMP_ECHO;
        hdr->un.echo.id = 1; 
        hdr->un.echo.sequence = (*msg_count)++; 
}

void fill_data(unsigned char * data)
{
    memset(data, 'J', PING_PKT_S-sizeof(struct icmphdr));
}

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
    eth->h_proto = htons(ETH_P_IP);
}

int recv_ping(int ping_sockfd)
{ 
    int addr_len,msg_received_count=0,on =1; 
    struct sockaddr_ll r_addr; 
    /*struct timespec time_start, time_end, tfs, tfe; 
    long double rtt_msec=0, total_msec=0; 
    struct timeval tv_out;*/
	char* buffer = (char*) malloc (65536);

    /*Timer Settings*/
    //tv_out.tv_sec = RECV_TIMEOUT; 
    //tv_out.tv_usec = 0; 
    //clock_gettime(CLOCK_MONOTONIC, &tfs);

    addr_len=sizeof(r_addr);
	memset(buffer,0,65536);

    if ( recvfrom(ping_sockfd, buffer, 65536, 0, 
                    (struct sockaddr*)&r_addr, &addr_len) <= 0 ) { 
            printf("\nPacket receive failed!\n"); 
    } 

    else{ 
        /*clock_gettime(CLOCK_MONOTONIC, &time_end); 

        double timeElapsed = ((double)(time_end.tv_nsec - 
                        time_start.tv_nsec))/1000000.0; 
        rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000.0 + timeElapsed; 
			
		msg_received_count++;*/
            
		struct ethhdr *eth = (struct ethhdr*) (buffer);
		unsigned short iphdrlen;
        struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
		if((unsigned int)ip->protocol==1){
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
            return 1;
		}
        else{
            return 0;
        }
    }	 
} 

 

void send_ping(ping_ctx* ctx,int r_sock)
{ 
    int  msg_count=0, i, addr_len, pkt_sent=1, 
        msg_received_count=0,on =1; 
    struct sockaddr_ll remote_addr;	
    struct ip *ip = NULL;
    struct icmphdr* icmph = NULL;
    unsigned  char* data = NULL;
    struct ethhdr* eth = NULL;
    struct sockaddr_ll r_addr; 
    struct timespec time_start, time_end, tfs, tfe; 
    long double rtt_msec=0, total_msec=0; 
    struct timeval tv_out;

    /*Set params*/ 
    char *to = ctx->dstIP;
    char *from = ctx->srcIP;
    int   ping_sockfd = ctx->r_sock;
    int ttl = ctx->ttl;
    int tos =ctx->tos;

    /*Timer Settings*/
    tv_out.tv_sec = RECV_TIMEOUT; 
    tv_out.tv_usec = 0; 
    //clock_gettime(CLOCK_MONOTONIC, &tfs); 
    
    //GET INTERFACE INDEX FOR INTERFACE enp0s3
    /*struct ifreq ifr;
    size_t if_name_len = strlen(if_name);
    if(if_name_len<sizeof(ifr.ifr_name)){
         memcpy(ifr.ifr_name,if_name,if_name_len);
         ifr.ifr_name[if_name_len]=0;
    }
    else{
         die("Interface name is too long");
    }
    memset(&ifr,0,sizeof(ifr));
    strncpy(ifr.ifr_name,enp0s3,IFNAMSIZ-1);
    int fd = socket(AF_UNIX,SOCK_DGRAM,0);
    if(fd==-1)
         printf("Error opening socket");
    if(ioctl(fd,SIOCGIFINDEX,&ifr)==-1){
         printf("Error getting index name");
    }*/
    //int ifindex = ifr.ifr_ifindex;
    int ifindex = if_nametoindex("enp2s0f0");
    printf("The interface number is : %d \n",ifindex);
    
    remote_addr.sll_ifindex = ifindex;
    remote_addr.sll_halen = ETH_ALEN;
    remote_addr.sll_family = AF_PACKET;
    remote_addr.sll_addr[0] = 0xc0;
    remote_addr.sll_addr[1] = 0x42;
    remote_addr.sll_addr[2] = 0xd0;
    remote_addr.sll_addr[3] = 0x6e;
    remote_addr.sll_addr[4] = 0xe1;
    remote_addr.sll_addr[5] = 0xd9;

    while(pingloop) 
    { 
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

        usleep(PING_SLEEP_RATE); 

        /*printf("----------------ETHERNET HEADER---------------\n");
        printf("\t| Source Address : %.2X- %.2X- %.2X- %.2X- %.2X- %.2X \n",g_pckt.eth.h_source[0],g_pckt.eth.h_source[1],g_pckt.eth.h_source[2],g_pckt.eth.h_source[3],g_pckt.eth.h_source[4],g_pckt.eth.h_source[5]);
        printf("\t| Destination Address : %.2X- %.2X- %.2X- %.2X- %.2X- %.2X\n",g_pckt.eth.h_dest[0],g_pckt.eth.h_dest[1],g_pckt.eth.h_dest[2],g_pckt.eth.h_dest[3],g_pckt.eth.h_dest[4],g_pckt.eth.h_dest[5]);
        printf("\t| Protocol: %d\n",g_pckt.eth.h_proto);
        printf("\t| Size of Ethernet Header : %lu\n",sizeof(struct ethhdr));
        //struct iphdr* dummyip = (struct iphdr*)&g_pckt.ip;
        //printf("----------------INTERNET HEADER----------------\n");
        //printf(g_pckt.ip);
        /*printf("\t|-Version : %u\n",g_pckt.ip.ip_v);
        printf("\t|-Internet Header Length : %d DWORDS or %d Bytes\n",g_pckt.ip.ip_hl,g_pckt.ip.ip_hl*4);
        printf("\t|-Type Of Service : %d\n",g_pckt.ip.ip_tos);
        printf("\t|-Total Length : %d Bytes\n",ntohs(g_pckt.ip.ip_len));
        printf("\t|-Identification : %d\n",ntohs(g_pckt.ip.ip_id));
        printf("\t|-Time To Live : %d\n",(unsigned int)g_pckt.ip.ip_ttl);
        printf("\t|-Protocol : %d\n",(unsigned int)g_pckt.ip.ip_p);
        printf("\t|-Header Checksum : %d\n",ntohs(g_pckt.ip.ip_sum));*/
        //printf("\t|-Source IP : %d\n", saddr);
        //printf("\t|-Destination IP : %d\n",dummyip->daddr);



        /*TX*/ 
        clock_gettime(CLOCK_MONOTONIC, &time_start); 
        int ret = sendto(ping_sockfd, &g_pckt, sizeof(g_pckt), 0,(struct sockaddr*) &remote_addr, sizeof(remote_addr));

        if ( ret <= 0) 
        { 
            printf("\nPacket Sending Failed!\n"); 
            pkt_sent=0; 
        } 
        /*else{
            printf("%d\n",ret);
            //printf("Received error in packet with ICMP type %d and error code %d \n" ,g_pckt.hdr.type,g_pckt.hdr.code);
        }*/

        /*RX*/ 
        /*addr_len=sizeof(r_addr); 

        if ( recvfrom(ping_sockfd, icmph, PING_PKT_S, 0, 
                    (struct sockaddr*)&r_addr, &addr_len) <= 0 ) 
        { 
            printf("\nPacket receive failed!\n"); 
        } */

        else
        { 
            clock_gettime(CLOCK_MONOTONIC, &time_end); 

            double timeElapsed = ((double)(time_end.tv_nsec - 
                        time_start.tv_nsec))/1000000.0; 
            rtt_msec = (time_end.tv_sec- 
                    time_start.tv_sec) * 1000.0 
                + timeElapsed;

            // if packet was not sent, don't receive 
            if(pkt_sent) 
            { 
				msg_count++;
                if(recv_ping(r_sock)){
                    msg_received_count++;
                }
				
				//printf("Total Messages received : %d\n", msg_received_count ); 
                /*if(!(g_pckt.hdr.type ==69 && g_pckt.hdr.code==0)) 
                { 
                    printf("Error..Packet received with ICMP" 
                            "type %d code %d\n", 
                            g_pckt.hdr.type, g_pckt.hdr.code); 
                } 
                else
                { 
                    printf("%d bytes Received reply from %s: icmp_seq=:%d ttl=%d time=%Lf ms\n",PING_PKT_S, ctx->dstIP, msg_count, ctx->ttl, rtt_msec);	
                    msg_received_count++;
                } */
            }
        }	 
    } 
   /*clock_gettime(CLOCK_MONOTONIC, &tfe); 
    double timeElapsed = ((double)(tfe.tv_nsec - 
                tfs.tv_nsec))/1000000.0; 

    total_msec = (tfe.tv_sec-tfs.tv_sec)*1000.0+ 
        timeElapsed ;*/

    printf("\n%d packets sent, %d packets received\n\n", 
            msg_count, msg_received_count); 
} 

int main(int argc, char *argv[]) 
{ 
    ping_ctx ctx = {0};

	if(argc!=3) 
	{ 
		printf("sudo ./myping 10.117.159.251 10.39.51.117\n"); 
		return 0; 
	} 

	signal(SIGINT, intHandler); 
	ctx.r_sock = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW); 
	int r_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP)); 
	if(r_sock <0) 
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

	send_ping(&ctx,r_sock); 
	
	return 0; 
} 
