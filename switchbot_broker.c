#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <mosquitto.h>

#define VERSION "switchbot_broker v1.1 by GM @2026\n"

#define MQTT_HOST "localhost"
#define MQTT_PORT 1883
#define MQTT_KEEPALIVE 60
#define MQTT_TOPIC "home/TheengsGateway/BTtoMQTT/+"

#define SENSOR_IP "2a06:de00:400:7800::c"
#define SENSOR_PORT 54321

#define MAXPAYLOAD 1024
#define MAXVAL 128
#define MAXMSG 256
#define MAXDEV 64
#define SEND_INTERVAL 300

struct devstate{
  int used;
  char mac[MAXVAL];
  char temp[MAXVAL];
  char hum[MAXVAL];
  time_t last_send;
};

static int sensor_fd=-1;
static struct sockaddr_in6 sensor_addr;
static struct devstate devs[MAXDEV];

static const char *json_skip_ws(const char *p){
  while(*p==' ' || *p=='\t' || *p=='\r' || *p=='\n'){
    p++;
  }

  return p;
}

static const char *json_find_value(const char *s,const char *key){
  char pat[64];
  const char *p;

  snprintf(pat,sizeof(pat),"\"%s\"",key);

  p=strstr(s,pat);
  if(!p)return NULL;

  p+=strlen(pat);
  p=json_skip_ws(p);

  if(*p!=':')return NULL;

  p++;
  p=json_skip_ws(p);

  return p;
}

static int get_json_string(const char *s,const char *key,char *out,int max){
  const char *p;
  int i;

  p=json_find_value(s,key);
  if(!p)return 0;
  if(*p!='"')return 0;

  p++;
  i=0;

  while(*p && *p!='"' && i<max-1){
    out[i++]=*p++;
  }

  out[i]='\0';

  if(i>0)return 1;
  return 0;
}

static int get_json_value(const char *s,const char *key,char *out,int max){
  const char *p;
  int i;

  p=json_find_value(s,key);
  if(!p)return 0;

  i=0;

  if(*p=='"'){
    p++;
    while(*p && *p!='"' && i<max-1){
      out[i++]=*p++;
    }
  }else{
    while(*p && *p!=',' && *p!='}' && *p!=' ' && *p!='\t' && *p!='\r' && *p!='\n' && i<max-1){
      out[i++]=*p++;
    }
  }

  out[i]='\0';

  if(i>0)return 1;
  return 0;
}

static void mac_clean(char *dst,int max,const char *src){
  int i,j;

  i=0;
  j=0;

  while(src[i] && j<max-1){
    if(src[i]!=':'){
      dst[j++]=src[i];
    }
    i++;
  }

  dst[j]='\0';
}

static int sensor_init(){
  if(sensor_fd>=0)return 1;

  sensor_fd=socket(AF_INET6,SOCK_DGRAM,0);
  if(sensor_fd<0)return 0;

  memset(&sensor_addr,0,sizeof(sensor_addr));
  sensor_addr.sin6_family=AF_INET6;
  sensor_addr.sin6_port=htons(SENSOR_PORT);

  if(inet_pton(AF_INET6,SENSOR_IP,&sensor_addr.sin6_addr)!=1){
    close(sensor_fd);
    sensor_fd=-1;
    return 0;
  }

  return 1;
}

static struct devstate *get_devstate(char *mac){
  int i;
  int freepos;

  freepos=-1;

  for(i=0;i<MAXDEV;i++){
    if(devs[i].used && strcmp(devs[i].mac,mac)==0){
      return &devs[i];
    }

    if(!devs[i].used && freepos<0){
      freepos=i;
    }
  }

  if(freepos<0)return NULL;

  devs[freepos].used=1;
  strncpy(devs[freepos].mac,mac,sizeof(devs[freepos].mac)-1);
  devs[freepos].mac[sizeof(devs[freepos].mac)-1]='\0';
  devs[freepos].temp[0]='\0';
  devs[freepos].hum[0]='\0';
  devs[freepos].last_send=0;

  return &devs[freepos];
}

