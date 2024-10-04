#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#define TOTEK 500

int main(){
  FILE *fp;
  char buf[100];
  char *token,*f;
  uint16_t i,j,q,*lK,nlK,*lR,nlR,*lE,nlE,nlD,nlC,*lC,slC,nK;
  uint32_t *lD;
  struct ek{
    uint8_t act;
    uint16_t nR;
    uint16_t *R;
    uint16_t nC;
    uint16_t *C;
    struct ek *next;
  };
  struct ek *ee;

  ee=(struct ek *)malloc(TOTEK*sizeof(struct ek));
  for(q=0;q<TOTEK;q++)ee[q].act=0;

  lK=(uint16_t *)malloc(100*sizeof(uint16_t));
  lR=(uint16_t *)malloc(100*sizeof(uint16_t));
  lE=(uint16_t *)malloc(100*sizeof(uint16_t));
  lD=(uint32_t *)malloc(100*sizeof(uint16_t));

  nK=0;
  fp=fopen("config","r");
  for(;;){
    fgets(buf,100,fp);
    if(feof(fp))break;
    nlK=nlR=nlE=nlD=nlC=0;
    for(token=strtok(buf," \n\r\t");token;token=strtok(NULL," \n\r\t")){
      switch(token[0]){
        case 'K':
          f=strchr(token,','); *f='\0';
          lK[nlK++]=10*atoi(token+1)+atoi(f+1);
          break;
        case 'R':
          f=strchr(token,','); *f='\0';
          lR[nlR++]=10*atoi(token+1)+atoi(f+1);
          break;
        case 'E':
          lE[nlE++]=atoi(token+1);
          break;
        case 'D':
          f=strchr(token,','); *f='\0';
          lD[nlD++]=((*(token+1)-'0')*10+(*(token+2)-'0'))*60+(*(token+3)-'0')*10+(*(token+4)-'0')+10000*(((*(f+1)-'0')*10+(*(f+2)-'0'))*60+(*(f+3)-'0')*10+(*(f+4)-'0'));
          break;
        case 'C':
          slC=0;
          if(strcmp(token+1,"onoff"))slC=1;
          else if(strcmp(token+1,"on"))slC=2;
          else if(strcmp(token+1,"off"))slC=3;
          if(slC>0)lC[nlC++]=slC;
          break;
      }
    }
    
    for(q=0;q<nlK;q++){
      i=lK[q];
      if(ee[i].act==0){
        ee[i].act=1;
        ee[i].nR=lnR;
        ee[i].R=(uint16_t *)malloc(lnR*sizeof(uint16_t));
        for(j=0;j<lnR;j++)ee[i].R[j]=lR[j];
        ee[i].nC=lnC;
        ee[i].C=(uint16_t *)malloc(lnC*sizeof(uint16_t));
        for(j=0;j<lnC;j++)ee[i].C[j]=lC[j];
        ee[i].next=NULL;
      }
    }
    
  }
  fclose(fp);


}
