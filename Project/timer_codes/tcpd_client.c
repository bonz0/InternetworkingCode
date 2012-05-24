#include "tcpd.h"

int main(void){
    packet1 pkt1;
    timer_client_pkt tcpkt1;
    char in_out[BUFFER_SIZE];
    memset(in_out,0,BUFFER_SIZE);
//    struct sockaddr_in name1,name2;
 
    fd_set rfds;

    int sock_ftpc,sock_troll,sock_ti,sock_to,bytes_send_troll=0,bytes_recv_ftpc=0,bytes_send_timer=0,bytes_recv_timer=0,seqno=0,timeout_pkt_seqno;
    socklen_t addr_len;
    struct sockaddr_in ftpc_addr,ti_addr,to_addr,troll_addr;
    memset((void *)&ftpc_addr,0,sizeof(struct sockaddr_in));
    memset((void *)&ti_addr,0,sizeof(struct sockaddr_in));
    memset((void *)&to_addr,0,sizeof(struct sockaddr_in));
    memset((void *)&troll_addr,0,sizeof(struct sockaddr_in));

    /*create sockets and bind them to ports 
    sock_ftpc for communication between ftpc and tcpd_client processes and 
    sock_troll for communication between tcpd_client and troll processes
    */
    //first socket
    sock_ftpc = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_ftpc < 0) {
    printf("error opening ftpc socket\n");
    exit(1);
    }
    printf("ftpc socket created\n");
    ftpc_addr.sin_family = AF_INET;
    ftpc_addr.sin_port = htons(FTOP);
    ftpc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_len=sizeof(struct sockaddr_in);
    if(bind(sock_ftpc, (struct sockaddr *)&ftpc_addr, sizeof(ftpc_addr)) < 0) {
    printf("error binding ftpc_addr to sock_ftpc\n");
    exit(2);
    }
    printf("ftpc socket bound\n");
    printf("tcpd_client waiting on port # %d for connection with ftpc...\n",FTOP);
    //second socket
    sock_troll = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_troll < 0) {
    printf("error opening troll socket\n");
    exit(1);
    }
    printf("troll socket created\n");
    troll_addr.sin_family=AF_INET;
    troll_addr.sin_port=htons(TRIP);
    printf("sending to troll via port:%d\n",htons(TRIP));
    troll_addr.sin_addr.s_addr=inet_addr(LOCALHOST);
    //third socket
    sock_ti = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_ti < 0) {
    printf("error opening timer input socket\n");
    exit(1);
    }
    printf("timer input socket created\n");    
    ti_addr.sin_family=AF_INET;
    ti_addr.sin_port=htons(TIIP);
    printf("sending to timer via port:%d\n",htons(TIIP));
    ti_addr.sin_addr.s_addr=inet_addr(LOCALHOST);
    //fourth socket
    sock_to = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_to < 0) {
    printf("error opening timer output socket\n");
    exit(1);
    }
    printf("timer output socket created\n");    
    to_addr.sin_family=AF_INET;
    to_addr.sin_port=htons(TIOP);
    printf("receiving from timer via port:%d\n",htons(TIOP));
    to_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(sock_to, (struct sockaddr *)&to_addr, sizeof(to_addr)) < 0) {
    printf("error binding to_addr to sock_to\n");
    exit(2);
    }
    printf("timer output socket bound\n");
    printf("tcpd_client waiting on port # %d for connection with timer...\n",TIOP);	

    FD_ZERO(&rfds);
    FD_SET(sock_ftpc, &rfds);
    FD_SET(sock_to, &rfds);
    
    while(1){
	if (select(sock_to+1, &rfds, NULL, NULL, NULL) < 0 ){
	printf("Error: select returned -1");
	exit(2);
}	
        if (FD_ISSET(sock_ftpc, &rfds)){
        //receive data from ftpc
	bytes_recv_ftpc = recvfrom(sock_ftpc, in_out, sizeof(in_out), 0, (struct sockaddr*)&ftpc_addr, &addr_len);
        deserializeStruct(&pkt1.pyld,in_out);
        pkt1.addr.sin_family = htons(AF_INET);
    	pkt1.addr.sin_addr.s_addr = inet_addr(pkt1.pyld.rip);
	pkt1.addr.sin_port = htons(TROP);
    	printf("troll will send data to tcpd_server via port:%d\n",htons(TROP));
	if((bytes_send_troll=sendto(sock_troll,&pkt1,sizeof(packet1),0,(struct sockaddr *)&troll_addr,sizeof(troll_addr)))<0)
	printf("error sending data to troll process\n");
        tcpkt1.seqnum=seqno;
	tcpkt1.timeout=TIMEOUT;
	seqno++;
        if((bytes_send_timer=sendto(sock_ti,&tcpkt1,sizeof(timer_client_pkt),0,(struct sockaddr *)&ti_addr,sizeof(ti_addr)))<0)
	printf("error sending data to troll process\n");
}
        if (FD_ISSET(sock_to,&rfds)){
	// receive data from timer
        bytes_recv_timer = recvfrom(sock_to,&timeout_pkt_seqno,sizeof(timeout_pkt_seqno),0,(struct sockaddr*)&to_addr,&addr_len);
        printf ("\nTimeout occured for node :%d\n",timeout_pkt_seqno);
}
	FD_ZERO(&rfds);
    	FD_SET(sock_ftpc, &rfds);
    	FD_SET(sock_to, &rfds); 
}
}
/*
    if (BYPASS_TROLL){
    //bypass troll
    //receive,deserialize and forward only the data part of the packet  
    while((bytes_recv = recvfrom(sock_ftpc, in_out, sizeof(in_out), 0, (struct sockaddr*)&ftpc_addr, &addr_len)) > 0){
    deserializeStruct(&pkt1.pyld,in_out);
    troll_addr.sin_family=AF_INET;
    troll_addr.sin_port=htons(TRIP);
    troll_addr.sin_addr.s_addr=inet_addr(pkt1.pyld.rip);
    printf("BYPASSING TROLL:tcpd_client will send data to tcpd_server with IP Address:%s @ port %d\n",pkt1.pyld.rip,TRIP);
    if((bytes_send=sendto(sock_troll,in_out,sizeof(in_out),0,(struct sockaddr *)&troll_addr,sizeof(troll_addr)))<0)
    printf("error sending data to tcpd_server process\n");
    }
    }
    else
    {
    //send via troll
    while((bytes_recv=recvfrom(sock_ftpc,in_out,sizeof(in_out),0,(struct sockaddr *)&ftpc_addr,&addr_len))>0){
    deserializeStruct(&pkt1.pyld,in_out);
    pkt1.addr.sin_family = htons(AF_INET);
    pkt1.addr.sin_addr.s_addr = inet_addr(pkt1.pyld.rip);
    pkt1.addr.sin_port = htons(TROP);
    printf("troll will send data to tcpd_server via port:%d\n",htons(TROP));
    if((bytes_send=sendto(sock_troll,&pkt1,sizeof(packet1),0,(struct sockaddr *)&troll_addr,sizeof(troll_addr)))<0)
    printf("error sending data to troll process\n");
    }
    }

    close(sock_ftpc);
    close(sock_troll);
    return 0;
*/
