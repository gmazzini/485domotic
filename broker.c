#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <mosquitto.h>

#define VERSION "zigbee_broker v2.0 by GM @2026\n"
#define MQTT_HOST "localhost"
#define MQTT_PORT 1883
#define MQTT_KEEPALIVE 60
#define MQTT_TOPIC "zigbee2mqtt/+"

#define DOMOTIC_IP "2001:678:1158:102:7a7b:8aff:fec3:b986"
#define DOMOTIC_PORT 55556

#define SENSOR_IP "2a06:de00:400:7800::c"
#define SENSOR_PORT 54321

#define MAXPAYLOAD 512
#define MAXACTION 64
#define MAXDEVICE 128
#define MAXMSG 256
#define MAXVAL 64

struct conf{
  char domotic_ip[128];
  int domotic_port;
  char sensor_ip[128];
  int sensor_port;
  char mqtt_host[128];
  int mqtt_port;
};

static struct conf cfg;
static int domotic_fd=-1;
static int sensor_fd=-1;
static struct sockaddr_in6 domotic_addr;
static struct sockaddr_in6 sensor_addr;

static int get_json_string(const char *s,const char *key,char *out,int max){
  char pat[64];
  const char *p;
  int i;

  snprintf(pat,sizeof(pat),"\"%s\":\"",key);

  p=strstr(s,pat);
  if(!p)return 0;

  p+=strlen(pat);
  i=0;

  while(*p && *p!='"' && i<max-1){
    out[i++]=*p++;
  }

  out[i]='\0';

  if(i>0)return 1;
  return 0;
}

static int get_json_value(const char *s,const char *key,char *out,int max){
  char pat[64];
  const char *p;
  int i;

  snprintf(pat,sizeof(pat),"\"%s\":",key);

  p=strstr(s,pat);
  if(!p)return 0;

  p+=strlen(pat);
  i=0;

  if(*p=='"'){
    p++;
    while(*p && *p!='"' && i<max-1){
      out[i++]=*p++;
    }
  }else{
    while(*p && *p!=',' && *p!='}' && *p!=' ' && i<max-1){
      out[i++]=*p++;
    }
  }

  out[i]='\0';

  if(i>0)return 1;
  return 0;
}

static int udp_addr_init(int *fd,struct sockaddr_in6 *addr,char *ip,int port){
  if(*fd>=0)return 1;

  *fd=socket(AF_INET6,SOCK_DGRAM,0);
  if(*fd<0)return 0;

  memset(addr,0,sizeof(*addr));
  addr->sin6_family=AF_INET6;
  addr->sin6_port=htons(port);

  if(inet_pton(AF_INET6,ip,&addr->sin6_addr)!=1){
    close(*fd);
    *fd=-1;
    return 0;
  }

  return 1;
}

static int domotic_init(){
  return udp_addr_init(&domotic_fd,&domotic_addr,cfg.domotic_ip,cfg.domotic_port);
}

static int sensor_init(){
  return udp_addr_init(&sensor_fd,&sensor_addr,cfg.sensor_ip,cfg.sensor_port);
}

static int send_domotic(char *device,char *action){
  char msg[MAXMSG];
  int len,n;

  if(device==NULL || action==NULL)return 0;
  if(!domotic_init())return 0;

  len=snprintf(msg,sizeof(msg),"zigbee %s %s",device,action);
  if(len<1 || len>=(int)sizeof(msg))return 0;

  n=sendto(domotic_fd,msg,strlen(msg),0,(struct sockaddr *)&domotic_addr,sizeof(domotic_addr));
  if(n<0)return 0;

  return 1;
}

