#define VERSION "domotic v3.6 by GM @2026\n"
#define LOGLEN 1000
#define PAGE 500
#define PI 3.1415926
#define ZENITH 1
#define RELAY_PORT 4196
#define RELAY_CONNECT_TIMEOUT_US 150000
#define RELAY_IO_TIMEOUT_US 150000
#define RELAY_SET_REPLY_TIMEOUT_US 80000

struct sockaddr_in6 from;
socklen_t fromlen=sizeof(from);
char *cmd[]={"","onoff","on","off","condon","condoff","set","alloff"};

void trim_line(char *s){
  int i,n;

  if(s==NULL)return;

  n=strlen(s);
  while(n>0 && (s[n-1]=='\n' || s[n-1]=='\r' || s[n-1]==' ' || s[n-1]=='\t')){
    s[n-1]='\0';
    n--;
  }

  i=0;
  while(s[i]==' ' || s[i]=='\t')i++;

  if(i>0)memmove(s,s+i,strlen(s+i)+1);
}

int parse_acl_line(char *line,struct acl *a){
  char buf[100],*p;
  int prefix;

  if(line==NULL || a==NULL)return 0;

  strncpy(buf,line,sizeof(buf)-1);
  buf[sizeof(buf)-1]='\0';

  p=strchr(buf,'/');
  prefix=128;

  if(p!=NULL){
    *p='\0';
    p++;
    prefix=atoi(p);
    if(prefix<0 || prefix>128)return 0;
  }

  if(inet_pton(AF_INET6,buf,&a->addr)!=1)return 0;

  a->prefix=(uint8_t)prefix;
  return 1;
}

int acl_match_addr(struct in6_addr *ip,struct acl *a){
  int full,rem,i;
  unsigned char mask;

  if(ip==NULL || a==NULL)return 0;

  full=a->prefix/8;
  rem=a->prefix%8;

  for(i=0;i<full;i++){
    if(ip->s6_addr[i]!=a->addr.s6_addr[i])return 0;
  }

  if(rem){
    mask=(unsigned char)(0xff<<(8-rem));
    if((ip->s6_addr[full]&mask)!=(a->addr.s6_addr[full]&mask))return 0;
  }

  return 1;
}

int acl_allowed(struct in6_addr *ip){
  uint16_t i;

  if(ip==NULL)return 0;

  for(i=0;i<totwhite;i++){
    if(acl_match_addr(ip,&white[i]))return 1;
  }

  return 0;
}

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

static int relay_wait_fd(int fd,int writeflag,int usec){
  fd_set rfds,wfds;
  struct timeval tv;
  int r;

  FD_ZERO(&rfds);
  FD_ZERO(&wfds);

  if(writeflag)FD_SET(fd,&wfds);
  else FD_SET(fd,&rfds);

  tv.tv_sec=usec/1000000;
  tv.tv_usec=usec%1000000;

  if(writeflag)r=select(fd+1,NULL,&wfds,NULL,&tv);
  else r=select(fd+1,&rfds,NULL,NULL,&tv);

  if(r>0)return 1;
  return 0;
}

static int relay_ip_from_code(char *code,struct sockaddr_in *addr){
  int dev;

  if(code==NULL)return 0;

  dev=code[0]-'0';
  if(dev<0 || dev>9)return 0;

  memset(addr,0,sizeof(struct sockaddr_in));
  addr->sin_family=AF_INET;
  addr->sin_port=htons(RELAY_PORT);
  addr->sin_addr.s_addr=htonl(0x0a0a0a0a+dev);

  return 1;
}

static int relay_connect(char *code){
  struct sockaddr_in addr;
  int fd,flags,err;
  socklen_t len;

  if(!relay_ip_from_code(code,&addr))return -1;

  fd=socket(AF_INET,SOCK_STREAM,0);
  if(fd<0)return -1;

  flags=fcntl(fd,F_GETFL,0);
  if(flags<0){
    close(fd);
    return -1;
  }

  if(fcntl(fd,F_SETFL,flags|O_NONBLOCK)<0){
    close(fd);
    return -1;
  }

  if(connect(fd,(struct sockaddr *)&addr,sizeof(addr))<0){
    if(errno!=EINPROGRESS){
      close(fd);
      return -1;
    }

    if(!relay_wait_fd(fd,1,RELAY_CONNECT_TIMEOUT_US)){
      close(fd);
      return -1;
    }

    err=0;
    len=sizeof(err);
    if(getsockopt(fd,SOL_SOCKET,SO_ERROR,&err,&len)<0 || err!=0){
      close(fd);
      return -1;
    }
  }

  return fd;
}

