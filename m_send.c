#define noDEBUG
#define SERIAL "/dev/ttyACM1"
#define HOSTNAME "2a0e:97c0:3ea:4fa::1"
#define PORT 44444
#include "m_functions.c"

void main(int argc,char **argv){
  int fd,fd2,ow,mode;
  float *of;
  uint8_t *os;
  struct sockaddr_in6 servaddr;
  char buf[100];

  mode=atoi(argv[1]);
  fd=open(SERIAL,O_RDWR);
  setserial(fd);
  fd2=socket(AF_INET,SOCK_DGRAM,0);
  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin6_family=AF_INET6;
  net_pton(AF_INET6,HOSTNAME,&(servaddr.sin6_addr));
  servaddr.sin6_port=htons(PORT);

  switch(mode){
    case 1: // 1 v1 v2 v3 i1 i2 i3
      myw(fd,"\x01\x03\x00\x0E",14); 
      of=myr_fn(fd,7);
      if(of==NULL)break;
      *buf=1;
      *((float *)(buf+1))=of[0];
      *((float *)(buf+5))=of[1];
      *((float *)(buf+9))=of[2];
      *((float *)(buf+13))=of[4];
      *((float *)(buf+17))=of[5];
      *((float *)(buf+21))=of[6];
      sendto(fd2,buf,25,0,(struct sockaddr *)&servaddr,sizeof(servaddr));
      break;
    case 2: // 2 E1 E2 E3
      myw(fd,"\x01\x03\x01\x02",6); 
      of=myr_fn(fd,3);
      if(of==NULL)break;
      *buf=2;
      *((float *)(buf+1))=of[0];
      *((float *)(buf+5))=of[1];
      *((float *)(buf+9))=of[2];
      sendto(fd2,buf,13,0,(struct sockaddr *)&servaddr,sizeof(servaddr));
      break;
  }
  close(fd2); 
  close(fd);
}
