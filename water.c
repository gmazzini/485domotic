#define FFF "/home/gmazzini/water.txt"
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#define PIN_IN 2

uint64_t cwater;
void iwater(void){
  cwater++;
}

int main(void){
  FILE *fp;

  wiringPiSetup();
  pinMode(PIN_IN,INPUT);
  wiringPiISR(PIN_IN,INT_EDGE_RISING,&iwater);
  fp=fopen(FFF,"r");
  fscanf(fp,"%" SCNu64,&cwater);
  fclose(fp);
  for(;;){
    sleep(10);
    fp=fopen(FFF,"w");
    fprintf(fp,"%" PRIu64 "\n",cwater);
    fclose(fp);
  }
}
