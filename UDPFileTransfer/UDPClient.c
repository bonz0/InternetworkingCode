/******CLIENT******/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

long getFileSize (char* fileName);

int main(int argc, char *argv[])
{
	int clientSocket;
	struct sockaddr_in serverAddress;
	int bytesSent, bytesRead;
	char buffer[1000], fileName[20], fileSize[10];

	// If argument count doesn't match
	if (argc != 4) {
		printf("Usage: ./ftpc <server's IP address> <server's port number> <file to transfer>\n");
		exit(-1);
	}

	// set bits to 0
	memset(fileSize, 0, 10);
	memset(fileName, 0, 20);
	memset(buffer, 0, 1000);
	
	strcpy(fileName, argv[3]);
	sprintf(fileSize, "%ld", getFileSize(argv[3]));
	printf("Struct file size: %s", fileSize);

	// read from file
	FILE* filePointer = fopen(fileName, "r");
	if (filePointer < 0) {
		perror("Error: Unable to open file!");
		exit(-2);
	}

	// Open client socket to communicate with server
	clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientSocket < 0) {
		perror("Error: Unable to open socket!\n");
		exit(-3);
	}

	// Set up server address parameters
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(atoi(argv[2]));
	serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
	printf("Sending to server @%s:%d\n", argv[1], atoi(argv[2]));

	// Send file size
	bytesSent = sendto(clientSocket, fileSize, sizeof(fileSize), 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send file size on socket!\n");
		exit(-5);
	}
	printf("File size sent!\n");

	// Send file name
	bytesSent = sendto(clientSocket, fileName, sizeof(fileName), 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send file name on socket!\n");
		exit(-6);
	}
	printf("File name sent!\n");

	// Send file
	bytesRead = fread(buffer, sizeof(char), sizeof(buffer), filePointer);
	printf("%s", buffer);
	int iii;
	for (iii = 0; iii < 32; iii++) {
		buffer[iii] = iii+20;
	}
	int packetCount = 0;
	int totalPackets = (int)(getFileSize(argv[3])/1000) + 1;
	while (bytesRead > 0) {
		bytesSent = sendto(clientSocket, buffer, bytesRead, 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));
		packetCount++;
		printf("Packet %d / %d sent\n", packetCount, totalPackets);
		bytesRead = fread(buffer, sizeof(char), sizeof(buffer), filePointer);
	}
	printf("File sent!\n");

	// close file and socket
	fclose(filePointer);
	close(clientSocket);

	return 0;
}

long getFileSize(char* fileName)
{
	FILE* filePointer;
	long size = 0L;
	filePointer = fopen(fileName, "r");
	if (filePointer != NULL) {
		fseek(filePointer, 0L, SEEK_END);
		size = ftell(filePointer);
		fclose(filePointer);
	}
	return size;
}
