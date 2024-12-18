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
#include "/root/mysetup.c"

void main(){
  int fd,rr;
  char buf[100],query[200];
  struct sockaddr from;
  struct sockaddr_in server_addr;
  unsigned int fromlen=sizeof(from);
  time_t t;
  MYSQL *con=mysql_init(NULL);

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
    t=time(NULL);
    switch(*buf){
      case 1:
        sprintf(query,"insert into vi (epoch,v1,v2,v3,i1,i2,i3) values(%ld,%f,%f,%f,%f,%f,%f)",t,*(float *)(buf+1),*(float *)(buf+5),*(float *)(buf+9),*(float *)(buf+13),*(float *)(buf+17),*(float *)(buf+21));
        mysql_query(con,query);
        // printf("v1:%f v2:%f v3:%f i1:%f i2:%f i3:%f\n",*(float *)(buf+1),*(float *)(buf+5),*(float *)(buf+9),*(float *)(buf+13),*(float *)(buf+17),*(float *)(buf+21));
      break;
      case 2:
        sprintf(query,"insert into energy (epoch,e1,e2,e3) values(%ld,%f,%f,%f)",t,*(float *)(buf+1),*(float *)(buf+5),*(float *)(buf+9));
        mysql_query(con,query);
        // printf("e1:%f e2:%f e3:%f\n",*(float *)(buf+1),*(float *)(buf+5),*(float *)(buf+9));
        break;
    }
  }
}
