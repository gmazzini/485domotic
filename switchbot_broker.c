#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <mosquitto.h>

#define VERSION "switchbot_broker v1.2 by GM @2026\n"

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
#define MAXBYTES 64
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
  unsigned char c;

  i=0;
  j=0;

  while(src[i] && j<max-1){
    if(src[i]!=':'){
      c=(unsigned char)src[i];
      dst[j++]=(char)toupper(c);
    }
    i++;
  }

  dst[j]='\0';
}

static int hexval(char c){
  if(c>='0' && c<='9')return c-'0';
  if(c>='a' && c<='f')return c-'a'+10;
  if(c>='A' && c<='F')return c-'A'+10;
  return -1;
}

static int hex_to_bytes(const char *hex,unsigned char *out,int maxout){
  int len;
  int i;
  int hi,lo;
  int n;

  if(hex==NULL)return -1;

  len=strlen(hex);
  if(len<2)return -1;
  if(len%2!=0)return -1;

  n=len/2;
  if(n>maxout)return -1;

  for(i=0;i<n;i++){
    hi=hexval(hex[i*2]);
    lo=hexval(hex[i*2+1]);

    if(hi<0 || lo<0)return -1;

    out[i]=(unsigned char)((hi<<4)|lo);
  }

  return n;
}

static void mac_from_mfg(char *dst,int max,const unsigned char *b,int n){
  if(n<8){
    dst[0]='\0';
    return;
  }

  snprintf(dst,max,"%02X%02X%02X%02X%02X%02X",
    b[2],b[3],b[4],b[5],b[6],b[7]);
}

static int decode_switchbot_mfg(const char *hex,char *mac,int macmax,char *temp,int tempmax,char *hum,int hummax){
  unsigned char b[MAXBYTES];
  int n;
  int dec;
  int ti;
  int sign;
  int h;

  n=hex_to_bytes(hex,b,sizeof(b));
  if(n<13)return 0;

  /*
    Formato visto dai Meter/Meter Plus pubblicati grezzi da Theengs:

      69 09  e7 76 01 46 67 4c  ce 03  04 a1 30
      -----  -----------------  -----  --------
      cid    mac                aux    temp/hum

    byte 10 = decimale temperatura
    byte 11 = intero temperatura + segno
    byte 12 = umidita

    Esempio:
      04 a1 30 -> 33.4 C, 48 %
      05 9e 37 -> 30.5 C, 55 %
  */

  if(b[0]!=0x69 || b[1]!=0x09)return 0;

  dec=b[10] & 0x0f;
  ti=b[11] & 0x7f;
  sign=b[11] & 0x80;
  h=b[12] & 0x7f;

  if(mac!=NULL && macmax>0){
    mac_from_mfg(mac,macmax,b,n);
  }

  if(sign){
    snprintf(temp,tempmax,"%d.%d",ti,dec);
  }else{
    snprintf(temp,tempmax,"-%d.%d",ti,dec);
  }

  snprintf(hum,hummax,"%d",h);

  return 1;
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
  snprintf(devs[freepos].mac,sizeof(devs[freepos].mac),"%s",mac);
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

  snprintf(st->temp,sizeof(st->temp),"%s",temp);
  snprintf(st->hum,sizeof(st->hum),"%s",hum);

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

static int extract_switchbot_data(const char *payload,char *mac,int macmax,char *temp,int tempmax,char *hum,int hummax){
  char brand[MAXVAL];
  char mfr[MAXVAL];
  char id[MAXVAL];
  char mfg[MAXVAL];

  brand[0]='\0';
  mfr[0]='\0';
  id[0]='\0';
  mfg[0]='\0';

  get_json_string(payload,"brand",brand,sizeof(brand));
  get_json_string(payload,"mfr",mfr,sizeof(mfr));
  get_json_string(payload,"id",id,sizeof(id));
  get_json_string(payload,"manufacturerdata",mfg,sizeof(mfg));

  /*
    Caso 1: Theengs ha gia' decodificato.
  */
  if(strcmp(brand,"SwitchBot")==0){
    if(get_json_value(payload,"tempc",temp,tempmax) &&
       get_json_value(payload,"hum",hum,hummax)){

      if(id[0]){
        mac_clean(mac,macmax,id);
      }else if(mfg[0]){
        if(!decode_switchbot_mfg(mfg,mac,macmax,temp,tempmax,hum,hummax)){
          return 0;
        }
      }else{
        return 0;
      }

      return 1;
    }
  }

  /*
    Caso 2: Theengs non ha decodificato, ma ha riconosciuto il produttore
    come Woan Technology, cioe' SwitchBot, oppure espone manufacturerdata
    con company id 0x0969 in forma little-endian "6909...".
  */
  if((mfr[0] && strstr(mfr,"Woan Technology")!=NULL) ||
     (mfg[0] && strncmp(mfg,"6909",4)==0)){

    if(!mfg[0])return 0;

    if(!decode_switchbot_mfg(mfg,mac,macmax,temp,tempmax,hum,hummax)){
      return 0;
    }

    /*
      Se l'id e' presente, preferiamo quello pubblicato da Theengs.
      Se non c'e', resta il MAC ricavato dal manufacturerdata.
    */
    if(id[0]){
      mac_clean(mac,macmax,id);
    }

    return 1;
  }

  return 0;
}

static void on_message(struct mosquitto *m,void *u,const struct mosquitto_message *msg){
  char payload[MAXPAYLOAD];
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

  mac[0]='\0';
  temp[0]='\0';
  hum[0]='\0';

  if(!extract_switchbot_data(payload,mac,sizeof(mac),temp,sizeof(temp),hum,sizeof(hum))){
    return;
  }

  if(mac[0]=='\0' || temp[0]=='\0' || hum[0]=='\0'){
    return;
  }

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
    return 1;
  }

  printf("%s",VERSION);
  printf("mqtt: %s:%d topic %s\n",MQTT_HOST,MQTT_PORT,MQTT_TOPIC);
  printf("sensor: [%s]:%d\n",SENSOR_IP,SENSOR_PORT);
  printf("output: switchbot_sensor MAC temperature TEMP humidity HUM\n");
  printf("send rule: on change or every %d seconds\n",SEND_INTERVAL);
  printf("decode: Theengs decoded fields or raw Woan/SwitchBot manufacturerdata 6909\n");
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
