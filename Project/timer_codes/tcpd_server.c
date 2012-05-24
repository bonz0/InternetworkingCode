#include "tcpd.h"
    
int main(void)
    {
    packet1 pkt1;
    int sock1,sock2,bytes_recv=0,bytes_send=0;
    socklen_t name2len; 
    char in_out[BUFFER_SIZE+sizeof(struct sockaddr_in)];
    char payload_in_out[BUFFER_SIZE];
    memset(in_out,0,BUFFER_SIZE);	
    struct sockaddr_in name1,name2;
    /*create socket*/
    sock1 = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock1 < 0) {
    printf("error opening datagram socket 1\n");
    exit(1);
    }
    printf("socket 1 created\n");
    sock2 = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock2 < 0){
    printf("error opening datagram socket 2\n");
    exit(1);
    }
    printf("socket 2 created\n");
    name2.sin_family = AF_INET;
    if(BYPASS_TROLL){
    //name2.sin_port = htons();
    }
    else{
    name2.sin_port = htons(TROP);
    }
    printf("tcpd_server waiting for data on port %d\n",TROP);
    name2.sin_addr.s_addr = htonl(INADDR_ANY);
    name2len=sizeof(name2);
    if(bind(sock2, (struct sockaddr *)&name2, sizeof(name2)) < 0) {
    printf("error binding name2 to sock2\n");
    exit(2);
    }
    printf("socket 2 bound\n");
    while((bytes_recv=recvfrom(sock2,in_out,sizeof(in_out),0,(struct sockaddr *)&name2,&name2len)) > 0){
    printf("bytes received from tcpd_client:%d\n",bytes_recv);
    memmove(payload_in_out,in_out+sizeof(struct sockaddr_in),BUFFER_SIZE);
    deserializeStruct(&pkt1.pyld,payload_in_out);
    printf("ftps waiting for data @ port %s\n",pkt1.pyld.rport);
    name1.sin_family = AF_INET;
    name1.sin_port = htons(atoi(pkt1.pyld.rport));
    name1.sin_addr.s_addr =inet_addr(LOCALHOST);
    if((bytes_send=sendto(sock1,pkt1.pyld.data,sizeof(pkt1.pyld.data),0,(struct sockaddr *)&name1,sizeof(name1)))<0)
    printf("error sending data to ftps process\n");
    printf("bytes send to ftps:%d\n",bytes_send);
    }
    close(sock1);
    close(sock2);
    return 0;
}
