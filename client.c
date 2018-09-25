/*

typedef struct hostent {
  char  *h_name;
  char  **h_aliases;
  short h_addrtype;
  short h_length;
  char  **h_addr_list;
} HOSTENT, *PHOSTENT, *LPHOSTENT;

Members

h_name

The official name of the host (PC). If using the DNS or similar resolution system, it is the Fully Qualified Domain Name (FQDN) that caused the server to return a reply. If using a local hosts file, it is the first entry after the IPv4 address.

h_aliases

A NULL-terminated array of alternate names.

h_addrtype

The type of address being returned.

h_length

The length, in bytes, of each address.

h_addr_list

A NULL-terminated list of addresses for the host. Addresses are returned in network byte order. The macro h_addr is defined to be h_addr_list[0] for compatibility with older software.

Stages for Client

    Socket connection: Exactly same as that of server’s socket creation
    Connect:

    int connect(int sockfd, const struct sockaddr *addr,  
                                 socklen_t addrlen);

    The connect() system call connects the socket referred to by the file descriptor sockfd to the address specified by addr. Server’s address and port is specified in addr.

	The gethostbyname() function returns a structure of type hostent for
       the given host name.  Here name is either a hostname or an IPv4
       address in standard dot notation (as for inet_addr(3)).  If name is
       an IPv4 address, no lookup is performed and gethostbyname() simply
       copies name into the h_name field and its struct in_addr equivalent
       into the h_addr_list[0] field of the returned hostent structure.  If
       name doesn't end in a dot and the environment variable HOSTALIASES is
       set, the alias file pointed to by HOSTALIASES will first be searched
       for name (see hostname(7) for the file format).  The current domain
       and its parents are searched unless name ends in a dot.
	   
	   

    The bcopy() function shall copy n bytes from the area pointed to by s1 to the area pointed to by s2.

    The bytes are copied correctly even if the area pointed to by s1 overlaps the area pointed to by s2.
	
	the htons() function converts values between host and network byte orders. There is a difference between big-endian and little-endian and network byte order depending on your machine and network protocol in use.

	
*/
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>	//For gethostbyname function

void error(char* msg){
		perror(msg);
		exit(1);
}

int main(int argc, char *argv[]){
	if(argc<3){
			fprintf(stderr,"No port provided\n");
			exit(1);
	}	
	int sockfd, portno, n;	//File descriptor for Sockets
	char buffer[255];	//Temporary buffer to read and write data
	
	struct sockaddr_in serv_addr;	//Structure to hold Server and Client details
	struct hostent* server;
	
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);	//AF_INET refers to addresses from the internet, IP addresses specifically. SOCK_STREAM to refer TCP(SOCK_DGRAM for UDP), 0 again for TCP
	if(sockfd<0){
			error("Socket failed \n");
	}
	
	server = gethostbyname(argv[1]);	//Resolving the host server and storing its details
	if(server == NULL){
		error("No such host\n");
	}
	
	bzero((char*) &serv_addr, sizeof(serv_addr));	//Empty contents of serv_addr
	serv_addr.sin_family = AF_INET;	//Assign IPv4 family to server_address
	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);	//Copy contents of host server address resolved into serv_addr
	serv_addr.sin_port = htons(portno);
	
	if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr))<0)	//Connecting to Server
		error("Connection failed\n");
	
	while(1){
		bzero(buffer, 256);
		printf("Me: ");
		fgets(buffer, 255, stdin);
		n = write(sockfd, buffer, strlen(buffer));	//Write to server
		if(n<0)
			error("Error writing to server\n");
		n = read(sockfd, buffer, 255);	//Read incoming data streams
		if(n<0)
			error("Error reading from Server");
		printf("Server : %s \n", buffer);
		if(strncmp(buffer, "Bye", 3) == 0)
			break;
	}
	
	return 0;
}
	