#include "timer.h"

int main(void) {
	socklen_t addr_len;
	timerPacket tcpkt;		// timer client packet 
	struct timeval timeOutForSelectFunction, startTimeForSelect, endTimeForSelect;
	fd_set socketSet;
	int numberOfBytesReceived;
  
	int selectReturnValue;
  
	unsigned int socketToReceiveFromTcpdClient;		
	struct sockaddr_in timer_addr; 
	
	tlist *list;
	tnode *tnode_ptr;
	list = create_tlist();

	timer_addr.sin_family = AF_INET;
	timer_addr.sin_port = htons(TIMER_PORT);
	timer_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_len=sizeof(struct sockaddr_in);
	socketToReceiveFromTcpdClient = socket(AF_INET, SOCK_DGRAM, 0); 
	if(socketToReceiveFromTcpdClient <0) {
		printf("ERROR: socket creation failed\n");
		exit(2);
	}
	printf("Socket to communicate with TCPD_CLIENT created\n");
	
	if(bind(socketToReceiveFromTcpdClient,(struct sockaddr *)&timer_addr,sizeof(timer_addr)) < 0) {
		printf("ERROR: socket binding failed\n");
		exit(2);
	}
	printf("Socket to communicate with TCPD_CLIENT bound\n");  

	while (1) {
		timeOutForSelectFunction.tv_sec = 0;
		timeOutForSelectFunction.tv_usec = DECREMENT_VALUE;		
		FD_ZERO(&socketSet);
		FD_SET(socketToReceiveFromTcpdClient, &socketSet);
		gettimeofday(&startTimeForSelect, NULL);
		selectReturnValue = select(socketToReceiveFromTcpdClient + 1, &socketSet, NULL, NULL, &timeOutForSelectFunction);
		printf("Select returned: %d\n", selectReturnValue);
		if (selectReturnValue == -1) {
			printf("ERROR: select returned -1\n");
		}
		else if (selectReturnValue == 0) {
			printf("Calling tlist_clock when selectReturnValue: 0\n");
			tlist_clock(list, (double) DECREMENT_VALUE);
		}
		else {
			usleep(10000);		//sleep for 10000 micro seconds
			printf("Waiting for data from tcpd client\n");
			numberOfBytesReceived = recvfrom(socketToReceiveFromTcpdClient, &tcpkt, sizeof(tcpkt), 0, (struct sockaddr*)&timer_addr, &addr_len);
			if(numberOfBytesReceived < 0) {
				printf("No data received\n");
			}
			tnode_ptr = create_tnode(tcpkt.seqnum, tcpkt.timeout);
			if (tcpkt.timeout == 0.0) {
				printf("ACK RECEIVED FOR %d\n", tcpkt.seqnum);
				int removeValue = removeFromtlist(list, tnode_ptr, 1);
				if (removeValue == 0) {
					printf("Node not found!\n");
				}
				else if (removeValue == 1) {
					printf("Node deleted!\n");
					printf("Number of nodes in list: %d\n", list->numtnodes);
				}
			}
			/* 
			 * if(numberOfBytesReceived = sizeof(int)) {
			 *	acknowledgement received
			 *	remove node from the list
			 *  }
			 *  else if(numberOfBytesReceived = sizeof(timer_client_pkt)) {
			 *  	new packet has been sent
			 *  	add a new node to the list
			 */
			else {
//				tnode_ptr = create_tnode(tcpkt.seqnum,tcpkt.timeout);	
				if (addTotlist(list,tnode_ptr) < 1) {
				    printf("ERROR: Addition of tnode to delta list failed\n");
				    exit(1);
				}
				gettimeofday(&endTimeForSelect, NULL);
				tlist_clock(list, (double)((endTimeForSelect.tv_sec * 1000000) + endTimeForSelect.tv_usec) - ((startTimeForSelect.tv_sec*1000000) + startTimeForSelect.tv_usec));
			}       
		}
	}
	return 0;
}
