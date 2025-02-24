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
#include "/home/tools/setup_energycc.c"

union uw {uint16_t w; uint8_t u[2]; };
union ul {uint32_t l; uint8_t u[4]; };
union uf {float f; uint8_t u[4]; };

uint16_t crc(uint8_t *buf,int lenbuf){
  int i,j;
  uint16_t out=0xFFFF;
  for(j=0;j<lenbuf;j++){
    out^=(uint16_t)buf[j];
    for(i=0;i<8;++i){
      if(out&1)out=(out>>1)^0xA001;
      else out=(out>>1);
    }
  }
  return out;
}

float myr_f(int fd){
  union uw uw;
  union uf uf;
  int x,i;
  static uint8_t aux[9];
  for(i=0;i<9;i++)read(fd,aux+i,1);
  uw.u[0]=aux[7]; uw.u[1]=aux[8];
  if(crc(aux,7)!=uw.w)return -1000;
  uf.u[3]=aux[3]; uf.u[2]=aux[4]; uf.u[1]=aux[5]; uf.u[0]=aux[6];
  return uf.f;
}

void myw(int fd,uint8_t *ss,uint8_t nn){
  union uw uw;
  uint8_t aux[8];
  int i;
  memcpy(aux,ss,4);
  aux[4]=0;
  aux[5]=nn;
  uw.w=crc(aux,6);
  aux[6]=uw.u[0];
  aux[7]=uw.u[1];
  write(fd,aux,8);
  usleep(10000);
}

int main(){
  int fd,ow;
  float v1,v2,v3,i1,i2,i3;
  struct sockaddr_in server;
  MYSQL *con=mysql_init(NULL);
  time_t t;

  fd=socket(AF_INET,SOCK_STREAM,0);
  server.sin_family = AF_INET;
  server.sin_port = htons(4196);
  server.sin_addr.s_addr = inet_addr(IP);
  connect(fd,(struct sockaddr *)&server,sizeof(server)); 
  mysql_real_connect(con,"localhost",USER,PASSWORD,DB,0,NULL,0);
  t=time(NULL);

  myw(fd,(uint8_t *)"\x01\x03\x00\x0E",2); v1=myr_f(fd);
  myw(fd,(uint8_t *)"\x01\x03\x00\x10",2); v2=myr_f(fd);
  myw(fd,(uint8_t *)"\x01\x03\x00\x12",2); v3=myr_f(fd);
  myw(fd,(uint8_t *)"\x01\x03\x00\x16",2); i1=myr_f(fd);
  myw(fd,(uint8_t *)"\x01\x03\x00\x18",2); i2=myr_f(fd);
  myw(fd,(uint8_t *)"\x01\x03\x00\x1A",2); i3=myr_f(fd);
  sprintf(query,"insert into vi (epoch,v1,v2,v3,i1,i2,i3) values(%ld,%f,%f,%f,%f,%f,%f)",t,v1,v2,v3,i1,i2,i3);
  mysql_query(con,query);
  
  // myw(fd,(uint8_t *)"\x01\x03\x01\x02",2); of=myr_f(fd); printf("Energia 1: %f\n",of);
  // myw(fd,(uint8_t *)"\x01\x03\x01\x04",2); of=myr_f(fd); printf("Energia 2: %f\n",of);
  // myw(fd,(uint8_t *)"\x01\x03\x01\x06",2); of=myr_f(fd); printf("Energia 3: %f\n",of);

  close(fd);
}

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
