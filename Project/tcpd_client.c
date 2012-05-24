/******TCPD_CLIENT******/

#include "header.h"

int main(int argc, char *argv[])
{
	int socketFromClient, socketToTroll, socketFromTroll, socketFromTimer, socketToTimer;
	struct sockaddr_in sockaddrFromClient, sockaddrToTroll, sockaddrFromTroll, sockaddrToTimer, sockaddrFromTimer;
	struct messageToTroll dataGramToTroll, ackFromTroll;
	timerPacket packetToTimer;
	int bytesSentToTroll, bytesSentToTimer, bytesReceivedFromTroll, bytesReceivedFromClient, bytesReceivedFromTimer, totalbytesReceivedFromClient = 0, selectReturnValue, packetNumber = -1, packetToResend = -1; 
	char buffer[BUFFER_SIZE], fileName[20], fileSize[10], serverIPAddress[15], serverPort[5], hostIpAddress[15];
	unsigned long receivedChecksum;
	fd_set socketReadSet, socketWriteSet;
	window sendingWindow;
	sendingWindow.nextFree = 0;
	sendingWindow.base = 0;

	// set bits to 0
	memset(fileSize, 0, 10);
	memset(fileName, 0, 20);
	memset(buffer, 0, BUFFER_SIZE);
	memset(serverIPAddress, 0, 15);
	memset(serverPort, 0, 5);
	memset(&packetToTimer, 0, sizeof(packetToTimer));
	
	// Open client socket to receive data from the client 
	printf("TCPD_CLIENT waiting on port number: %d\n", TCPD_CLIENT_PORT);
	socketFromClient = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketFromClient < 0) {
		perror("Error: Unable to open socket to communicate with client!\n");
		exit(-1);
	}

	// open socket to TROLL
	socketToTroll = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketToTroll < 0) {
		perror("Error: Unable to open socket to communicate with troll!\n");
		exit(-2);
	}

	// open socket to send packet to timer
	socketToTimer = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketToTimer < 0) {
		perror("Error: Unable to open socket to communicate with timer!\n");
		exit(-3);
	}

	// open socket to listen from the timer
	socketFromTimer = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketFromTimer < 0) {
		perror("Error: Unable to open socket to receive data from the timer!\n");
		exit(-4);
	}
	
	// open socket to .payloadlisten from troll
	socketFromTroll = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketFromTroll < 0) {
		perror("Error: Unable to open socket to receive data from the troll!\n");
		exit(-5);
	}

	// Set up client address parameters to receive data from client
	sockaddrFromClient.sin_family = AF_INET;
	sockaddrFromClient.sin_port = htons(TCPD_CLIENT_PORT);
	sockaddrFromClient.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(socketFromClient, (struct sockaddr*)&sockaddrFromClient, sizeof(struct sockaddr_in)) < 0) {
		perror("Error: Unable to bind socket to communicate with client!\n");
		exit(-3);
	}
	printf("Socket to communicate with client bound!\n");

	// set up sockaddr details to receive from the timer
	sockaddrFromTimer.sin_family = AF_INET;
	sockaddrFromTimer.sin_port = htons(TIMER_TO_TCPD_PORT);
	sockaddrFromTimer.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(socketFromTimer, (struct sockaddr*)&sockaddrFromTimer, sizeof(struct sockaddr_in)) < 0) {
		perror("Error: Unable to bind socket to receive from the timer!\n");
		exit(-4);
	}

	// setup sockaddr details to receive from the troll
	sockaddrFromTroll.sin_family = AF_INET;
	sockaddrFromTroll.sin_port = htons(TCPD_CLIENT_ACK_PORT);
	sockaddrFromTroll.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(socketFromTroll, (struct sockaddr*)&sockaddrFromTroll, sizeof(struct sockaddr_in)) < 0) {
		perror("Error: Unable to bind socket to receive from the troll!\n");
		exit(-4);
	}

	// Receive server details: IP Address and port number
	int fromLen = sizeof(struct sockaddr_in);

	bytesReceivedFromClient = recvfrom(socketFromClient, hostIpAddress, sizeof(hostIpAddress), 0, (struct sockaddr*)&sockaddrFromClient, &fromLen);
	if (bytesReceivedFromClient < 0) {
		perror("Error: Unable to read host IP address!\n");
		exit(0);
	}

	bytesReceivedFromClient = recvfrom(socketFromClient, serverIPAddress, sizeof(serverIPAddress), 0, (struct sockaddr*)&sockaddrFromClient, &fromLen);
	if (bytesReceivedFromClient < 0) {
		perror("Error: Unable to server IP address\n");
		exit(-4);
	}
	printf("Sending to IP Address: %s\n", serverIPAddress);
	bytesReceivedFromClient = recvfrom(socketFromClient, serverPort, sizeof(serverPort), 0, (struct sockaddr*)&sockaddrFromClient, &fromLen);
	if (bytesReceivedFromClient < 0) {
		perror("Error: Unable to read server port!\n");
		exit(-5);
	}
	printf("Sending to Port: %s\n", serverPort);
	
	// set up details for forwarding data to the troll
	sockaddrToTroll.sin_family = AF_INET;
	sockaddrToTroll.sin_addr.s_addr = inet_addr(LOCALHOST);
	sockaddrToTroll.sin_port = htons(TROLL_PORT);

	// set up details of sockaddr_in for sending to the timer
	sockaddrToTimer.sin_family = AF_INET;
	sockaddrToTimer.sin_addr.s_addr = inet_addr(LOCALHOST);
	sockaddrToTimer.sin_port = htons(TIMER_LISTEN_PORT);

	// setup details of message that is supposed to be sent to the troll
	dataGramToTroll.trollHeader.sin_family = htons(AF_INET);
	dataGramToTroll.trollHeader.sin_addr.s_addr = (inet_addr(serverIPAddress));
	dataGramToTroll.trollHeader.sin_port = htons(TCPD_SERVER_PORT);

	memset(dataGramToTroll.payload.data, 0, sizeof(dataGramToTroll.payload.data));
	strcpy(dataGramToTroll.payload.data, hostIpAddress);
	dataGramToTroll.payload.size = sizeof(hostIpAddress);
	dataGramToTroll.checksum = crc((void*)&dataGramToTroll.payload, sizeof(dataGramToTroll.payload), 0);

	// send host IP address to TROLL
	bytesSentToTroll = sendto(socketToTroll, (void*)&dataGramToTroll, sizeof(dataGramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
	if (bytesSentToTroll < 0) {
		perror("Error: Unable to server IP address to troll!\n");
		exit(-6);
	}
	printf("Host IP Address sent to server!\n");
	memset(&dataGramToTroll.payload, 0, sizeof(dataGramToTroll.payload));
	
	// send server port to troll
	strcpy(dataGramToTroll.payload.data, serverPort);
	dataGramToTroll.payload.size = sizeof(serverPort);
	dataGramToTroll.checksum = crc((void *)&dataGramToTroll.payload, sizeof(dataGramToTroll.payload), 0);
	bytesSentToTroll = sendto(socketToTroll, (void*)&dataGramToTroll, sizeof(dataGramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
	if (bytesSentToTroll < 0) {
		perror("Error: Unable to send server port number to troll!\n");
		exit(-6);
	}
	printf("Server port sent to server\n");

	// send notification to timer
	packetToTimer.SEQ = -1;
	packetToTimer.timeout = 1000000.0;
	bytesSentToTimer = sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
	memset(&packetToTimer, 0, sizeof(packetToTimer));

	// receive ack for server port
	bytesReceivedFromTroll = recvfrom(socketFromTroll, (void*)&ackFromTroll, sizeof(ackFromTroll), 0, (struct sockaddr*)&sockaddrFromTroll, &fromLen);
	if (bytesReceivedFromTroll < 0) {
		perror("Error: Unable to receive ack for server port from troll!\n");
		exit(-4);
	}
	receivedChecksum = crc((void*)&ackFromTroll.payload, sizeof(ackFromTroll.payload), 0);
	if (receivedChecksum != ackFromTroll.checksum) {
		printf("Error: Error in ack for server port!\n");
	}
	printf("SEQ = %d\nACK = %d\n", ackFromTroll.payload.SEQ, ackFromTroll.payload.ACK);
	packetToTimer.SEQ = ackFromTroll.payload.SEQ;
	packetToTimer.timeout = 0.0;

	// send notification to timer to delete node
	bytesSentToTimer = sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
	
	// Receive file size
//	bytesReceivedFromClient = recvfrom(socketFromClient, fileSize, sizeof(fileSize), 0, (struct sockaddr*)&sockaddrFromClient, &fromLen);
//	if (bytesReceivedFromClient < 0) {
//		perror("Error: Unable to read file size socket communicating with client!\n");
//		exit(-5);
//	}
//	printf("File size: %s\n", fileSize);

	// Send file size to TROLL
//	memset(&dataGramToTroll.payload, 0, sizeof(dataGramToTroll.payload));
//	strcpy(dataGramToTroll.payload.data, fileSize);
//	dataGramToTroll.payload.size = sizeof(fileSize);
//	dataGramToTroll.checksum = crc((void *)&dataGramToTroll.payload, sizeof(dataGramToTroll.payload), 0);
/*	bytesSentToTroll = sendto(socketToTroll, (void*)&dataGramToTroll, sizeof(dataGramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
	if (bytesSentToTroll < 0) {
		perror("Error: Unable to server IP address to troll!\n");
		exit(-6);
	}
	printf("File size sent to server\n");
	
	// Receive file name
	bytesReceivedFromClient = recvfrom(socketFromClient, fileName, sizeof(fileName), 0, (struct sockaddr*)&sockaddrFromClient, &fromLen);
	if (bytesReceivedFromClient < 0) {
		perror("Error: Unable to read file name from socket communicating with client!\n");
		exit(-5);
	}
	printf("File name: %s\n", fileName);

	memset(&dataGramToTroll.payload, 0, sizeof(dataGramToTroll.payload));
	strcpy(dataGramToTroll.payload.data, fileName);
	dataGramToTroll.payload.size = sizeof(fileName);
	dataGramToTroll.checksum = crc((void *)&dataGramToTroll.payload, sizeof(dataGramToTroll.payload), 0);
	bytesSentToTroll = sendto(socketToTroll, (void*)&dataGramToTroll, sizeof(dataGramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
	if (bytesSentToTroll < 0) {
		perror("Error: Unable to server IP address to troll!\n");
		exit(-6);
	}
	printf("File name sent to server\n");
*/	
	// Receive file
	printf("Sending file to server side!\n");
	
	// Send all received bytes to server side
	while(1) {
		FD_ZERO(&socketReadSet);
		FD_SET(socketFromClient, &socketReadSet);
		FD_SET(socketFromTimer, &socketReadSet);
		FD_SET(socketFromTroll, &socketReadSet);
		selectReturnValue = select(socketFromTroll + 1, &socketReadSet, NULL, NULL, NULL);
		if (selectReturnValue < 0) {
			perror("Error: Select returned a negative value!\n");
			exit(-99);
		}
		memset(buffer, 0, sizeof(buffer));
		memset(&dataGramToTroll.payload, 0, sizeof(dataGramToTroll.payload));
		if (FD_ISSET(socketFromClient, &socketReadSet)) {
			bytesReceivedFromClient = recvfrom(socketFromClient, buffer, sizeof(buffer), 0, (struct sockaddr*)&sockaddrFromClient, &fromLen);
			if (bytesReceivedFromClient > 0) {
				packetNumber++;
				packetToTimer.SEQ = packetNumber;
				packetToTimer.timeout = 1000000.0; 
				memcpy(dataGramToTroll.payload.data, buffer, bytesReceivedFromClient);
				dataGramToTroll.payload.size = bytesReceivedFromClient;
				dataGramToTroll.checksum = crc((void *)&dataGramToTroll.payload, sizeof(dataGramToTroll.payload), 0);
				dataGramToTroll.payload.SEQ = packetNumber;
				dataGramToTroll.payload.sent = 0;
				sendingWindow.packetArray[packetNumber % WINDOW_SIZE] = dataGramToTroll;
				sendingWindow.nextFree = ((packetNumber + 1) % WINDOW_SIZE);
				if (sendingWindow.nextFree == sendingWindow.base) {
					printf("Circular buffer full!\n");
//					exit(-1);
				}
				sendFromWindow(&sendingWindow);
				bytesSentToTroll = sendto(socketToTroll, (void*)&dataGramToTroll, sizeof(dataGramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
				if (bytesSentToTroll > 0) {
					bytesSentToTimer = sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
					if (bytesSentToTimer <= 0) {
						printf("Error: Unable to send %d packet to timer!\n", packetNumber);
						exit(-100);
					}
				}
				else {
					printf("Error: Unable to send packet %d to troll!\n", packetNumber);
					exit(-101);
				}
			}
		}
		if (FD_ISSET(socketFromTimer, &socketReadSet)) {
			bytesReceivedFromTimer = recvfrom(socketFromTimer, &packetToResend, sizeof(int), 0, (struct sockaddr*)&sockaddrFromTimer, &fromLen);
			printf("Timeout occured for packet number %d!\n", packetToResend);
			// RESEND PACKET
		}
		if (FD_ISSET(socketFromTroll, &socketReadSet)) {
			memset(&ackFromTroll.payload, 0, sizeof(ackFromTroll.payload));
			printf("Receiving from troll!\n");
			bytesReceivedFromTroll = recvfrom(socketFromTroll, (void*)&ackFromTroll, sizeof(ackFromTroll), 0, (struct sockaddr*)&sockaddrFromTroll, &fromLen);
			if (bytesReceivedFromTroll < 0) {
				perror("Error: Unable to receive ack for server port from troll!\n");
//				exit(-4);
			}
			receivedChecksum = crc((void*)&ackFromTroll.payload, sizeof(ackFromTroll.payload), 0);
			if (receivedChecksum != ackFromTroll.checksum) {
				printf("Error: Error in ack for server port!\n");
			}
			printf("SEQ = %d\nACK = %d\n", ackFromTroll.payload.SEQ, ackFromTroll.payload.ACK);
			packetToTimer.SEQ = ackFromTroll.payload.SEQ - 1;
			packetToTimer.timeout = 0.0;
			// send notification to timer to delete node
			bytesSentToTimer = sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
		}
	}

	// close file and socket
	close(socketFromClient);
	close(socketToTroll);
	return 0;
}