static int send_sensor(char *device,char *payload){
  char msg[MAXMSG];
  char temp[MAXVAL],hum[MAXVAL],bat[MAXVAL],lqi[MAXVAL];
  int has_temp,has_hum,has_bat,has_lqi;
  int len,n;

  if(device==NULL || payload==NULL)return 0;
  if(!sensor_init())return 0;

  has_temp=get_json_value(payload,"temperature",temp,sizeof(temp));
  has_hum=get_json_value(payload,"humidity",hum,sizeof(hum));
  has_bat=get_json_value(payload,"battery",bat,sizeof(bat));
  has_lqi=get_json_value(payload,"linkquality",lqi,sizeof(lqi));

  if(!has_temp && !has_hum)return 0;

  if(!has_temp)strcpy(temp,"na");
  if(!has_hum)strcpy(hum,"na");
  if(!has_bat)strcpy(bat,"na");
  if(!has_lqi)strcpy(lqi,"na");

  len=snprintf(msg,sizeof(msg),
    "zigbee_sensor %s temperature %s humidity %s battery %s linkquality %s\n",
    device,temp,hum,bat,lqi);

  if(len<1 || len>=(int)sizeof(msg))return 0;

  n=sendto(sensor_fd,msg,strlen(msg),0,(struct sockaddr *)&sensor_addr,sizeof(sensor_addr));
  if(n<0)return 0;

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

  if(get_json_string(payload,"action",action,sizeof(action))){
    printf("zigbee %s %s\n",device,action);
    fflush(stdout);

    if(!send_domotic(device,action)){
      fprintf(stderr,"send error domotic: %s %s\n",device,action);
      fflush(stderr);
    }

    return;
  }

  if(strstr(payload,"\"temperature\"") || strstr(payload,"\"humidity\"")){
    printf("zigbee_sensor %s %s\n",device,payload);
    fflush(stdout);

    if(!send_sensor(device,payload)){
      fprintf(stderr,"send error sensor: %s\n",device);
      fflush(stderr);
    }

    return;
  }
}

static void usage(char *name){
  printf("%s",VERSION);
  printf("usage:\n");
  printf("  %s [domotic_ipv6] [domotic_port] [mqtt_host] [mqtt_port] [sensor_ipv6] [sensor_port]\n",name);
  printf("\n");
  printf("defaults:\n");
  printf("  domotic_ipv6: %s\n",DOMOTIC_IP);
  printf("  domotic_port: %d\n",DOMOTIC_PORT);
  printf("  mqtt_host: %s\n",MQTT_HOST);
  printf("  mqtt_port: %d\n",MQTT_PORT);
  printf("  sensor_ipv6: %s\n",SENSOR_IP);
  printf("  sensor_port: %d\n",SENSOR_PORT);
}

int main(int argc,char **argv){
  struct mosquitto *m;
  int rc;

  strcpy(cfg.domotic_ip,DOMOTIC_IP);
  cfg.domotic_port=DOMOTIC_PORT;
  strcpy(cfg.sensor_ip,SENSOR_IP);
  cfg.sensor_port=SENSOR_PORT;
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

  if(argc>5){
    strncpy(cfg.sensor_ip,argv[5],sizeof(cfg.sensor_ip)-1);
    cfg.sensor_ip[sizeof(cfg.sensor_ip)-1]='\0';
  }

  if(argc>6)cfg.sensor_port=atoi(argv[6]);

  if(!domotic_init()){
    fprintf(stderr,"domotic udp init error\n");
    return 1;
  }

  if(!sensor_init()){
    fprintf(stderr,"sensor udp init error\n");
    return 1;
  }

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
  printf("sensor: [%s]:%d\n",cfg.sensor_ip,cfg.sensor_port);
  printf("mode: udp fire and forget\n");
  fflush(stdout);

  rc=mosquitto_loop_forever(m,-1,1);

  if(domotic_fd>=0)close(domotic_fd);
  if(sensor_fd>=0)close(sensor_fd);

  mosquitto_destroy(m);
  mosquitto_lib_cleanup();

  if(rc!=MOSQ_ERR_SUCCESS){
    fprintf(stderr,"mosquitto_loop_forever error: %s\n",mosquitto_strerror(rc));
    return 1;
  }

  return 0;
}
