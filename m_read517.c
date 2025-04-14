#define noDEBUG
#define SERIAL "/dev/ttyACM1"
#include "m_functions.c"

void main(){
  int fd,ow;
  float of;
  uint8_t *os;

  fd=open(SERIAL,O_RDWR);
  setserial(fd);

  myw(fd,"\x01\x03\x00\x03",1); ow=myr_w(fd); printf("Baud: %d\n",ow);
  myw(fd,"\x01\x03\x00\x0E",2); of=myr_f(fd); printf("Tensione 1: %f\n",of);
  myw(fd,"\x01\x03\x00\x10",2); of=myr_f(fd); printf("Tensione 2: %f\n",of);
  myw(fd,"\x01\x03\x00\x12",2); of=myr_f(fd); printf("Tensione 3: %f\n",of);
  myw(fd,"\x01\x03\x00\x16",2); of=myr_f(fd); printf("Corrente 1: %f\n",of);
  myw(fd,"\x01\x03\x00\x18",2); of=myr_f(fd); printf("Corrente 2: %f\n",of);
  myw(fd,"\x01\x03\x00\x1A",2); of=myr_f(fd); printf("Corrente 3: %f\n",of);
  myw(fd,"\x01\x03\x00\x1C",2); of=myr_f(fd); printf("Potenza: %f\n",of);
  myw(fd,"\x01\x03\x01\x00",2); of=myr_f(fd); printf("Energia: %f\n",of);
  myw(fd,"\x01\x03\x01\x02",2); of=myr_f(fd); printf("Energia 1: %f\n",of);
  myw(fd,"\x01\x03\x01\x04",2); of=myr_f(fd); printf("Energia 2: %f\n",of);
  myw(fd,"\x01\x03\x01\x06",2); of=myr_f(fd); printf("Energia 3: %f\n",of);
  myw(fd,"\x01\x03\x00\x3C",4); os=myr_s(fd,8); if(os!=NULL)printf("Data: %02x/%02x/%02x %02x:%02x:%02x\n",os[4],os[5],os[6],os[2],os[1],os[0]);

  close(fd);
}
