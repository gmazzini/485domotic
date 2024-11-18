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

union uw {uint16_t w; uint8_t u[2]; };
union uf {float f; uint8_t u[4]; };

void setserial(int fd){
  struct termios tty;
  tcgetattr(fd,&tty);
  tty.c_cflag &= PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag |= CREAD | CLOCAL;
  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO;
  tty.c_lflag &= ~ECHOE;
  tty.c_lflag &= ~ECHONL;
  tty.c_lflag &= ~ISIG;
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
  tty.c_oflag &= ~OPOST;
  tty.c_oflag &= ~ONLCR;
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 0;
  cfsetispeed(&tty,B9600);
  cfsetospeed(&tty,B9600);
  tcsetattr(fd,TCSANOW,&tty);
}

uint16_t crc(char *buf,int lenbuf){
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
}

void myw_raw(int fd,uint8_t *ss,uint8_t len){
  union uw uw;
  uint8_t aux[100];
  memcpy(aux,ss,len);
  uw.w=crc(aux,len);
  aux[len]=uw.u[0];
  aux[len+1]=uw.u[1];
  write(fd,aux,len+2);
}

int myr_w(int fd){
  union uw uw;
  int x,i;
  static uint8_t aux[100];
  for(i=0;;i++){
    if(i>3000)return -1000;
    x=read(fd,aux,100);
    if(x!=0)break;
    usleep(1000);
  }
  uw.u[0]=aux[5]; uw.u[1]=aux[6];
  if(crc(aux,5)!=uw.w)return -1000;
  #ifdef DEBUG
  for(i=0;i<x;i++)printf("%02x ",aux[i]); printf("\n");
  #endif
  uw.u[1]=aux[3]; uw.u[0]=aux[4];
  return uw.w;
}

float myr_f(int fd){
  union uw uw;
  union uf uf;
  int x,i;
  static uint8_t aux[100];
  for(i=0;;i++){
    if(i>3000)return -1000;
    x=read(fd,aux,100);
    if(x!=0)break;
    usleep(1000);
  }
  if(x!=9)return -1000;
  uw.u[0]=aux[7]; uw.u[1]=aux[8];
  if(crc(aux,7)!=uw.w)return -1000;
  #ifdef DEBUG
  for(i=0;i<x;i++)printf("%02x ",aux[i]); printf("\n");
  #endif
  uf.u[3]=aux[3]; uf.u[2]=aux[4]; uf.u[1]=aux[5]; uf.u[0]=aux[6];
  return uf.f;
}

uint8_t *myr_s(int fd,int len){
  union uw uw;
  int x,i;
  static uint8_t aux[100];
  for(i=0;;i++){
    if(i>3000)return NULL;
    x=read(fd,aux,100);
    if(x!=0)break;
    usleep(1000);
  }
  if(x!=5+len)return NULL;
  uw.u[0]=aux[3+len]; uw.u[1]=aux[4+len];
  if(crc(aux,3+len)!=uw.w)return NULL;
  #ifdef DEBUG
  for(i=0;i<x;i++)printf("%02x ",aux[i]); printf("\n");
  #endif
  return aux+3;
}
