#include "stdio.h"
#include "stdlib.h"

int main(){
  FILE *fp;
  char buf[100];
  char *token,*f;
  unsigned int q,*lK,nlK,*lR,nlR,*lE,nlE,nLD;
  unsigned long *lD;

  lK=(unsigned int)malloc(100);
  lR=(unsigned int)malloc(100);
  lE=(unsigned int)malloc(100);
  lD=(unsigned long)malloc(100);
  
  fp=fopen("config","r");
  for(;;){
    fgets(buf,100,fp);
    if(feof(fp))break;
    nlK=nlR=nlE=nlD=0;
    for(token=strtok(buf," ");token;token=strtok(NULL," ")){
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


          
          lD[nlD++]=10*atoi(token+1)+atoi(f+1);
      $q=((int)substr($v,1,2))*60+((int)substr($v,3,2))+10000*((int)substr($v,6,2))*60+((int)substr($v,8,2));
      $lD[$nlD++]=$q;
      break;
      case "C":
      $q=substr($v,1);
      $lC[$nlC++]=$q;
      break;
    }
  }
}
fclose($fp);

$K=array();
for($w=0;$w<$nlk;$w++)$K[$lk[$w]]=array($lD,$lC,$lR);

}
