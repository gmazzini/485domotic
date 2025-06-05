#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#define HOSTNAME "2a0e:97c0:3ea:4fa::1"
#define PORT 44444

int main(void){
  int fd2;
  char buf[100];
  long temp;
  struct sockaddr_in6 servaddr;
  FILE *fp;
  
  fd2=socket(AF_INET6,SOCK_DGRAM,0);
  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin6_family=AF_INET6;
  inet_pton(AF_INET6,HOSTNAME,&(servaddr.sin6_addr));
  servaddr.sin6_port=htons(PORT);

  for(;;){
    fp=fopen("/sys/class/thermal/thermal_zone0/temp","r");
    if(fp!=NULL){
      fscanf(fp,"%ld",&temp);
      temp/=100;
      *buf=4;
      memcpy((void *)buf+1,(void *)&temp,4);
      sendto(fd2,buf,5,0,(struct sockaddr *)&servaddr,sizeof(servaddr));
      fclose(fp);
    }
    sleep(10);
  }
}
