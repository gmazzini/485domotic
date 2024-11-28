#define PI 3.1415926
#define ZENITH 1
#define PAGE 500

struct sockaddr from;
unsigned int fromlen=sizeof(from);
char *cmd[]={"","onoff","on","off","condon","condoff"};
static unsigned char const crc8_table[] = {
  0xea, 0xd4, 0x96, 0xa8, 0x12, 0x2c, 0x6e, 0x50, 0x7f, 0x41, 0x03, 0x3d,
  0x87, 0xb9, 0xfb, 0xc5, 0xa5, 0x9b, 0xd9, 0xe7, 0x5d, 0x63, 0x21, 0x1f,
  0x30, 0x0e, 0x4c, 0x72, 0xc8, 0xf6, 0xb4, 0x8a, 0x74, 0x4a, 0x08, 0x36,
  0x8c, 0xb2, 0xf0, 0xce, 0xe1, 0xdf, 0x9d, 0xa3, 0x19, 0x27, 0x65, 0x5b,
  0x3b, 0x05, 0x47, 0x79, 0xc3, 0xfd, 0xbf, 0x81, 0xae, 0x90, 0xd2, 0xec,
  0x56, 0x68, 0x2a, 0x14, 0xb3, 0x8d, 0xcf, 0xf1, 0x4b, 0x75, 0x37, 0x09,
  0x26, 0x18, 0x5a, 0x64, 0xde, 0xe0, 0xa2, 0x9c, 0xfc, 0xc2, 0x80, 0xbe,
  0x04, 0x3a, 0x78, 0x46, 0x69, 0x57, 0x15, 0x2b, 0x91, 0xaf, 0xed, 0xd3,
  0x2d, 0x13, 0x51, 0x6f, 0xd5, 0xeb, 0xa9, 0x97, 0xb8, 0x86, 0xc4, 0xfa,
  0x40, 0x7e, 0x3c, 0x02, 0x62, 0x5c, 0x1e, 0x20, 0x9a, 0xa4, 0xe6, 0xd8,
  0xf7, 0xc9, 0x8b, 0xb5, 0x0f, 0x31, 0x73, 0x4d, 0x58, 0x66, 0x24, 0x1a,
  0xa0, 0x9e, 0xdc, 0xe2, 0xcd, 0xf3, 0xb1, 0x8f, 0x35, 0x0b, 0x49, 0x77,
  0x17, 0x29, 0x6b, 0x55, 0xef, 0xd1, 0x93, 0xad, 0x82, 0xbc, 0xfe, 0xc0,
  0x7a, 0x44, 0x06, 0x38, 0xc6, 0xf8, 0xba, 0x84, 0x3e, 0x00, 0x42, 0x7c,
  0x53, 0x6d, 0x2f, 0x11, 0xab, 0x95, 0xd7, 0xe9, 0x89, 0xb7, 0xf5, 0xcb,
  0x71, 0x4f, 0x0d, 0x33, 0x1c, 0x22, 0x60, 0x5e, 0xe4, 0xda, 0x98, 0xa6,
  0x01, 0x3f, 0x7d, 0x43, 0xf9, 0xc7, 0x85, 0xbb, 0x94, 0xaa, 0xe8, 0xd6,
  0x6c, 0x52, 0x10, 0x2e, 0x4e, 0x70, 0x32, 0x0c, 0xb6, 0x88, 0xca, 0xf4,
  0xdb, 0xe5, 0xa7, 0x99, 0x23, 0x1d, 0x5f, 0x61, 0x9f, 0xa1, 0xe3, 0xdd,
  0x67, 0x59, 0x1b, 0x25, 0x0a, 0x34, 0x76, 0x48, 0xf2, 0xcc, 0x8e, 0xb0,
  0xd0, 0xee, 0xac, 0x92, 0x28, 0x16, 0x54, 0x6a, 0x45, 0x7b, 0x39, 0x07,
  0xbd, 0x83, 0xc1, 0xff};

