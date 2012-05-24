/******CLIENT******/

#include "header.h"

int main(int argc, char *argv[])
{
	int clientToTCPDClientSocket;
	struct sockaddr_in serverAddress;
	struct hostent *he;
	struct in_addr **addr_list;
	int bytesSent, bytesRead;
	char buffer[900], fileName[20], fileSize[10], serverIPAddress[15], serverPort[5], hostName[128], hostIpAddress[15];
	struct timespec time1, time2;
	int i;
	
	// If argument count doesn't match
	if (argc != 4) {
		printf("Usage: ./ftpc <server's IP address> <server's port number> <file to transfer>\n");
		exit(-1);
	}

	// set bits to 0
	memset(fileSize, 0, 10);
	memset(fileName, 0, 20);
	memset(buffer, 0, 900);
	memset(serverIPAddress, 0, 15);
	memset(serverPort, 0, 5);
	
	strcpy(serverIPAddress, argv[1]);
	strcpy(serverPort, argv[2]);
	strcpy(fileName, argv[3]);
	sprintf(fileSize, "%ld", getFileSize(argv[3]));
	printf("File size: %s\n", fileSize);
	
	gethostname(hostName, sizeof(hostName));
	if((he = gethostbyname(hostName)) == NULL) {
		printf("Unable to get host ip address by name!\n");
		exit(-3);
	}
	addr_list = (struct in_addr **)he->h_addr_list;
	strcpy(hostIpAddress, inet_ntoa(*addr_list[0]));
	printf("%s\n", hostIpAddress);

	// read from file
	FILE* filePointer = fopen(fileName, "r");
	if (filePointer < 0) {
		perror("Error: Unable to open file!");
		exit(-2);
	}

	// Open client socket to communicate with server
	clientToTCPDClientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientToTCPDClientSocket < 0) {
		perror("Error: Unable to open socket!\n");
		exit(-3);
	}

	// Set up server address parameters
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(TCPD_CLIENT_PORT);
	serverAddress.sin_addr.s_addr = inet_addr(LOCALHOST);
	printf("Sending to server @%s:%d\n", serverIPAddress, atoi(serverPort));

	// send host IP address
	bytesSent = SEND(clientToTCPDClientSocket, hostIpAddress, sizeof(hostIpAddress), 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send host IP address to tcpd_client\n");
		exit(0);
	}
	sleep(1);

	// Send server IP address
	bytesSent = SEND(clientToTCPDClientSocket, serverIPAddress, sizeof(serverIPAddress), 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send server IP Address on socket!\n");
		exit(-5);
	}
	printf("Server IP address sent!\n");
	sleep(1);
	
	// Send server port
	bytesSent = SEND(clientToTCPDClientSocket, serverPort, sizeof(serverPort), 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send server port on socket!\n");
		exit(-5);
	}
	printf("Server port sent!\n");
	sleep(1);

	// Send file size
	bytesSent = SEND(clientToTCPDClientSocket, fileSize, sizeof(fileSize), 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send file size on socket!\n");
		exit(-5);
	}
	printf("File size sent!\n");
	sleep(1);

	// Send file name
	bytesSent = SEND(clientToTCPDClientSocket, fileName, sizeof(fileName), 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send file name on socket!\n");
		exit(-6);
	}
	printf("File name sent!\n");
	sleep(1);

	// Send file
	bytesRead = fread(buffer, sizeof(char), sizeof(buffer), filePointer);
	int packetCount = 0;
	int totalPackets = (int)(getFileSize(argv[3])/900) + 1;
	while (bytesRead > 0) {
		usleep(50000);
		bytesSent = SEND(clientToTCPDClientSocket, buffer, bytesRead, 0, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));
		packetCount++;
		printf("Packet %d / %d sent\n", packetCount, totalPackets);
		bytesRead = fread(buffer, sizeof(char), sizeof(buffer), filePointer);
	}
	printf("File sent!\n");

	// close file and socket
	fclose(filePointer);
	close(clientToTCPDClientSocket);

	return 0;
}
