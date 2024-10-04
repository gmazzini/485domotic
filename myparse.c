#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#define TOTEK 500

int main(){
  FILE *fp;
  char buf[100];
  char *token,*f;
  uint16_t i,j,q,*lK,nlK,*lR,nlR,*lE,nlE,nlC,*lC,slC,nK;
  uint64_t *lD;
  struct ek{
    uint8_t act;
    uint16_t nR;
    uint16_t *R;
    uint16_t nC;
    uint16_t *C;
    uint64_t *D;
    struct ek *next;
  };
  struct ek *ee,*en,*em;

  // parsing configuration file
  ee=(struct ek *)malloc(TOTEK*sizeof(struct ek));
  for(q=0;q<TOTEK;q++)ee[q].act=0;
  lK=(uint16_t *)malloc(100*sizeof(uint16_t));
  lR=(uint16_t *)malloc(100*sizeof(uint16_t));
  lE=(uint16_t *)malloc(100*sizeof(uint16_t));
  lC=(uint16_t *)malloc(100*sizeof(uint16_t));
  lD=(uint64_t *)malloc(23*sizeof(uint64_t));
  nK=0;
  fp=fopen("config","r");
  for(;;){
    fgets(buf,100,fp);
    if(feof(fp))break;
    nlK=nlR=nlE=nlC=0;
    for(q=0;q<23;q++)lD[q]=18446744073709551615U;
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
          i=((*(f+1)-'0')*10+(*(f+2)-'0'))*60+(*(f+3)-'0')*10+(*(f+4)-'0');
          for(q=((*(token+1)-'0')*10+(*(token+2)-'0'))*60+(*(token+3)-'0')*10+(*(token+4)-'0');q<=i;q++)lD[q>>6]&=(18446744073709551615U^(1>>(q%64)));
          break;
        case 'C':
          slC=0;
          if(strcmp(token+1,"onoff")==0)slC=1;
          else if(strcmp(token+1,"on")==0)slC=2;
          else if(strcmp(token+1,"off")==0)slC=3;
          if(slC>0)lC[nlC++]=slC;
          break;
      }
    }
    for(q=0;q<nlK;q++){
      i=lK[q];
      en=ee+i;      
      if(en->act>0){
        for(;en->next!=NULL;en=en->next);
        em=(struct ek *)malloc(sizeof(struct ek));
        en->next=em;
        en=em;
      } 
      en->act=1;
      en->nR=nlR;
      en->R=(uint16_t *)malloc(nlR*sizeof(uint16_t));
      for(j=0;j<nlR;j++)en->R[j]=lR[j];
      en->nC=nlC;
      en->C=(uint16_t *)malloc(nlC*sizeof(uint16_t));
      for(j=0;j<nlC;j++)en->C[j]=lC[j];
      en->D=(uint64_t *)malloc(23*sizeof(uint64_t));
      for(j=0;j<23;j++)en->D[j]=lD[j];
      en->next=NULL;
    }
    
  }
  fclose(fp);
  free(lK);
  free(lR);
  free(lE);
  free(lC);
  free(lD);

  // checking
  for(q=0;q<TOTEK;q++){
    en=ee+q;
    for(;;){
      if(en->act==0)break;
      printf("K %d\n",q);
      for(j=0;j<en->nR;j++)printf("-- R %d\n",en->R[j]);
      for(j=0;j<en->nC;j++)printf("-- C %d\n",en->C[j]);
      peinrf("-- D");
      for(j=0;j<24*60;j++)if(en->D[j>>6] & (1>>(j%64)))printf(" %d",j);
      printf("\n");
      if(en->next==NULL)break;
      en=en->next;
    }
  }


}
