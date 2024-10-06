#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include "time.h"
#include "fcntl.h"
#include "math.h"
#include "arpa/inet.h"
#include "sys/socket.h"
#include "unistd.h"
#define PORTWWW 55556
#define TOTEK 500
#define TOTEX 20
#define TOTRELAIS 120
#define LOGLEN 1000
#define LAT 44.5
#define LNG 11.3
#define CONFIG "config"
#define SAVESTATUS "status"

struct ek{
  uint16_t event;
  uint16_t nR;
  uint16_t *R;
  uint16_t nC;
  uint16_t *C;
  uint64_t *D;
  uint16_t nT;
  uint16_t *T;
  struct ek *next;
};
struct log{
  time_t time;
  uint8_t action;
  char desc[10];
};
struct ek *ee,*ex;
struct log *mylog;
uint8_t HHr,MMr,HHs,MMs,relais[TOTRELAIS];
uint16_t nevent;
time_t start;
#include "functions.c"

int main(){
  FILE *fp;
  char buf[100];
  uint8_t mod[TOTRELAIS];
  char *token,*f,*g,*mye;
  time_t myt;
  struct tm *info;
  uint16_t i,j,q,*lK,nlK,*lR,nlR,*lE,nlE,nlC,*lC,slC,*lT,nlT,last_min,last_hour,sched,every10,every30,esun;
  uint64_t *lD;
  struct ek *en,*em;
  int sockwww;
  unsigned int fromlen;
  struct sockaddr from;
  struct sockaddr_in server_addr;

  // parsing configuration file
  ee=(struct ek *)malloc(TOTEK*sizeof(struct ek));
  ex=(struct ek *)malloc(TOTEX*sizeof(struct ek));
  for(q=0;q<TOTEK;q++)ee[q].event=0;
  for(q=0;q<TOTEX;q++)ex[q].event=0;
  lK=(uint16_t *)malloc(100*sizeof(uint16_t));
  lR=(uint16_t *)malloc(100*sizeof(uint16_t));
  lE=(uint16_t *)malloc(100*sizeof(uint16_t));
  lC=(uint16_t *)malloc(100*sizeof(uint16_t));
  lD=(uint64_t *)malloc(23*sizeof(uint64_t));
  lT=(uint16_t *)malloc(100*sizeof(uint16_t));
  fp=fopen(CONFIG,"r");
  for(;;){
    fgets(buf,100,fp);
    if(feof(fp))break;
    nlK=nlR=nlE=nlC=nlT=0;
    nevent=1;
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
      if(en->event>0){
        for(;en->next!=NULL;en=en->next);
        em=(struct ek *)malloc(sizeof(struct ek));
        en->next=em;
        en=em;
      } 
      en->event=nevent++;
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
      if(en->event>0){
        for(;en->next!=NULL;en=en->next);
        em=(struct ek *)malloc(sizeof(struct ek));
        en->next=em;
        en=em;
      } 
      en->event=nevent++;
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
  time(&start);
  fromlen=sizeof(from);
  sockwww=socket(AF_INET,SOCK_DGRAM,0);
  fcntl(sockwww,F_SETFL,O_NONBLOCK);
  server_addr.sin_family=AF_INET;
  server_addr.sin_port=htons(PORTWWW);
  server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
  bind(sockwww,(struct sockaddr *)&server_addr,sizeof(server_addr));
  time(&myt);
  info=localtime(&myt);
  last_min=info->tm_min;
  last_hour=info->tm_hour;
  sched=0;
  every10=every30=100;
  sun(1900+info->tm_year,1+info->tm_mon,info->tm_mday,LAT,LNG,&HHr,&MMr,&HHs,&MMs);
  esun=0;
  fp=fopen(SAVESTATUS,"rb");
  fread(relais,sizeof(uint8_t),TOTRELAIS,fp);
  fclose(fp);
  en=ex; en->event=nevent; 
  en->nR=1; en->R=(uint16_t *)malloc(sizeof(uint16_t)); en->R[0]=0;
  en->nC=1; en->C=(uint16_t *)malloc(sizeof(uint16_t)); en->C[0]=0;
  en->D=(uint64_t *)malloc(23*sizeof(uint64_t)); for(q=0;q<23;q++)en->D[q]=0;
  en->nT=0; en->T=NULL;
  en->next=NULL;
  mylog=(struct log *)malloc(LOGLEN*sizeof(struct log));
  
  // receiving events
  for(;;){
    mye=managewww(sockwww);
    time(&myt);
    info=localtime(&myt);
    if(strlen(mye)>0)strcpy(buf,mye);
    else {
      if(info->tm_min!=last_min){
        last_min=info->tm_min;
        strcpy(buf,"E1");
      }
      else if(info->tm_hour!=last_hour){
        last_hour=info->tm_hour;
        strcpy(buf,"E2");
      }
      else if(every10!=info->tm_min && info->tm_min%10==0){
        every10=info->tm_min;
        strcpy(buf,"E3");
      }
      else if(every30!=info->tm_min && info->tm_min%30==0){
        every30=info->tm_min;
        strcpy(buf,"E4");
      }
      else if(sched!=5 && info->tm_hour==0 && info->tm_min==0){
        sched=5;
        sun(1900+info->tm_year,1+info->tm_mon,info->tm_mday,LAT,LNG,&HHr,&MMr,&HHs,&MMs);
        strcpy(buf,"E5");
      }
      else if(sched!=6 && info->tm_hour==6 && info->tm_min==0){
        sched=6;
        strcpy(buf,"E6");
      }
      else if(sched!=7 && info->tm_hour==12 && info->tm_min==0){
        sched=7;
        strcpy(buf,"E7");
      }
      else if(sched!=8 && info->tm_hour==18 && info->tm_min==0){
        sched=8;
        strcpy(buf,"E8");
      }
      else if(esun!=9 && info->tm_hour==HHr && info->tm_min==MMr){
        esun=9;
        strcpy(buf,"E9");
      }
      else if(esun!=10 && info->tm_hour==HHs && info->tm_min==MMs){
        esun=10;
        strcpy(buf,"E10");
      }
      else {
        usleep(10000);
        continue;
      }
    }
    printf("input: %s\n",buf);
    if(buf[0]=='K'){
      f=strchr(buf,','); *f='\0';
      q=10*atoi(buf+1)+atoi(f+1);
      en=ee+q;
    }
    else if(buf[0]=='E'){
      q=atoi(buf+1);
      en=ex+q;
    }
    else continue;

    // processing
    for(q=0;q<TOTRELAIS;q++)mod[q]=relais[q];
    for(;;){
      if(en->event==0)break;
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
