#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char *argv[])
{
	int serverSocket, connectSocket;
	struct sockaddr_in serverAddress, clientAddress;
	int bytesReceived, totalBytesReceived, bytesWritten, portNumber;
	char inputBuffer[1000], fileName[20], fileSize[10];
	char newFilename[25] = "new";
	FILE *filePointer;
	
	// Check if argument count does not match
	if (argc != 2) {
	    printf("USAGE: ./ftps <port number>\n");
	    exit(-1);    
	}

//	bzero(inputBuffer, 1000);
//	bzero(fileSize, 10);
//	bzero(fileName, 20);

	// Set bits to 0
	memset(inputBuffer, 0, 1000);
	memset(fileSize, 0, 10);
	memset(fileName, 0, 20);
	
	// Open server socket
	portNumber = atoi(argv[1]);
	printf("Port Number: %d\n", portNumber);
	if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error: Unable to open stream socket!\n");
		exit(-2);
	}
	printf("Socket Created!\n");

	// Bind server socket
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(portNumber);
	if(bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof (struct sockaddr_in)) < 0) {
		perror("Error: Unable to bind server socket!\n");
		exit(-3);
	}
	printf("Socket bound!\n");

	// Listen for connections on the socket
	listen(serverSocket, 1);
	if((connectSocket = accept(serverSocket, (struct sockaddr*) NULL, (int*) NULL)) == -1) {
	    perror("Error: Unable to connect to socket!\n");
	    exit(-4);
	}

	// Receive file size
	printf("Socket connection successful!\n");
	bytesReceived = recv(connectSocket, fileSize, sizeof(fileSize), 0);
	if(bytesReceived < 0) {
	    perror("Error: Unable to read from the socket! (1)\n");
	    exit(-5);
	}
	printf("File Size: %d\n", atoi(fileSize));

	// Receive file name
	if(recv(connectSocket, fileName, sizeof(fileName), 0) < 0)	{
	    perror("Error: Unable to read from socket! (2)\n");
	    exit(-6);
	}
	printf("File Name: %s\n", fileName);
	
	// Open file
	filePointer = fopen(strcat(newFilename, fileName), "w");
	if (filePointer < 0) {
		printf("Error: Unable to open file!\n");
		exit(-7);
	}
	totalBytesReceived = 0;

	// Receive file
	while((bytesReceived = recv(connectSocket, inputBuffer, sizeof(inputBuffer), 0)) > 0) {
	    totalBytesReceived += bytesReceived; 
	    bytesWritten = fwrite(inputBuffer, sizeof(char), bytesReceived, filePointer);
	}

	// Check if file received
	if(totalBytesReceived < atoi(fileSize)) {
		printf("Error: Did not receive complete file!\n");
		exit(-8);
	}
	printf("File received!\n");

	// close file and sockets
	fclose(filePointer);
	close(connectSocket);
	close(serverSocket);
	return 0;
}
