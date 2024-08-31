// 9 keys by GM ver 20240831
// cmd FF <dev> <key(1..9)><status(01)>

#define DEV 2
#define STAY 6
#define LED 13
#define ENABLETX 11
char mymap[9]={0x70,0x80,0x90,0x40,0x50,0x60,0x10,0x20,0x30};
char o[9],c[9],u[9],toreset;
unsigned long laststart;

void setup() {
  char i;
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
  char i,j;
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
          Serial.write(mymap[i]|(1-j));
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
