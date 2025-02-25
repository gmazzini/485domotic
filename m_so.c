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

union uw {uint16_t w; uint8_t u[2]; };
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

void main(int argc,char **argv){
  int fd,ow,mode;
  float v1,v2,v3,i1,i2,i3,e1,e2,e3;
  struct sockaddr_in server;
  MYSQL *con=mysql_init(NULL);
  time_t t;
  char buf[100],query[200];
  
  mode=atoi(argv[1]);
  fd=socket(AF_INET,SOCK_STREAM,0);
  server.sin_family=AF_INET;
  server.sin_port=htons(4196);
  server.sin_addr.s_addr=inet_addr(IP);
  connect(fd,(struct sockaddr *)&server,sizeof(server)); 
  mysql_real_connect(con,"localhost",USER,PASSWORD,DB,0,NULL,0);
  t=time(NULL);

  switch(mode){
    case 1:
      myw(fd,(uint8_t *)"\x01\x03\x00\x0E",2); v1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x10",2); v2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x12",2); v3=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x16",2); i1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x18",2); i2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x1A",2); i3=myr_f(fd);
      if(v1>=0 && v2>=0 && v3>=0 && i1>=0 && i2>=0 && i3>=0){
        sprintf(query,"insert into vi_so (epoch,v1,v2,v3,i1,i2,i3) values(%ld,%f,%f,%f,%f,%f,%f)",t,v1,v2,v3,i1,i2,i3);
        mysql_query(con,query);
      }
      break;

    case 2:
      myw(fd,(uint8_t *)"\x01\x03\x01\x02",2); e1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x01\x04",2); e2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x01\x06",2); e3=myr_f(fd);
      if(e1>=0 && e2>=0 && e3>=0){
        sprintf(query,"insert into energy_so (epoch,e1,e2,e3) values(%ld,%f,%f,%f)",t,e1,e2,e3);
        mysql_query(con,query);
      }
      break;
  }

  close(fd);
}
