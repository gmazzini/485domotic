#define VERSION "domotic v3.0 by GM @2026\n"
#define LOGLEN 1000
#define PAGE 500
#define PI 3.1415926
#define ZENITH 1
#define RELAY_PORT 4196

static int relay_fd=-1;
struct sockaddr_in6 from;
socklen_t fromlen=sizeof(from);
char *cmd[]={"","onoff","on","off","condon","condoff","set"};

static unsigned short modbus_crc16(unsigned char *data,int len){
  unsigned short crc;
  int i,b;

  crc=0xffff;
  for(i=0;i<len;i++){
    crc^=data[i];
    for(b=0;b<8;b++){
      if(crc&1)crc=(crc>>1)^0xa001;
      else crc=crc>>1;
    }
  }
  return crc;
}

static int relay_socket(){
  struct timeval tv;

  if(relay_fd>=0)return relay_fd;

  relay_fd=socket(AF_INET,SOCK_DGRAM,0);
  if(relay_fd<0)return -1;

  tv.tv_sec=0;
  tv.tv_usec=200000;
  setsockopt(relay_fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

  return relay_fd;
}

int parse_relais(char *s,uint16_t *v){
  int i,n;

  if(s==NULL)return 0;
  if(s[0]=='R')s++;

  for(i=0;i<3;i++){
    if(s[i]<'0' || s[i]>'9')return 0;
  }

  if(s[3]!='\0')return 0;

  n=(s[0]-'0')*100+(s[1]-'0')*10+(s[2]-'0');
  *v=(uint16_t)n;

  return 1;
}

void relais_code(uint16_t r,char *code){
  sprintf(code,"%03u",(unsigned int)r);
}

void setrelais(char *code,int on){
  struct sockaddr_in addr;
  unsigned char frame[8];
  unsigned short coil,value,crc;
  int dev,relais;

  if(code==NULL)return;
  if(relay_socket()<0)return;

  dev=code[0]-'0';
  relais=((code[1]-'0')*10)+(code[2]-'0');
  if(relais<1)return;

  coil=relais-1;
  value=on?0xff00:0x0000;

  frame[0]=0x01;
  frame[1]=0x05;
  frame[2]=(unsigned char)(coil>>8);
  frame[3]=(unsigned char)(coil&0xff);
  frame[4]=(unsigned char)(value>>8);
  frame[5]=(unsigned char)(value&0xff);

  crc=modbus_crc16(frame,6);
  frame[6]=(unsigned char)(crc&0xff);
  frame[7]=(unsigned char)(crc>>8);

  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_port=htons(RELAY_PORT);
  addr.sin_addr.s_addr=htonl(0x0a0a0a0a+dev);

  sendto(relay_fd,frame,8,0,(struct sockaddr *)&addr,sizeof(addr));
}

int readrelais(char *code){
  struct sockaddr_in addr;
  unsigned char frame[8],resp[32];
  unsigned short coil,crc;
  int dev,relais,n;
  socklen_t addrlen;

  if(code==NULL)return 2;
  if(relay_socket()<0)return 2;

  dev=code[0]-'0';
  relais=((code[1]-'0')*10)+(code[2]-'0');
  if(relais<1)return 2;

  coil=relais-1;

  frame[0]=0x01;
  frame[1]=0x01;
  frame[2]=(unsigned char)(coil>>8);
  frame[3]=(unsigned char)(coil&0xff);
  frame[4]=0x00;
  frame[5]=0x01;

  crc=modbus_crc16(frame,6);
  frame[6]=(unsigned char)(crc&0xff);
  frame[7]=(unsigned char)(crc>>8);

  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_port=htons(RELAY_PORT);
  addr.sin_addr.s_addr=htonl(0x0a0a0a0a+dev);

  sendto(relay_fd,frame,8,0,(struct sockaddr *)&addr,sizeof(addr));

  addrlen=sizeof(addr);
  n=recvfrom(relay_fd,resp,sizeof(resp),0,(struct sockaddr *)&addr,&addrlen);

  if(n<4)return 2;

  return (resp[3]&1)?1:0;
}

int effective_relais(uint16_t relay,uint16_t *modR,uint8_t *modS,uint16_t nmod){
  char code[4];
  uint16_t i;
  int s;

  for(i=0;i<nmod;i++){
    if(modR[i]==relay)return modS[i];
  }

  relais_code(relay,code);
  s=readrelais(code);

  if(s==1)return 1;
  return 0;
}

void plan_relais(uint16_t relay,uint8_t state,uint16_t *modR,uint8_t *modS,uint16_t *nmod){
  uint16_t i;

  for(i=0;i<*nmod;i++){
    if(modR[i]==relay){
      modS[i]=state;
      return;
    }
  }

  if(*nmod<MAXEVENTRELAIS){
    modR[*nmod]=relay;
    modS[*nmod]=state;
    (*nmod)++;
  }
}

void add_event(struct es **head,uint16_t event,time_t when){
  struct es *e,*p,*n;

  e=(struct es *)malloc(sizeof(struct es));
  if(e==NULL)return;

  e->event=event;
  e->time=when;
  e->next=NULL;

  if(*head==NULL || (*head)->time>when){
    e->next=*head;
    *head=e;
    return;
  }

  p=*head;
  n=(*head)->next;

  while(n!=NULL && n->time<=when){
    p=n;
    n=n->next;
  }

  p->next=e;
  e->next=n;
}

int pop_event(struct es **head,time_t now,uint16_t *event){
  struct es *e;

  if(*head==NULL)return 0;
  if((*head)->time>now)return 0;

  e=*head;
  *event=e->event;
  *head=e->next;
  free(e);

  return 1;
}

void myout(int sock,int end,char *format,...){
  static char out[PAGE*3];
  unsigned int len;
  va_list argptr;

  if(end==0)*out='\0';

  va_start(argptr,format);
  vsprintf(out+strlen(out),format,argptr);
  va_end(argptr);

  len=strlen(out);

  if(len>PAGE || end==2){
    if(end==1)sprintf(out+strlen(out),"<next>");
    else sprintf(out+strlen(out),"<end>");

    fromlen=sizeof(from);
    sendto(sock,out,strlen(out),0,(struct sockaddr *)&from,fromlen);
    *out='\0';
  }
}

static void show_relais_state(int sock,uint16_t relay){
  char code[4];
  int s;

  relais_code(relay,code);
  s=readrelais(code);

  if(s==0)myout(sock,1,"R%s off\n",code);
  else if(s==1)myout(sock,1,"R%s on\n",code);
  else myout(sock,1,"R%s unreadable\n",code);
}

char *managewww(int sock){
  static char ret[50];
  char buf[100],code[4],*t1,*t2;
  int rr,quit;
  uint16_t i,j,k,q,dis,totdis,fb,fe,relay;
  uint64_t flag;
  FILE *fp;
  time_t myt;
  struct tm info;
  struct ek *en;

  *ret='\0';
  quit=0;

  fromlen=sizeof(from);
  rr=recvfrom(sock,buf,99,0,(struct sockaddr *)&from,&fromlen);
  if(rr<1)return ret;

  for(i=0;i<totwhite;i++){
    if(memcmp(&(from.sin6_addr),&white[i],sizeof(struct in6_addr))==0)break;
  }
  if(i==totwhite)return ret;

  buf[rr]='\0';

  myout(sock,0,VERSION);
  myout(sock,1,"cmd: %s\n",buf);

  t1=strtok(buf," \n\r\t");
  t2=strtok(NULL," \n\r\t");

  if(t1==NULL){
    myout(sock,2,"empty command\n");
    return ret;
  }

  if(strcmp(t1,"status")==0){
    myout(sock,1,"max event relais: %d\n",MAXEVENTRELAIS);
    myout(sock,1,"events: %d\n",nevent);

    time(&myt);
    memcpy(&info,localtime(&myt),sizeof(struct tm));
    strftime(buf,100,"%d/%m/%y %H:%M:%S %A",&info);
    myout(sock,1,"time now: %s\n",buf);

    memcpy(&info,localtime(&start),sizeof(struct tm));
    strftime(buf,100,"%d/%m/%y %H:%M:%S %A",&info);
    myout(sock,1,"time start: %s\n",buf);

    myout(sock,1,"time sunrise: %02d:%02d\n",HHr,MMr);
    myout(sock,1,"time sunset: %02d:%02d\n",HHs,MMs);
    myout(sock,2,"log dimension: %d\n",(fulllog)?LOGLEN:poslog);
  }
  else if(strcmp(t1,"seton")==0){
    if(parse_relais(t2,&relay)){
      relais_code(relay,code);
      myout(sock,2,"set relais to on: R%s\n",code);
      en=ex;
      en->R[0]=relay;
      en->nC=1;
      en->C[0]=2;
      strcpy(ret,"E0");
    }
    else myout(sock,2,"bad relais\n");
  }
  else if(strcmp(t1,"setoff")==0){
    if(parse_relais(t2,&relay)){
      relais_code(relay,code);
      myout(sock,2,"set relais to off: R%s\n",code);
      en=ex;
      en->R[0]=relay;
      en->nC=1;
      en->C[0]=3;
      strcpy(ret,"E0");
    }
    else myout(sock,2,"bad relais\n");
  }
  else if(strcmp(t1,"read")==0){
    if(parse_relais(t2,&relay)){
      show_relais_state(sock,relay);
      myout(sock,2,"");
    }
    else myout(sock,2,"bad relais\n");
  }
  else if(strcmp(t1,"showon")==0){
    myout(sock,2,"showon disabled: use read Rabc\n");
  }
  else if(strcmp(t1,"showevents")==0){
    for(q=0;q<TOTEK+TOTEX;q++){
      if(q<TOTEK)en=ee+q;
      else en=ex+q-TOTEK;

      for(;;){
        if(en->event==0)break;

        if(q<TOTEK)myout(sock,1,"---- EK K%d,%d %d ----\n",q/10,q%10,en->event);
        else myout(sock,1,"---- EX E%d %d ----\n",q-TOTEK,en->event);

        if(en->nR>0){
          for(j=0;j<en->nR;j++){
            relais_code(en->R[j],code);
            myout(sock,1,"R%s ",code);
          }
          myout(sock,1,"\n");
        }

        if(en->nC>0){
          for(j=0;j<en->nC;j++)myout(sock,1,"C%s ",cmd[en->C[j]]);
          myout(sock,1,"\n");
        }

        totdis=0;
        dis=1;

        for(j=0;j<1440;j++){
          flag=en->D[j/64]&mask[j%64];
          if((flag>0 && dis) || (flag==0 && !dis)){
            totdis++;
            dis=1-dis;
            myout(sock,1,"D%02d%02d ",(j-dis)/60,(j-dis)%60);
          }
        }

        if(totdis)myout(sock,1,"\n");

        if(en->nT>0){
          for(j=0;j<en->nT;j++){
            relais_code(en->T[j]%1000,code);
            myout(sock,1,"T%s,%d ",code,en->T[j]/1000);
          }
          myout(sock,1,"\n");
        }

        if(en->nS>0){
          for(j=0;j<en->nS;j++)myout(sock,1,"S%d,%d ",en->Se[j],en->St[j]);
          myout(sock,1,"\n");
        }

        if(en->next==NULL)break;
        en=en->next;
      }
    }

    myout(sock,2,"");
  }
  else if(strcmp(t1,"inject")==0){
    if(t2!=NULL){
      myout(sock,2,"inject: %s\n",t2);
      strcpy(ret,t2);
    }
    else myout(sock,2,"missing event\n");
  }
  else if(strcmp(t1,"showlog")==0){
    if(t2!=NULL)k=atoi(t2)%LOGLEN;
    else k=10;

    i=0;
    fb=(fulllog)?poslog-1+LOGLEN:poslog-1;
    fe=(fulllog)?poslog:0;

    for(q=fb;q>=fe && i<k;q--){
      j=q%LOGLEN;
      memcpy(&info,localtime(&mylog[j].time),sizeof(struct tm));
      strftime(buf,100,"%d/%m/%y %H:%M:%S %A",&info);
      myout(sock,1,"%s %03d %d %s\n",buf,j,mylog[j].action,mylog[j].desc);
      i++;
    }

    myout(sock,2,"End showlog of %03d entries, total %03d\n",i,(fulllog)?LOGLEN:poslog);
  }
  else if(strcmp(t1,"quit")==0){
    myout(sock,2,"quitting\n");
    quit=1;
  }
  else if(strcmp(t1,"help")==0){
    myout(sock,1,"status, show actual status informations\n");
    myout(sock,1,"seton Rabc, set relay Rabc to on\n");
    myout(sock,1,"setoff Rabc, set relay Rabc to off\n");
    myout(sock,1,"read Rabc, read relay Rabc state\n");
    myout(sock,1,"showon, disabled: use read Rabc\n");
    myout(sock,1,"showevents, show all the events\n");
    myout(sock,1,"inject xxx, inject the xxx event, like K1,2 or E5\n");
    myout(sock,1,"showlog n, show rotative log last n lines\n");
    myout(sock,1,"quit, shutdown the system\n");
    myout(sock,2,"help, this help\n");
  }
  else myout(sock,2,"command not found\n");

  if(quit==1){
    fp=fopen(SAVELOG,"wb");
    if(fp!=NULL){
      fwrite(&poslog,sizeof(uint16_t),1,fp);
      fwrite(&fulllog,sizeof(uint8_t),1,fp);
      fwrite(mylog,sizeof(struct log),LOGLEN,fp);
      fclose(fp);
    }

    usleep(2000000);
    exit(0);
  }

  return ret;
}

void sun(int year,int month,int day,float lat,float lng,uint8_t *HHr,uint8_t *MMr,uint8_t *HHs,uint8_t *MMs){
  float N1,N2,N3,N,lngHour,tr,ts,Mr,Ms,Lr,Ls;
  float RAr,RAs,Lquadrantr,Lquadrants,RAquadrantr,RAquadrants;
  float sinDecr,sinDecs,cosDecr,cosDecs,cosHr,cosHs,Hr,Hs,Tr,Ts,UTr,UTs;
  time_t myt;
  struct tm loctime,gmttime;
  int delta;

  N1=floor(275*month/9);
  N2=floor((month+9)/12);
  N3=(1+floor((year-4*floor(year/4)+2)/3));
  N=N1-(N2*N3)+day-30;
  lngHour=lng/15.0;

  tr=N+((6-lngHour)/24);
  ts=N+((18-lngHour)/24);

  Mr=(0.9856*tr)-3.289;
  Ms=(0.9856*ts)-3.289;

  Lr=fmod(Mr+(1.916*sin((PI/180)*Mr))+(0.020*sin(2*(PI/180)*Mr))+282.634,360.0);
  Ls=fmod(Ms+(1.916*sin((PI/180)*Ms))+(0.020*sin(2*(PI/180)*Ms))+282.634,360.0);

  RAr=fmod(180/PI*atan(0.91764*tan((PI/180)*Lr)),360.0);
  RAs=fmod(180/PI*atan(0.91764*tan((PI/180)*Ls)),360.0);

  Lquadrantr=floor(Lr/90)*90;
  Lquadrants=floor(Ls/90)*90;
  RAquadrantr=floor(RAr/90)*90;
  RAquadrants=floor(RAs/90)*90;

  RAr=RAr+(Lquadrantr-RAquadrantr);
  RAs=RAs+(Lquadrants-RAquadrants);

  RAr=RAr/15;
  RAs=RAs/15;

  sinDecr=0.39782*sin((PI/180)*Lr);
  sinDecs=0.39782*sin((PI/180)*Ls);

  cosDecr=cos(asin(sinDecr));
  cosDecs=cos(asin(sinDecs));

  cosHr=(sin((PI/180)*ZENITH)-(sinDecr*sin((PI/180)*lat)))/(cosDecr*cos((PI/180)*lat));
  cosHs=(sin((PI/180)*ZENITH)-(sinDecs*sin((PI/180)*lat)))/(cosDecs*cos((PI/180)*lat));

  Hr=360-(180/PI)*acos(cosHr);
  Hs=(180/PI)*acos(cosHs);

  Hr=Hr/15;
  Hs=Hs/15;

  Tr=Hr+RAr-(0.06571*tr)-6.622;
  Ts=Hs+RAs-(0.06571*ts)-6.622;

  time(&myt);
  memcpy(&loctime,localtime(&myt),sizeof(struct tm));
  memcpy(&gmttime,gmtime(&myt),sizeof(struct tm));

  delta=fmod(24.0+loctime.tm_hour-gmttime.tm_hour,24.0);

  UTr=fmod(24.0+Tr-lngHour+delta,24.0);
  UTs=fmod(24.0+Ts-lngHour+delta,24.0);

  *HHr=(int)UTr;
  *MMr=(int)((UTr-(int)UTr)*60);
  *HHs=(int)UTs;
  *MMs=(int)((UTs-(int)UTs)*60);
}

void inslog(time_t myt,int action,char *desc){
  mylog[poslog].time=myt;
  mylog[poslog].action=action;
  strncpy(mylog[poslog].desc,desc,11);
  mylog[poslog].desc[11]='\0';

  poslog++;
  if(poslog>=LOGLEN){
    poslog=0;
    fulllog=1;
  }
}
