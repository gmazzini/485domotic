#include "stdio.h"
#include "stdlib.h"
#include "string.h"

int main(){
  FILE *fp;
  char buf[100];
  char *token,*f;
  uint16_t q,*lK,nlK,*lR,nlR,*lE,nlE,nlD,nlC,*lC,slC,nK;
  uint32_t *lD;
  struct ek{
    uint16_t nR;
    uint16_t *R;
    uint16_t nC;
    uint16_t *C;
    struct ek *next;
  } ee[500];

  lK=(uint16_t *)malloc(100);
  lR=(uint16_t *)malloc(100);
  lE=(uint16_t *)malloc(100);
  lD=(uint32_t *)malloc(100);

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

  for(q=0;q<nlK;q++)printf("K %d %d\n",q,lK[q]);
  for(q=0;q<nlR;q++)printf("R %d %d\n",q,lR[q]);
  for(q=0;q<nlD;q++)printf("D %d %li\n",q,lD[q]);
    
  }
  fclose(fp);


}
