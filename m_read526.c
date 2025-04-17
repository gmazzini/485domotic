#define noDEBUG
#define SERIAL "/dev/ttyACM2"
#include "m_functions.c"

void main(){
  int fd,ow;
  uint32_t *ol;
  uint8_t *os;

  fd=open(SERIAL,O_RDWR);
  setserial(fd,'n');

  myw(fd,"\x01\x03\x01\x0A",1); ow=myr_w(fd); printf("Hz: %d\n",ow);
  myw(fd,"\x01\x03\x01\x00",4); 
  ol=myr_ln(fd,2);
  if(ol!=NULL){
    printf("V: %ld\n",ol[0]);
    printf("I: %ld\n",ol[1]);
  }
  myw(fd,"\x01\x03\x01\x0E",2); 
  ol=myr_ln(fd,1);
  if(ol!=NULL){
    printf("E: %ld\n",ol[0]);
  }
  
  close(fd);
}
