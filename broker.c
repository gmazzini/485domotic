#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <mosquitto.h>

#define VERSION "zigbee_broker v1.0 by GM @2026\n"
#define MQTT_HOST "localhost"
#define MQTT_PORT 1883
#define MQTT_KEEPALIVE 60
#define MQTT_TOPIC "zigbee2mqtt/+"
#define DOMOTIC_IP "2001:678:1158:101:23d4:c492:b014:8810"
#define DOMOTIC_PORT 55556
#define MAXPAYLOAD 512
#define MAXACTION 64
#define MAXDEVICE 128
#define MAXMSG 256

struct conf{
  char domotic_ip[128];
  int domotic_port;
  char mqtt_host[128];
  int mqtt_port;
};

static struct conf cfg;

static int get_action(const char *s,char *out,int max){
  const char *p;
  int i;

  p=strstr(s,"\"action\":\"");
  i=0;

  if(!p)return 0;

  p+=strlen("\"action\":\"");

  while(*p && *p!='"' && i<max-1){
    out[i++]=*p++;
  }

  out[i]='\0';

  if(i>0)return 1;
  return 0;
}

static int send_domotic(char *device,char *action){
  struct sockaddr_in6 addr;
  struct timeval tv;
  char msg[MAXMSG],buf[2000];
  int fd,n,len,ret;
  socklen_t addrlen;

  if(device==NULL || action==NULL)return 0;

  len=snprintf(msg,sizeof(msg),"zigbee %s %s",device,action);
  if(len<1 || len>=(int)sizeof(msg))return 0;

  fd=socket(AF_INET6,SOCK_DGRAM,0);
  if(fd<0)return 0;

  tv.tv_sec=3;
  tv.tv_usec=0;
  setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

  memset(&addr,0,sizeof(addr));
  addr.sin6_family=AF_INET6;
  addr.sin6_port=htons(cfg.domotic_port);

  if(inet_pton(AF_INET6,cfg.domotic_ip,&addr.sin6_addr)!=1){
    close(fd);
    return 0;
  }

  ret=sendto(fd,msg,strlen(msg),0,(struct sockaddr *)&addr,sizeof(addr));
  if(ret<0){
    close(fd);
    return 0;
  }

  for(;;){
    addrlen=sizeof(addr);
    n=recvfrom(fd,buf,sizeof(buf)-1,0,(struct sockaddr *)&addr,&addrlen);
    if(n<1)break;

    buf[n]='\0';
    printf("%s",buf);

    if(strstr(buf,"<end>")!=NULL)break;
  }

  close(fd);
  return 1;
}

static void on_message(struct mosquitto *m,void *u,const struct mosquitto_message *msg){
  char payload[MAXPAYLOAD];
  char action[MAXACTION];
  char device[MAXDEVICE];
  const char *prefix;
  const char *p;
  int n,len;

  (void)m;
  (void)u;

  prefix="zigbee2mqtt/";

  if(msg==NULL)return;
  if(msg->retain)return;
  if(msg->topic==NULL)return;
  if(msg->payload==NULL)return;

  if(strncmp(msg->topic,prefix,strlen(prefix))!=0)return;

  p=msg->topic+strlen(prefix);

  if(strncmp(p,"bridge/",7)==0)return;
  if(strcmp(p,"bridge")==0)return;

  len=strlen(p);
  if(len<1 || len>=MAXDEVICE)return;

  strcpy(device,p);

  n=msg->payloadlen;
  if(n>=MAXPAYLOAD)n=MAXPAYLOAD-1;

  memcpy(payload,msg->payload,n);
  payload[n]='\0';

  if(get_action(payload,action,sizeof(action))){
    printf("zigbee %s %s\n",device,action);
    fflush(stdout);

    if(!send_domotic(device,action)){
      fprintf(stderr,"send_domotic error: %s %s\n",device,action);
      fflush(stderr);
    }
  }
}

static void usage(char *name){
  printf("%s",VERSION);
  printf("usage:\n");
  printf("  %s [domotic_ipv6] [domotic_port] [mqtt_host] [mqtt_port]\n",name);
  printf("\n");
  printf("defaults:\n");
  printf("  domotic_ipv6: %s\n",DOMOTIC_IP);
  printf("  domotic_port: %d\n",DOMOTIC_PORT);
  printf("  mqtt_host: %s\n",MQTT_HOST);
  printf("  mqtt_port: %d\n",MQTT_PORT);
}

int main(int argc,char **argv){
  struct mosquitto *m;
  int rc;

  strcpy(cfg.domotic_ip,DOMOTIC_IP);
  cfg.domotic_port=DOMOTIC_PORT;
  strcpy(cfg.mqtt_host,MQTT_HOST);
  cfg.mqtt_port=MQTT_PORT;

  if(argc>1){
    if(strcmp(argv[1],"-h")==0 || strcmp(argv[1],"--help")==0){
      usage(argv[0]);
      return 0;
    }
    strncpy(cfg.domotic_ip,argv[1],sizeof(cfg.domotic_ip)-1);
    cfg.domotic_ip[sizeof(cfg.domotic_ip)-1]='\0';
  }

  if(argc>2)cfg.domotic_port=atoi(argv[2]);

  if(argc>3){
    strncpy(cfg.mqtt_host,argv[3],sizeof(cfg.mqtt_host)-1);
    cfg.mqtt_host[sizeof(cfg.mqtt_host)-1]='\0';
  }

  if(argc>4)cfg.mqtt_port=atoi(argv[4]);

  mosquitto_lib_init();

  m=mosquitto_new(NULL,1,NULL);
  if(m==NULL){
    fprintf(stderr,"mosquitto_new error\n");
    mosquitto_lib_cleanup();
    return 1;
  }

  mosquitto_message_callback_set(m,on_message);

  rc=mosquitto_connect(m,cfg.mqtt_host,cfg.mqtt_port,MQTT_KEEPALIVE);
  if(rc!=MOSQ_ERR_SUCCESS){
    fprintf(stderr,"mosquitto_connect error: %s\n",mosquitto_strerror(rc));
    mosquitto_destroy(m);
    mosquitto_lib_cleanup();
    return 1;
  }

  rc=mosquitto_subscribe(m,NULL,MQTT_TOPIC,0);
  if(rc!=MOSQ_ERR_SUCCESS){
    fprintf(stderr,"mosquitto_subscribe error: %s\n",mosquitto_strerror(rc));
    mosquitto_disconnect(m);
    mosquitto_destroy(m);
    mosquitto_lib_cleanup();
    return 1;
  }

  printf("%s",VERSION);
  printf("mqtt: %s:%d topic %s\n",cfg.mqtt_host,cfg.mqtt_port,MQTT_TOPIC);
  printf("domotic: [%s]:%d\n",cfg.domotic_ip,cfg.domotic_port);
  fflush(stdout);

  rc=mosquitto_loop_forever(m,-1,1);

  mosquitto_destroy(m);
  mosquitto_lib_cleanup();

  if(rc!=MOSQ_ERR_SUCCESS){
    fprintf(stderr,"mosquitto_loop_forever error: %s\n",mosquitto_strerror(rc));
    return 1;
  }

  return 0;
}
