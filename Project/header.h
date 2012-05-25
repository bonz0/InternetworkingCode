#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <arpa/inet.h>

#define LOCALHOST "127.0.0.1"
#define TCPD_CLIENT_PORT 7777
#define TCPD_CLIENT_ACK_PORT 4444
#define TCPD_SERVER_PORT 8888
#define TIMER_LISTEN_PORT 6666
#define TIMER_TO_TCPD_PORT 5555
#define TROLL_PORT 9999
#define SERVER_TROLL_PORT 3333
#define BUFFER_SIZE 900
#define WINDOW_SIZE 20
#define CIRCULAR_BUFFER_SIZE 64

struct mainData {
	int SEQ;
	int ACK;
	int FIN;
	int ACKSEQ;
	int FINACK;
	int size;
	int CLOSE;
	int sent;
	char data[900];
};

struct messageToTroll {
	struct sockaddr_in trollHeader;
	struct mainData payload;
//	char payload[BUFFER_SIZE];
	unsigned long checksum;
};

typedef struct timerPacket {
	int SEQ;
	double timeout;
} timerPacket;

typedef struct {
	int base;
	int head;
	struct messageToTroll packetArray[CIRCULAR_BUFFER_SIZE];
	int ackBuffer[CIRCULAR_BUFFER_SIZE];
} CircularBuffer;

long getFileSize (char* fileName);		// returns the file size
int SEND(int, const void*, int, unsigned int, const struct sockaddr*, socklen_t);
int RECV(int, void *, int, unsigned int, struct sockaddr*, int*);
int windowWrappedAround (int);			// returns true if window has wrapped around the buffer
int inWindow (int, int);			// returns true if index is inside the window
void printWindow (int[]);			
double getRTO(double, int);
