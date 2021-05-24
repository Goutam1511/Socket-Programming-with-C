#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>  //For gethostbyname function

void error(char* msg){
    perror(msg);
    exit(1);
}

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
    int n = write(sockfd, buffer, size);    //Write to server

    if (n < 0) {
        error("\t Error writing to server\n");
        free(buffer);
    }
    return n;
}

void client_func(void *arg)
{	
   	char** argv = (char** )arg; 
    int sockfd, portno, n;  //File descriptor for Sockets
    char *buffer = NULL;   //Temporary buffer to read and write data
    int size_of_file;

    struct sockaddr_in serv_addr;   //Structure to hold Server and Client details
    struct hostent* server;
    int len;
    char reply[1024];
	
	  server = gethostbyname(argv[1]);
    portno = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //AF_INET refers to addresses from the internet, IP addresses specifically. SOCK_STREAM to refer TCP(SOCK_DGRAM for UDP), 0 again for TCP
    if(sockfd<0){
        error("Socket failed \n");
    }

   
    if(server == NULL){
        error("No such host\n");
    }

    bzero((char*) &serv_addr, sizeof(serv_addr));   //Empty contents of serv_addr
    serv_addr.sin_family = AF_INET; //Assign IPv4 family to server_address
    bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);    //Copy contents of host server address resolved into serv_addr
    serv_addr.sin_port = htons(portno);

    if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr))<0) //Connecting to Server
        error("Connection failed\n");


    if (read_image_file("ddt-logo1.png", &buffer, &size_of_file)) {
        error("\t Reading Image Failed");
    }
    //printf("\t Size of file : %d\n", size_of_file);
    n = send_to_server(sockfd, &size_of_file, sizeof(int)); //Write to server
    if (n < 0) {
        error("\t Sending Size Failed");
    }
    printf("\t Sending File of size : %d\n",size_of_file);
    n = send_to_server(sockfd, buffer, size_of_file);   //Write to server
    if (n < size_of_file) {
        error("\t Sending Image Failed");
    }
       
    len = recv(sockfd, reply, 1024, 0);
    reply[len] = '\0';
    printf("%s\n",reply);
}

int main(int argc, char *argv[]){
    int i;
    for (i = 0; i < 2; i++) {
        client_func(argv);
        sleep(2);
    }
    return 0;
}
