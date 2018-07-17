#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
// CE pin is hardwired to 3.3V, so not needed
#define CE_PIN -1
#define CSN_PIN 10

const uint64_t pipe = 0xE8E8F0F0E1LL;   //change this pipe if you're getting someone else's data
RF24 radio(CE_PIN, CSN_PIN);

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

#define MINPRESSURE 10
#define MAXPRESSURE 1000

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY    0x8410

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

char history[3][3][100]; //use char because its range is -128 to 127
char d[4];               //buffer for incoming
int sampleno=0;          //number of sample
unsigned long t;         //for measuring time between samples
byte smode=0;            //whether showing days/hours/minutes on graph
int viewsample=0;        //current sample
#define SAMPLET 60000

void setup(){
  Serial.begin(9600);
  tft.reset();                    //initialise screen
  tft.begin(0x9341);
  tft.setRotation(2);
  tft.fillScreen(BLACK);          //blank screen
  radio.begin();                  //open XC4508 in reading mode
  radio.openReadingPipe(1,pipe);
  radio.startListening();
  pinMode(13, OUTPUT);
  tft.setCursor(0, 0);            //show fixed parts of screen
  tft.setTextColor(GREY);
  tft.setTextSize(2);
  tft.println("   GARDEN MONITOR");
  tft.setTextSize(1);
  for(int i=0;i<11;i++){
    tft.setTextColor(BLUE);
    tft.setCursor(218, i*20+16);
    tft.print((10-i)*10);
    tft.setTextColor(RED);
    tft.setCursor(202, i*20+16);
    tft.print((10-i)*5);    
  }
  tft.setCursor(0, 240);
  tft.setTextColor(RED);
  tft.println("  TEMPERATURE");
  tft.setCursor(80, 240);
  tft.setTextColor(BLUE);
  tft.println("  HUMIDITY");
  tft.setCursor(160, 240);
  tft.setTextColor(GREEN);
  tft.println("SOIL MOISTURE");
  tft.setTextColor(GREY);
  tft.setCursor(0, 310);
  tft.println("   ONE WEEK     ONE DAY       ONE HOUR");
  
  for(int i=0;i<100;i++){    //change to default invalid value
    for(int x=0;x<3;x++){
      d[x]=-128;
      for(int y=0;y<3;y++){
        history[x][y][i]=-128;
      }
    }
  }
  updatescreen();             //update the rest of the screen
  t=millis();                 //start sample timer
}

void loop(){
  checktouch();               //check touch sensor
  if(radio.available()){
    rstatus(GREEN);           //show a radio signal received
    radio.read(d,sizeof(d));  //read data
    delay(200);
    rstatus(BLACK);
  }
  if(millis()>(t+SAMPLET)){    //grab a sample
    t=t+SAMPLET;
    for(int i=98;i>-1;i--){    //minutely sample
      history[0][0][i+1]=history[0][0][i];    //move data one spot
      history[0][1][i+1]=history[0][1][i];    //move data one spot
      history[0][2][i+1]=history[0][2][i];    //move data one spot
    }
    history[0][0][0]=d[0];
    history[0][1][0]=d[1];
    history[0][2][0]=d[2];    
    if((sampleno%15)==0){     //hourly sample
      for(int i=98;i>-1;i--){
        history[1][0][i+1]=history[1][0][i];    //move data one spot
        history[1][1][i+1]=history[1][1][i];    //move data one spot
        history[1][2][i+1]=history[1][2][i];    //move data one spot
      }
      history[1][0][0]=d[0];
      history[1][1][0]=d[1];
      history[1][2][0]=d[2];          
    }
    if((sampleno%120)==0){     //daily sample
      for(int i=98;i>-1;i--){
        history[2][0][i+1]=history[2][0][i];    //move data one spot
        history[2][1][i+1]=history[2][1][i];    //move data one spot
        history[2][2][i+1]=history[2][2][i];    //move data one spot
      }
      history[2][0][0]=d[0];
      history[2][1][0]=d[1];
      history[2][2][0]=d[2];          
    }
    sampleno=(sampleno+1)%120;
    d[0]=-128;    //change to default invalid value
    d[1]=-128;
    d[2]=-128;
  updatescreen();
  }
}

