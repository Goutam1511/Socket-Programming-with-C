/*
Select()
A better way to handle multiple clients is by using select() linux command. 
Select command allows to monitor multiple file descriptors, waiting until one of the file descriptors become active. 
For example, if there is some data to be read on one of the sockets select will provide that information. 
Select works like an interrupt handler, which gets activated as soon as any file descriptor sends any data.
Data structure used for select: fd_set
It contains the list of file descriptors to monitor for some activity.
There are four functions associated with fd_set:
fd_set readfds;

// Clear an fd_set
FD_ZERO(&readfds);  

// Add a descriptor to an fd_set
FD_SET(master_sock, &readfds);   

// Remove a descriptor from an fd_set
FD_CLR(master_sock, &readfds); 

//If something happened on the master socket , then its an incoming connection  
FD_ISSET(master_sock, &readfds); 

manual page of select : https://linux.die.net/man/2/select

activity = select( max_fd + 1 , &readfds , NULL , NULL , NULL);
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>//for FD macros

#define PORT 4444

int main(){

	int sockfd, ret,maxsd=0,sd;//temporary variables to store socket descriptor
	struct sockaddr_in serverAddr;
	int i, socketcount, bytesize;	
	int newSocket;
	int clientsocket[10];//Hold all the socket numbers of client
	struct sockaddr_in newAddr;
	fd_set readfds;//File descriptor set to handle all fds.
	FD_ZERO(&readfds);//Clear the file descriptor set

	socklen_t addr_size;

	char buffer[1024];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Server Socket is created.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	for (i = 0; i < 10; i++)//Initialise the client sockets to 0 
	{
		clientsocket[i] = 0;
	}

	ret = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if(ret < 0){
		printf("[-]Error in binding.\n");
		exit(1);
	}
	printf("[+]Bind to port %d\n", 4444);

	if(listen(sockfd, 10) == 0){
		printf("[+]Listening....\n");
	}else{
		printf("[-]Error in listening.\n");
	}
	
	while(1){
		bzero(buffer, sizeof(buffer));
		FD_ZERO(&readfds);//Clear the fd_set
		FD_SET( sockfd, &readfds);//Insert the master socket into the fd-set
		maxsd = sockfd;
		
		for(i=0;i<10;i++){
			sd = clientsocket[i];
			if(sd>0)
				FD_SET(clientsocket[i],&readfds);//Insert the client to fd-set if a valid socket and not 0
			if(sd>maxsd){//get the highest descriptor number for first argument of select function
				maxsd = sd;
			}
		}
		
		socketcount = select( maxsd+1, &readfds, NULL, NULL, NULL );
		
		if(socketcount<1){
			printf("[-]Error in select");
		}
		
		if(FD_ISSET(sockfd, &readfds)){//Check if the activity is on master socket for incoming connection
			newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);
			if(newSocket < 0){
				printf("[-]Error in accepting.\n");
				exit(1);
			}
			printf("[+]Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
			strcpy(buffer,"Hey, Welcome to the Chat Server");
			send(newSocket, buffer, strlen(buffer), 0);
			for(i=0;i<10;i++){
				if(clientsocket[i]==0)
					clientsocket[i]=newSocket;
				break;
			}
		}
		else{
			for(i=0;i<10;i++){
				sd = clientsocket[i];
				if(FD_ISSET(sd, &readfds)){
					bytesize = recv(sd, buffer, 1024,0);
					if(bytesize==0){
						close(sd);
						clientsocket[i]=0;
					}
					else{
						buffer[bytesize]='\0';
						send(sd,buffer,strlen(buffer),0);
					}
				}
			}
		}	
	}
	close(sockfd);
	return 0;
}