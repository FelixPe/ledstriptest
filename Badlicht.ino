// public domain
// Interactive RGB-Strip Test

#include "bitlash.h"

int pinRed = 3;
int pinGreen = 6;
int pinBlue = 9;

// NRF24L01-Pins
// IRQ - D2
// CE  - D7
// CSN - D10 / SS


int pinLight = A6;  // the analog pin connected to the light sensor
int pinPir = 8;    //the digital pin connected to the PIR sensor's output
int pinLed = 13;    // onboard LED as debug output

//the time we give the sensor to calibrate (10-60 secs according to the datasheet)
int calibrationTime = 10;        
int fadedelay = 10;   // make this higher to slow down
long unsigned int lastFade = 0;

//the time when the sensor outputs a low impulse
long unsigned int lowIn;         

//the amount of milliseconds the sensor has to be low 
//before we assume all motion has stopped
long unsigned int pause = 90000;  

#define dark 0
#define dawn 1
#define bright 2
int lockStatus = dark;

boolean takeLowTime;  

boolean fading = false;

int helligkeit = 0; // Variabel helligkeit auf 0 setzen

int lr = 0, lg = 0, lb = 0;
int lrs = 1, lgs = 1, lbs = 1;


void setColorRGB(int r, int g, int b){
    assignVar('r'-'a', r);
    assignVar('g'-'a', g);
    assignVar('b'-'a', b);
    fade();
}

numvar fade(void){
  setStep();
  fading = true;
  return 0;
}

numvar setColor(void){
  if(getarg(0) == 3){
    setColorRGB(getarg(1), getarg(2), getarg(3));
    return 1;  
  }else if(getarg(0) == 1){
    setColorRGB(getarg(1), getarg(1), getarg(1));
    return 1;
  }
  return 0;
}

void setStep(){
    int r_soll = getVar('r'-'a') << 6;
    int g_soll = getVar('g'-'a') << 6;
    int b_soll = getVar('b'-'a') << 6;
    
    lrs = abs(r_soll - lr)>>6;    
    lgs = abs(g_soll - lg)>>6;
    lbs = abs(b_soll - lb)>>6;
    
    if(lrs < 1) lrs = 1;  
    if(lbs < 1) lbs = 1;
    if(lgs < 1) lgs = 1;

}

void setup() {
  TCCR1B &= ~_BV(CS12);
  initBitlash(57600);     // must be first to initialize serial port
  
  assignVar('p'-'a', pause); // delay to turn-off
  assignVar('h'-'a', 40); // brightness
  assignVar('d'-'a',fadedelay); // fadedelay
  assignVar('r'-'a',5000); // random delay
  assignVar('c'-'a',0); // candle mode
  
  addBitlashFunction("sc", (bitlash_function)setColor);
  addBitlashFunction("fade", (bitlash_function)fade);
  
  
  pinMode(pinPir, INPUT);
  pinMode(pinLed, OUTPUT);

  pinMode(pinRed, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  pinMode(pinBlue, OUTPUT);

  digitalWrite(pinPir, LOW);

  //give the sensor some time to calibrate
  Serial.print("calibrating sensor ");
    for(int i = 0; i < calibrationTime; i++){
      for( int t = 0; t < 250; t++){
             analogWrite(pinRed, t); 
             analogWrite(pinGreen, t); 
             analogWrite(pinBlue, t);
             delay(2);
      }
      for( int t = 250; t >= 0; t--){
             analogWrite(pinRed, t); 
             analogWrite(pinGreen, t); 
             analogWrite(pinBlue, t);
             delay(2);
      }
      Serial.print(".");
      }
    Serial.println(" done");
    Serial.println("SENSOR ACTIVE");
    delay(50);
    setColorRGB(0,0,0);
}
 

void loop() {
  
  runBitlash();


     if(digitalRead(pinPir) == HIGH){
       if(lockStatus < bright){  
         helligkeit = analogRead(pinLight); // Helligkeitssensor auslesen
         if(lockStatus == dawn || helligkeit < getVar('h'-'a')){
           //setColorRGB(240,192,112);
           doCommand("on()");
		   digitalWrite(pinLed, HIGH);   //the led visualizes the sensors output pin state
           //makes sure we wait for a transition to LOW before any further output is made:
           lockStatus = bright; // State-Change last
         }         
       }         
       takeLowTime = true;  
     }

     if(digitalRead(pinPir) == LOW){       
       digitalWrite(pinLed, LOW);  //the led visualizes the sensors output pin state

       if(takeLowTime){
        lowIn = millis();          //save the time of the transition from high to LOW
        takeLowTime = false;       //make sure this is only done at the start of a LOW phase
       }
       
       //if the sensor is low for more than the given pause, 
       //we assume that no more motion is going to happen
       if(lockStatus == bright && millis() - lowIn > getVar('p'-'a')/2){  
           //setColorRGB(192,96,48);
           doCommand("un()");
           //makes sure this block of code is only executed again after 
           //a new motion sequence has been detected
           lockStatus = dawn; 
        }
        
        if(lockStatus == dawn  && millis() - lowIn > getVar('p'-'a')){  
           //setColorRGB(0,0,0);
           doCommand("off()");
           //makes sure this block of code is only executed again after 
           //a new motion sequence has been detected
           lockStatus = dark;  
        }
     }
     
     if(fading && millis() >= lastFade+getVar('d'-'a')){
           int r_soll = getVar('r'-'a') << 6;
           int g_soll = getVar('g'-'a') << 6;
           int b_soll = getVar('b'-'a') << 6;
           
           if( r_soll == lr && g_soll == lg && b_soll == lb) fading = false;
           else
           { 
             if( r_soll < lr ){if( lr-lrs < 0 )lr = r_soll; else lr-=lrs;}
             else if( r_soll > lr) lr+=lrs;
             
             if( g_soll < lg ){if( lg-lgs < 0 )lg = g_soll; else lg-=lgs;}
             else if( g_soll > lg) lg+=lgs;
             
             if( b_soll < lb ){if( lb-lbs < 0 )lb = b_soll; else lb-=lbs;}
             else if( b_soll > lb) lb+=lbs;
/*       
             if( r_soll < lr )lr-=1+(lr-r_soll)>>6;
             else if( r_soll > lr) lr+=1+(r_soll-lr)>>6;
             
             if( g_soll < lg )lg-=1+(lg-g_soll)>>6;
             else if( g_soll > lg) lg+=1+(g_soll-lg)>>6;
             
             if( b_soll < lb )lb-=1+(lb-b_soll)>>6;
             else if( b_soll > lb) lb+=1+(b_soll-lb)>>6;     
*/             
             analogWrite(pinRed, lr >> 6); 
             analogWrite(pinGreen, lg >> 6); 
             analogWrite(pinBlue, lb >> 6);
             
             lastFade = millis();
           }
     }
     
}
