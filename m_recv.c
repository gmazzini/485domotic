#include <mysql/mysql.h>
#include "m_functions.c"
#include "/home/tools/setup_energy.c"

int main(int argc,char **argv){
  const char *host=IPCC,*port="80";
  struct addrinfo h={0},*r;
  char path[30],buf[1024],*aa,query[200];
  ssize_t n;
  time_t t;
  float v[6];
  MYSQL *con=mysql_init(NULL);
  
  sprintf(path,"/cgi-bin/m_read?%d",atoi(argv[1]));
  h.ai_socktype=SOCK_STREAM;
  if(getaddrinfo(host,port,&h,&r))return 1;
  int s=socket(r->ai_family,r->ai_socktype,r->ai_protocol);
  fcntl(s,F_SETFL,O_NONBLOCK);
  connect(s,r->ai_addr,r->ai_addrlen);
  struct pollfd p={s,POLLOUT,0};
  if(poll(&p,1,2000)<=0)return 2;
  dprintf(s,"GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",path,host);
  p.events=POLLIN;
  mysql_real_connect(con,"localhost",USER,PASSWORD,DB,0,NULL,0);
  t=time(NULL);
  while(poll(&p,1,2000)>0 && (n=read(s,buf,sizeof buf))>0){
    aa=strstr(buf,"\r\n1,");
    if(aa!=NULL && sscanf(aa,"\r\n1,%f,%f,%f,%f,%f,%f",&v[0],&v[1],&v[2],&v[3],&v[4],&v[5])==6){
      sprintf(query,"insert into vi_cc (epoch,v1,v2,v3,i1,i2,i3) values(%ld,%f,%f,%f,%f,%f,%f)",t,v[0],v[1],v[2],v[3],v[4],v[5]);
      mysql_query(con,query);
    }
  }
  mysql_close(con);
  close(s);
  freeaddrinfo(r);
  return 0;
}
