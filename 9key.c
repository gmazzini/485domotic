// 9 keys by GM ver 20240829
#define DEV 2
#define STAY 6
#define LED 13
#define ENABLETX 11
int o[9],c[9],u[9],toreset;
unsigned long laststart;

void setup() {
  int i;
  Serial.begin(9600);
  for(i=0;i<9;i++){
    pinMode(i+2,INPUT_PULLUP);
    o[i]=1;
    c[i]=STAY;
    u[i]=1;
  }
  pinMode(ENABLETX,OUTPUT);
  digitalWrite(ENABLETX,0);
  pinMode(LED,OUTPUT);
  digitalWrite(LED,0);
  toreset=0;
}

void loop() {
  int i,j;
  for(i=0;i<9;i++){
    if(toreset && (unsigned long)millis()-laststart>4){
      toreset=0;
      digitalWrite(ENABLETX,0);
      digitalWrite(LED,0);
    }
    j=digitalRead(i+2);
    if(o[i]==j){
      if(c[i]>0)c[i]--;
      else {
        if(u[i]!=j){
          u[i]=j;
          digitalWrite(ENABLETX,1);
          digitalWrite(LED,1);
          delay(1);
          Serial.write(0xff);
          Serial.write(DEV);
          Serial.write((9-i)<<4|(1-j));
          toreset=1;
          laststart=millis();
        }
      }
    }
    else {
      o[i]=j;
      c[i]=STAY;
    }
  }
}
