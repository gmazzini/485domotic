#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include "time.h"
#include "fcntl.h"
#include "arpa/inet.h"
#include "unistd.h"
#define PORT 12345
#define TOTEK 500
#define TOTEX 20
#define TOTRELAIS 120

int main(){
  FILE *fp;
  char buf[100],relais[TOTRELAIS],mod[TOTRELAIS];
  char *token,*f,*g;
  time_t myt;
  struct tm *info;
  uint16_t i,j,q,*lK,nlK,*lR,nlR,*lE,nlE,nlC,*lC,slC,*lT,nlT;
  uint64_t *lD;
  struct ek{
    uint8_t act;
    uint16_t nR;
    uint16_t *R;
    uint16_t nC;
    uint16_t *C;
    uint64_t *D;
    uint16_t nT;
    uint16_t *T;
    struct ek *next;
  };
  struct ek *ee,*ex,*en,*em;
  int sock;
  struct sockaddr_in servaddr,cliaddr;
  socklen_t len=sizeof(cliaddr);

  // parsing configuration file
  ee=(struct ek *)malloc(TOTEK*sizeof(struct ek));
  ex=(struct ek *)malloc(TOTEX*sizeof(struct ek));
  for(q=0;q<TOTEK;q++)ee[q].act=0;
  for(q=0;q<TOTEX;q++)ex[q].act=0;
  lK=(uint16_t *)malloc(100*sizeof(uint16_t));
  lR=(uint16_t *)malloc(100*sizeof(uint16_t));
  lE=(uint16_t *)malloc(100*sizeof(uint16_t));
  lC=(uint16_t *)malloc(100*sizeof(uint16_t));
  lD=(uint64_t *)malloc(23*sizeof(uint64_t));
  lT=(uint16_t *)malloc(100*sizeof(uint16_t));
  fp=fopen("config","r");
  for(;;){
    fgets(buf,100,fp);
    if(feof(fp))break;
    nlK=nlR=nlE=nlC=nlT=0;
    for(q=0;q<23;q++)lD[q]=0;
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
          for(q=((*(token+1)-'0')*10+(*(token+2)-'0'))*60+(*(token+3)-'0')*10+(*(token+4)-'0');q<=i;q++)lD[q>>6]|=(1ULL<<(q%64));
          break;
        case 'C':
          slC=0;
          if(strcmp(token+1,"onoff")==0)slC=1;
          else if(strcmp(token+1,"on")==0)slC=2;
          else if(strcmp(token+1,"off")==0)slC=3;
          else if(strcmp(token+1,"condon")==0)slC=4;
          else if(strcmp(token+1,"condoff")==0)slC=5;
          if(slC>0)lC[nlC++]=slC;
          break;
        case 'T':
          f=strchr(token,','); *f='\0'; g=strchr(f+1,','); *g='\0';
          lT[nlT++]=10*atoi(token+1)+atoi(f+1)+1000*atoi(g+1);
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
      en->nT=nlT;
      en->T=(uint16_t *)malloc(nlT*sizeof(uint16_t));
      for(j=0;j<nlT;j++)en->T[j]=lT[j];
      en->next=NULL;
    }
    for(q=0;q<nlE;q++){
      i=lE[q];
      en=ex+i;      
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
      en->nT=nlT;
      en->T=(uint16_t *)malloc(nlT*sizeof(uint16_t));
      for(j=0;j<nlT;j++)en->T[j]=lT[j];
      en->next=NULL;
    }
    
  }
  fclose(fp);
  free(lK);
  free(lR);
  free(lE);
  free(lC);
  free(lD);
  free(lT);

  // initilize 
  for(q=0;q<TOTRELAIS;q++)relais[q]=0;
  sock=socket(AF_INET,SOCK_DGRAM,0);
  fcntl(sock,F_SETFL,O_NONBLOCK);
  memset(&servaddr,0,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  servaddr.sin_addr.s_addr=INADDR_ANY;
  servaddr.sin_port=htons(PORT);
  bind(sock,(struct sockaddr *)&servaddr,sizeof(servaddr));
  
  // receiving events
  for(;;){
    i=recvfrom(sock,buf,100,0,(struct sockaddr *)&cliaddr,&len);
    if(i==0){sleep(1); printf(":"); continue;
    }

    printf("input: ");
    scanf("%s",buf);
    if(buf[0]=='K'){
      f=strchr(buf,','); *f='\0';
      q=10*atoi(buf+1)+atoi(f+1);
      if(q==0)break;
      en=ee+q;
    }
    else if(buf[0]=='E'){
      q=atoi(buf+1);
      if(q==0)break;
      en=ex+q;
    }
    else continue;

    // processing
    for(q=0;q<TOTRELAIS;q++)mod[q]=relais[q];
    for(;;){
      if(en->act==0)break;
      time(&myt);
      info=localtime(&myt);
      j=info->tm_hour*60+info->tm_min;
      if((en->D[j>>6] & (1ULL<<(j%64)))==0){
        for(j=0;j<en->nC;j++){
          switch(en->C[j]){
            case 1:
              for(q=0,j=0;j<en->nR;j++)q+=relais[en->R[j]];
              if(q>(en->nR/2))for(j=0;j<en->nR;j++)mod[en->R[j]]=0;
              else for(j=0;j<en->nR;j++)mod[en->R[j]]=1;
              break;
            case 2:
              for(j=0;j<en->nR;j++)mod[en->R[j]]=1;
              break;
            case 3:
              for(j=0;j<en->nR;j++)mod[en->R[j]]=0;
              break;
            case 4:
              for(q=1,j=0;j<en->nT;j++)q&=(relais[en->T[j]%1000]==(en->T[j]/1000));
              if(q)for(j=0;j<en->nR;j++)mod[en->R[j]]=1;
              break;
            case 5:
              for(q=1,j=0;j<en->nT;j++)q&=(relais[en->T[j]%1000]==(en->T[j]/1000));
              if(q)for(j=0;j<en->nR;j++)mod[en->R[j]]=0;
              break;
          }
        }
      }
      if(en->next==NULL)break;
      en=en->next;
    }
    for(q=0;q<TOTRELAIS;q++)if(mod[q]!=relais[q]){
      printf("R %d %d %d\n",q,relais[q],mod[q]);
      relais[q]=mod[q];
    }
  }


}
