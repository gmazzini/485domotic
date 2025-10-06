#include <mysql/mysql.h>
#include "m_functions.c"
#include "/home/tools/setup_energy.c"

int main(int argc,char **argv){
  const char *host="
", *port="80", *path="/cgi-bin/m_read?1";
    struct addrinfo h={0},*r;
    char buf[1024],*aa;
    ssize_t n;
    float v[6];
    h.ai_socktype=SOCK_STREAM;
    if(getaddrinfo(host,port,&h,&r))return 1;
    int s=socket(r->ai_family,r->ai_socktype,r->ai_protocol);
    fcntl(s,F_SETFL,O_NONBLOCK);
    connect(s,r->ai_addr,r->ai_addrlen);
    struct pollfd p={s,POLLOUT,0};
    if(poll(&p,1,2000)<=0)return 2;
    dprintf(s,"GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",path,host);
    p.events=POLLIN;
    while(poll(&p,1,2000)>0 && (n=read(s,buf,sizeof buf))>0){
    aa=strstr(buf,"\r\n1,");
    if(aa!=NULL)if(sscanf(aa,"\r\n1,%f,%f,%f,%f,%f,%f",&v[0],&v[1],&v[2],&v[3],&v[4],&v[5])==6)printf("%f %f %f %f %f %f\n",v[0],v[1],v[2],v[3],v[4],v[5]);
    }
    close(s);
    freeaddrinfo(r);
    return 0;


  
  
  int fd,ow,mode;
  float v1,v2,v3,i1,i2,i3,e1,e2,e3;
  struct sockaddr_in server;
  MYSQL *con=mysql_init(NULL);
  time_t t;
  char query[200];
  
  mode=atoi(argv[1]);
  fd=socket(AF_INET,SOCK_STREAM,0);
  server.sin_family=AF_INET;
  server.sin_port=htons(4196);
  server.sin_addr.s_addr=inet_addr(IP);
  connect(fd,(struct sockaddr *)&server,sizeof(server)); 
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
