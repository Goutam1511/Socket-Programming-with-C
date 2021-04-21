/*
NOTES :

Stages for server

    Socket creation:

    int sockfd = socket(domain, type, protocol)

    sockfd: socket descriptor, an integer (like a file-handle)
    domain: integer, communication domain e.g., AF_INET (IPv4 protocol) , AF_INET6 (IPv6 protocol)
    type: communication type
    SOCK_STREAM: TCP(reliable, connection oriented)
    SOCK_DGRAM: UDP(unreliable, connectionless)
    protocol: Protocol value for Internet Protocol(IP), which is 0. This is the same number which appears on protocol field in the IP header of a packet.(man protocols for more details)
    Setsockopt:

    int setsockopt(int sockfd, int level, int optname,  
                       const void *optval, socklen_t optlen);

    This helps in manipulating options for the socket referred by the file descriptor sockfd. This is completely optional, but it helps in reuse of address and port. Prevents error such as: “address already in use”.
    Bind:

    int bind(int sockfd, const struct sockaddr *addr, 
                              socklen_t addrlen);

    After creation of the socket, bind function binds the socket to the address and port number specified in addr(custom data structure). In the example code, we bind the server to the localhost, hence we use INADDR_ANY to specify the IP address.
    Listen:

    int listen(int sockfd, int backlog);

    It puts the server socket in a passive mode, where it waits for the client to approach the server to make a connection. The backlog, defines the maximum length to which the queue of pending connections for sockfd may grow. If a connection request arrives when the queue is full, the client may receive an error with an indication of ECONNREFUSED.
    Accept:

    int new_socket= accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

    It extracts the first connection request on the queue of pending connections for the listening socket, sockfd, creates a new connected socket, and returns a new file descriptor referring to that socket. At this point, connection is established between client and server, and they are ready to transfer data.


struct sockaddr {
        ushort  sa_family;
        char    sa_data[14];
};

struct sockaddr_in{  
    short sin_family;  
    unsigned short sin_port;  
struct in_addr sin_addr;  
    char sin_zero[8];  
};  

Parameters :

sin_family
Address family (must be AF_INET).

sin_port
IP port.

sin_addr
IP address.

sin_zero
Padding to make structure the same size as SOCKADDR.

This is the form of the SOCKADDR structure specific to the Internet address family and can be cast to SOCKADDR.

The IP address component of this structure is of type IN_ADDR. The IN_ADDR structure is defined in Windows Sockets header file WINSOCK.H


*/
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#define ALLOC_SLAB 1024

void error(char* msg){
		perror(msg);
		exit(1);
}

int main(int argc, char *argv[]){
	if(argc<2){
			fprintf(stderr,"No port provided\n");
			exit(1);
	}	
	int sockfd, newsockfd, portno, n;	//File descriptor for Sockets
	
	struct sockaddr_in serv_addr,cli_addr;	//Structure to hold Server and Client details
	socklen_t cli_len;	//Client length 
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);	//AF_INET refers to addresses from the internet, IP addresses specifically. SOCK_STREAM to refer TCP(SOCK_DGRAM for UDP), 0 again for TCP
	if(sockfd<0){
			error("Socket failed \n");
	}
	bzero((char*) &serv_addr,sizeof(serv_addr));	//The bzero() function shall place n zero-valued bytes in the area pointed to by s.
	portno = atoi(argv[1]);
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;	//INADDR_ANY is used when you don't need to bind a socket to a specific IP. When you use this value as the address when calling bind() , the socket accepts connections to all the IPs of the machine.
	serv_addr.sin_port = htons(portno);	//the htons() function converts values between host and network byte orders. There is a difference between big-endian and little-endian and network byte order depending on your machine and network protocol in use.
	
	if(bind(sockfd, (struct sockaddr*) &serv_addr,sizeof(serv_addr))<0){	//Binding the socket
		error("Binding failed\n");
	}
	
	listen(sockfd, 5);//Listening for new Connections
	cli_len = sizeof(cli_addr);
	
	newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &cli_len);//Accept the new connection
	
	if(newsockfd<0)
		error("error accepting new connection \n");
	
	do {
	        char *buffer    = NULL;
                int buffer_size = 0;
                char extension[4];
                char file_name[256];
                FILE* fp = NULL;
               
                /*do {
                        if (read_size >= buffer_size) {
                            buffer = realloc(buffer, buffer_size + ALLOC_SLAB);
                            buffer_size = buffer_size + ALLOC_SLAB;
                        }
		        n = read(newsockfd, buffer, ALLOC_SLAB);//Read incoming data streams
                        read_size = read_size + n;
                        if(n < 0)
                            error("Error reading from Client");
                } while(n);*/

                n = read(newsockfd, &buffer_size, sizeof(int));//Read incoming data streams
                if(n < 0)
                    error("Error reading size from Client");
                printf("%d", buffer_size);
                buffer = malloc(buffer_size);
                n = read(newsockfd, extension, 4);//Read incoming data streams
                if(n < 4)
                    error("Error reading extension from Client");
                n = read(newsockfd, buffer, buffer_size);
                if(n < buffer_size)
                    error("Error reading file from Client");
                
                strcpy(file_name, "new-file");
                strcat(file_name, extension);
                fp = fopen(file_name, "wb");
                if (fp == NULL) {
                    error("Error opening file");
                }
                fwrite(buffer, buffer_size, 1, fp);
                fclose(fp);
	} while(0);
	
	close(sockfd);
	close(newsockfd);
	return 0;
}
