#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdarg.h>
#include <termios.h>
#define FAKE -11111
#define CHSLEEP 1150 

union uw {uint16_t w; uint8_t u[2]; };
union ul {uint32_t l; uint8_t u[4]; };
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

int myr_w(int fd){
  union uw uw;
  uint8_t aux[7],i;
  for(i=0;i<7;i++)while(!read(fd,aux+i,1))usleep(CHSLEEP);
  uw.u[0]=aux[5]; uw.u[1]=aux[6];
  if(crc(aux,5)!=uw.w)return FAKE;
  uw.u[1]=aux[3]; uw.u[0]=aux[4];
  return uw.w;
}

float myr_f(int fd){
  union uw uw;
  union uf uf;
  int x,i;
  uint8_t aux[9];
  for(i=0;i<9;i++)while(!read(fd,aux+i,1))usleep(CHSLEEP);
  uw.u[0]=aux[7]; uw.u[1]=aux[8];
  if(crc(aux,7)!=uw.w)return FAKE;
  uf.u[3]=aux[3]; uf.u[2]=aux[4]; uf.u[1]=aux[5]; uf.u[0]=aux[6];
  return uf.f;
}

uint32_t *myr_ln(int fd,int n){
  union uw uw;
  static union ul ul[10];
  uint8_t aux[50],i;
  for(i=0;i<5+4*n;i++)while(!read(fd,aux+i,1))usleep(CHSLEEP);
  uw.u[0]=aux[3+4*n]; uw.u[1]=aux[4+4*n];
  if(crc(aux,3+4*n)!=uw.w)return NULL;
  for(i=0;i<n;i++){
    ul[i].u[3]=aux[3+4*i]; 
    ul[i].u[2]=aux[4+4*i];
    ul[i].u[1]=aux[5+4*i];
    ul[i].u[0]=aux[6+4*i];
  }
  return &ul[0].l;
}

uint8_t *myr_s(int fd,int len){
  union uw uw;
  uint8_t i;
  static uint8_t aux[50];
  for(i=0;i<5+len;i++)while(!read(fd,aux+i,1))usleep(CHSLEEP);
  uw.u[0]=aux[3+len]; uw.u[1]=aux[4+len];
  if(crc(aux,3+len)!=uw.w)return NULL;
  return aux+3;
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

float *myr_fn(int fd,int n){
  union uw uw;
  static union uf uf[10];
  int x,i;
  static uint8_t aux[100];
  for(i=0;;i++){
    if(i>3000)return NULL;
    x=read(fd,aux,100);
    if(x!=0)break;
    usleep(1000);
  }
  if(x!=5+4*n)return NULL;
  uw.u[0]=aux[3+4*n]; uw.u[1]=aux[4+4*n];
  if(crc(aux,3+4*n)!=uw.w)return NULL;
  #ifdef DEBUG
  for(i=0;i<x;i++)printf("%02x ",aux[i]); printf("\n");
  #endif
  for(i=0;i<n;i++){
    uf[i].u[3]=aux[3+4*i]; 
    uf[i].u[2]=aux[4+4*i];
    uf[i].u[1]=aux[5+4*i];
    uf[i].u[0]=aux[6+4*i];
  }
  return &uf[0].f;
}


