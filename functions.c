#define PI 3.1415926
#define ZENITH 1

char * managewww(int sock){
  static char ret[50];
  struct sockaddr from;
  unsigned int fromlen;
  fromlen=sizeof(from);
  char buf[100],out[1000],*t1,*t2;
  int rr,quit;
  uint8_t q;
  FILE *fp;
  time_t myt;
  struct tm *info;

  *ret='\0';
  quit=0;
  rr=recvfrom(sock,buf,100,0,&from,&fromlen);
  if(rr<1)return ret;
  *(buf+rr)='\0';
  t1=strtok(buf," \n\r\t");
  t2=strtok(NULL," \n\r\t");
  if(strcmp(t1,"time")==0){
    time(&myt); info=localtime(&myt); strftime(buf,100,"%d.%m.%Y %H:%M:%S %A",info);
    sprintf(out,"now: %s\n",buf);
    info=localtime(&start); strftime(buf,100,"%d.%m.%Y %H:%M:%S %A",info);
    sprintf(out+strlen(out),"start: %s\n",buf);
    sprintf(out+strlen(out),"sunrise: %02d%02d\n",HHr,MMr);
    sprintf(out+strlen(out),"sunset: %02d%02d\n",HHs,MMs);
  }
  else if(strcmp(t1,"showon")==0){
    sprintf(out,"relains on:");
    for(q=0;q<TOTRELAIS;q++)if(relais[q]==1)sprintf(out+strlen(out)," R%d,%d",q/10,q%10);
    sprintf(out+strlen(out),"\n");
  }
  else if(strcmp(t1,"inject")==0){
    sprintf(out,"inject: %s\n",t2);
    strcpy(ret,t2);
  }
  else if(strcmp(t1,"quit")==0){
    sprintf(out,"quitting\n");
    quit=1;
  }
  else if(strcmp(t1,"help")==0){
    printf(out,"time, show actual time informations\n");
    printf(out+strlen(out),"showon, show relais in on state\n");
    printf(out+strlen(out),"inject xxx, inject the xxx event (like Ki,j or Ew) in the system\n");
    printf(out+strlen(out),"quit, shutdown the system\n");
    printf(out+strlen(out),"help, this help\n");
  }
  else sprintf(out,"command not find\n");
  sendto(sock,out,strlen(out),0,&from,fromlen);
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