void setserial(int fd){
  struct termios tty;
  tcgetattr(fd,&tty);
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag |= CREAD | CLOCAL;
  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO;
  tty.c_lflag &= ~ECHOE;
  tty.c_lflag &= ~ECHONL;
  tty.c_lflag &= ~ISIG;
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
  tty.c_oflag &= ~OPOST;
  tty.c_oflag &= ~ONLCR;
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 0;
  cfsetispeed(&tty,B9600);
  cfsetospeed(&tty,B9600);
  tcsetattr(fd,TCSANOW,&tty);
}

void myset(int fd,uint8_t dev,uint8_t relais){
  uint8_t oo[4],i;
  oo[0]=0xCC;
  oo[1]=dev;
  oo[2]=relais;
  for(oo[3]=0,i=0;i<3;i++)oo[3]=crc8_table[(oo[3] ^ oo[i])];
  for(i=0;i<4;i++){
    write(fd,oo+i,1);
    usleep(1250);
  }
}

uint16_t myread(int fd){
  uint8_t dd,vv,cc,i;
  static uint8_t ss=0,v[4];
  vv=read(fd,&dd,1);
  if(ss==0 && dd==0xFF){
    v[0]=dd;
    ss=1;
    return 0;
  }
  if(ss>0 && dd==0xFF){
    ss=0;
    return 0;
  }
  if(ss==1){
    v[1]=dd;
    ss=2;
    return 0;
  }
  if(ss==2){
    v[2]=dd;
    ss=3;
    return 0;
  }
  v[3]=dd;
  ss=0;
  for(cc=0,i=0;i<3;i++)cc=crc8_table[(cc ^ v[i])];
  if(v[3]!=cc)return 0;
  return v[1]*256+v[2];
}

void myout(int sock,int end,char *format, ...){
  static char out[PAGE*3];
  if(end==0)*out='\0';
  unsigned int fromlen=sizeof(from);
  va_list argptr;
  va_start(argptr,format);
  vsprintf(out+strlen(out),format,argptr);
  va_end(argptr);
  if(strlen(out)>PAGE || end==2){
    if(end==1)sprintf(out+strlen(out),"<next>");
    else sprintf(out+strlen(out),"<end>");
    sendto(sock,out,strlen(out),0,&from,fromlen);
    *out='\0';
  }
}

