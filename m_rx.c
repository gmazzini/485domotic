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
    switch(*buf){
      case 1:
        printf("v1:%5.2f v2:%5.2f v3:%5.2f i1:%5.2f i2:%5.2f i3:%5.2f\n",*(float *)(buf+1),*(float *)(buf+5),*(float *)(buf+9),*(float *)(buf+13),*(float *)(buf+17),*(float *)(buf+21));
      break;
      case 2:
        printf("e1:%5.2f e2:%5.2f e3:%5.2f\n",*(float *)(buf+1),*(float *)(buf+5),*(float *)(buf+9));
        break;
    }
  }
}
