#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdarg.h>
#include <termios.h>

#define PORT 55556
#define TOTEK 500
#define TOTEX 20
#define TOTES 100
#define TOTRELAIS 120
#define TOTWHITE 50
#define LAT 44.5
#define LNG 11.3
#define SERIAL "/dev/ttyAMA0"
#define CONFIG "config"
#define SAVESTATUS "status"
#define SAVELOG "log"
#define WHITEACCESS "access.v6"

struct ek{
  uint16_t event;
  uint16_t nR;
  uint16_t *R;
  uint16_t nC;
  uint16_t *C;
  uint64_t *D;
  uint16_t nT;
  uint16_t *T;
  uint16_t nS;
  uint16_t *Se;
  uint16_t *St;
  struct ek *next;
};
struct log{
  time_t time;
  uint8_t action;
  char desc[12];
};
struct es{
  uint16_t event;
  time_t time;
  struct es *next;
};
struct ek *ee,*ex;
struct log *mylog;
uint8_t HHr,MMr,HHs,MMs,relais[TOTRELAIS],fulllog;
uint16_t nevent,poslog,rr,totwhite;
time_t start;
uint64_t mask[64];
struct in6_addr white[TOTWHITE];
#include "functions.c"

int main(){
  FILE *fp;
  char buf[100];
  uint8_t mod[TOTRELAIS];
  char *token,*f,*g,*mye;
  time_t myt;
  struct tm info;
  uint16_t i,j,q,*lK,nlK,*lR,nlR,*lE,nlE,nlC,*lC,slC,*lT,nlT,nlS,*lSe,*lSt,last_min,last_hour,sched,every10,every30,esun;
  uint64_t *lD;
  struct ek *en,*em;
  struct es *es,*esa,*esb;
  int sockwww,fd;
  struct sockaddr_in6 server_addr;

  // processing the configuration file
  for(q=0;q<64;q++)mask[q]=1ULL<<q;
  ee=(struct ek *)malloc(TOTEK*sizeof(struct ek));
  ex=(struct ek *)malloc(TOTEX*sizeof(struct ek));
  es=NULL;
  for(q=0;q<TOTEK;q++)ee[q].event=0;
  for(q=0;q<TOTEX;q++)ex[q].event=0;
  lK=(uint16_t *)malloc(100*sizeof(uint16_t));
  lR=(uint16_t *)malloc(100*sizeof(uint16_t));
  lE=(uint16_t *)malloc(100*sizeof(uint16_t));
  lC=(uint16_t *)malloc(100*sizeof(uint16_t));
  lD=(uint64_t *)malloc(23*sizeof(uint64_t));
  lT=(uint16_t *)malloc(100*sizeof(uint16_t));
  lSe=(uint16_t *)malloc(100*sizeof(uint16_t));
  lSt=(uint16_t *)malloc(100*sizeof(uint16_t));
  fp=fopen(CONFIG,"r");
  nevent=1;
  for(;;){
    fgets(buf,100,fp);
    if(feof(fp))break;
    if(strlen(buf)<5)continue;
    if(buf[0]=='#')continue;
    nlK=nlR=nlE=nlC=nlT=nlS=0;
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
          i=atoi(token+3); *(token+3)='\0'; i+=atoi(token+1)*60;
          j=atoi(f+3); *(f+3)='\0'; j+=atoi(f+1)*60;
          for(q=i;q<=j;q++)lD[q/64]|=mask[q%64];
          break;
        case 'C':
          slC=0;
          for(q=1;q<=6;q++)if(strcmp(token+1,cmd[q])==0){slC=q; break;}
          if(slC>0)lC[nlC++]=slC;
          break;
        case 'T':
          f=strchr(token,','); *f='\0'; g=strchr(f+1,','); *g='\0';
          lT[nlT++]=10*atoi(token+1)+atoi(f+1)+1000*atoi(g+1);
          break;
        case 'S':
          f=strchr(token,','); *f='\0';
          lSe[nlS]=atoi(token+1);
          lSt[nlS++]=atoi(f+1);
          printf("-- %d %d %d\n",nlS,lSe[nlS-1],lSt[nlS-1]);
          break;
      }
    }
    for(q=0;q<nlK+nlE;q++){
      if(q<nlK)en=ee+lK[q];
      else en=ex+lE[q-nlK]; 
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
      en->nS=nlS;
      en->Se=(uint16_t *)malloc(nlS*sizeof(uint16_t));
      for(j=0;j<nlS;j++)en->Se[j]=lSe[j];
      en->St=(uint16_t *)malloc(nlS*sizeof(uint16_t));
      for(j=0;j<nlS;j++)en->St[j]=lSt[j];        
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
  free(lSe);
  free(lSt);

  // initilize
  time(&start);
  sockwww=socket(PF_INET6,SOCK_DGRAM,0);
  fcntl(sockwww,F_SETFL,O_NONBLOCK);
  server_addr.sin6_family=AF_INET6;
  server_addr.sin6_port=htons(PORT);
  server_addr.sin6_addr=in6addr_any;
  bind(sockwww,(struct sockaddr *)&server_addr,sizeof(server_addr));
  
  time(&myt);
  memcpy(&info,localtime(&myt),sizeof(struct tm));
  last_min=info.tm_min;
  last_hour=info.tm_hour;
  
  sched=0;
  every10=every30=100;
  sun(1900+info.tm_year,1+info.tm_mon,info.tm_mday,LAT,LNG,&HHr,&MMr,&HHs,&MMs);
  esun=0;

  fp=fopen(SAVESTATUS,"rb");
  if(fp!=NULL){
    fread(relais,sizeof(uint8_t),TOTRELAIS,fp);
    fclose(fp);
  }
  else for(i=0;i<TOTRELAIS;i++)relais[i]=0;
  
  en=ex; en->event=nevent; 
  en->nR=1; en->R=(uint16_t *)malloc(sizeof(uint16_t)); en->R[0]=0;
  en->nC=1; en->C=(uint16_t *)malloc(sizeof(uint16_t)); en->C[0]=0;
  en->D=(uint64_t *)malloc(23*sizeof(uint64_t)); for(q=0;q<23;q++)en->D[q]=0;
  en->nT=0; en->T=NULL;
  en->next=NULL;
  mylog=(struct log *)malloc(LOGLEN*sizeof(struct log));

  totwhite=0;
  fp=fopen(WHITEACCESS,"rt");
  for(totwhite=0;;){
    if(fgets(buf,100,fp)==NULL)break;
    i=strlen(buf);
    if(i>10){
      buf[i-1]='\0';
      if(inet_pton(AF_INET6,buf,&white[totwhite])==1)totwhite++;
    }
  }
  fclose(fp);
  
  fp=fopen(SAVELOG,"rb");
  if(fp!=NULL){
    fread(&poslog,sizeof(uint16_t),1,fp);
    fread(&fulllog,sizeof(uint8_t),1,fp);
    fread(mylog,sizeof(struct log),LOGLEN,fp);
    fclose(fp);
  }
  else {
    poslog=0;
    fulllog=0;
  }
  fd=open(SERIAL,O_RDWR);
  setserial(fd);
  
  // receiving events
  for(;;){
    time(&myt);
    memcpy(&info,localtime(&myt),sizeof(struct tm));
    mye=managewww(sockwww,fd);
    if(strlen(mye)>0){
      strcpy(buf,mye);
      inslog(myt,1,buf);
    }
    else {
      if(info.tm_min!=last_min){
        last_min=info.tm_min;
        strcpy(buf,"E1");
      }
      else if(info.tm_hour!=last_hour){
        last_hour=info.tm_hour;
        strcpy(buf,"E2");
      }
      else if(every10!=info.tm_min && info.tm_min%10==0){
        every10=info.tm_min;
        strcpy(buf,"E3");
      }
      else if(every30!=info.tm_min && info.tm_min%30==0){
        every30=info.tm_min;
        strcpy(buf,"E4");
      }
      else if(sched!=5 && info.tm_hour==0 && info.tm_min==0){
        sched=5;
        sun(1900+info.tm_year,1+info.tm_mon,info.tm_mday,LAT,LNG,&HHr,&MMr,&HHs,&MMs);
        strcpy(buf,"E5");
      }
      else if(sched!=6 && info.tm_hour==6 && info.tm_min==0){
        sched=6;
        strcpy(buf,"E6");
      }
      else if(sched!=7 && info.tm_hour==12 && info.tm_min==0){
        sched=7;
        strcpy(buf,"E7");
      }
      else if(sched!=8 && info.tm_hour==18 && info.tm_min==0){
        sched=8;
        strcpy(buf,"E8");
      }
      else if(esun!=9 && info.tm_hour==HHr && info.tm_min==MMr){
        esun=9;
        strcpy(buf,"E9");
      }
      else if(esun!=10 && info.tm_hour==HHs && info.tm_min==MMs){
        esun=10;
        strcpy(buf,"E10");
      }
      else {
        for(esa=esb=es;esa!=NULL && esa->time>myt;esa=esa->next)esb=esa;
        if(esa!=NULL){
          printf("-- C %d\n",esa->event);
          sprintf(buf,"E%d",esa->event);
          esb->next=esa->next;
          free(esa);
          continue;
        }
        rr=myread(fd);
        if(rr && rr%2){
          sprintf(buf,"K%d,%d",rr/256,(rr%256)>>4);
          inslog(myt,2,buf);
        }
        else {
          usleep(10000);
          continue;
        }
      }
    }
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
      j=info.tm_hour*60+info.tm_min;
      if((en->D[j/64] & mask[j%64])==0){
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
            case 6:
              for(j=0;j<en->nS;j++){
                printf("-- I %d %d %d\n",en->nS,en->Se[j],en->St[j]);
                if(es==NULL)es=esa=(struct es *)malloc(sizeof(struct es));
                else {
                  for(esa=es;esa->next!=NULL;esa=esa->next);
                  esa->next=(struct es *)malloc(sizeof(struct es));
                  esa=esa->next;
                }
                esa->next=NULL;
                esa->time=myt+en->St[j];
                esa->event=en->Se[j];
              }
              for(esa=es;esa!=NULL;esa=esa->next)printf("-- V %d %ld\n",esa->event,esa->time);
              break;
          }
        }
      }
      if(en->next==NULL)break;
      en=en->next;
    }
    for(q=0;q<TOTRELAIS;q++)if(mod[q]!=relais[q]){
      myset(fd,q/10,(q%10)*16+mod[q]);
      sprintf(buf,"R%d,%d,%d->%d",q/10,q%10,relais[q],mod[q]);
      inslog(myt,3,buf);
      relais[q]=mod[q];
    }
  }
}
