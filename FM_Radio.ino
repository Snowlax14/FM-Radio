
#include <Wire.h> //knižnica pre I2C komunikáciu
#include <LiquidCrystal.h> //knižnica pre LCD display
#include <M62429.h> //knižnica pre digitálny potenciometer

#define clk 2 // clock pin Enkódera
#define dt 3  // data pin Enkódera
#define sw 4  //pin pre tlačítko Enkódera
#define meterPin 10 //pin pre ručičkového ukazovateľa Frekvencie 

const int rs=5;  // LCD RS pin
const int en=6;  // LCD Enable pin
const int d4=7;  // LCD data bit 4 pin
const int d5=8;  // LCD data bit 5 pin
const int d6=9;  // LCD data bit 6 pin
const int d7=13;  //LCD data bit 7 pin

LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // inicializácia pinov pre LCD diplay

double frequencySetting;
float memoryFrequency[10] = 
{
  88.0,
  90.2,
  90.8,
  91.6,
  97.3,
  98.5,
  99.7,
  100.7,
  101.8,
  103.2,
};

char *memoryFrequencyName[] = 
{
  "JEMNE",
  "RADIO PN",
  "DEVIN",
  "BEST FM",
  "ANT ROCK",
  "EUROPA 2",
  "EXPRES",
  "REGINA",
  "VLNA",
  "SRO "
};

int Vol = 0;
int percentage;
double meterOutput;   // výstup pre ručičkového ukazovateľa Frekvencie  - 0..255
unsigned char frequencyH = 0;
unsigned char frequencyL = 0;
unsigned int frequencyB;
boolean mode;
boolean memoryMode = 0;
int n;
int m;

const int PRESS_TIME = 1500; // 1000 milliseconds


int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

bool isPressing = false;
bool isLongDetected = false;

volatile boolean TurnDetected;
volatile boolean up;


void audio(){
  setVolume (11,12,0,0,Vol); 
  setVolume (11,12,0,1,Vol); 
  
  Serial.println(Vol);
  /* pin CLK
     pin DATA
     0 - každý kanál samostatne, 1 - oba kanály
     0 - pravý kanál 1 - ľavý kanál
     83 ... 0  hodnota stíšenia 83 = -83 db */  
}



void isr0()  {
  TurnDetected = true;
  up = (digitalRead(clk) == digitalRead(dt));
}

void setFrequency(float frequency)  {                       // funkcia pre poslanie frekvencie do TEA5767
  frequencyB = 4 * (frequency * 1000000 + 225000) / 32768;
  frequencyH = frequencyB >> 8;
  frequencyL = frequencyB & 0XFF;
  Wire.beginTransmission(0x60);
  Wire.write(frequencyH);
  Wire.write(frequencyL);
  Wire.write(0xB0);
  Wire.write(0x12);
  Wire.write(0x00);
  Wire.endTransmission(); 
  Serial.println(frequencySetting);
} 


void displaydata() {     // funkcia pre zobrazenie informacií na displeji
  lcd.setCursor(1,0);
  if(memoryMode == 0){
  lcd.print("MHz:");
  lcd.print(frequencySetting);
  }
  if(memoryMode == 1){
  lcd.print("FM:");
  lcd.print(memoryFrequencyName[m]);
  }
  lcd.setCursor(1,1);
  lcd.print("Vol:");
  percentage = map(Vol,-50,0,0,100);
  lcd.print(percentage);
  lcd.print("%");
} 

void arrow() {       //funkcia pre zobrazenie a prepínanie polohy šípky na displeji
  lcd.begin(16, 2);
  if (mode == 1 && memoryMode == 0){
    lcd.setCursor(0,0);
    lcd.write(246);}
  if(mode == 0 && memoryMode == 0){
    lcd.setCursor(0,1);
    lcd.write(246);} 
  if(mode == 1 && memoryMode == 1 ) {
    lcd.setCursor(0,0);
    lcd.write(188);
   }
   if(mode == 0 && memoryMode == 1 ) {
    lcd.setCursor(0,1);
    lcd.write(188);
   }
}