static int should_send(struct devstate *st,char *temp,char *hum){
  time_t now;

  if(st==NULL)return 0;

  now=time(NULL);

  if(st->last_send==0)return 1;

  if(strcmp(st->temp,temp)!=0)return 1;
  if(strcmp(st->hum,hum)!=0)return 1;

  if(now-st->last_send>=SEND_INTERVAL)return 1;

  return 0;
}

static void update_devstate(struct devstate *st,char *temp,char *hum){
  if(st==NULL)return;

  strncpy(st->temp,temp,sizeof(st->temp)-1);
  st->temp[sizeof(st->temp)-1]='\0';

  strncpy(st->hum,hum,sizeof(st->hum)-1);
  st->hum[sizeof(st->hum)-1]='\0';

  st->last_send=time(NULL);
}

static int send_sensor(char *mac,char *temp,char *hum){
  char msg[MAXMSG];
  int len,n;

  if(mac==NULL || temp==NULL || hum==NULL)return 0;
  if(!sensor_init())return 0;

  len=snprintf(msg,sizeof(msg),
    "switchbot_sensor %s temperature %s humidity %s\n",
    mac,temp,hum);

  if(len<1 || len>=(int)sizeof(msg))return 0;

  n=sendto(sensor_fd,msg,strlen(msg),0,(struct sockaddr *)&sensor_addr,sizeof(sensor_addr));
  if(n<0)return 0;

  return 1;
}

static void on_message(struct mosquitto *m,void *u,const struct mosquitto_message *msg){
  char payload[MAXPAYLOAD];
  char brand[MAXVAL];
  char id[MAXVAL];
  char mac[MAXVAL];
  char temp[MAXVAL];
  char hum[MAXVAL];
  struct devstate *st;
  int n;

  (void)m;
  (void)u;

  if(msg==NULL)return;
  if(msg->retain)return;
  if(msg->payload==NULL)return;

  n=msg->payloadlen;
  if(n>=MAXPAYLOAD)n=MAXPAYLOAD-1;

  memcpy(payload,msg->payload,n);
  payload[n]='\0';

  if(!get_json_string(payload,"brand",brand,sizeof(brand)))return;
  if(strcmp(brand,"SwitchBot")!=0)return;

  if(!get_json_string(payload,"id",id,sizeof(id)))return;
  if(!get_json_value(payload,"tempc",temp,sizeof(temp)))return;
  if(!get_json_value(payload,"hum",hum,sizeof(hum)))return;

  mac_clean(mac,sizeof(mac),id);

  st=get_devstate(mac);
  if(st==NULL){
    fprintf(stderr,"too many switchbot devices\n");
    fflush(stderr);
    return;
  }

  if(!should_send(st,temp,hum)){
    return;
  }

  printf("switchbot_sensor %s temperature %s humidity %s\n",mac,temp,hum);
  fflush(stdout);

  if(!send_sensor(mac,temp,hum)){
    fprintf(stderr,"send error switchbot_sensor %s\n",mac);
    fflush(stderr);
    return;
  }

  update_devstate(st,temp,hum);
}

int main(){
  struct mosquitto *m;
  int rc;

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

  rc=mosquitto_connect(m,MQTT_HOST,MQTT_PORT,MQTT_KEEPALIVE);
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
  printf("mqtt: %s:%d topic %s\n",MQTT_HOST,MQTT_PORT,MQTT_TOPIC);
  printf("sensor: [%s]:%d\n",SENSOR_IP,SENSOR_PORT);
  printf("output: switchbot_sensor MAC temperature TEMP humidity HUM\n");
  printf("send rule: on change or every %d seconds\n",SEND_INTERVAL);
  fflush(stdout);

  rc=mosquitto_loop_forever(m,-1,1);

  if(sensor_fd>=0)close(sensor_fd);

  mosquitto_destroy(m);
  mosquitto_lib_cleanup();

  if(rc!=MOSQ_ERR_SUCCESS){
    fprintf(stderr,"mosquitto_loop_forever error: %s\n",mosquitto_strerror(rc));
    return 1;
  }

  return 0;
}