char * managewww(int sock){
  static char ret[50];
  char buf[100],*t1,*t2,*t3,*f;
  int rr,quit,q;
  uint16_t i,j,k,dis,totdis;
  uint64_t flag;
  FILE *fp;
  time_t myt;
  struct tm info;
  struct ek *en;
  
  *ret='\0';
  quit=0;
  rr=recvfrom(sock,buf,100,0,&from,&fromlen);
  if(rr<1)return ret;
  *(buf+rr)='\0';
  myout(sock,0,"domotic by GM @2024\n");
  myout(sock,1,">> %s\n",buf);
  t1=strtok(buf," \n\r\t");
  t2=strtok(NULL," \n\r\t");
  t3=strtok(NULL," \n\r\t");
  if(strcmp(t1,"status")==0){
    for(j=0,q=0;q<TOTRELAIS;q++)if(relais[q]==1)j++;
    myout(sock,1,"relais on: %d\n",j);
    myout(sock,1,"events: %d\n",nevent);
    time(&myt); memcpy(&info,localtime(&myt),sizeof(struct tm)); strftime(buf,100,"%d.%m.%Y %H:%M:%S %A",&info);
    myout(sock,1,"time now: %s\n",buf);
    memcpy(&info,localtime(&start),sizeof(struct tm)); strftime(buf,100,"%d.%m.%Y %H:%M:%S %A",&info);
    myout(sock,1,"time start: %s\n",buf);
    myout(sock,1,"time sunrise: %02d:%02d\n",HHr,MMr);
    myout(sock,1,"time sunset: %02d:%02d\n",HHs,MMs);
    myout(sock,2,"log dimension: %d\n",(fulllog)?LOGLEN:poslog);
  }
  else if(strcmp(t1,"seton")==0){
    myout(sock,2,"set relais to on: %s\n",t2);
    f=strchr(t2,','); *f='\0';
    en=ex; en->R[0]=10*atoi(t2+1)+atoi(f+1); en->nC=1; en->C[0]=2;
    strcpy(ret,"E0");
  }
  else if(strcmp(t1,"setoff")==0){
    myout(sock,2,"set relais to off: %s\n",t2);
    f=strchr(t2,','); *f='\0';
    en=ex; en->R[0]=10*atoi(t2+1)+atoi(f+1); en->nC=1; en->C[0]=3;
    strcpy(ret,"E0");
  }
  else if(strcmp(t1,"showon")==0){
    myout(sock,1,"show relais on:");
    for(q=0;q<TOTRELAIS;q++)if(relais[q]==1)myout(sock,1," R%d,%d",q/10,q%10);
    myout(sock,2,"\n");
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
          for(j=0;j<en->nR;j++)myout(sock,1,"R%d,%d ",en->R[j]/10,en->R[j]%10);
          myout(sock,1,"\n");
        }
        if(en->nC>0){
          for(j=0;j<en->nC;j++)myout(sock,1,"C%s ",cmd[en->C[j]]);
          myout(sock,1,"\n");
        }
        totdis=0; dis=1;
        for(j=0;j<1440;j++){
          flag=en->D[j/64]&mask[j%64];
          if(flag>0 && dis || flag==0 && !dis){
            totdis++;
            dis=1-dis;
            myout(sock,1,"D%02d%02d ",(j-dis)/60,(j-dis)%60);
          }
        }
        if(totdis)myout(sock,1,"\n");
        if(en->nT>0){
          for(j=0;j<en->nT;j++)myout(sock,1,"T%d,%d,%d ",(en->T[j]%1000)/10,en->T[j]%10,en->T[j]/1000);
          myout(sock,1,"\n");
        }
        if(en->next==NULL)break;
        en=en->next;
      }
    }
    myout(sock,2,"");
  }
  else if(strcmp(t1,"inject")==0){
    myout(sock,2,"inject: %s\n",t2);
    strcpy(ret,t2);
  }
  else if(strcmp(t1,"showlog")==0){
    i=0;
    if(t2!=NULL)k=atoi(t2)%LOGLEN;
    else k=0;
    if(t3!=NULL)j=atoi(t3);
    else j=0;
    for(q=poslog-1;q>=0 && i<k;q--){
      if(mylog[q].action==j)continue;
      memcpy(&info,localtime(&mylog[q].time),sizeof(struct tm)); strftime(buf,100,"%d.%m.%Y %H:%M:%S %A",&info);
      myout(sock,1,"%s %03d %d %s\n",buf,q,mylog[q].action,mylog[q].desc);
      i++;
    }
    if(fulllog){
      for(q=LOGLEN-1;q>=poslog && i<k;q--){
        if(mylog[q].action==j)continue;
        memcpy(&info,localtime(&mylog[q].time),sizeof(struct tm)); strftime(buf,100,"%d.%m.%Y %H:%M:%S %A",&info);
        myout(sock,1,"%s %03d %d %s\n",buf,q,mylog[q].action,mylog[q].desc);
        i++;
      }
    }
    myout(sock,2,"End showlog of %03d entries, total %03d\n",i,(fulllog)?LOGLEN:poslog);
  }
  else if(strcmp(t1,"quit")==0){
    myout(sock,2,"quitting\n");
    quit=1;
  }
  else if(strcmp(t1,"help")==0){
    myout(sock,1,"status, show actual status informations\n");
    myout(sock,1,"seton Ri,j, set the relais Ri,j to on\n");
    myout(sock,1,"setoff Ri,j, set the relais Ri,j to off\n");
    myout(sock,1,"showon, show relais in on state\n");
    myout(sock,1,"showevents, show all the events\n");
    myout(sock,1,"inject xxx, inject the xxx event (like Ki,j or Ew) in the system\n");
    myout(sock,1,"showlog n m, show rotative log last n lines with esclusion of action m\n");
    myout(sock,1,"quit, shutdown the system\n");
    myout(sock,2,"help, this help\n");
  }
  else myout(sock,2,"command not find\n");
  if(quit==1){
    fp=fopen(SAVESTATUS,"wb");
    fwrite(relais,sizeof(uint8_t),TOTRELAIS,fp);
    fclose(fp);
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
  strcpy(mylog[poslog].desc,desc);
  if(++poslog>=LOGLEN){
    poslog=0;
    fulllog=1;
  }
}
