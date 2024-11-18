#define noDEBUG
#define SERIAL "/dev/ttyACM1"
#define HOSTNAME "64.112.124.143"
#define PORT 44444
#include "m_functions.c"
void main(){
  int fd,fd2;
  char buf[12];
  struct sockaddr_in servaddr;
  fd=open(SERIAL,O_RDWR);
  setserial(fd);
  fd2=socket(AF_INET,SOCK_DGRAM,0);
  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  servaddr.sin_addr.s_addr=inet_addr(HOSTNAME);
  servaddr.sin_port=htons(PORT);
  myw(fd,"\x01\x03\x01\x02",2);
  *((float *)buf)=myr_f(fd);
  myw(fd,"\x01\x03\x01\x04",2);
  *((float *)(buf+4))=myr_f(fd);
  myw(fd,"\x01\x03\x01\x06",2);
  *((float *)(buf+8))=myr_f(fd);
  sendto(fd,buf,12,0,(sockaddr*)&servaddr,sizeof(servaddr));
  close(fd2);
  close(fd);  
}
