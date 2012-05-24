#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#define TIIP 8447	// TImer Input Port 
#define TIOP 8467	// TImer Output Port
#define FTIP 3847	// FTpc Input Port
#define FTOP 3867	// FTpc Output Port
#define TRIP 8747	// TRoll Input Port
#define TROP 8767	// TRoll Output Port
//#define PORT1	5678	//for communication between ftpc and tcpd_client
//#define PORT2	6789	//for communication between tcpd_client and troll
//#define PORT3	7890	//for communication between troll and tcpd_server
#define LOCALHOST	"127.0.0.1"
#define RIP_SIZE	20
#define RPORT_SIZE	10
#define DATA_SIZE	970
#define BUFFER_SIZE 1000
#define BYPASS_TROLL	0
#define FILE_SIZE	10
#define FILE_NAME_SIZE	20
#define TIMEOUT 1000000
    
    typedef struct {
    char rport[RPORT_SIZE];
    char rip[RIP_SIZE];
    char data[DATA_SIZE];
}payload;
    
    typedef struct{
    struct sockaddr_in addr;
    payload pyld;
}packet1;

    typedef struct{
    int seqnum;
    double timeout;
}timer_client_pkt;

    int SEND(int sockfd, packet1 *msg, int len, int flags);

    ssize_t RECV(int sockfd, packet1 *msg, size_t len, int flags);

    int CLOSE(int s);

    void serializeStruct (payload* pkt, char buffer[]);

    void deserializeStruct (payload* pkt, char buffer[]);
