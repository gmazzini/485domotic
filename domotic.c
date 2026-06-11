#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdarg.h>

#define PORT 55556
#define TOTEK 1000
#define TOTEX 500
#define MAXEVENTRELAIS 500
#define TOTWHITE 50
#define MAXKMAP 500
#define KDEVLEN 64
#define KACTLEN 32
#define LABELLEN 64
#define MAXKNOWNRELAIS 500
#define LAT 44.5
#define LNG 11.3
#define CONFIG "/home/tools/485domotic/config"
#define SAVELOG "/home/tools/misc/domotic.log"
#define WHITEACCESS "/home/tools/misc/domotic.access"

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
  char label[LABELLEN];
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

struct kmap{
  char dev[KDEVLEN];
  char action[KACTLEN];
  uint16_t key;
};

struct ek *ee,*ex;
struct log *mylog;
struct kmap kmap[MAXKMAP];
uint16_t knownrelais[MAXKNOWNRELAIS];
uint8_t HHr,MMr,HHs,MMs,fulllog;
uint16_t nevent,poslog,totwhite,nkmap,nknownrelais;
time_t start,last_reload;
uint16_t last_reload_error;
uint64_t mask[64];
struct acl{
  struct in6_addr addr;
  uint8_t prefix;
};
struct acl white[TOTWHITE];

#include "functions.c"

int main(){
  FILE *fp;
  char buf[100],code[4];
  uint8_t modS[MAXEVENTRELAIS];
  char *mye;
  time_t myt;
  struct tm info;
  uint16_t j,q,last_min,last_hour,sched,every10,every30,esun,event,nmod,*modR,relay,errline;
  struct ek *en;
  struct es *es;
  int sockwww,state,oncount,ok,before;
  struct sockaddr_in6 server_addr;

  ee=(struct ek *)malloc(TOTEK*sizeof(struct ek));
  ex=(struct ek *)malloc(TOTEX*sizeof(struct ek));
  es=NULL;

  memset(ee,0,TOTEK*sizeof(struct ek));
  memset(ex,0,TOTEX*sizeof(struct ek));

  for(q=0;q<64;q++)mask[q]=1ULL<<q;

  errline=load_config();

  modR=(uint16_t *)malloc(MAXEVENTRELAIS*sizeof(uint16_t));

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
  every10=100;
  every30=100;
  esun=0;

  sun(1900+info.tm_year,1+info.tm_mon,info.tm_mday,LAT,LNG,&HHr,&MMr,&HHs,&MMs);

  mylog=(struct log *)malloc(LOGLEN*sizeof(struct log));

  totwhite=0;
  fp=fopen(WHITEACCESS,"rt");
  if(fp!=NULL){
    for(totwhite=0;totwhite<TOTWHITE;){
      if(fgets(buf,100,fp)==NULL)break;
      trim_line(buf);
      if(buf[0]=='\0')continue;
      if(buf[0]=='#')continue;
      if(parse_acl_line(buf,&white[totwhite]))totwhite++;
    }
    fclose(fp);
  }
  
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

  for(;;){
    time(&myt);
    memcpy(&info,localtime(&myt),sizeof(struct tm));

    mye=managewww(sockwww);

    if(strcmp(mye,"RELOAD")==0){
      clear_pending_events(&es);
      clear_config();
      errline=load_config();

      if(errline==0){
        myout(sockwww,2,"config loaded\n");
        inslog(myt,1,"reload ok");
      }
      else {
        myout(sockwww,2,"config error at line %d\n",errline);
        sprintf(buf,"reload %d",errline);
        inslog(myt,1,buf);
      }

      continue;
    }

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
      else if(pop_event(&es,myt,&event)){
        sprintf(buf,"E%d",event);
      }
      else {
        usleep(10000);
        continue;
      }
    }

    if(buf[0]=='K'){
      if(!parse_key(buf+1,&q))continue;
      en=ee+q;
    }
    else if(buf[0]=='E'){
      q=atoi(buf+1);
      en=ex+q;
    }
    else continue;

    nmod=0;

    for(;;){
      if(en->event==0)break;

      j=info.tm_hour*60+info.tm_min;

      if((en->D[j/64]&mask[j%64])==0){
        for(j=0;j<en->nC;j++){
          switch(en->C[j]){
            case 1:
              oncount=0;
              for(q=0;q<en->nR;q++)oncount+=effective_relais(en->R[q],modR,modS,nmod);
              state=(oncount>(en->nR/2))?0:1;
              for(q=0;q<en->nR;q++)plan_relais(en->R[q],state,modR,modS,&nmod);
              break;

            case 2:
              for(q=0;q<en->nR;q++)plan_relais(en->R[q],1,modR,modS,&nmod);
              break;

            case 3:
              for(q=0;q<en->nR;q++)plan_relais(en->R[q],0,modR,modS,&nmod);
              break;

            case 4:
              ok=1;
              for(q=0;q<en->nT;q++){
                relay=en->T[q]%1000;
                state=en->T[q]/1000;
                if(effective_relais(relay,modR,modS,nmod)!=state)ok=0;
              }
              if(ok){
                for(q=0;q<en->nR;q++)plan_relais(en->R[q],1,modR,modS,&nmod);
              }
              break;

            case 5:
              ok=1;
              for(q=0;q<en->nT;q++){
                relay=en->T[q]%1000;
                state=en->T[q]/1000;
                if(effective_relais(relay,modR,modS,nmod)!=state)ok=0;
              }
              if(ok){
                for(q=0;q<en->nR;q++)plan_relais(en->R[q],0,modR,modS,&nmod);
              }
              break;

            case 6:
              for(q=0;q<en->nS;q++)add_event(&es,en->Se[q],myt+en->St[q]);
              break;

            case 7:
              for(q=0;q<nknownrelais;q++){
                relay=knownrelais[q];
                if(excluded_relais(relay,en->R,en->nR))continue;
                if(effective_relais(relay,modR,modS,nmod)==1){
                  plan_relais(relay,0,modR,modS,&nmod);
                }
              }
              break;
            
          }
        }
      }

      if(en->next==NULL)break;
      en=en->next;
    }

    for(q=0;q<nmod;q++){
      relais_code(modR[q],code);
      before=readrelais(code);

      if(before==modS[q])continue;

      setrelais(code,modS[q]);

      if(before==2)sprintf(buf,"R%s ?->%d",code,modS[q]);
      else sprintf(buf,"R%s %d->%d",code,before,modS[q]);

      inslog(myt,3,buf);
    }
  }

  return 0;
}
