#define DEBUG
#define SERIAL "/dev/ttyACM1"
#define HOSTNAME "64.112.124.143"
#define PORT 44444
#include "m_functions.c"

void main(int argc,char **argv){
  int fd,fd2,ow,mode;
  float *of;
  uint8_t *os;
  struct sockaddr_in servaddr;
  char buf[100];

  mode=atoi(argv[1]);
  fd=open(SERIAL,O_RDWR);
  setserial(fd);
  fd2=socket(AF_INET,SOCK_DGRAM,0);
  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  servaddr.sin_addr.s_addr=inet_addr(HOSTNAME);
  servaddr.sin_port=htons(PORT);

  switch(mode){
    case 1:
      myw(fd,"\x01\x03\x00\x0E",14); 
      of=myr_fn(fd,7);
      if(of==NULL)break;
      *buf=1;
      *((float *)(buf+1))=of[0];
      *((float *)(buf+5))=of[1];
          *((float *)(buf+1))=of[0];

          *((float *)(buf+1))=of[0];

          *((float *)(buf+1))=of[0];

          *((float *)(buf+1))=of[0];

   
  }

    
  close(fd);
}
