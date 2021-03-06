/******TCPD_CLIENT******/

#include "header.h"

int main(int argc, char *argv[])
{
	int socketFromClient, socketToTroll, socketFromTroll, socketFromTimer, socketToTimer;
	struct sockaddr_in sockaddrFromClient, sockaddrToTroll, sockaddrFromTroll, sockaddrToTimer, sockaddrFromTimer;
	struct messageToTroll datagramToTroll, ackFromTroll;
	timerPacket packetToTimer;
	int bytesSentToTroll, bytesSentToTimer, bytesReceivedFromTroll, bytesReceivedFromClient, bytesReceivedFromTimer, totalbytesReceivedFromClient = 0, selectReturnValue, packetCount = -1, packetToResend = -1; 
	char buffer[BUFFER_SIZE], fileName[20], fileSize[10], serverIPAddress[15], serverPort[5], hostIpAddress[15], hostName[20];
	unsigned long receivedChecksum;
	struct hostent *he;
	struct in_addr **addr_list;
	struct timeval start, end;
	fd_set socketReadSet, socketWriteSet;
	int iii, rttSequenceNumber = 0;
	double difference = 0.0, currentTimeout = 100000.0;
	double* estimatedRtt, *devRtt;
	CircularBuffer sendingCircularBuffer;
	int rtoBeingCalculated = 0;
	int serverPortSent = 0;
	sendingCircularBuffer.head = 0;
	sendingCircularBuffer.base = 0;
	for (iii = 0; iii < CIRCULAR_BUFFER_SIZE; iii++) {
		sendingCircularBuffer.ackBuffer[iii] = -1;
	}
	estimatedRtt = (double*)malloc(sizeof(double));
	devRtt = (double*)malloc(sizeof(double));
	*estimatedRtt = 20000.0;
	*devRtt = 20000.0;

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

	gethostname(hostName, sizeof(hostName));
	if ((he = gethostbyname(hostName)) == NULL) {
		printf("Error: Unable to get host IP address!\n");
		exit(0);
	}
	addr_list = (struct in_addr **)he->h_addr_list;
	strcpy(hostIpAddress, inet_ntoa(*addr_list[0]));

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
	datagramToTroll.trollHeader.sin_family = htons(AF_INET);
	datagramToTroll.trollHeader.sin_addr.s_addr = (inet_addr(serverIPAddress));
	datagramToTroll.trollHeader.sin_port = htons(TCPD_SERVER_PORT);

	memset(datagramToTroll.payload.data, 0, sizeof(datagramToTroll.payload.data));
	strcpy(datagramToTroll.payload.data, hostIpAddress);
	datagramToTroll.payload.size = sizeof(hostIpAddress);
	datagramToTroll.payload.SEQ = -2;
	datagramToTroll.checksum = crc((void*)&datagramToTroll.payload, sizeof(datagramToTroll.payload), 0);

	// send host IP address to TROLL
	bytesSentToTroll = sendto(socketToTroll, (void*)&datagramToTroll, sizeof(datagramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
	if (bytesSentToTroll < 0) {
		perror("Error: Unable to send host IP address to troll!\n");
		exit(-6);
	}
	packetToTimer.SEQ = -2;
	packetToTimer.timeout = 60000.0;
	bytesSentToTimer = sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
	while (1) {
		FD_ZERO(&socketReadSet);
		FD_SET(socketFromTroll, &socketReadSet);
		FD_SET(socketFromTimer, &socketReadSet);

		selectReturnValue = select(socketFromTroll + 1, &socketReadSet, NULL, NULL, NULL);
		if (selectReturnValue < 0) {
			printf("Error: Select 1 returned a negative value!\n");
			exit(0);
		}
		else if (selectReturnValue == 0) {
		
		}
		else {
			if (FD_ISSET(socketFromTroll, &socketReadSet)) {
				printf("ACK for host IP address received from troll\n");
				recvfrom(socketFromTroll, (void*)&ackFromTroll, sizeof(ackFromTroll), 0, (struct sockaddr*)&sockaddrFromTroll, &fromLen);
				packetToTimer.SEQ = -2;
				packetToTimer.timeout = 0.0;
				sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
				break;
			}
			if (FD_ISSET(socketFromTimer, &socketReadSet)) {
				bytesReceivedFromTimer = recvfrom(socketFromTimer, &packetToResend, sizeof(int), 0, (struct sockaddr*)&sockaddrFromTimer, &fromLen);
				if (bytesReceivedFromTimer > 0) {
					printf("Timeout occured for host IP address!\n");
				}
				packetToTimer.SEQ = -2;
				packetToTimer.timeout = 60000.0;
				sendto(socketToTroll, (void*)&datagramToTroll, sizeof(datagramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
				sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
			}
		}
	}
	printf("Host IP Address sent to server!\n");
	memset(&datagramToTroll.payload, 0, sizeof(datagramToTroll.payload));
	
	// send server port to troll
	strcpy(datagramToTroll.payload.data, serverPort);
	datagramToTroll.payload.size = sizeof(serverPort);
	datagramToTroll.payload.SEQ = -1;
	datagramToTroll.checksum = crc((void *)&datagramToTroll.payload, sizeof(datagramToTroll.payload), 0);
	
	sendto(socketToTroll, (void*)&datagramToTroll, sizeof(datagramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
	packetToTimer.SEQ = -1;
	packetToTimer.timeout = 60000.0;
	bytesSentToTimer = sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
	while (1) {
		FD_ZERO(&socketReadSet);
		FD_SET(socketFromTroll, &socketReadSet);
		FD_SET(socketFromTimer, &socketReadSet);

		selectReturnValue = select(socketFromTroll + 1, &socketReadSet, NULL, NULL, NULL);
		if (selectReturnValue < 0) {
			printf("Error: Select 1 returned a negative value!\n");
			exit(0);
		}
		else if (selectReturnValue == 0) {
			
		}
		else {
			if (FD_ISSET(socketFromTroll, &socketReadSet)) {
				printf("ACK Received from troll\n");
				recvfrom(socketFromTroll, (void*)&ackFromTroll, sizeof(ackFromTroll), 0, (struct sockaddr*)&sockaddrFromTroll, &fromLen);
				receivedChecksum = crc((void*)&ackFromTroll.payload, sizeof(ackFromTroll.payload), 0);
				if (receivedChecksum == ackFromTroll.checksum) {
					if (ackFromTroll.payload.SEQ == -1) {
						packetToTimer.SEQ = -1;
						packetToTimer.timeout = 0.0;
						sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
						break;
					}
				}
			}
			if (FD_ISSET(socketFromTimer, &socketReadSet)) {
				bytesReceivedFromTimer = recvfrom(socketFromTimer, &packetToResend, sizeof(int), 0, (struct sockaddr*)&sockaddrFromTimer, &fromLen);
				if (bytesReceivedFromTimer > 0) {
					printf("Timeout occured for server port!\n");
				}
				packetToTimer.SEQ = -1;
				packetToTimer.timeout = 60000.0;
				sendto(socketToTroll, (void*)&datagramToTroll, sizeof(datagramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
				sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
			}
		}
	}
	printf("Server port sent to server\n");
	
/*	printf("SEQ = %d\nACK = %d\n", ackFromTroll.payload.SEQ, ackFromTroll.payload.ACK);
	packetToTimer.SEQ = ackFromTroll.payload.SEQ;
	packetToTimer.timeout = 0.0;
*/

	// send notification to timer to delete node
//	bytesSentToTimer = sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
	
	// Receive file
	printf("Sending file to server side!\n");
	packetCount = -1;	
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
		memset(&datagramToTroll.payload, 0, sizeof(datagramToTroll.payload));
		if (FD_ISSET(socketFromClient, &socketReadSet)) {
			bytesReceivedFromClient = recvfrom(socketFromClient, buffer, sizeof(buffer), 0, (struct sockaddr*)&sockaddrFromClient, &fromLen);
			if (bytesReceivedFromClient > 0) {
				packetCount++;
				
				// prepare data of the packet to be sent
				datagramToTroll.payload.SEQ = packetCount % CIRCULAR_BUFFER_SIZE;
				sendingCircularBuffer.head = packetCount % CIRCULAR_BUFFER_SIZE;
				printf("Packet number: %d received from client\n", packetCount);
				datagramToTroll.payload.ACK = 0;
				datagramToTroll.payload.FIN = 0;
				datagramToTroll.payload.size = bytesReceivedFromClient;
				printf("Value of head before packet is stored in circular buffer: %d\n", sendingCircularBuffer.head);
				sendingCircularBuffer.ackBuffer[sendingCircularBuffer.head] = 0;
				memcpy(datagramToTroll.payload.data, buffer, bytesReceivedFromClient);
				datagramToTroll.checksum = crc((void *)&datagramToTroll.payload, sizeof(datagramToTroll.payload), 0);
				// store packet in circular buffer
				printf("Head = %d\n", sendingCircularBuffer.head);
				sendingCircularBuffer.packetArray[sendingCircularBuffer.head] = datagramToTroll;

				if (inWindow(sendingCircularBuffer.head, sendingCircularBuffer.base)) {
					printf("Head in window!\n");
					// set up details of packet to be sent to the timer
					packetToTimer.SEQ = packetCount % CIRCULAR_BUFFER_SIZE;
					packetToTimer.timeout = currentTimeout;
					printf("Timeout value = %f", currentTimeout, packetToTimer.SEQ);
					// send the packet to the troll
					bytesSentToTroll = sendto(socketToTroll, (void*)&sendingCircularBuffer.packetArray[sendingCircularBuffer.head], sizeof(datagramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
					if (bytesSentToTroll < 0) {
						printf("Error: Unable to send packet %d to troll!\n", sendingCircularBuffer.head);
					}
					else {
//						if (!rtoBeingCalculated) {
							gettimeofday(&start, NULL);
//							rtoBeingCalculated = 1;
//							rttSequenceNumber = sendingCircularBuffer.head;
//						}
						printf("Packet %d sent to troll!\n", sendingCircularBuffer.head);
						// mark packet as sent
						sendingCircularBuffer.ackBuffer[sendingCircularBuffer.head] = 1;
						printWindow(sendingCircularBuffer.ackBuffer);
						sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
						printf("Head: %d\n", sendingCircularBuffer.head);
						printf("Base: %d\n", sendingCircularBuffer.base);
					}
				}
			}

		}

		// if timeout occured and data received from the timer
		if (FD_ISSET(socketFromTimer, &socketReadSet)) {
			bytesReceivedFromTimer = recvfrom(socketFromTimer, &packetToResend, sizeof(int), 0, (struct sockaddr*)&sockaddrFromTimer, &fromLen);
			printf("Timeout occured for packet number %d!\n", packetToResend);
			bytesSentToTroll = sendto(socketToTroll, (void*)&sendingCircularBuffer.packetArray[packetToResend], sizeof(datagramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
			if (bytesSentToTroll < 0) {
				printf("Error: Unable to RESEND packet %d to troll!\n", packetToResend);
			}
			else {
//				if (!rtoBeingCalculated) {
					gettimeofday(&start, NULL);
//					rtoBeingCalculated = 1;
//					rttSequenceNumber = packetToResend;
//				}
				sendingCircularBuffer.ackBuffer[packetToResend] = 1;
				printf("Packet %d RESENT to troll!\n", packetToResend);
				packetToTimer.SEQ = packetToResend;
				packetToTimer.timeout = currentTimeout;
				printf("Timeout value: %f\n", packetToTimer.timeout);
				sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
			}
		}

		// if acknowledgement received from the troll
		if (FD_ISSET(socketFromTroll, &socketReadSet)) {
			memset(&ackFromTroll.payload, 0, sizeof(ackFromTroll.payload));
			recvfrom(socketFromTroll, (void*)&ackFromTroll, sizeof(ackFromTroll), 0, (struct sockaddr*)&sockaddrFromTroll, &fromLen);
			receivedChecksum = crc((void*)&ackFromTroll.payload, sizeof(ackFromTroll.payload), 0);
//			gettimeofday(&end, NULL);
//			difference = timeval_diff(&end, &start);
			if (receivedChecksum != ackFromTroll.checksum) {
				printf("Error: Error in received acknowledgement!\n");
			}
			else {
				if (ackFromTroll.payload.SEQ == -1) {
					continue;
				}
//				if ((ackFromTroll.payload.SEQ == rttSequenceNumber) && rtoBeingCalculated) {
					gettimeofday(&end, NULL);
					difference = timeval_diff(&end, &start);
					currentTimeout = getRTO(difference, estimatedRtt, devRtt);
//					rtoBeingCalculated = 0;
					printf("\n\n\nRTT VALUE for %d = %f\nTimeout value = %f\n\n", ackFromTroll.payload.SEQ, difference, currentTimeout);
//				}
				printf("ACK received for SEQ = %d\n", ackFromTroll.payload.SEQ);
				packetToTimer.SEQ = ackFromTroll.payload.SEQ;
				packetToTimer.timeout = 0.0;
				// send notification to timer to delete node
				sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
				printf("Before updating base:\n");
				printWindow(sendingCircularBuffer.ackBuffer);
				if (inWindow(ackFromTroll.payload.SEQ, sendingCircularBuffer.base)) {
					sendingCircularBuffer.ackBuffer[ackFromTroll.payload.SEQ] = 2;
				}
				printWindow(sendingCircularBuffer.ackBuffer);
				while (sendingCircularBuffer.ackBuffer[sendingCircularBuffer.base] == 2) {		// move the window forward over the circular buffer
					sendingCircularBuffer.ackBuffer[sendingCircularBuffer.base] = -1;
					sendingCircularBuffer.base = (sendingCircularBuffer.base + 1) % CIRCULAR_BUFFER_SIZE;
				}
				printf("After updating base:\n");
				printWindow(sendingCircularBuffer.ackBuffer);
				iii = sendingCircularBuffer.base;
				printf("Base: %d\n", sendingCircularBuffer.base);
				if (!windowWrappedAround(sendingCircularBuffer.base)) {							// check if window has wrapped around
					for (iii = sendingCircularBuffer.base; iii < (sendingCircularBuffer.base + WINDOW_SIZE) % CIRCULAR_BUFFER_SIZE; iii++) {		// send all unsent packets in newly moved window
						if (sendingCircularBuffer.ackBuffer[iii] == 0) {
							packetToTimer.SEQ = sendingCircularBuffer.packetArray[iii].payload.SEQ;
							packetToTimer.timeout = currentTimeout;
							// send the packet to the troll
							bytesSentToTroll = sendto(socketToTroll, (void*)&sendingCircularBuffer.packetArray[iii], sizeof(datagramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
							if (bytesSentToTroll < 0) {
								printf("Error: Unable to send packet %d to troll!\n", sendingCircularBuffer.packetArray[iii].payload.SEQ);
							}
							else {
//								if (!rtoBeingCalculated) {
									gettimeofday(&start, NULL);
//									rtoBeingCalculated = 1;
//									rttSequenceNumber = sendingCircularBuffer.packetArray[iii].payload.SEQ;
//								}
								printf("Packet %d sent to troll due to window movement!\n", sendingCircularBuffer.packetArray[iii].payload.SEQ);
								// mark packet as sent
								sendingCircularBuffer.ackBuffer[iii] = 1;
								// send nofication to timer
								sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
							}
						}
					}
					printf("After sending everything in the window that wasn't sent!\n");
					printWindow(sendingCircularBuffer.ackBuffer);
				}
				else {
					for (iii = sendingCircularBuffer.base; iii < CIRCULAR_BUFFER_SIZE; iii++) {
						if (sendingCircularBuffer.ackBuffer[iii] == 0) {
							packetToTimer.SEQ = sendingCircularBuffer.packetArray[iii].payload.SEQ;
							packetToTimer.timeout = currentTimeout;
							// send the packet to the troll
							bytesSentToTroll = sendto(socketToTroll, (void*)&sendingCircularBuffer.packetArray[iii], sizeof(datagramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
							if (bytesSentToTroll < 0) {
								printf("Error: Unable to send packet %d to troll!\n", sendingCircularBuffer.packetArray[iii].payload.SEQ);
							}
							else {
//								if (!rtoBeingCalculated) {
									gettimeofday(&start, NULL);
//									rtoBeingCalculated = 1;
//									rttSequenceNumber = sendingCircularBuffer.packetArray[iii].payload.SEQ;
//								}
								printf("Packet %d sent to troll due to window movement 1!\n", sendingCircularBuffer.packetArray[iii].payload.SEQ);
								// mark packet as sent
								sendingCircularBuffer.ackBuffer[iii] = 1;
								// send nofication to timer
								sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
							}
						}
					}
					for (iii = 0; iii < (sendingCircularBuffer.base + WINDOW_SIZE) % CIRCULAR_BUFFER_SIZE; iii++) {
						if (sendingCircularBuffer.ackBuffer[iii] == 0) {
							packetToTimer.SEQ = sendingCircularBuffer.packetArray[iii].payload.SEQ;
							packetToTimer.timeout = currentTimeout;
							// send the packet to the troll
							bytesSentToTroll = sendto(socketToTroll, (void*)&sendingCircularBuffer.packetArray[iii], sizeof(datagramToTroll), 0, (struct sockaddr*)&sockaddrToTroll, sizeof(struct sockaddr_in));
							if (bytesSentToTroll < 0) {
								printf("Error: Unable to send packet %d to troll!\n", sendingCircularBuffer.packetArray[iii].payload.SEQ);
							}
							else {
//								if (!rtoBeingCalculated) {
									gettimeofday(&start, NULL);
//									rtoBeingCalculated = 1;
//									rttSequenceNumber = sendingCircularBuffer.packetArray[iii].payload.SEQ;
//								}
								printf("Packet %d sent to troll due to window movement 2!\n", sendingCircularBuffer.packetArray[iii].payload.SEQ);
								// mark packet as sent
								sendingCircularBuffer.ackBuffer[iii] = 1;
								// send nofication to timer
								sendto(socketToTimer, (void*)&packetToTimer, sizeof(packetToTimer), 0, (struct sockaddr*)&sockaddrToTimer, sizeof(struct sockaddr_in));
							}
						}
					}
					printf("After sending everything in the window that wasn't sent!\n");
					printWindow(sendingCircularBuffer.ackBuffer);
				}
			}
		}
	}

	// close file and socket
	close(socketFromClient);
	close(socketToTroll);
	close(socketFromTroll);
	close(socketFromTimer);
	close(socketToTimer);
	return 0;
}
