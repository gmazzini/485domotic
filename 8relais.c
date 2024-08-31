// 8 relais by GM ver 20240831
// cmd CC <dev,0=all> <relais(1..8,f=all)><status(01)>

#define DEV 1
#define LED 13
#define ENABLETX 11

void setup() {
  int r;
  Serial.begin(9600);
  for(r=1;r<=8;r++)pinMode(r+1,OUTPUT);
  pinMode(ENABLETX,OUTPUT);
  digitalWrite(ENABLETX,0);
  pinMode(LED,OUTPUT);
  digitalWrite(LED,0);
}

void loop() {
  int in,r,o;
  if(Serial.available()>0){
    in=Serial.read();
    if(in==0xcc){
      in=Serial.read();
      if(in==DEV || in==0){
        in=Serial.read();
        r=in>>4;
        o=in&1;
        if(r>=1 && r<=8)digitalWrite(r+1,o);
        if(r==15)for(r=1;r<=8;r++)digitalWrite(r+1,o);
      }
    }
  }
}
