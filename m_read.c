#define noDEBUG
#define SERIAL "/dev/ttyACM1"
#include "m_functions.c"

int main(void){
  int fd;
  float *of;
  char *query;
  
  query=getenv("QUERY_STRING");  
  printf("Content-Type: text/plain\n\n");
  switch(atoi(query)){
    case 1: // 1 v1 v2 v3 i1 i2 i3
      fd=open(SERIAL,O_RDWR); setserial(fd,'e');
      myw(fd,"\x01\x03\x00\x0E",14); 
      of=myr_fn(fd,7);
      close(fd);
      if(of==NULL)break;
      printf("1,%f,%f,%f,%f,%f,%f\n",of[0],of[1],of[2],of[4],of[5],of[6]);
      break;
    case 2: // 2 E1 E2 E3
      fd=open(SERIAL,O_RDWR); setserial(fd,'e');
      myw(fd,"\x01\x03\x01\x02",6); 
      of=myr_fn(fd,3);
      close(fd);
      if(of==NULL)break;
      printf("2,%f,%f,%f\n",of[0],of[1],of[2]);
      break;
  }
}
