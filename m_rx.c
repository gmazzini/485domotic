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
#include "/home/tools/setup_energy.c"

void main(){
  int fd,rr;
  char buf[100],query[200];
  struct sockaddr_in6 server_addr,from;
  struct in6_addr white;
  unsigned int fromlen=sizeof(from);
  time_t t;
  MYSQL *con=mysql_init(NULL);

  fd=socket(PF_INET6,SOCK_DGRAM,0);
  fcntl(fd,F_SETFL,O_NONBLOCK);
  server_addr.sin6_family=AF_INET6;
  server_addr.sin6_port=htons(PORT);
  server_addr.sin6_addr=in6addr_any;
  bind(fd,(struct sockaddr *)&server_addr,sizeof(server_addr));
  mysql_real_connect(con,"localhost",USER,PASSWORD,DB,0,NULL,0);
  inet_pton(AF_INET6,WHITE,&white);

  for(;;){
    rr=recvfrom(fd,buf,100,0,(struct sockaddr *)&from,&fromlen);
    if(rr<1){usleep(10000); continue;}
    if(memcmp(&(from.sin6_addr),&white,sizeof(struct in6_addr))){usleep(10000); continue;}
    t=time(NULL);
    switch(*buf){
      case 1:
        sprintf(query,"insert into vi_cc (epoch,v1,v2,v3,i1,i2,i3) values(%ld,%f,%f,%f,%f,%f,%f)",t,*(float *)(buf+1),*(float *)(buf+5),*(float *)(buf+9),*(float *)(buf+13),*(float *)(buf+17),*(float *)(buf+21));
        mysql_query(con,query);
      break;
      case 2:
        sprintf(query,"insert into energy_cc (epoch,e1,e2,e3) values(%ld,%f,%f,%f)",t,*(float *)(buf+1),*(float *)(buf+5),*(float *)(buf+9));
        mysql_query(con,query);
      break;
      case 3:
        sprintf(query,"insert into water_cc (epoch,w) values(%ld,%ld)",t,*(uint32_t *)(buf+1));
        mysql_query(con,query);
      break;
      
    }
  }
}
