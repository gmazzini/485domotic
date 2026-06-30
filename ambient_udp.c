#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <mysql/mysql.h>
#include "/home/tools/setup_energy.c"

#define PORTAMBIENT 54321
#define MAXBUF 512
#define MAXDEVICE 32

int parse_ambient(char *buf,char *device,float *temperature,float *humidity){
  char tag[64],k1[64],k2[64];

  tag[0]=0;
  device[0]=0;
  k1[0]=0;
  k2[0]=0;

  if(sscanf(buf,"%63s %31s %63s %f %63s %f",tag,device,k1,temperature,k2,humidity)!=6)return -1;

  if(strcmp(tag,"zigbee_sensor")!=0 && strcmp(tag,"switchbot_sensor")!=0)return -1;
  if(strcmp(k1,"temperature")!=0)return -1;
  if(strcmp(k2,"humidity")!=0)return -1;

  return 0;
}

int main(){
  int fd,n;
  struct sockaddr_in6 addr;
  MYSQL *con;
  time_t t;
  char buf[MAXBUF];
  char device[MAXDEVICE];
  char query[300];
  float temperature,humidity;

  con=mysql_init(NULL);
  if(con==NULL)return 1;

  if(mysql_real_connect(con,"localhost",USER,PASSWORD,DB,0,NULL,0)==NULL){
    mysql_close(con);
    return 1;
  }

  fd=socket(AF_INET6,SOCK_DGRAM,0);
  if(fd<0){
    mysql_close(con);
    return 1;
  }

  memset(&addr,0,sizeof(addr));
  addr.sin6_family=AF_INET6;
  addr.sin6_addr=in6addr_any;
  addr.sin6_port=htons(PORTAMBIENT);

  if(bind(fd,(struct sockaddr *)&addr,sizeof(addr))<0){
    close(fd);
    mysql_close(con);
    return 1;
  }

  printf("ambient udp listen port %d\n",PORTAMBIENT);
  fflush(stdout);

  while(1){
    n=recvfrom(fd,buf,sizeof(buf)-1,0,NULL,NULL);
    if(n<=0)continue;

    buf[n]=0;

    if(parse_ambient(buf,device,&temperature,&humidity)<0){
      fprintf(stderr,"bad packet: %s\n",buf);
      fflush(stderr);
      continue;
    }

    t=time(NULL);

    snprintf(query,sizeof(query),
      "insert into ambient (epoch,device,temperature,humidity) values(%ld,'%s',%f,%f)",
      (long)t,device,temperature,humidity);

    if(mysql_query(con,query)!=0){
      fprintf(stderr,"mysql error: %s\n",mysql_error(con));
      fflush(stderr);
      continue;
    }

    printf("saved %ld %s %.2f %.2f\n",(long)t,device,temperature,humidity);
    fflush(stdout);
  }

  close(fd);
  mysql_close(con);
  return 0;
}
