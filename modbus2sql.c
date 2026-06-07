#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdarg.h>
#include <termios.h>
#include <inttypes.h>
#include <mysql/mysql.h>
#include "/home/tools/setup_energy.c"

#define RWTIMEOUT 3000
#define CHSLEEP 10000
#define FAKE -999999.0

union uw {uint16_t w; uint8_t u[2]; };
union uf {float f; uint8_t u[4]; };
union ul {uint32_t l; uint8_t u[4]; };

uint16_t crc(uint8_t *buf,uint8_t lenbuf){
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

int readn(int fd,uint8_t *buf,int len){
  int i,r,n;
  struct pollfd pfd;

  i=0;
  while(i<len){
    pfd.fd=fd;
    pfd.events=POLLIN;
    pfd.revents=0;

    r=poll(&pfd,1,RWTIMEOUT);
    if(r<0 && errno==EINTR)continue;
    if(r<=0)return -1;
    if(pfd.revents&(POLLERR|POLLHUP|POLLNVAL))return -1;

    n=read(fd,buf+i,len-i);
    if(n<0 && errno==EINTR)continue;
    if(n<=0)return -1;
    i+=n;
  }
  return 0;
}

int writen(int fd,uint8_t *buf,int len){
  int i,r,n;
  struct pollfd pfd;

  i=0;
  while(i<len){
    pfd.fd=fd;
    pfd.events=POLLOUT;
    pfd.revents=0;

    r=poll(&pfd,1,RWTIMEOUT);
    if(r<0 && errno==EINTR)continue;
    if(r<=0)return -1;
    if(pfd.revents&(POLLERR|POLLHUP|POLLNVAL))return -1;

    n=write(fd,buf+i,len-i);
    if(n<0 && errno==EINTR)continue;
    if(n<=0)return -1;
    i+=n;
  }
  return 0;
}

void myw(int fd,uint8_t *ss,uint8_t nn){
  union uw uw;
  uint8_t aux[8];
  memcpy(aux,ss,4);
  aux[4]=0;
  aux[5]=nn;
  uw.w=crc(aux,6);
  aux[6]=uw.u[0];
  aux[7]=uw.u[1];
  writen(fd,aux,8); usleep(8*CHSLEEP);
}

float myr_f(int fd){
  union uw uw;
  union uf uf;
  uint8_t aux[9];

  if(readn(fd,aux,9)<0)return FAKE;

  uw.u[0]=aux[7]; uw.u[1]=aux[8];
  if(crc(aux,7)!=uw.w)return FAKE;
  if(aux[1]&0x80)return FAKE;
  if(aux[2]!=4)return FAKE;

  uf.u[3]=aux[3]; uf.u[2]=aux[4]; uf.u[1]=aux[5]; uf.u[0]=aux[6];
  return uf.f;
}

uint32_t *myr_ln(int fd,int n){
  union uw uw;
  static union ul ul[10];
  int i;
  uint8_t aux[50];

  if(n<1 || n>10)return NULL;

  if(readn(fd,aux,5+4*n)<0)return NULL;

  uw.u[0]=aux[3+4*n]; uw.u[1]=aux[4+4*n];
  if(crc(aux,3+4*n)!=uw.w)return NULL;
  if(aux[1]&0x80)return NULL;
  if(aux[2]!=(4*n))return NULL;

  for(i=0;i<n;i++){
    ul[i].u[3]=aux[3+4*i];
    ul[i].u[2]=aux[4+4*i];
    ul[i].u[1]=aux[5+4*i];
    ul[i].u[0]=aux[6+4*i];
  }

  return &ul[0].l;
}

int main(int argc,char **argv){
  int fd,ow,mode;
  float v1,v2,v3,i1,i2,i3,e1,e2,e3;
  uint32_t *ol;
  struct sockaddr_in server;
  MYSQL *con;
  time_t t;
  char query[200];

  if(argc<2){
    fprintf(stderr,"usage: %s mode\n",argv[0]);
    return 1;
  }

  mode=atoi(argv[1]);
  con=mysql_init(NULL);
  if(con==NULL)return 1;

  fd=socket(AF_INET,SOCK_STREAM,0);
  if(fd<0){
    mysql_close(con);
    return 1;
  }

  memset(&server,0,sizeof(server));
  server.sin_family=AF_INET;

  if(mysql_real_connect(con,"localhost",USER,PASSWORD,DB,0,NULL,0)==NULL){
    close(fd);
    mysql_close(con);
    return 1;
  }

  t=time(NULL);

  switch(mode){
    case 1:
      server.sin_port=htons(PORTSO);
      server.sin_addr.s_addr=inet_addr(IPSO);
      ow=connect(fd,(struct sockaddr *)&server,sizeof(server));
      if(ow<0)break;

      myw(fd,(uint8_t *)"\x01\x03\x00\x0E",2); v1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x10",2); v2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x12",2); v3=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x16",2); i1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x18",2); i2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x1A",2); i3=myr_f(fd);

      if(v1>=0 && v2>=0 && v3>=0 && i1>=0 && i2>=0 && i3>=0){
        snprintf(query,sizeof(query),"insert into vi_so (epoch,v1,v2,v3,i1,i2,i3) values(%ld,%f,%f,%f,%f,%f,%f)",(long)t,v1,v2,v3,i1,i2,i3);
        mysql_query(con,query);
      }
      break;

    case 2:
      server.sin_port=htons(PORTSO);
      server.sin_addr.s_addr=inet_addr(IPSO);
      ow=connect(fd,(struct sockaddr *)&server,sizeof(server));
      if(ow<0)break;

      myw(fd,(uint8_t *)"\x01\x03\x01\x02",2); e1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x01\x04",2); e2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x01\x06",2); e3=myr_f(fd);

      if(e1>=0 && e2>=0 && e3>=0){
        snprintf(query,sizeof(query),"insert into energy_so (epoch,e1,e2,e3) values(%ld,%f,%f,%f)",(long)t,e1,e2,e3);
        mysql_query(con,query);
      }
      break;

    case 3:
      server.sin_port=htons(PORTCC1);
      server.sin_addr.s_addr=inet_addr(IPCC1);
      ow=connect(fd,(struct sockaddr *)&server,sizeof(server));
      if(ow<0)break;

      myw(fd,(uint8_t *)"\x01\x03\x00\x0E",2); v1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x10",2); v2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x12",2); v3=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x16",2); i1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x18",2); i2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x1A",2); i3=myr_f(fd);

      if(v1>=0 && v2>=0 && v3>=0 && i1>=0 && i2>=0 && i3>=0){
        snprintf(query,sizeof(query),"insert into vi_cc (epoch,v1,v2,v3,i1,i2,i3) values(%ld,%f,%f,%f,%f,%f,%f)",(long)t,v1,v2,v3,i1,i2,i3);
        mysql_query(con,query);
      }
      break;

    case 4:
      server.sin_port=htons(PORTCC1);
      server.sin_addr.s_addr=inet_addr(IPCC1);
      ow=connect(fd,(struct sockaddr *)&server,sizeof(server));
      if(ow<0)break;

      myw(fd,(uint8_t *)"\x01\x03\x01\x02",2); e1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x01\x04",2); e2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x01\x06",2); e3=myr_f(fd);

      if(e1>=0 && e2>=0 && e3>=0){
        snprintf(query,sizeof(query),"insert into energy_cc (epoch,e1,e2,e3) values(%ld,%f,%f,%f)",(long)t,e1,e2,e3);
        mysql_query(con,query);
      }
      break;

    case 5:
      server.sin_port=htons(PORTCC2);
      server.sin_addr.s_addr=inet_addr(IPCC2);
      ow=connect(fd,(struct sockaddr *)&server,sizeof(server)); 
      if(ow<0)break;

      myw(fd,(uint8_t *)"\x01\x03\x01\x0E",2); ol=myr_ln(fd,1);

      if(ol!=NULL){
        snprintf(query,sizeof(query),"insert into energy_le1 (epoch,e1) values(%ld,%lu)",(long)t,(unsigned long)ol[0]);
        mysql_query(con,query);
      }
      break;

    default:
      fprintf(stderr,"bad mode: %d\n",mode);
      break;
  }

  close(fd);
  mysql_close(con);
  return 0;
}
