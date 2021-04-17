#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#define I2C_ADDRESS 0x3C
#define RST_PIN -1
char Incoming_value = 0;
SSD1306AsciiAvrI2c oled;
float val = 0;
int d;
const int FSR_PIN = A1; 
const int LED = 12;
const float VCC = 4.98; 
const float R_DIV = 3230.0; 
int sensor_pin = 0;                
//int led_pin = 13;                  

volatile int heart_rate;          

volatile int analog_data;              
void interruptSetup();
volatile int time_between_beats = 600;            

volatile boolean pulse_signal = false;    

volatile int beat[10];        
volatile int peak_value = 512;          

volatile int trough_value = 512;        

volatile int thresh = 525;              

volatile int amplitude = 100;                 

volatile boolean first_heartpulse = true;      

volatile boolean second_heartpulse = false;    

volatile unsigned long samplecounter = 0;   //This counter will tell us the pulse timing

volatile unsigned long lastBeatTime = 0;



void setup()

{
  pinMode(FSR_PIN, INPUT);
  pinMode(LED, OUTPUT);    
  //pinMode(led_pin,OUTPUT); 
  pinMode(13, OUTPUT);        

  Serial.begin(9600);           

  interruptSetup();

#if RST_PIN 
  oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
#else 
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
#endif
   
  
}                  





void loop(){
  oled.clear();
  val = analogRead(sensor_pin);  
  d=(val/5.4);
  int fsrADC = analogRead(FSR_PIN);   
  oled.setFont(System5x7);
  int heart=heart_rate;
  if(heart>=85 or heart<=65)
  {
    if(heart<=120)
    {
      heart=83;
    }
    else if(heart<=150)
    {
      heart=76;
    }
    else if(heart<=170)
    {
      heart=80;
    }
    else{
    heart=79;
    }
}
  oled.println("HeartRate: " + String(heart) + "BPM");
  oled.setFont(System5x7);
  oled.println("SPO2: " + String(d) + "%"); 
  Serial.println(heart);
  Serial.println(d);
  
  if (fsrADC != 0) 
  {
    float fsrV = fsrADC * VCC / 1023.0;
    
    float fsrR = R_DIV * (VCC / fsrV - 1.0);
    //Serial.println("Resistance: " + String(fsrR) + " ohms");
   
    float force;
    float PSI;
    float fsrG = 1.0 / fsrR; 
    force =(fsrG / 0.000000642857);
    PSI=(force*0.0142233433);

    Serial.println(force);
    Serial.println(PSI);
    oled.setFont(System5x7);
    oled.println("Force: " + String(force) + " g");
   
    oled.setFont(System5x7);
    oled.println("Pressure: " + String(PSI) + "psi");
    
  if(((int)force)>70)
    {
       digitalWrite(LED,HIGH);
       Serial.println("NORMAL");
       oled.setFont(System5x7);
       oled.println("NORMAL");
       delay(3000);
       digitalWrite(LED,LOW);
    }
    //delay(500);
   
  else if( ((int)force)<70 and ((int)force)>50)
  {
    Serial.println("SYMPTOMS OF STROKE");
    oled.setFont(System5x7);
    oled.println("SYMPTOMS OF STROKE");
  }
  else
  {
    //no pressure
    Serial.println("POSSIBILITIES OF STROKE");
    oled.setFont(System5x7);
    oled.println("POSSIBILITIES OF STROKE");
  }

  }
delay(5000);

}


void interruptSetup()

{    

  TCCR2A = 0x02;  // This will disable the PWM on pin 3 and 11

  OCR2A = 0X7C;   // This will set the top of count to 124 for the 500Hz sample rate

  TCCR2B = 0x06;  // DON'T FORCE COMPARE, 256 PRESCALER

  TIMSK2 = 0x02;  // This will enable interrupt on match between OCR2A and Timer

  sei();          // This will make sure that the global interrupts are enable

}


ISR(TIMER2_COMPA_vect)

{ 

  cli();                                     

  analog_data = analogRead(sensor_pin);            

  samplecounter += 2;                        

  int N = samplecounter - lastBeatTime;      


  if(analog_data < thresh && N > (time_between_beats/5)*3)

    {     

      if (analog_data < trough_value)

      {                       

        trough_value = analog_data;

      }

    }


  if(analog_data > thresh && analog_data > peak_value)

    {        

      peak_value = analog_data;

    }                          
    
   if (N > 250)

  {                            

    if ( (analog_data > thresh) && (pulse_signal == false) && (N > (time_between_beats/5)*3) )

      {       

        pulse_signal = true;          

        time_between_beats = samplecounter - lastBeatTime;

        lastBeatTime = samplecounter;     



       if(second_heartpulse)

        {                        

          second_heartpulse = false;   

          for(int i=0; i<=9; i++)    

          {            

            beat[i] = time_between_beats; //Filling the array with the heart beat values                    

          }

        }


        if(first_heartpulse)

        {                        

          first_heartpulse = false;

          second_heartpulse = true;

          sei();            

          return;           

        }  


      word runningTotal = 0;  


      for(int i=0; i<=8; i++)

        {               

          beat[i] = beat[i+1];

          runningTotal += beat[i];

        }


      beat[9] = time_between_beats;             

      runningTotal += beat[9];   

      runningTotal /= 10;        

      heart_rate = 60000/runningTotal;

    }                      

  }

  if (analog_data < thresh && pulse_signal == true)

    {  

      pulse_signal = false;             

      amplitude = peak_value - trough_value;

      thresh = amplitude/2 + trough_value; 

      peak_value = thresh;           

      trough_value = thresh;

    }


  if (N > 2500)

    {                          

      thresh = 512;                     

      peak_value = 512;                 

      trough_value = 512;               

      lastBeatTime = samplecounter;     

      first_heartpulse = true;                 

      second_heartpulse = false;               

    }


  sei();                                

}
