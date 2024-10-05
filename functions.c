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
#define PI 3.1415926
#define ZENITH 1

void sun(int year,int month,int day,float lat,float lng){
  float N1,N2,N3,N,lngHour,tr,ts,Mr,Ms,Lr,Ls;
  float RAr,RAs,Lquadrantr,Lquadrants,RAquadrantr,RAquadrants;
  float sinDecr,sinDecs,cosDecr,cosDecs,cosHr,cosHs,Hr,Hs,Tr,Ts,UTr,UTs;
  
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
  UTr=fmod(24.0+Tr-lngHour,24.0);
  UTs=fmod(24.0+Ts-lngHour,24.0);
  printf("SUN %f %f\n",UTr,UTs);
}
