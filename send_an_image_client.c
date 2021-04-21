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

#define MAX_FILE_NAME 255
#define EXT_SIZE      4

enum errcodes {
        SOCKET_FAIL  = -1,
        READ_FAIL    = -2,
        WRITE_FAIL   = -3,
        INVALID_HOST = -4,
        CONNECT_FAIL = -5
};


int err = 0;

void error(char* msg, int errno) {
	perror(msg);
        err = errno;
}

/*int read_image_file(char *file_name, char **buffer, int *sizeof_buffer) {
        int c, i;
        int char_read = 0;
        FILE* fp = fopen(file_name, "rb");

        if (fp == NULL) {
                fprintf(stderr, "\t Can't open file : %s", file_name);
                return -1;
        }

        while (1) {
                * Continously read the file and if the size of the buffer is
                 * exhausted then reallocate and increase the buffer size by
                 * 1024.
                 *
                c = fgetc(fp);
                if (char_read == *sizeof_buffer) {
                        *buffer = (char *)realloc(*buffer, *sizeof_buffer + ALLOC_SLAB);
                        *sizeof_buffer = *sizeof_buffer + ALLOC_SLAB;
                }
                (*buffer)[char_read] = c;
                char_read++;
                if (c == EOF) {
                    break;
                }
        }

        return 0;
}*/

/* Find the size of a file using fseek() and ftell() and then read
 * the file in a buffer of size found.
 */
int read_image_file(char *file_name, char **buffer, int *sizeof_buffer) {
        int c, i;
        int char_read = 0;
        FILE* fp = fopen(file_name, "rb");

        if (fp == NULL) {
                fprintf(stderr, "\t Can't open file : %s", file_name);
                return -1;
        }
        
        fseek(fp, 0L, SEEK_END);
        *sizeof_buffer = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        *buffer = (char *)malloc(*sizeof_buffer);
        fread(*buffer, *sizeof_buffer, 1, fp);
        return 0;
}

int send_to_server(int sockfd, void *buffer, int size) {
        int n = write(sockfd, buffer, size);	//Write to server

        if (n < 0) {
            error("\t Error writing to server\n", WRITE_FAIL);
            free(buffer);
        }
}

int main(int argc, char *argv[]){
	int sockfd, portno;	//File descriptor for Sockets
	struct sockaddr_in serv_addr;	//Structure to hold Server and Client details
	struct hostent* server;

        if (argc < 3) {
	        fprintf(stderr, "Please provide the IP/hostname and port of the server\n");
                fprintf(stderr, "Usage : ./client <Server IP> <Server Port>\n");
		exit(1);
	}	
	
	portno = atoi(argv[2]);
        /* AF_INET refers to addresses from the internet, IP addresses specifically.
         * SOCK_STREAM to refer TCP(SOCK_DGRAM for UDP), 0 again for TCP
         */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("Socket failed\n", SOCKET_FAIL);
                goto safe_exit;
	}
	
	server = gethostbyname(argv[1]);	//Resolving the host server and storing its details
	if (server == NULL) {
		error("No such host\n", INVALID_HOST);
                goto safe_exit;
	}
	
	bzero((char*) &serv_addr, sizeof(serv_addr));	//Empty contents of serv_addr
	serv_addr.sin_family = AF_INET;	//Assign IPv4 family to server_address
        //Copy contents of host server address resolved into serv_addr
	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	
	if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {   //Connecting to Server
		error("Connection failed\n", CONNECT_FAIL);
                goto safe_exit;
        }
	
	do {
                int n;
                char file_name[MAX_FILE_NAME];
                char extension[EXT_SIZE];
                int size_of_file = 0;
                char *buffer     = NULL;

                printf("****************** WELCOME TO IMAGE TRANSFER WITHOUT FTP *****************\n");
                printf("\t Please enter a image to send : ");
		scanf("%s", &file_name);
                printf("\t Extension : ");
		scanf("%s", &extension);

                if (read_image_file(file_name, &buffer, &size_of_file)) {
                        error("\t Reading Image Failed", READ_FAIL);
                        goto safe_exit;
                }
                printf("\t Size of file : %d\n", size_of_file);
		n = send_to_server(sockfd, &size_of_file, sizeof(int));	//Write to server
		if (n < 0) {
                        goto safe_exit;
                }
		n = send_to_server(sockfd, extension, EXT_SIZE);	//Write to server
		if (n < 0) {
                        goto safe_exit;
                }
                printf("\t Sending File\n");
		n = send_to_server(sockfd, buffer, size_of_file);	//Write to server
		if (n < size_of_file) {
                        goto safe_exit;
                }
	} while(0);

safe_exit:
        if (sockfd > 0)
            close(sockfd);
	return err;
}
	