void updatescreen(){
  int n,q;
  tft.fillRect(0, 20, 200, 200, BLACK);        //blank graph display area  
  tft.drawFastVLine(0, 20, 200, GREY);
  for(int i=20;i<240;i=i+20){tft.drawFastHLine(0, i, 200, GREY);}    //horizontal gridlines
  q=history[smode][0][0];
  for(int i=1;i<100;i++){
    n=history[smode][0][i];
    if(n>-1){if(n<51){if(q>-1){if(q<51){tft.drawLine(200-2*i, 220-4*q, 198-2*i,220-4*n , RED);}}}}   //temperature
    q=n;
  }
  q=history[smode][1][0];
  for(int i=1;i<100;i++){
    n=history[smode][1][i];
    if(n>-1){if(n<101){if(q>-1){if(q<101){tft.drawLine(200-2*i, 220-2*q, 198-2*i,220-2*n , BLUE);}}}}   //humidity
    q=n;
  }
  q=history[smode][2][0];
  for(int i=1;i<100;i++){
    n=history[smode][2][i];
    if(n>-1){if(n<101){if(q>-1){if(q<101){tft.drawLine(200-2*i, 220-2*q, 198-2*i,220-2*n , GREEN);}}}}   //soil
    q=n;
  }
  tft.fillRect(0, 224, 230, 10, BLACK);       //blank x-scale area
  tft.setTextColor(GREY);         //do vertical gridlines based on mode
  tft.setTextSize(1);
  if(smode==0){
    for(int i=0;i<100;i=i+15){  //line every 15 minutes
    tft.drawFastVLine(200-i*2, 20, 200, GREY);
    tft.setCursor(194-i*2,224);
    tft.print(i);
    tft.print("m");  
    }
  }
  if(smode==1){
    for(int i=0;i<100;i=i+12){  //line every 3 hours
    tft.drawFastVLine(200-i*2, 20, 200, GREY);
    tft.setCursor(194-i*2,224);
    tft.print(i/4);
    tft.print("h");  
    }
  }
  if(smode==2){
    for(int i=0;i<100;i=i+12){  //line every 1 day
    tft.drawFastVLine(200-i*2, 20, 200, GREY);
    tft.setCursor(194-i*2,224);
    tft.print(i/12);
    tft.print("d");  
    }
  }
  tft.drawFastVLine(200-viewsample*2, 20, 200, WHITE);                  //cursor for sample in focus

  tft.fillRect(0, 250, 239, 50, BLACK);                                 //blank numeric display area
  if(viewsample==0){tft.setTextColor(WHITE);}else{tft.setTextColor(GREY);}    //change text to grey if highlighting old data
  tft.setTextSize(4);
  tft.setCursor(20, 260);                                              //display numeric data "--" if invalid
  n=history[smode][0][viewsample];
  if(n==-128){tft.print("--");}else{tft.print(n);}
  tft.setCursor(100, 260);
  n=history[smode][1][viewsample];
  if(n==-128){tft.print("--");}else{tft.print(n);}
  tft.setCursor(180, 260);
  n=history[smode][2][viewsample];
  if(n==-128){tft.print("--");}else{tft.print(n);}

}
void rstatus(unsigned int c){               //flash 'R' in top right corner
  tft.setCursor(224,0);
  tft.setTextColor(c);
  tft.setTextSize(2);
  tft.println("R");
}

void checktouch(){                          
  static byte lasttouch=0;                  //avoid repeated presses when held down
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  if(p.z > MINPRESSURE && p.z < MAXPRESSURE){
    if(lasttouch==0){       //only act on first press, ignore if held
      if(p.y>800){      //bottom of screen- choosing week/day/hour
        smode=p.x/350;    //0=hour,1=day,2=week
      }
      if(p.y<650){          //set cursor position
        viewsample=(p.x-230)/7;     //these values might change depending on touchscreen calibration
        if(viewsample<0){viewsample=0;}
        if(viewsample>99){viewsample=99;}        
      }
      lasttouch=1;
      updatescreen();
    }
  }else{
    lasttouch=0;
  }
}

