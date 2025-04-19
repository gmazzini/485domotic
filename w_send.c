#include <wiringPi.h>
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
#define PIN_IN 2
#define HOSTNAME "2a0e:97c0:3ea:4fa::1"
#define PORT 44444

uint32_t cwater=0;
void iwater(void){
  cwater++;
}

int main(void){
  int fd2;
  char buf[100];
  struct sockaddr_in6 servaddr;

  wiringPiSetup();
  pinMode(PIN_IN,INPUT);
  wiringPiISR(PIN_IN,INT_EDGE_RISING,&iwater);
  fd2=socket(AF_INET6,SOCK_DGRAM,0);
  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin6_family=AF_INET6;
  inet_pton(AF_INET6,HOSTNAME,&(servaddr.sin6_addr));
  servaddr.sin6_port=htons(PORT);

  for(;;){
    if(cwater>0){
      *buf=3;
      memcpy((void *)buf+1,(void *)&cwater,4);
      sendto(fd2,buf,5,0,(struct sockaddr *)&servaddr,sizeof(servaddr));
    }
    cwater=0;
    sleep(10);
  }
}