static int relay_send_all(int fd,unsigned char *buf,int len,int usec){
  int sent,n;

  sent=0;

  while(sent<len){
    if(!relay_wait_fd(fd,1,usec))return sent;

    n=send(fd,buf+sent,len-sent,0);
    if(n<0){
      if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR)continue;
      return sent;
    }
    if(n==0)return sent;

    sent+=n;
  }

  return sent;
}

static int relay_recv_wait(int fd,unsigned char *buf,int len,int usec){
  int n;

  if(!relay_wait_fd(fd,0,usec))return -1;

  n=recv(fd,buf,len,0);
  if(n<0){
    if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR)return -1;
    return -1;
  }

  return n;
}

static int relay_check_crc(unsigned char *buf,int len){
  unsigned short got,calc;

  if(len<3)return 0;

  got=buf[len-2]|(buf[len-1]<<8);
  calc=modbus_crc16(buf,len-2);

  if(got==calc)return 1;
  return 0;
}

int parse_key(char *s,uint16_t *v){
  int i,n;

  if(s==NULL)return 0;
  if(s[0]=='K')s++;

  for(i=0;i<3;i++){
    if(s[i]<'0' || s[i]>'9')return 0;
  }

  if(s[3]!='\0')return 0;

  n=(s[0]-'0')*100+(s[1]-'0')*10+(s[2]-'0');
  if(n<1 || n>=TOTEK)return 0;

  *v=(uint16_t)n;
  return 1;
}

int parse_ex(char *s,uint16_t *v){
  int i,n;

  if(s==NULL)return 0;
  if(s[0]=='E')s++;
  if(s[0]=='\0')return 0;

  n=0;
  for(i=0;s[i]!='\0';i++){
    if(s[i]<'0' || s[i]>'9')return 0;
    n=(n*10)+(s[i]-'0');
    if(n>=TOTEX)return 0;
  }

  *v=(uint16_t)n;
  return 1;
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
  if(n<1 || n>999)return 0;

  *v=(uint16_t)n;
  return 1;
}

void relais_code(uint16_t r,char *code){
  r=r%1000;
  code[0]='0'+(r/100);
  code[1]='0'+((r/10)%10);
  code[2]='0'+(r%10);
  code[3]='\0';
}

int is_kmap_line(char *line){
  char *p;

  if(line==NULL)return 0;

  p=line;
  while(*p==' ' || *p=='\t')p++;

  if(strncmp(p,"Kmap",4)!=0)return 0;
  if(p[4]==' ' || p[4]=='\t' || p[4]=='\n' || p[4]=='\r')return 1;

  return 0;
}

int is_rrange_line(char *line){
  char *p;

  if(line==NULL)return 0;

  p=line;
  while(*p==' ' || *p=='\t')p++;

  if(strncmp(p,"Rrange",6)!=0)return 0;
  if(p[6]==' ' || p[6]=='\t' || p[6]=='\n' || p[6]=='\r')return 1;

  return 0;
}

int load_kmap_line(char *line){
  char *t,*dev,*action,*keytxt;
  uint16_t key;

  if(line==NULL)return 0;

  t=strtok(line," \n\r\t");
  dev=strtok(NULL," \n\r\t");
  action=strtok(NULL," \n\r\t");
  keytxt=strtok(NULL," \n\r\t");

  if(t==NULL)return 0;
  if(strcmp(t,"Kmap")!=0)return 0;
  if(dev==NULL || action==NULL || keytxt==NULL)return 1;
  if(nkmap>=MAXKMAP)return 1;
  if(!parse_key(keytxt,&key))return 1;

  strncpy(kmap[nkmap].dev,dev,KDEVLEN-1);
  kmap[nkmap].dev[KDEVLEN-1]='\0';

  strncpy(kmap[nkmap].action,action,KACTLEN-1);
  kmap[nkmap].action[KACTLEN-1]='\0';

  kmap[nkmap].key=key;
  nkmap++;

  return 1;
}

int add_known_relais(uint16_t relay){
  uint16_t i;

  for(i=0;i<nknownrelais;i++){
    if(knownrelais[i]==relay)return 1;
  }

  if(nknownrelais>=MAXKNOWNRELAIS)return 0;

  knownrelais[nknownrelais++]=relay;
  return 1;
}

int load_rrange_line(char *line){
  char *t,*a,*b;
  uint16_t r1,r2,r;

  if(line==NULL)return 0;

  t=strtok(line," \n\r\t");
  a=strtok(NULL," \n\r\t");
  b=strtok(NULL," \n\r\t");

  if(t==NULL)return 0;
  if(strcmp(t,"Rrange")!=0)return 0;
  if(a==NULL || b==NULL)return 1;
  if(!parse_relais(a,&r1))return 1;
  if(!parse_relais(b,&r2))return 1;

  if(r1<=r2){
    for(r=r1;r<=r2;r++)add_known_relais(r);
  }
  else {
    for(r=r2;r<=r1;r++)add_known_relais(r);
  }

  return 1;
}

