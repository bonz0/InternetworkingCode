/******TCPD_SERVER******/

#include "header.h"

int main(int argc, char *argv[])
{
	int socketFromTroll, socketToServer, socketToTroll;
	struct sockaddr_in sockaddrFromTroll, sockaddrToServer, sockaddrToTroll;
	struct messageToTroll ackToTroll;
	int bytesSent, bytesReceived, totalBytesReceived = 0;
	char buffer[BUFFER_SIZE], fileName[20], fileSize[10], serverIPAddress[15], serverPort[5], bigBuffer[1500], hostIPAddress[15];
	struct messageToTroll datagramFromTroll;
	int packetCount = 0, incorrectPacketCount = 0;
	unsigned long receivedChecksum;
	CircularBuffer receivingBuffer;
	int iii;
	receivingBuffer.base = 0;

	for (iii = 0; iii < CIRCULAR_BUFFER_SIZE; iii++) {
		receivingBuffer.ackBuffer[iii] = -1;
	}
	printWindow(receivingBuffer.ackBuffer);

	// set bits to 0
	memset(fileSize, 0, 10);
	memset(fileName, 0, 20);
	memset(buffer, 0, BUFFER_SIZE);
	memset(serverIPAddress, 0, 15);
	memset(serverPort, 0, 5);
	memset(&datagramFromTroll.payload, 0, sizeof(datagramFromTroll.payload));

	// Open client socket to communicate with server
	printf("TCPD_SERVER waiting on port number: %d\n", TCPD_SERVER_PORT);
	socketFromTroll = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketFromTroll < 0) {
		perror("Error: Unable to open socket to communicate with TCPD_client!\n");
		exit(-2);
	}

	// open socket to send data to SERVER
	socketToServer = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketToServer < 0) {
		perror("Error: Unable to open socket to SERVER!\n");
		exit(-3);
	}

	// open socket to send data to the troll
	socketToTroll = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketToTroll < 0) {
		perror("Error: Unable to open socket to troll!\n");
		exit(-4);
	}

	// Set up TCPD_SERVER address parameters to listen from TROLL
	sockaddrFromTroll.sin_family = AF_INET;
	sockaddrFromTroll.sin_port = htons(TCPD_SERVER_PORT);
	sockaddrFromTroll.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(socketFromTroll, (struct sockaddr*)&sockaddrFromTroll, sizeof(struct sockaddr_in)) < 0) {
		perror("Error: Unable to bind socket to communicate with the troll!\n");
		exit(-4);
	}
	printf("Socket to communicate with Troll bound!\n");

	// Receive server details: IP Address and port number
	int fromLen = sizeof(struct sockaddr_in);
	bytesReceived = recvfrom(socketFromTroll, (void*)&datagramFromTroll, sizeof(datagramFromTroll), 0, (struct sockaddr*)&sockaddrFromTroll, &fromLen);
	if (bytesReceived < 0) {
		perror("Error: Unable to read host IP Address socket communicating with client!\n");
		exit(-4);
	}
	packetCount++;
	strcpy(hostIPAddress, datagramFromTroll.payload.data);
	receivedChecksum = crc((void *)&datagramFromTroll.payload, sizeof(datagramFromTroll.payload), 0);
	if (receivedChecksum != datagramFromTroll.checksum) {
		printf("Error in host IP Address\n");
		incorrectPacketCount++;
	}
	printf("Receiving from IP Address: %s\n", hostIPAddress);
	memset(&datagramFromTroll.payload, 0, sizeof(datagramFromTroll.payload));

	// set up sockaddr details to send ACK to tcpd_client
	sockaddrToTroll.sin_family = AF_INET;
	sockaddrToTroll.sin_addr.s_addr = inet_addr(hostIPAddress);
	sockaddrToTroll.sin_port = htons(TROLL_PORT);

	// set up details of message that is supposed to be sent to the troll
	ackToTroll.trollHeader.sin_family = htons(AF_INET);
	ackToTroll.trollHeader.sin_addr.s_addr = inet_addr(LOCALHOST);
	ackToTroll.trollHeader.sin_port = htons(TCPD_CLIENT_ACK_PORT);

	bytesReceived = recvfrom(socketFromTroll, (void*)&datagramFromTroll, sizeof(datagramFromTroll), 0, (struct sockaddr*)&sockaddrFromTroll, &fromLen);
	if (bytesReceived < 0) {
		perror("Error: Unable to read server port from socket!\n");
		exit(-4);
	}
	packetCount++;
	receivedChecksum = crc((void *)&datagramFromTroll.payload, sizeof(datagramFromTroll.payload), 0);
	if (receivedChecksum != datagramFromTroll.checksum) {
		printf("Error in PORT NUMBER\n");
		incorrectPacketCount++;
	}
	strcpy(serverPort, datagramFromTroll.payload.data);
	printf("Sending to Port: %s\n", serverPort);
	memset(&datagramFromTroll.payload, 0, sizeof(datagramFromTroll.payload));
	ackToTroll.payload.ACK = 1;
	ackToTroll.payload.SEQ = -1;
	ackToTroll.payload.size = 0;
	ackToTroll.checksum = crc((void*)&ackToTroll.payload, sizeof(ackToTroll.payload), 0);

	// send acknowledgement for server port
	bytesSent = sendto(socketToTroll, (void*)&ackToTroll, sizeof(ackToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send ack for server port to troll!\n");
		exit(-3);
	}

	// Set server details into struct
	sockaddrToServer.sin_family = AF_INET;
	sockaddrToServer.sin_addr.s_addr = inet_addr(LOCALHOST);
	sockaddrToServer.sin_port = htons(atoi(serverPort));

	// Receive file size
/*	bytesReceived = recvfrom(socketFromTroll, (void*)&datagramFromTroll, sizeof(datagramFromTroll), 0, (struct sockaddr*)&sockaddrFromTroll, &fromLen);
	if (bytesReceived < 0) {
		perror("Error: Unable to read file size socket communicating with TCPD_client!\n");
		exit(-5);
	}
	packetCount++;
	receivedChecksum = crc((void *)&datagramFromTroll.payload, sizeof(datagramFromTroll.payload), 0);
	printf("Seq %d checksum:%lu\n", datagramFromTroll.payload.SEQ, datagramFromTroll.checksum);
	if (receivedChecksum != datagramFromTroll.checksum) {
		printf("Error in FILE SIZE\n");
		incorrectPacketCount++;
	}
	strcpy(fileSize, datagramFromTroll.payload.data);
	printf("File size: %s\n", fileSize);
	memset(&datagramFromTroll.payload, 0, sizeof(datagramFromTroll.payload));
	memset(&ackToTroll.payload, 0, sizeof(ackToTroll.payload));
	ackToTroll.payload.ACK = 1;
	ackToTroll.payload.SEQ = 0;
	ackToTroll.payload.size = 0;
	ackToTroll.checksum = crc((void*)&ackToTroll.payload, sizeof(ackToTroll.payload), 0);
	// send acknowledgement for file size
	bytesSent = sendto(socketToTroll, (void*)&ackToTroll, sizeof(ackToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send ack for file size to troll!\n");
		exit(-3);
	}

	// Send file size to ftps
	bytesSent = sendto(socketToServer, fileSize, sizeof(fileSize), 0, (struct sockaddr*)&sockaddrToServer, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send file size to server!\n");
		exit(-7);
	}
	printf("File size sent to server side\n");

	// Receive file name
	bytesReceived = recvfrom(socketFromTroll, (void*)&datagramFromTroll, sizeof(datagramFromTroll), 0, (struct sockaddr*)&sockaddrFromTroll, &fromLen);
	if (bytesReceived < 0) {
		perror("Error: Unable to read file name from socket communicating with client!\n");
		exit(-8);
	}
	packetCount++;
	receivedChecksum = crc((void *)&datagramFromTroll.payload, sizeof(datagramFromTroll.payload), 0);
	printf("Seq %d checksum:%lu\n", datagramFromTroll.payload.SEQ, datagramFromTroll.checksum);
	if (receivedChecksum != datagramFromTroll.checksum) {
		printf("Error in FILE NAME\n");
		incorrectPacketCount++;
	}
	strcpy(fileName, datagramFromTroll.payload.data);
	printf("File name: %s\n", fileName);
	memset(&datagramFromTroll.payload, 0, sizeof(datagramFromTroll.payload));
	memset(&ackToTroll.payload, 0, sizeof(ackToTroll.payload));
	ackToTroll.payload.ACK = 1;
	ackToTroll.payload.SEQ = 1;
	ackToTroll.payload.size = 0;
	ackToTroll.checksum = crc((void*)&ackToTroll.payload, sizeof(ackToTroll.payload), 0);
	// send acknowledgement for file name
	bytesSent = sendto(socketToTroll, (void*)&ackToTroll, sizeof(ackToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send ack for file name to troll!\n");
		exit(-3);
	}

	// Send file name to ftps 
	bytesSent = sendto(socketToServer, fileName, sizeof(fileName), 0, (struct sockaddr*)&sockaddrToServer, sizeof(struct sockaddr_in));
	if (bytesSent < 0) {
		perror("Error: Unable to send file size to server!\n");
		exit(-9);
	}
	printf("File name sent to server side\n");
*/
	// Receive file
	printf("Sending file to server side!\n");
	
	// Send all received bytes to server side
	packetCount = -1;
	while(1) {
		memset(&datagramFromTroll.payload, 0, sizeof(datagramFromTroll.payload));
		memset(&ackToTroll.payload, 0, sizeof(ackToTroll.payload));
		memset(buffer, 0, sizeof(buffer));
		bytesReceived = recvfrom(socketFromTroll, (void*)&datagramFromTroll, sizeof(datagramFromTroll), 0, (struct sockaddr*)&sockaddrFromTroll, &fromLen);
		if (bytesReceived > 0) {
			packetCount++;
			receivedChecksum = crc((void *)&datagramFromTroll.payload, sizeof(datagramFromTroll.payload), 0);
			if (receivedChecksum != datagramFromTroll.checksum) {
				printf("Error in FILE %d\n", datagramFromTroll.payload.SEQ);
				printf("Error in packet number %d\n", packetCount % CIRCULAR_BUFFER_SIZE);
				incorrectPacketCount++;
			}
			else {
				printf("Packet %d received!\n", datagramFromTroll.payload.SEQ);
				ackToTroll.payload.ACK = 1;
				ackToTroll.payload.SEQ = datagramFromTroll.payload.SEQ;
				ackToTroll.payload.size = 0;
				ackToTroll.checksum = crc((void*)&ackToTroll.payload, sizeof(ackToTroll.payload), 0);
				// send acknowledgement for data
				bytesSent = sendto(socketToTroll, (void*)&ackToTroll, sizeof(ackToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
				if (bytesSent < 0) {
					printf("Error: Unable to send ack for packet %d to troll!\n", ackToTroll.payload.SEQ);
				}
				printf("Ack sent for %d\n", datagramFromTroll.payload.SEQ);
				if((inWindow(datagramFromTroll.payload.SEQ, receivingBuffer.base) && receivingBuffer.ackBuffer[datagramFromTroll.payload.SEQ] != 0)) {
//				if (receivingBuffer.ackBuffer[datagramFromTroll.payload.SEQ] != 0 && receivingBuffer.base <= datagramFromTroll.payload.SEQ) {
					receivingBuffer.packetArray[datagramFromTroll.payload.SEQ] = datagramFromTroll;
					receivingBuffer.ackBuffer[datagramFromTroll.payload.SEQ] = 0;
				}
				if (receivingBuffer.base == datagramFromTroll.payload.SEQ) {
					printf("Base = SEQ = %d\n", datagramFromTroll.payload.SEQ);
					printWindow(receivingBuffer.ackBuffer);
					if (!windowWrappedAround(receivingBuffer.base)) {
						printf("Window not wrapped around!\n");
						if (receivingBuffer.base == CIRCULAR_BUFFER_SIZE - WINDOW_SIZE) {
							for (iii = receivingBuffer.base; iii < CIRCULAR_BUFFER_SIZE; iii++) {
								if (receivingBuffer.ackBuffer[iii] == 0) {
									// FORWARD THE PACKET
									bytesSent = sendto(socketToServer, &receivingBuffer.packetArray[iii].payload.data, datagramFromTroll.payload.size, 0, (struct sockaddr*)&sockaddrToServer, sizeof(struct sockaddr_in));
									if (bytesSent < 0) {
										printf("Error: Unable to forward packet number %d!\n", receivingBuffer.packetArray[iii].payload.SEQ);
									}
									else {
										printf("Packet number %d forwarded!\n", receivingBuffer.packetArray[iii].payload.SEQ);
										receivingBuffer.ackBuffer[iii] = -1;
										receivingBuffer.base = (receivingBuffer.base + 1) % CIRCULAR_BUFFER_SIZE;
									}
								}
								else {
									break;
								}
							}
						}
						else {
							for (iii = receivingBuffer.base; iii < (receivingBuffer.base + WINDOW_SIZE) % CIRCULAR_BUFFER_SIZE; iii++) {
								if (receivingBuffer.ackBuffer[iii] == 0) {
									// FORWARD THE PACKET
									bytesSent = sendto(socketToServer, &receivingBuffer.packetArray[iii].payload.data, datagramFromTroll.payload.size, 0, (struct sockaddr*)&sockaddrToServer, sizeof(struct sockaddr_in));
									if (bytesSent < 0) {
										printf("Error: Unable to forward packet number %d!\n", receivingBuffer.packetArray[iii].payload.SEQ);
									}
									else {
										printf("Packet number %d forwarded!\n", receivingBuffer.packetArray[iii].payload.SEQ);
										receivingBuffer.ackBuffer[iii] = -1;
										receivingBuffer.base = (receivingBuffer.base + 1) % CIRCULAR_BUFFER_SIZE;
									}
								}
								else {
									break;
								}
							}
						}
					}
					else {
						for (iii = receivingBuffer.base; iii < CIRCULAR_BUFFER_SIZE; iii++) {
							if (receivingBuffer.ackBuffer[iii] == 0) {
								// FORWARD THE PACKET
								bytesSent = sendto(socketToServer, &receivingBuffer.packetArray[iii].payload.data, datagramFromTroll.payload.size, 0, (struct sockaddr*)&sockaddrToServer, sizeof(struct sockaddr_in));
								if (bytesSent < 0) {
									printf("Error: Unable to forward packet number %d!\n", receivingBuffer.packetArray[iii].payload.SEQ);
								}
								else {
									receivingBuffer.ackBuffer[iii] = -1;
									receivingBuffer.base = (receivingBuffer.base + 1) % CIRCULAR_BUFFER_SIZE;
								}
							}
							else {
								break;
							}
						}
						for (iii = 0; iii < (receivingBuffer.base + WINDOW_SIZE) % CIRCULAR_BUFFER_SIZE; iii++) {
							if (receivingBuffer.ackBuffer[iii] == 0) {
								// FORWARD THE PACKET
								bytesSent = sendto(socketToServer, &receivingBuffer.packetArray[iii].payload.data, datagramFromTroll.payload.size, 0, (struct sockaddr*)&sockaddrToServer, sizeof(struct sockaddr_in));
								if (bytesSent < 0) {
									printf("Error: Unable to forward packet number %d!\n", receivingBuffer.packetArray[iii].payload.SEQ);
								}
								else {
									receivingBuffer.ackBuffer[iii] = -1;
									receivingBuffer.base = (receivingBuffer.base + 1) % CIRCULAR_BUFFER_SIZE;
								}
							}
							else {
								break;
							}
						}
					}
				}
			}
			// store in the window
//			memcpy(buffer, datagramFromTroll.payload.data, datagramFromTroll.payload.size);
//			bytesSent = sendto(socketToServer, buffer, datagramFromTroll.payload.size, 0, (struct sockaddr*)&sockaddrToServer, sizeof(struct sockaddr_in));
		}
	}

	// close file and socket
	close(socketFromTroll);
	close(socketToServer);
	close(socketToTroll);
	return 0;
}
