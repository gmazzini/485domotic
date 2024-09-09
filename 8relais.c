// 8 relais by GM ver 20240909
// cmd CC <dev,0=all> <relais(1..8,f=all)><status(01)> <crc8>

#define DEV 1

void setup(){
  int r;
  Serial.begin(9600);
  for(r=1;r<=8;r++){
    pinMode(r+1,OUTPUT);
    digitalWrite(r+1,1);
  }
}

void loop(){
  unsigned char v[4],i,r,o;
  if(Serial.available()==0)return;
  v[0]=Serial.read();
  if(v[0]!=0xcc)return;
  v[1]=Serial.read();
  v[2]=Serial.read();
  v[3]=Serial.read();
  for(v[4]=0,i=0;i<3;i++)v[4]=crc8_table[(v[4] ^ v[i])];
  if(v[3]!=v[4])return;
  if(v[1]==DEV || v[1]==0){
    r=v[2]>>4;
    o=v[2]&1;
    if(r>=1 && r<=8)digitalWrite(r+1,1-o);
    if(r==15)for(r=1;r<=8;r++)digitalWrite(r+1,1-o);
  }
}
