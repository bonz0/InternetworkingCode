/* 
This code implements the client side of the file transfer protocol.
USAGE: ./ftpc <remote-IP-address> <remote-port-number> <local-file-to-transfer>
Below are the details of the implementation:
1. create socket
2. send connection request to the server.
3. open the file to be transferred.
4. compute the file size and send it to the server. The file size is computed by calling flength function
5. send the file name 
6. keep transferring the data till the number of bytes read from the file drops to zero.
7. close the file and close the sockets.
*/
#include "tcpd.h"
long flength(char *fname)
{
          FILE *fptr;
          long length = 0L;
     
          fptr = fopen(fname, "r");
          if(fptr != NULL)
          {
                fseek(fptr, 0L, SEEK_END);
                length = ftell(fptr);
                fclose(fptr);
          }
          return length;
}

int main(int argc, char* argv[]) {
    packet1 *pkt1=(packet1 *)calloc(1,sizeof(packet1));
    int client_s;
    int bytes_send,bytes_read;
    char file_size[FILE_SIZE];
    memset(file_size,0,FILE_SIZE);
    char *colon=":";
    FILE *fp;
    if (argc < 4){
    printf("USAGE: ./ftpc <remote-ip-address> <remote-port-num> <local-file-to-transfer>\n");
    exit(1);
    }
    strcpy(pkt1->pyld.rip,argv[1]);
    strcpy(pkt1->pyld.rport,argv[2]);
    pkt1->addr.sin_family=AF_INET;
    pkt1->addr.sin_port=htons(FTOP);
    pkt1->addr.sin_addr.s_addr=inet_addr(LOCALHOST);
    //compute and print the file size by using flength function
    sprintf(file_size,"%ld",flength(argv[3]));
    printf("file size:%s\n",file_size);
    //copy the file size and file name in the data portion of the pyld.
    strcpy(pkt1->pyld.data,file_size);
    strcat(pkt1->pyld.data,colon);
    strcat(pkt1->pyld.data,argv[3]);
    //open the file for reading
    fp=fopen(argv[3],"r");
    if (fp < 0){
    printf("unable to open file\n");
    exit(1);
    }
    //create socket
    client_s=socket(AF_INET,SOCK_DGRAM,0);
    if(client_s < 0){
    printf("error opening datagram socket\n");
    //printf("SOCKET CREATION FAILED\n");
    exit(1);
    }
    bytes_send=SEND(client_s, pkt1, sizeof(packet1), 0);
    if (bytes_send < 0){
    printf("error sending the file size and file name to the tcpd_client process\n");
    exit(1);
    }
    printf("file size and file name successfully send to the tcpd_client process\n");     
    usleep(10000);		// sleep for 10ms 
    bytes_read=fread(pkt1->pyld.data,sizeof(char),sizeof(pkt1->pyld.data),fp);
    while(bytes_read > 0){
    bytes_send=SEND(client_s,pkt1,sizeof(packet1),0);
    usleep(10000);		//sleep for 10ms
    bytes_read=fread(pkt1->pyld.data,sizeof(char),sizeof(pkt1->pyld.data),fp);
    }
    printf("file successfully transmitted\n");
    fclose(fp);
    if(CLOSE(client_s)<0){
    printf("error closing socket\n");
    }
    return 0;
}
