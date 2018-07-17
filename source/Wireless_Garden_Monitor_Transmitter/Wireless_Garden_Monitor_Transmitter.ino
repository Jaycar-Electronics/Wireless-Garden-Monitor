//Transmitter for Wireless Garden Monitor using XC4508 Radio

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#define CE_PIN -1
#define CSN_PIN 10
const uint64_t pipe = 0xE8E8F0F0E1LL;   //this pipe needs to match receiver
RF24 radio(-1, 10);                     //CE pin is hardwired to 3.3V, CS is 10
char d[4];                              //data to be transmitted- 3 bytes plus checksum

//for DHT11 interface (XC4520)
#define DHT11PIN 8
byte DHTtemp,DHThum;
int DHTstatus=0;

//analog pin for reading soil sensor (XC4604)
#define SOILPIN A5

void setup(){
  radio.begin();
  radio.openWritingPipe(pipe);        //set up radio
  DHTsetup();                         //start temp/humidity sensor
}

void loop(){
  d[0]=-128;                          //default error value
  d[1]=-128;
  if(DHT11()){                        //if valid data available
    d[0]=DHTtemp;                     //load data
    d[1]=DHThum;
  }
  d[2]=(analogRead(SOILPIN)*25)/256;  //convert to 0-100 scale
  d[3]=d[0]^d[1]^d[2];                //calculate checksum
  radio.write(d,sizeof(d));           //send data
  delay(5000);                        //wait 5s til next send
}

void DHTsetup(){                      //set pin to output, set high for idle state
  pinMode(DHT11PIN,OUTPUT);
  digitalWrite(DHT11PIN,HIGH);
}

int DHT11(){                    //returns 1 on ok, 0 on fail (eg checksum, data not received)
  unsigned int n[83];           //to store bit times
  byte p=0;                     //pointer to current bit
  unsigned long t;              //time
  byte old=0;
  byte newd;
  for(int i=0;i<83;i++){n[i]=0;}
  digitalWrite(DHT11PIN,LOW);   //start signal
  delay(25);
  digitalWrite(DHT11PIN,HIGH);
  delayMicroseconds(20);
  pinMode(DHT11PIN,INPUT);
  delayMicroseconds(40);
  t=micros()+10000L;
  while((micros()<t)&&(p<83)){    //read bits
    newd=digitalRead(DHT11PIN);
    if(newd!=old){
      n[p]=micros();
      p++;
      old=newd;
    }
  }
  pinMode(DHT11PIN,OUTPUT);      //reset for next cycle
  digitalWrite(DHT11PIN,HIGH);
  if(p!=83){return 0;}           //not a valid datastream
  byte data[5]={0,0,0,0,0};
  for(int i=0;i<40;i++){         //store data in array
    if(n[i*2+3]-n[i*2+2]>50){data[i>>3]=data[i>>3]|(128>>(i&7));}
  }
  byte k=0;     //checksum
  for(int i=0;i<4;i++){k=k+data[i];}
  if((k^data[4])){return 0;}      //checksum error
  DHTtemp=data[2];                //temperature
  DHThum=data[0];                 //humidity
  return 1;                       //data valid
}

