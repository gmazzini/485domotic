#define DEBUG
#define SERIAL "/dev/ttyACM2"
#include "m_functions.c"

void main(){
  int fd,ow;
  long *ol;
  float of;
  uint8_t *os;

  fd=open(SERIAL,O_RDWR);
  setserial(fd);

  myw(fd,"\x01\x03\x01\x0A",1); ow=myr_w(fd); printf("Hz: %d\n",ow);
  myw(fd,"\x01\x03\x01\x00",4); 
  ol=myr_ln(fd,2);
  if(ol!=NULL){
    printf("V: %ld\n",ol[0]);
    printf("I: %ld\n",ol[1]);
  }
  
  close(fd);
}
