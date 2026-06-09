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
time_t start;
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
  char *token,*f,*mye;
  time_t myt;
  struct tm info;
  uint16_t i,j,q,*lK,nlK,*lR,nlR,*lE,nlE,nlC,*lC,slC,*lT,nlT,nlS,*lSe,*lSt;
  uint16_t last_min,last_hour,sched,every10,every30,esun,event,nmod,*modR,relay,key;
  uint64_t *lD;
  struct ek *en,*em;
  struct es *es;
  int sockwww,state,oncount,ok,before;
  struct sockaddr_in6 server_addr;

  for(q=0;q<64;q++)mask[q]=1ULL<<q;

  ee=(struct ek *)malloc(TOTEK*sizeof(struct ek));
  ex=(struct ek *)malloc(TOTEX*sizeof(struct ek));
  es=NULL;
  nkmap=0;
  nknownrelais=0;

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
  modR=(uint16_t *)malloc(MAXEVENTRELAIS*sizeof(uint16_t));

  fp=fopen(CONFIG,"r");
  nevent=1;

  if(fp!=NULL){
    for(;;){
      if(fgets(buf,100,fp)==NULL)break;
      if(strlen(buf)<5)continue;
      if(buf[0]=='#')continue;

      if(is_kmap_line(buf)){
        load_kmap_line(buf);
        continue;
      }

      if(is_rrange_line(buf)){
        load_rrange_line(buf);
        continue;
      }

      nlK=0;
      nlR=0;
      nlE=0;
      nlC=0;
      nlT=0;
      nlS=0;

      for(q=0;q<23;q++)lD[q]=0;

      for(token=strtok(buf," \n\r\t");token;token=strtok(NULL," \n\r\t")){
        switch(token[0]){
          case 'K':
            if(parse_key(token+1,&key))lK[nlK++]=key;
            break;

          case 'R':
            if(parse_relais(token+1,&relay))lR[nlR++]=relay;
            break;

          case 'E':
            lE[nlE++]=atoi(token+1);
            break;

          case 'D':
            f=strchr(token,',');
            if(f!=NULL){
              *f='\0';
              i=atoi(token+3);
              *(token+3)='\0';
              i+=atoi(token+1)*60;
              j=atoi(f+3);
              *(f+3)='\0';
              j+=atoi(f+1)*60;
              for(q=i;q<=j;q++)lD[q/64]|=mask[q%64];
            }
            break;

          case 'C':
            slC=0;
            for(q=1;q<=7;q++){
              if(strcmp(token+1,cmd[q])==0){
                slC=q;
                break;
              }
            }
            if(slC>0)lC[nlC++]=slC;
            break;

          case 'T':
            f=strchr(token,',');
            if(f!=NULL){
              *f='\0';
              if(parse_relais(token+1,&relay)){
                lT[nlT++]=relay+1000*atoi(f+1);
              }
            }
            break;

          case 'S':
            f=strchr(token,',');
            if(f!=NULL){
              *f='\0';
              lSe[nlS]=atoi(token+1);
              lSt[nlS++]=atoi(f+1);
            }
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
        en->St=(uint16_t *)malloc(nlS*sizeof(uint16_t));
        for(j=0;j<nlS;j++){
          en->Se[j]=lSe[j];
          en->St[j]=lSt[j];
        }

        en->next=NULL;
      }
    }

    fclose(fp);
  }

  sort_kmap();
  sort_known_relais();

  free(lK);
  free(lR);
  free(lE);
  free(lC);
  free(lD);
  free(lT);
  free(lSe);
  free(lSt);

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

  en=ex;
  en->event=nevent;
  en->nR=1;
  en->R=(uint16_t *)malloc(sizeof(uint16_t));
  en->R[0]=0;
  en->nC=1;
  en->C=(uint16_t *)malloc(sizeof(uint16_t));
  en->C[0]=0;
  en->D=(uint64_t *)malloc(23*sizeof(uint64_t));
  for(q=0;q<23;q++)en->D[q]=0;
  en->nT=0;
  en->T=NULL;
  en->nS=0;
  en->Se=NULL;
  en->St=NULL;
  en->next=NULL;

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
