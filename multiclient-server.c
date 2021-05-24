#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 4444


char img_id = 'a';

void error(char *msg) {
    perror(msg);
    exit(1);
}

void* thread_server(void *sockfd)
{
    char *buffer;
    char file_name[255];
    int buffer_size;
    int newsockfd = *(int *)sockfd;
    int n;
    FILE *fp;
	  char img[] = "image-";
    char *reply = "Image received to server";
    

    n = read(newsockfd, &buffer_size, sizeof(int));//Read incoming data streams
    if(n < 0)
        error("Error reading size from Client");
    printf("File size : %d\n", buffer_size);
    buffer = (char *)malloc(buffer_size + 1);
    if (buffer == NULL)
        printf("Alloc failed\n");
    
    printf("Buffer Allocated\n");
    n = read(newsockfd, buffer, buffer_size);
    /*if(n < buffer_size)
        error("Error reading file from Client");*/
    printf("\nFile read of size : %d\n", n);
	
	  strncat(img, &img_id, 1);
	  strcat(img, ".png");
    strcpy(file_name, img);
    ++img_id;
	
    fp = fopen(file_name, "wb");
    if (fp == NULL) {
        error("Error opening file");
    }
    fwrite(buffer, buffer_size, 1, fp);
    free(buffer);
	
    fclose(fp);
    send(newsockfd, (char *)reply, strlen(reply), 0);
    printf("\n Reply sent to Client from Server \n");
    close(newsockfd);
}

int main(int argc, char *argv[]){
	  if(argc<2){
		  	fprintf(stderr,"No port provided\n");
			  exit(1);
    }		
    
    pthread_t tid[5];
    int sockfd, ret, i = 0, j = 0, portno;
    struct sockaddr_in serverAddr;
    int newSocket;
    struct sockaddr_in newAddr;
    socklen_t addr_size;
    char buffer[1024];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        printf("[-]Error in connection.\n");
        exit(1);
    }
    printf("[+]Server Socket is created.\n");
	
    portno = atoi(argv[1]);

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portno);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if(ret < 0){
        printf("[-]Error in binding.\n");
        exit(1);
    }
    printf("[+]Bind to port %d\n", portno);

    if(listen(sockfd, 10) == 0){
        printf("[+]Listening....\n");
    }else{
        printf("[-]Error in binding.\n");
    }

    while (1) {
        newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);
        if(newSocket < 0){
            exit(1);
        } else {
            printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
            pthread_create(&tid[i], NULL, thread_server, &newSocket);
            i++;
        }
        if (i >= 5)
            break;
    }
    while (j < i) {
        pthread_join(tid[j++], NULL);
    }
    close(sockfd);

    return 0;
}
