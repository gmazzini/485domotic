#define PI 3.1415926
#define ZENITH 1
#define PAGE 500
#include <stdarg.h>

struct sockaddr from;
unsigned int fromlen=sizeof(from);
char *cmd[]={"","onoff","on","off","condon","condoff"};

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
  char buf[100],*t1,*t2,*f;
  int rr,quit;
  uint16_t q,i,j,k,dis,totdis;
  uint64_t flag;
  FILE *fp;
  time_t myt;
  struct tm *info;
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
  if(strcmp(t1,"status")==0){
    for(j=0,q=0;q<TOTRELAIS;q++)if(relais[q]==1)j++;
    myout(sock,1,"relais on: %d\n",j);
    myout(sock,1,"events: %d\n",nevent);
    time(&myt); info=localtime(&myt); strftime(buf,100,"%d.%m.%Y %H:%M:%S %A",info);
    myout(sock,1,"time now: %s\n",buf);
    info=localtime(&start); strftime(buf,100,"%d.%m.%Y %H:%M:%S %A",info);
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
    if(fulllog || poslog>0){
      if(t2==NULL){
        if(fulllog==0){i=0; j=poslog;}
        else {i=poslog; j=poslog+LOGLEN;} 
      }
      printf("%d %d %d\n,i,j,poslog);
      for(q=i;q<j;q++){
        info=localtime(&mylog[q%LOGLEN].time); strftime(buf,100,"%d.%m.%Y %H:%M:%S %A",info);
        myout(sock,(q==j-1)?2:1,"%s %d %s\n",buf,mylog[q%LOGLEN].action,mylog[q%LOGLEN].desc); 
      }
    }
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
    myout(sock,1,"showlog [n], show rotative log last n lines or all\n");
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
  struct tm *loctime,*gmttime;
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
  loctime=localtime(&myt);
  gmttime=gmtime(&myt);
  delta=loctime->tm_hour-gmttime->tm_hour;
  UTr=fmod(24.0+Tr-lngHour+delta,24.0);
  UTs=fmod(24.0+Ts-lngHour+delta,24.0);
  *HHr=(int)UTr;
  *MMr=(int)((UTr-(int)UTr)*60);
  *HHs=(int)UTs;
  *MMs=(int)((UTs-(int)UTs)*60);
}
