#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdarg.h>
#include <termios.h>
#include <mysql/mysql.h>
#include "mysetup.c"

void main(){
  int fd,rr;
  char buf[100],query[200];
  struct sockaddr from;
  struct sockaddr_in server_addr;
  unsigned int fromlen=sizeof(from);
  time_t t;
  float e1,e2,e3,oe1,oe2,oe3;
  MYSQL *con=mysql_init(NULL);
  oe1=oe2=oe3=0;

  fd=socket(AF_INET,SOCK_DGRAM,0);
  fcntl(fd,F_SETFL,O_NONBLOCK);
  server_addr.sin_family=AF_INET;
  server_addr.sin_port=htons(PORT);
  server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
  bind(fd,(struct sockaddr *)&server_addr,sizeof(server_addr));
  mysql_real_connect(con,"localhost",USER,PASSWORD,DB,0,NULL,0);

  for(;;){
    rr=recvfrom(fd,buf,100,0,&from,&fromlen);
    if(rr<1){usleep(10000); continue;}
    e1=*((float *)(buf));
    e2=*((float *)(buf+4));
    e3=*((float *)(buf+8));
    if(e1<0 || e2<0 || e3<0){
      oe1=e1; oe2=e2; oe3=e3;
      continue;
    }
    if(e1!=oe1 || e2!=oe2 || e3!=oe3){
      t=time(NULL);
      sprintf(query,"insert into viadagola (epoch,energy1,energy2,energy3) values (%ld,%f,%f,%f)",t,e1,e2,e3);
      mysql_query(con,query);
      oe1=e1; oe2=e2; oe3=e3;
    }
  }

}
