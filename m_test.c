#define DEBUG
#define SERIAL "/dev/ttyACM1"
#include "m_functions.c"

void main(){
  int fd,ow;
  float *of;
  uint8_t *os;

  fd=open(SERIAL,O_RDWR);
  setserial(fd);

  myw(fd,"\x01\x03\x00\x0E",14); 
  of=myr_fn(fd,7);
  if(of!=NULL){
    printf("V1: %6.2f\n",*of);
    printf("V2: %6.2f\n",*(of+1));
    printf("V3: %6.2f\n",*(of+2));
    printf("HZ: %6.2f\n",*(of+3));
    printf("I1: %6.2f\n",*(of+4));
    printf("I2: %6.2f\n",*(of+5));
    printf("I3: %6.2f\n",*(of+6));
  }

    
  close(fd);
}