static int known_relais_cmp(const void *a,const void *b){
  uint16_t ra,rb;

  ra=*((uint16_t *)a);
  rb=*((uint16_t *)b);

  if(ra<rb)return -1;
  if(ra>rb)return 1;
  return 0;
}

void sort_known_relais(){
  if(nknownrelais>1)qsort(knownrelais,nknownrelais,sizeof(uint16_t),known_relais_cmp);
}

static int kmap_cmp_item(char *dev,char *action,struct kmap *m){
  int c;

  c=strcmp(dev,m->dev);
  if(c!=0)return c;

  return strcmp(action,m->action);
}

static int kmap_cmp_qsort(const void *a,const void *b){
  struct kmap *ka,*kb;
  int c;

  ka=(struct kmap *)a;
  kb=(struct kmap *)b;

  c=strcmp(ka->dev,kb->dev);
  if(c!=0)return c;

  return strcmp(ka->action,kb->action);
}

void sort_kmap(){
  if(nkmap>1)qsort(kmap,nkmap,sizeof(struct kmap),kmap_cmp_qsort);
}

int find_kmap(char *dev,char *action,uint16_t *key){
  int lo,hi,mid,c;

  if(dev==NULL || action==NULL || key==NULL)return 0;

  lo=0;
  hi=nkmap-1;

  while(lo<=hi){
    mid=(lo+hi)/2;
    c=kmap_cmp_item(dev,action,&kmap[mid]);

    if(c==0){
      *key=kmap[mid].key;
      return 1;
    }

    if(c<0)hi=mid-1;
    else lo=mid+1;
  }

  return 0;
}

void setrelais(char *code,int on){
  unsigned char frame[8],resp[32];
  unsigned short coil,value,crc;
  int fd,dev,relais,n;

  if(code==NULL)return;

  dev=code[0]-'0';
  relais=((code[1]-'0')*10)+(code[2]-'0');
  if(dev<0 || dev>9)return;
  if(relais<1)return;

  fd=relay_connect(code);
  if(fd<0)return;

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

  n=relay_send_all(fd,frame,8,RELAY_IO_TIMEOUT_US);
  if(n==8)relay_recv_wait(fd,resp,sizeof(resp),RELAY_SET_REPLY_TIMEOUT_US);

  close(fd);
}

int readrelais(char *code){
  unsigned char frame[8],resp[32];
  unsigned short coil,crc;
  int fd,dev,relais,n;

  if(code==NULL)return 2;

  dev=code[0]-'0';
  relais=((code[1]-'0')*10)+(code[2]-'0');
  if(dev<0 || dev>9)return 2;
  if(relais<1)return 2;

  fd=relay_connect(code);
  if(fd<0)return 2;

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

  n=relay_send_all(fd,frame,8,RELAY_IO_TIMEOUT_US);
  if(n<8){
    close(fd);
    return 2;
  }

  n=relay_recv_wait(fd,resp,sizeof(resp),RELAY_IO_TIMEOUT_US);
  close(fd);

  if(n<5)return 2;
  if(!relay_check_crc(resp,n))return 2;
  if(resp[0]!=0x01)return 2;

  if(resp[1]&0x80)return 2;
  if(resp[1]!=0x01)return 2;
  if(n<6)return 2;
  if(resp[2]<1)return 2;

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

  if(s==0 || s==1)return s;
  return 0;
}

