/*
This code implements the server side of the file transfer protocol.
USAGE: ./ftps <local-port-number>
Below are the details of the implementation:
1. create socket. 
2. bind socket to a port number.
3. receive the file size.
4. receive the file name.
5. open a new file with the recived file name to write the data coming from the client.
6. keep writing to the file till the number of bytes received becomes zero.
7. compare the number of bytes received to the file size to ensure complete file transfer.
8. close the file and close the sockets.    
*/
#include "tcpd.h"
int main(int argc,char *argv[]){
    packet1 *pkt1=(packet1 *)calloc(1,sizeof(packet1));
    int server_s;
    int bytes_recv=0,total_bytes_recv=0,bytes_wrote=0,index1,index2;
    char file_name[FILE_NAME_SIZE],file_size[FILE_SIZE];
    memset(file_size,0,FILE_SIZE);
    memset(file_name,0,FILE_NAME_SIZE);
    char full_file_name[25]="../";
    FILE *fp;
    if (argc != 2){
        printf("USAGE: ftps <local-port-number>\n");
        exit(1);    
    }
    if((server_s = socket(AF_INET, SOCK_DGRAM,0)) < 0){
    printf("error opening datagram socket\n");
    exit(1);
    }
    printf("socket created\n");
    pkt1->addr.sin_family = AF_INET;
    pkt1->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    pkt1->addr.sin_port = htons(atoi(argv[1]));
    if(bind(server_s, (struct sockaddr *)&(pkt1->addr), sizeof(pkt1->addr)) < 0) {
    printf("error binding socket\n");
    exit(1);
    }
    printf("socket bound\n");
    bytes_recv=RECV(server_s,pkt1,sizeof(packet1),0);
    if(bytes_recv < 0){
    printf("error reading on socket\n");
    exit(1);
    }
    //get file_size
    //get file_name
    index1=0;
    while(pkt1->pyld.data[index1]!=':'){
    file_size[index1]=pkt1->pyld.data[index1];
    index1++;
    }
    index1++;
    for(index2=0;index2<FILE_NAME_SIZE;index2++){
    file_name[index2]=pkt1->pyld.data[index1+index2];
    }
    printf("incoming file size:%d\n",atoi(file_size));
    printf("incoming file name:%s\n",file_name);
    fp=fopen(strcat(full_file_name,file_name),"w");
    if (fp < 0){
    printf("unable to open new file\n");
    exit(1);
    }
    printf("new file opened\n");
    total_bytes_recv=0;
    while((bytes_recv=RECV(server_s,pkt1,sizeof(packet1),0)) > 0){
    total_bytes_recv+=bytes_recv; 
    bytes_wrote=fwrite(pkt1->pyld.data,sizeof(char),bytes_recv,fp);
    if(total_bytes_recv>=atoi(file_size))
    break;
    }
    if(total_bytes_recv<atoi(file_size)){
    printf("failed to receive complete file\n");
    exit(1);
    }
    printf("file successfully received\n");
    fclose(fp);
    CLOSE(server_s);
    return 0;
}
