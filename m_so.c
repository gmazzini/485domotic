#include <mysql/mysql.h>
#include "m_functions.c"
#include "/home/tools/setup_energy.c"

void handler(){
  exit(0);
}

void main(int argc,char **argv){
  int fd,ow,mode;
  float v1,v2,v3,i1,i2,i3,e1,e2,e3;
  struct sockaddr_in server;
  MYSQL *con=mysql_init(NULL);
  time_t t;
  char buf[100],query[200];

  signal(SIGALRM,handler);
  alarm(5);
  mode=atoi(argv[1]);
  fd=socket(AF_INET,SOCK_STREAM,0);
  server.sin_family=AF_INET;
  server.sin_port=htons(4196);
  server.sin_addr.s_addr=inet_addr(IP);
  if(connect(fd,(struct sockaddr *)&server,sizeof(server)))exit(1); 
  mysql_real_connect(con,"localhost",USER,PASSWORD,DB,0,NULL,0);
  t=time(NULL);

  switch(mode){
    case 1:
      myw(fd,(uint8_t *)"\x01\x03\x00\x0E",2); v1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x10",2); v2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x12",2); v3=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x16",2); i1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x18",2); i2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x00\x1A",2); i3=myr_f(fd);
      if(v1>=0 && v2>=0 && v3>=0 && i1>=0 && i2>=0 && i3>=0){
        sprintf(query,"insert into vi_so (epoch,v1,v2,v3,i1,i2,i3) values(%ld,%f,%f,%f,%f,%f,%f)",t,v1,v2,v3,i1,i2,i3);
        mysql_query(con,query);
      }
      break;

    case 2:
      myw(fd,(uint8_t *)"\x01\x03\x01\x02",2); e1=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x01\x04",2); e2=myr_f(fd);
      myw(fd,(uint8_t *)"\x01\x03\x01\x06",2); e3=myr_f(fd);
      if(e1>=0 && e2>=0 && e3>=0){
        sprintf(query,"insert into energy_so (epoch,e1,e2,e3) values(%ld,%f,%f,%f)",t,e1,e2,e3);
        mysql_query(con,query);
      }
      break;
  }

  close(fd);
}
