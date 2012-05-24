#include "tcpd.h"

void serializeStruct (payload* pkt, char *buffer) {
    int index;
    for (index = 0; index < RPORT_SIZE; index++) {
    *(buffer+index) = pkt->rport[index];
    }
    for (index = 0; index < RIP_SIZE; index++) {
    *(buffer+RPORT_SIZE+index) = pkt->rip[index];
    }
    for (index = 0; index < DATA_SIZE; index++) {
    *(buffer+RIP_SIZE+RPORT_SIZE+index) = pkt->data[index];
    }
    }

void deserializeStruct (payload* pkt, char buffer[]){
    int index;
    for (index = 0; index < RPORT_SIZE; index++) {
    pkt->rport[index]=buffer[index];
    }
    for (index = 0; index < RIP_SIZE; index++) {
    pkt->rip[index]=buffer[index+RPORT_SIZE];
    }
    for (index = 0; index < DATA_SIZE; index++) {
    pkt->data[index]=buffer[RIP_SIZE+RPORT_SIZE+index];
    }
    }

int SEND(int sockfd, packet1 *msg, int len, int flags){
    int bytes_send;
    char out[BUFFER_SIZE];
    memset(out,0,BUFFER_SIZE);
    //serialize and send the data
    serializeStruct(&(msg->pyld),out);
    bytes_send=sendto(sockfd,out,sizeof(out),0,(struct sockaddr*) &(msg->addr),sizeof(msg->addr));
    return bytes_send;
    }

ssize_t RECV(int sockfd, packet1 *msg, size_t len, int flags){
    ssize_t bytes_recv;
    socklen_t addrlen;
    addrlen=sizeof(msg->addr);
    //receive and deserialize the data
    bytes_recv=recvfrom(sockfd,msg->pyld.data,sizeof(msg->pyld.data),0,(struct sockaddr*) &(msg->addr),&addrlen);
    printf("bytes received inside RECV:%d\n",bytes_recv);
    return bytes_recv;
    }

int CLOSE(int s){
    //implement TCP CONNECTION SHUTDOWN HERE
    if(close(s) < 0)
    return -1;
    else
    return 0;
}
