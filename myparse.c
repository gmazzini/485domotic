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
  struct ek *ee,*en,*em;

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
      en=ee+i;      
      if(en->act>0){
        printf(".\n");
        for(;en->next!=NULL;en=en->next);
        printf(">\n");
        em=(struct ek *)malloc(sizeof(struct ek));
        en->next=em;
        printf("<\n");
        en=em;
      } 
      en->act=1;
      en->nR=nlR;
      en->R=(uint16_t *)malloc(nlR*sizeof(uint16_t));
      for(j=0;j<nlR;j++)en->R[j]=lR[j];
      en->nC=nlC;
      en->C=(uint16_t *)malloc(nlC*sizeof(uint16_t));
      for(j=0;j<nlC;j++)en->C[j]=lC[j];
      en->next=NULL;
    }
    
  }
  fclose(fp);
  free(lK);
  free(lR);
  free(lE);
  free(lC);

  for(q=0;q<TOTEK;q++)if(ee[q].act>0)printf("K %d\n",q);


}
