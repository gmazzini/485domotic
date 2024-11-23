#define noDEBUG
#define SERIAL "/dev/ttyACM1"
#include "m_functions.c"

void main(){
  int fd,ow;
  float *of;
  uint8_t *os;

  fd=open(SERIAL,O_RDWR);
  setserial(fd);

  myw(fd,"\x01\x03\x00\x0E",6); 
  of=myr_fn(fd,1); 
  printf("V1: %f\n",*of);

  close(fd);
}
