#include <Wire.h>
#include "ADS1X15.h"
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

unsigned long t1,t2;
float y,x,a1,a2,a3,oo;
int i;
int16_t v1,v2,v3;

ADS1115 conv1(0x48);
ADS1115 conv2(0x49);
ADS1115 conv3(0x4A);

EthernetUDP Udp;
byte mac[]={0x90, 0xA2, 0xDA, 0x0D, 0x5C, 0x18};
IPAddress ipsrc(10,0,0,29);
unsigned int portsrc=8888;
IPAddress ipdst(37,114,41,193);
unsigned int portdst=8888;
IPAddress dns(8,8,8,8);
IPAddress gateway(10,0,0,1);
IPAddress subnet(255,255,255,0);

char buf[10];

void setup() {
   Ethernet.begin(mac,ipsrc,dns,gateway,subnet);
   Udp.begin(portsrc);
   conv1.setGain(2);
   conv2.setGain(2);
   conv3.setGain(2);
   conv1.setMode(1);
   conv2.setMode(1);
   conv3.setMode(1);
   conv1.setDataRate(7);
   conv2.setDataRate(7);
   conv3.setDataRate(7);
   conv1.begin();
   conv2.begin();
   conv3.begin();
}

void loop() {
  a1=a2=a3=0.0;
  i=0;
  t1=millis();
  while(millis()-t1<1000){
    conv1.requestADC_Differential_2_3();
    conv2.requestADC_Differential_2_3();
    conv3.requestADC_Differential_2_3();
    v1=conv1.getValue();  
    v2=conv2.getValue();  
    v3=conv3.getValue();
    y=50*conv3.toVoltage(v1);
    a1+=y*y;
    y=50*conv3.toVoltage(v2);
    a2+=y*y;
    y=50*conv3.toVoltage(v3);
    a3+=y*y;
    i++;
  }
  Udp.beginPacket(ipdst,portdst);
  Udp.print(t1); Udp.print(" ");
  Udp.print(i); Udp.print(" ");
  oo=sqrt(a1/i); dtostrf(oo,7,5,buf); Udp.print(buf); Udp.print(" ");
  oo=sqrt(a2/i); dtostrf(oo,7,5,buf); Udp.print(buf); Udp.print(" ");
  oo=sqrt(a3/i); dtostrf(oo,7,5,buf); Udp.println(buf);
  Udp.endPacket();
}