double mapf(double val, double in_min, double in_max, double out_min, double out_max) {     //funkcia pre ručičkového ukazovateľa Frekvencie (skonvertuje hodnotu frekvencie na hodnotu PWM signálu 0...255)
  return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup() {
  Serial.begin(9600);   //debug
  Wire.begin();
  lcd.begin(16, 2);
  pinMode(clk,INPUT);
  pinMode(dt,INPUT);  
  pinMode(sw,INPUT);
  mode = 0;           //mód nastavenia frekvencie


  frequencySetting = 97.3;          //nastavená frekvencia pri zapnutí
  n = 4;
  m = 4;
  setFrequency(frequencySetting);
  arrow();
  displaydata();
  audio();
  attachInterrupt (0,isr0,FALLING);
  
}



void loop() 
{
  currentState = digitalRead(sw);

  if(lastState == HIGH && currentState == LOW)
  {
      pressedTime = millis();
      isPressing = true;
      isLongDetected = false;
  }
  else if (lastState == LOW && currentState == HIGH) 
  {
    isPressing = false;
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    if ( pressDuration < PRESS_TIME )
      mode=!mode;
      arrow();
      displaydata();
      delay(500);
    }
    
    if(isPressing == true && isLongDetected == false) 
    {
      long pressDuration = millis() - pressedTime;
      
    if( pressDuration > PRESS_TIME)
      {
      memoryMode=! memoryMode;
      isLongDetected = true;
      frequencySetting = memoryFrequency[n];
      setFrequency(frequencySetting);
      arrow();
      displaydata();
      delay(500);
      
      }  
   }
   lastState = currentState;

   meterOutput = mapf(frequencySetting, 88.0, 108.0, 0, 255);    // skonvertuje hodnotu frekvencie na hodnotu PWM signálu 0...255
   analogWrite(meterPin, meterOutput);     //zaslanie hodnoty do ručičkového ukazovateľa Frekvencie
    
  if (TurnDetected && mode == 1 && memoryMode == 0)
  {
    if(up && frequencySetting < 107.90)
    {
      frequencySetting = frequencySetting + 0.1;
    }
    else if (frequencySetting >= 88.1)
    {
       frequencySetting = frequencySetting - 0.1;
    }
     
     setFrequency(frequencySetting);
     arrow();
     displaydata();
     TurnDetected = false;
  }

  if (TurnDetected && mode == 0 )
  {
    if(up)
    {
      Vol = Vol + 1;
      if (Vol >= 1)
      {
        Vol = 0;
        audio();
        arrow();
        displaydata();
        }
      else
      {
      audio();
      arrow();
      displaydata();
      }
     }
    else
    {
      Vol = Vol - 1;
      if (Vol <= -50)
      {
        Vol = -50;
        audio();
        arrow();
        displaydata();
        }
      else
      {
      audio();
      arrow();
      displaydata();
      }
     }
      TurnDetected = false;
    }   
  
  if (TurnDetected &&  memoryMode == 1)
  {
    if(up)
    {
      if (frequencySetting >= 103.2)
      {
        setFrequency(frequencySetting);
        arrow();
        displaydata();
        }
      else{
        
      n++;
      m++;
      frequencySetting = memoryFrequency[n];
      setFrequency(frequencySetting);
      arrow();
      displaydata();
      }
      }
    else
    {
      if (frequencySetting <= 88.0)
      {
        setFrequency(frequencySetting);
        arrow();
        displaydata();
        }
      else
      {
      n--;
      m--;
      frequencySetting = memoryFrequency[n];
      setFrequency(frequencySetting);
      arrow();
      displaydata();
      }
     }
      TurnDetected = false;
    }
  
}
    
  



    
  
 
