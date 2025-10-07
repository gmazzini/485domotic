#include <mysql/mysql.h>
#include "m_functions.c"
#include "/home/tools/setup_energy.c"

int main(int argc,char **argv){
  const char *host[]={"",IPCC,IPCC,IPCC,IPSORC,IPFGM,IPCC};
  int ww[]={0,1,2,3,4,4,5};
  const char *port="80";
  struct addrinfo h={0},*r;
  char path[30],buf[1024],*aa,query[200];
  ssize_t n;
  uint64_t u1,u2;
  time_t t;
  float v[6];
  MYSQL *con=mysql_init(NULL);
  int ii,jj;
  MYSQL_RES *res;
  MYSQL_ROW row;

  ii=atoi(argv[1]);
  sprintf(path,"/cgi-bin/m_read?%d",ww[ii]);
  h.ai_socktype=SOCK_STREAM;
  if(getaddrinfo(host[ii],port,&h,&r))return 1;
  int s=socket(r->ai_family,r->ai_socktype,r->ai_protocol);
  fcntl(s,F_SETFL,O_NONBLOCK);
  connect(s,r->ai_addr,r->ai_addrlen);
  struct pollfd p={s,POLLOUT,0};
  if(poll(&p,1,2000)<=0)return 2;
  dprintf(s,"GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",path,host[ii]);
  p.events=POLLIN;
  mysql_real_connect(con,"localhost",USER,PASSWORD,DB,0,NULL,0);
  t=time(NULL);
  while(poll(&p,1,2000)>0 && (n=read(s,buf,sizeof buf))>0){
    aa=strstr(buf,"\r\n\r\n");
    if(aa==NULL)continue;
    if(sscanf(aa+4,"%d,",&jj)!=1)continue;
    if(ww[ii]!=jj)continue;
    aa=strstr(aa+4,",")+1;
    switch(ii){
      case 1:
        if(sscanf(aa,"%f,%f,%f,%f,%f,%f",&v[0],&v[1],&v[2],&v[3],&v[4],&v[5])==6){
          sprintf(query,"insert into vi_cc (epoch,v1,v2,v3,i1,i2,i3) values(%ld,%f,%f,%f,%f,%f,%f)",t,v[0],v[1],v[2],v[3],v[4],v[5]);
          mysql_query(con,query);
        }
        break;
      case 2:
        if(sscanf(aa,"%f,%f,%f",&v[0],&v[1],&v[2])==3){
          sprintf(query,"insert into energy_cc (epoch,e1,e2,e3) values(%ld,%f,%f,%f)",t,v[0],v[1],v[2]);
          mysql_query(con,query);
        }
        break;
      case 3:
        if(sscanf(aa,"%f",&v[0])==1){
          sprintf(query,"insert into energy_le1 (epoch,e) values(%ld,%f)",t,v[0]);
          mysql_query(con,query);
        }
        break;
      case 4:
        if(sscanf(aa,"%f",&v[0])==1){
          sprintf(query,"insert into temp_srso (epoch,t) values(%ld,%f)",t,v[0]);
          mysql_query(con,query);
        }
        break;
      case 5:
        if(sscanf(aa,"%f",&v[0])==1){
          sprintf(query,"insert into temp_fgm (epoch,t) values(%ld,%f)",t,v[0]);
          mysql_query(con,query);
        }
        break;
      case 6:
        if(sscanf(aa,"%" SCNu64,&u1)==1){
          sprintf(query,"select w from water_cc order by epoch desc limit 1");
          mysql_query(con,query);
          res=mysql_store_result(con);
          row=mysql_fetch_row(res);
          u2=strtoll(row[0],NULL,10);
          mysql_free_result(res);
          if(u2==u1)break;
          if(u1>u2)sprintf(query,"insert into water_cc (epoch,w,d) values(%ld,%" PRIu64 ",%ld)",t,u1,u1-u2);
          else sprintf(query,"insert into water_cc (epoch,w,d) values(%ld,%" PRIu64 ",%ld)",t,u1,0);
          mysql_query(con,query);
        }
        break;
    }
  }
  mysql_close(con);
  close(s);
  freeaddrinfo(r);
  return 0;
}