int excluded_relais(uint16_t relay,uint16_t *list,uint16_t nlist){
  uint16_t i;

  for(i=0;i<nlist;i++){
    if(list[i]==relay)return 1;
  }

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
  va_list argptr;
  int used,room,n;

  if(end==0)*out='\0';

  used=strlen(out);
  room=(int)sizeof(out)-used;

  if(room>1){
    va_start(argptr,format);
    n=vsnprintf(out+used,room,format,argptr);
    va_end(argptr);

    if(n<0)return;
    out[sizeof(out)-1]='\0';
  }

  if(strlen(out)>PAGE || end==2){
    used=strlen(out);
    room=(int)sizeof(out)-used;

    if(room>1){
      if(end==1)snprintf(out+used,room,"<next>");
      else snprintf(out+used,room,"<end>");
      out[sizeof(out)-1]='\0';
    }

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

static void show_relais_on(int sock){
  char code[4];
  uint16_t i,on,unreadable;
  int s;

  if(nknownrelais==0){
    myout(sock,2,"no Rrange configured\n");
    return;
  }

  on=0;
  unreadable=0;

  myout(sock,1,"relais on:");

  for(i=0;i<nknownrelais;i++){
    relais_code(knownrelais[i],code);
    s=readrelais(code);

    if(s==1){
      myout(sock,1," R%s",code);
      on++;
    }
    else if(s==2)unreadable++;
  }

  myout(sock,1,"\n");
  myout(sock,1,"checked: %d\n",nknownrelais);
  myout(sock,1,"on: %d\n",on);
  myout(sock,2,"unreadable: %d\n",unreadable);
}

char *managewww(int sock){
  static char ret[50];
  char buf[100],code[4],*t1,*t2,*t3;
  int rr,quit,qi;
  uint16_t i,j,k,q,dis,totdis,relay,key,event;
  int fb,fe;
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

  if(!acl_allowed(&(from.sin6_addr)))return ret;

  buf[rr]='\0';

  myout(sock,0,VERSION);
  myout(sock,1,"cmd: %s\n",buf);

  t1=strtok(buf," \n\r\t");
  t2=strtok(NULL," \n\r\t");
  t3=strtok(NULL," \n\r\t");

  if(t1==NULL){
    myout(sock,2,"empty command\n");
    return ret;
  }

  if(strcmp(t1,"status")==0){
    myout(sock,1,"max event relais: %d\n",MAXEVENTRELAIS);
    myout(sock,1,"known relais: %d\n",nknownrelais);
    myout(sock,1,"key events: %d\n",TOTEK-1);
    myout(sock,1,"kmap entries: %d\n",nkmap);
    myout(sock,1,"access entries: %d\n",totwhite);
    myout(sock,1,"system events: %d\n",nevent);

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
    show_relais_on(sock);
  }
  else if(strcmp(t1,"zigbee")==0){
    if(t2!=NULL && t3!=NULL && find_kmap(t2,t3,&key)){
      sprintf(ret,"K%03d",key);
      myout(sock,2,"Kmap %s %s -> K%03d\n",t2,t3,key);
    }
    else myout(sock,2,"Kmap not found\n");
  }
  else if(strcmp(t1,"showkmap")==0){
    for(i=0;i<nkmap;i++){
      myout(sock,1,"Kmap %s %s K%03d\n",kmap[i].dev,kmap[i].action,kmap[i].key);
    }
    myout(sock,2,"");
  }
  else if(strcmp(t1,"showevents")==0){
    for(q=0;q<TOTEK+TOTEX;q++){
      if(q<TOTEK)en=ee+q;
      else en=ex+q-TOTEK;

      for(;;){
        if(en->event==0)break;

        if(q<TOTEK)myout(sock,1,"---- EK K%03d %d ----\n",q,en->event);
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
      if(t2[0]=='K'){
        if(parse_key(t2+1,&key)){
          sprintf(ret,"K%03d",key);
          myout(sock,2,"inject: K%03d\n",key);
        }
        else myout(sock,2,"bad key event\n");
      }
      else if(t2[0]=='E'){
        if(parse_ex(t2+1,&event)){
          sprintf(ret,"E%d",event);
          myout(sock,2,"inject: E%d\n",event);
        }
        else myout(sock,2,"bad system event\n");
      }
      else myout(sock,2,"bad event\n");
    }
    else myout(sock,2,"missing event\n");
  }
  else if(strcmp(t1,"showlog")==0){
    if(!fulllog && poslog==0){
      myout(sock,2,"log empty\n");
    }
    else {
      if(t2!=NULL)k=atoi(t2)%LOGLEN;
      else k=10;

      i=0;
      fb=(fulllog)?poslog-1+LOGLEN:poslog-1;
      fe=(fulllog)?poslog:0;

      for(qi=fb;qi>=fe && i<k;qi--){
        j=qi%LOGLEN;
        memcpy(&info,localtime(&mylog[j].time),sizeof(struct tm));
        strftime(buf,100,"%d/%m/%y %H:%M:%S %A",&info);
        myout(sock,1,"%s %03d %d %s\n",buf,j,mylog[j].action,mylog[j].desc);
        i++;
      }

      myout(sock,2,"End showlog of %03d entries, total %03d\n",i,(fulllog)?LOGLEN:poslog);
    }
  }
  else if(strcmp(t1,"quit")==0){
    myout(sock,2,"quitting\n");
    quit=1;
  }
  else if(strcmp(t1,"help")==0){
    myout(sock,1,"status, show actual status informations\n");
    myout(sock,1,"seton Rabb, set relay Rabb to on, example R101\n");
    myout(sock,1,"setoff Rabb, set relay Rabb to off, example R101\n");
    myout(sock,1,"read Rabb, read relay Rabb state, example R101\n");
    myout(sock,1,"showon, show on relays declared by Rrange\n");
    myout(sock,1,"zigbee dev action, map input event using Kmap\n");
    myout(sock,1,"showkmap, show sorted Kmap table\n");
    myout(sock,1,"showevents, show all configured events\n");
    myout(sock,1,"inject Kxxx, inject key event K001..K999\n");
    myout(sock,1,"inject Ex, inject system event, example E5\n");
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
