#include <UTFT.h>
#include "DHT.h"
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <SPI.h>
#include <SdFat.h>
#include <UTFT_SdRaw.h>
#include <avr/pgmspace.h>

#define ET_DRIFT 4

#include "everytime.h"



// Define the orientation of the touch screen. Further 
// information can be found in the instructions.
//#define TOUCH_ORIENTATION  PORTRAIT

//SD Card
//#define SD_CHIP_SELECT  53  // SD chip select pin
// file system object

//SdFat sd;

UTFT myGLCD(CTE70, 38, 39, 40, 41); //(byte model, int RS, int WR, int CS, int RST, int SER)
//UTFT_SdRaw myFiles(&myGLCD);

//URTouch  myTouch( 6, 5, 4, 3, 2);
// Create a file to store the data


RTC_DS3231 rtc;



// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Grotesk16x32[];
extern uint8_t Grotesk32x64[];
extern uint8_t Grotesk24x48[];


#define DHT1PIN 6
#define DHT2PIN 7
#define DHT1TYPE DHT22   // DHT 22  (AM2302)
#define DHT2TYPE DHT11
DHT dht1(DHT1PIN, DHT1TYPE);
DHT dht2(DHT2PIN, DHT2TYPE);

    float tF;
    float dP;
    float dPF;

float Tmax_Ext = 0.0;
float Tmin_Ext = 80.0;
float Tmax_Int = 0.0;
float Tmin_Int = 80.0;

Adafruit_BMP280 bme;

String      time_str, weather_text, weather_extra_text;
int         last_reading_hour, reading_hour, hr_cnt;
float   temperature;

enum image_names { // enumerated table used to point to images
                  rain_img, sunny_img, mostlysunny_img, cloudy_img, tstorms_img,
                 } image;

// Define and enumerated type and assign values to expected weather types.
// These values help to determine the average weather preceeding a 'no-change' forecast 
//e.g. rain, rain then mostlysun = -1 (-1 + -1 + 1) resulting on balance = more rain
enum weather_type {unknown     =  4,
                   sunny       =  2,
                   mostlysunny =  1,
                   cloudy      =  0,
                   rain        = -1,
                   tstorms     = -2
                   };

enum weather_description {GoodClearWeather, BecomingClearer,
                          NoChange, ClearSpells, ClearingWithin12hrs, ClearingAndColder,
                          GettingWarmer, WarmerIn2daysRainLikely,
                          ExpectRain, WarmerRainWithin36hrs, RainIn18hrs, RainHighWindsClearAndCool,
                          GalesHeavyRainSnowInWinter
                          };

weather_type current_wx; // Enable the current wx to be recorded

// An array structure to record pressure, temperaturre, humidity and weather state
typedef struct {
  float p;            // air pressure at the designated hour
  float t;         // temperature at the designated hour
  float h;            // humidity at the designated hour
  weather_type wx_state_1hr; // weather state at 1-hour
  weather_type wx_state_3hr; // weather state at 3-hour point
} wx_record_type;

wx_record_type reading[24]; // An array covering 24-hours to enable P, T, % and Wx state to be recorded for every hour

int wx_average_1hr, wx_average_3hr; // Indicators of average weather

bool look_3hr = true;
bool look_1hr = false;

#define pressure_offset 3.8 // Used to adjust sensor reading to correct pressure for your location
int history[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float pression_atm;
static long tempo_init;

  

unsigned int *MoonPic;               //Pointer to the Lunar Phase Pics
extern unsigned int                  //Lunar Phase Pics
  New_Moon[0xA90],
  Waxing_Crescent[0xA90],
  First_Quarter[0xA90],
  Waxing_Gibbous[0xA90],
  Full_Moon[0xA90],
  Waning_Gibbous[0xA90],
  Last_Quarter[0xA90],
  Waning_Crescent[0xA90];
                            //Month type Length in days
                            //anomalistic 27.554549878
                            //sidereal  27.321661547
                            //tropical  27.321582241
                            //draconic  27.212220817
                            //synodic 29.530588853
float phase;
float LC = 27.321661547;    //sidereal month time used             
String LP;         //LP = Lunar Phase - variable used to print out Moon Phase
double AG;         //AG = Age

 
unsigned long previousMillis = 0;        // will store last time LED was updated
long OnTime = 1000;           // milliseconds of on-time
long OffTime = 60000;          // milliseconds of off-time
 

void setup()
{
  Wire.begin();
  Serial.begin(115200);
  
  while (!Serial) continue;
  delay(100);
    if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
     //rtc.adjust(DateTime(2019, 2, 6, 9, 59, 0));
  }
  Serial.println(F("Initialising LCD."));
  delay(3000); // wait for console opening
  myGLCD.InitLCD();
  delay(2000);
  dht1.begin();
  dht2.begin();
  
  if (!bme.begin()) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
  InitLcd(); 
  tempo_init = millis();
  
  float p,t;
  
  for (int i = 0; i <= 23; i++){ // At the start all array values are the same as a baseline 
    reading[i].p  = read_p();       // A rounded to 1-decimal place version of pressure
    reading[i].t  = dht1.readTemperature(); // Although not used, but avialable
    reading[i].h  = dht1.readHumidity();    // Although not used, but avialable
    reading[i].wx_state_1hr = unknown;               // To begin with  
    reading[i].wx_state_3hr = unknown;               // To begin with 
  }                                                  // Note that only 0,5,11,17,20,21,22,23 are used as display positions
  last_reading_hour = reading_hour;
  wx_average_1hr = 0; // Until we get a better idea
  wx_average_3hr = 0; // Until we get a better idea 


}

void loop()
{  
 DateTime now = rtc.now();
      
float lunarCycle = moonPhase(now.year(), now.month(), now.day()); //get a value for the lunar cycle

    pression_atm = read_p();
    
    Time();
    Date();
    Temp_Int();
    Temp_Ext();

   if (millis() >= (tempo_init + 300000))
  {
    DateTime now = rtc.now();
    Time();
    pressure();
    tempo_init = millis();
  }
}


void InitLcd()
{

    myGLCD.clrScr();
    myGLCD.setFont(BigFont);
    myGLCD.fillScr(VGA_BLACK);
    myGLCD.setColor(VGA_WHITE); 
    myGLCD.print("Weather Station Ver. 1.0c", CENTER, 10);
    myGLCD.setColor(255, 255, 255);
    myGLCD.setBackColor(VGA_BLACK);
    myGLCD.drawRect(5, 5, 795, 475);
    myGLCD.drawLine(5, 30, 795, 30);
    myGLCD.drawLine(5, 100, 795, 100);
    myGLCD.drawLine(5, 215, 795, 215);
    myGLCD.drawLine(400, 100, 400, 215);
    myGLCD.drawLine(610, 100, 610, 215);
    myGLCD.drawLine(665, 330, 665, 475);
    myGLCD.drawLine(5, 330, 795, 330);    
    myGLCD.setFont(Grotesk16x32);
    myGLCD.print("Inside:", 35, 110);
    myGLCD.print("Outside:", 220, 110);
    myGLCD.print("Humidity:", 625, 110);
    myGLCD.print("%", 770, 140);
    myGLCD.print("Pressure:", 10, 220);
    //myGLCD.print("Moon %:",575, 335);
    //myGLCD.print("Moon",575, 335);
    //myGLCD.print("Phase:",575, 365);
    //myGLCD.print("Age:",575, 400);
    //myGLCD.print("Age:",575, 430);
    myGLCD.setFont(SmallFont);
    myGLCD.setColor(VGA_YELLOW);
    myGLCD.print("In Max:",     160, 340);
    myGLCD.print("In Min:",     160, 360);
    myGLCD.print("Out Max:",    10, 340);
    myGLCD.print("Out Min:",    10, 360);
    myGLCD.print("DewPoint:",   305, 340);
    myGLCD.print("HeatInd:",  305, 360);
    myGLCD.print("Ins Humid:",  470, 335);
    myGLCD.setColor(VGA_WHITE);
    myGLCD.setFont(SmallFont);
    myGLCD.print("0",245,315);
    myGLCD.print("1",267,315);
    myGLCD.print("2",289,315);
    myGLCD.print("3",311,315);
    myGLCD.print("4",333,315);
    myGLCD.print("5",355,315);
    myGLCD.print("6",377,315);
    myGLCD.print("7",399,315);
    myGLCD.print("8",421,315);
    myGLCD.print("9",443,315);
    myGLCD.print("10",465,315);
    myGLCD.print("11",487,315);
    myGLCD.print("12",509,315);
    myGLCD.print("13",531,315);
    myGLCD.print("14",553,315);
    myGLCD.print("15",575,315);
    myGLCD.print("16",597,315);
    myGLCD.print("17",619,315);
    myGLCD.print("18",641,315);
    myGLCD.print("19",663,315);
    myGLCD.print("20",685,315);
    myGLCD.print("21",707,315);
    myGLCD.print("22",729,315);
    myGLCD.print("23",751,315);
    myGLCD.setFont(BigFont);
    myGLCD.setColor(255, 255, 0);
    myGLCD.drawLine(230, 269, 235, 269);
    myGLCD.drawLine(230, 220, 790, 220);
    myGLCD.drawLine(230, 310, 790, 310);
    myGLCD.print("hPa", 140, 273 );
    myGLCD.setColor(255, 255, 255);
    myGLCD.setFont(SmallFont);
    myGLCD.print("980", 200, 305);
    myGLCD.setColor(255, 255, 0);
    myGLCD.print("1013", 200, 270);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print("1040", 200, 217);
    myGLCD.setFont(Grotesk16x32); 
    myGLCD.setColor(VGA_YELLOW);
    myGLCD.print("Time", 425, 110);
    myGLCD.setColor(255, 255, 255);


}

/******************************* TIME & DATE FUNCTION *********************************/
void Time(){

    DateTime now = rtc.now();

    char daysOfTheWeek[7][110] = {"Sunday,   ", "Monday,   ", "Tuesday,  ", "Wednesday,", "Thursday, ", "Friday,   ", "Saturday, "};

    myGLCD.setFont(Grotesk32x64);
    if (now.minute() >= 0 && now.minute() < 10) {
    myGLCD.print("0", 500, 150);
    myGLCD.printNumI(now.minute(), 535, 150, 1);
  }
  else{
    myGLCD.printNumI(now.minute(), 500, 150, 2);
  }
    myGLCD.printNumI(now.hour(), 405, 150, 2);
    myGLCD.print(":", 470, 150);
    myGLCD.setFont(Grotesk24x48);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print(daysOfTheWeek[now.dayOfTheWeek()], 10, 40);
    myGLCD.printNumI(now.day(), 500, 40, 2);
    myGLCD.print(",", 545, 40);
    myGLCD.printNumI(now.year(), 600, 40, 2);

  
    float lunarCycle = moonPhase(now.year(), now.month(), now.day()); //get a value for the lunar cycle
    //myGLCD.drawBitmap(685,365,52,52,MoonPic,2);
    //myGLCD.drawBitmap(685,365,52,52,New_Moon,2);
    //myGLCD.drawBitmap(685,365,52,52,First_Quarter,2);
    //myGLCD.drawBitmap(685,365,52,52,Waxing_Gibbous,2);
    myGLCD.drawBitmap(680,365,52,52,Full_Moon,2);
    //myGLCD.drawBitmap(590, 365, 50, 50, cloudy, 2);      //Weather Icon

    
    char bufferLP[16];
    LP.toCharArray(bufferLP, 16);
    myGLCD.setFont(SmallFont);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print(bufferLP, 670, 340);   //Phase of the moon
    //myGLCD.setFont(Grotesk16x32);
    //myGLCD.setColor(255, 255, 255);
    //myGLCD.printNumF(AG,2,595, 435);    //Moon Age
    myGLCD.setFont(BigFont);

      reading[23].p = (reading[23].p + read_p())/2;                 // Update rolling average
      float  trend = reading[23].p - reading[22].p;                             // Get current trend over last 3-hours
      weather_description wx_text = get_forecast_text(reading[23].p, trend, look_1hr); // Convert to forecast text based on 1-hours
      ForecastToImgTxt(wx_text);
      //ForecastToImgTxt(get_forecast_text(reading[23].pressure, trend, look_3hr));
      //if (image == rain_icon) myGLCD.drawBitmap(x+0,y+15, 40, 40, rain_icon,1);               // Display corresponding image
      //if (image == sunny_icon) myGLCD.drawBitmap(x+0,y+15, 40, 40, sunny_icon,1);             // Display corresponding image
      //if (image == mostlysunny_icom) myGLCD.drawBitmap(x+0,y+15, 40, 40, mostlysunny_icon,1); // Display corresponding image
      //if (image == cloudy_icon) myGLCD.drawBitmap(x+0,y+15, 40, 40, cloudy_icon,1);           // Display corresponding image
      //if (image == tstorms_icon) myGLCD.drawBitmap(x+0,y+15, 40, 40, tstorms_icon,1);         // Display corresponding image
      //myGLCD.drawBitmap(685,365,52,52,New_Moon,2);
      myGLCD.setFont(BigFont);
      myGLCD.print(weather_text, 10, 410);
      myGLCD.print(get_trend_text(trend), 10, 305);
      myGLCD.print(weather_extra_text, 10, 380);
  
      if (reading_hour != last_reading_hour) { // If the hour has advanced, then shift readings left and record new values at array element [23]
        for (int i = 0; i < 23;i++){
      reading[i].p     = reading[i+1].p;
      reading[i].t  = reading[i+1].t;
      reading[i].wx_state_1hr = reading[i+1].wx_state_1hr;
      reading[i].wx_state_3hr = reading[i+1].wx_state_3hr;
    }
    reading[23].p     = read_p(); // Update time=now with current value of pressure
    reading[23].wx_state_1hr = current_wx;
    reading[23].wx_state_3hr = current_wx;
    last_reading_hour        = reading_hour;
    hr_cnt++;
    wx_average_1hr = reading[22].wx_state_1hr + current_wx;           // Used to predict 1-hour forecast extra text
    wx_average_3hr = 0;
    for (int i=23;i >= 21; i--){                                      // Used to predict 3-hour forecast extra text 
      wx_average_3hr = wx_average_3hr + (int)reading[i].wx_state_3hr; // On average the last 3-hours of weather is used for the 'no change' forecast - e.g. more of the same?
    }
  }

}  

void Date(){

    DateTime now = rtc.now();   
  char* strm[] = {"January  ","February ","March    ","April    ","May      ","June     ","July     ","August   ","September","October  ","November ","December "};  
      myGLCD.setFont(Grotesk24x48);
      myGLCD.setColor(255, 255, 255);
      myGLCD.print(strm[now.month()-1], 270 , 40);
}

/******************************* INTERIOR TEMP & HUMIDITY FUNCTION *********************************/
void Temp_Int()
{

  float converted = 0.0;
  float temp_Int = dht2.readTemperature();
  float hum_Int = dht2.readHumidity();
  converted = ( temp_Int * 1.8 ) + 32;
  myGLCD.setFont(Grotesk32x64);
  myGLCD.printNumF(converted, 1, 30, 150);
  
  if (Tmax_Int < converted)
  {
    Tmax_Int = converted;
  }
  if (converted < Tmin_Int)
  {
    Tmin_Int = converted;
  }
  myGLCD.setFont(BigFont);
  myGLCD.printNumF(hum_Int, 1, 560, 335);
  myGLCD.print("%",620,335);
  myGLCD.printNumF(Tmax_Int, 1, 230, 335);
  myGLCD.printNumF(Tmin_Int, 1, 230, 355);
}


/******************************* EXTERIOR TEMP & HUMIDITY FUNCTION *********************************/
void Temp_Ext ()
{
    float convert = 0.0; 
    float h = dht1.readHumidity();
    float t = dht1.readTemperature();
    convert = ( t * 1.8 ) + 32;
    myGLCD.setFont(Grotesk32x64);
    myGLCD.printNumF(h, 1, 640, 150);
    myGLCD.printNumF(convert, 1, 220, 150);
    myGLCD.setFont(BigFont);
    myGLCD.printNumF(dPF, 1, 380, 335);
    myGLCD.printNumF(heatIndex(tF,h), 1, 380, 355);
    myGLCD.setFont(SmallFont);
    //myGLCD.print((weather[forecast]), 460, 440);
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT Ext");
  } else {
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    // Serial.print(t);
    // Serial.print(" *C ");
    tF=((t*9)/5)+32;
    Serial.print(tF);
    Serial.print(" *F ");
    Serial.print(" \t");
  
    Serial.print("Dew Point: ");
    // Serial.print(dewPointFast(t, h));
    // Serial.print(" *C ");
    dP=(dewPointFast(t, h));
    dPF=((dP*9)/5)+32;
    Serial.print(dPF);
    Serial.print(" *F");
    Serial.print(" \t");
  
    Serial.print("Heat Index: ");
    Serial.print(heatIndex(tF,h));
    Serial.println(" *F");
  }    
    if (Tmax_Ext < convert)
   {
    Tmax_Ext = convert;
   }
    if (convert < Tmin_Ext)
   {
    Tmin_Ext = convert;
   }
    myGLCD.setFont(BigFont);
    myGLCD.printNumF(Tmax_Ext, 1, 85, 335);
    myGLCD.printNumF(Tmin_Ext, 1, 85, 355);
}


/******************************* PRESSURE GRAPHING FUNCTION *********************************/
void pressure()
{

  DateTime now = rtc.now();
  
  Time();

//  float read_p(){
//  float reading  = bme.readPressure() / 100.0F + pressure_offset; 
//  Serial.println(reading);
//  myGLCD.setFont(Grotesk16x32);
//  myGLCD.printNumF(reading, 2, 10, 260);
//  myGLCD.setFont(BigFont);
//  int altitude    = 74; // in METRES
//  temperature = (int)dht1.readTemperature();
//  return (double)reading*pow(1-0.0065*(double)altitude/(temperature+0.0065*(double)altitude+273.15),-5.275);
//}
  pression_atm = read_p();
  
  myGLCD.setFont(BigFont);
  myGLCD.setColor(255, 255, 0);
  myGLCD.drawLine(230, 269, 235, 269);
  myGLCD.drawLine(230, 220, 790, 220);
  myGLCD.drawLine(230, 310, 790, 310);
  myGLCD.print("hPa", 140, 273 );
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
  myGLCD.print("980", 200, 305);
  myGLCD.setColor(255, 255, 0);
  myGLCD.print("1013", 200, 270);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("1040", 200, 217);
  int x;
  int increment;

  if (now.hour() == 0)      //&& now.minute() >= 15 && now.second() >= 59
  {
    history[0] = pression_atm;
    x = 240;
    increment = 0;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 1 )
  {
    history[1] = pression_atm;
    x = 262;
    increment = 1;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 2 )
  {
    history[2] = pression_atm;
    x = 284;
    increment = 2;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 3 )
  {
    history[3] = pression_atm;
    x = 306;
    increment = 3;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 4 )
  {
    history[4] = pression_atm;
    x = 328;
    increment = 4;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 5 )
  {
    history[5] = pression_atm;
    x = 350;
    increment = 5;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 6 )
  {
    history[6] = pression_atm;
    x = 372;
    increment = 6;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 7 )
  {
    history[7] = pression_atm;
    x = 394;
    increment = 7;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 8 )
  {
    history[8] = pression_atm;
    x = 416;
    increment = 8;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 9 )
  {
    history[9] = pression_atm;
    x = 438;
    increment = 9;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 10 )
  {
    history[10] = pression_atm;
    x = 460;
    increment = 10;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 11 )
  {
    history[11] = pression_atm;
    x = 482;
    increment = 11;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 12)      //&& now.minute() >= 15 && now.second() >= 59
  {
    history[12] = pression_atm;
    x = 504;
    increment = 12;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 13 )
  {
    history[13] = pression_atm;
    x = 526;
    increment = 13;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 14 )
  {
    history[14] = pression_atm;
    x = 548;
    increment = 14;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 15 )
  {
    history[15] = pression_atm;
    x = 570;
    increment = 15;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 16 )
  {
    history[16] = pression_atm;
    x = 592;
    increment = 16;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 17 )
  {
    history[17] = pression_atm;
    x = 614;
    increment = 17;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 18 )
  {
    history[18] = pression_atm;
    x = 636;
    increment = 18;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 19 )
  {
    history[19] = pression_atm;
    x = 658;
    increment = 19;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 20 )
  {
    history[20] = pression_atm;
    x = 680;
    increment = 20;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 21 )
  {
    history[21] = pression_atm;
    x = 702;
    increment = 21;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 22 )
  {
    history[22] = pression_atm;
    x = 724;
    increment = 22;
    hist_pressure(x, increment);
  }
  else if (now.hour() == 23 )
  {
    history[23] = pression_atm;
    x = 746;
    increment = 23;
    hist_pressure(x, increment);
  }

}
void hist_pressure(int x, int position_h )
{
  int width = 15;
  int height ;
  height = 311 - (history[position_h] - 980);
  myGLCD.setColor(VGA_BLUE);
  myGLCD.fillRect(x, 311, x + width, height);
  Serial.print("Bar Height");
  Serial.println(height);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.fillRect(x, (height - 1), x + width, 222);
}

/******************************* DewPoiint & Heat Index FUNCTION *********************************/
// delta max = 0.6544 wrt dewPoint()
// 6.9 x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
 double a = 17.271;
 double b = 237.7;
 double temp = (a * celsius) / (b + celsius) + log(humidity*0.01);
 double Td = (b * temp) / (a - temp);
 return Td;
}

double heatIndex(double tempF, double humidity)
{
  double c1 = -42.38, c2 = 2.049, c3 = 10.14, c4 = -0.2248, c5= -6.838e-3, c6=-5.482e-2, c7=1.228e-3, c8=8.528e-4, c9=-1.99e-6  ;
  double T = tempF;
  double R = humidity;

  double A = (( c5 * T) + c2) * T + c1;
  double B = ((c7 * T) + c4) * T + c3;
  double C = ((c9 * T) + c8) * T + c6;

  double rv = (C * R + B) * R + A;
  return rv;
}

/******************************* LUNAR PHASE FUNCTION *********************************/
float moonPhase(int moonYear, int moonMonth, int moonDay)
{ 

  float phase;
  double IP; 
  long YY, MM, K1, K2, K3, JulianDay; 
  YY = moonYear - floor((12 - moonMonth) / 10); 
  MM = moonMonth + 9;
  if (MM >= 12)
    { MM = MM - 12; }
  K1 = floor(365.25 * (YY + 4712));
  K2 = floor(30.6 * MM + 0.5);
  K3 = floor(floor((YY / 100) + 49) * 0.75) - 38;
  JulianDay = K1 + K2 + moonDay + 59;
  if (JulianDay > 2299160)
    { JulianDay = JulianDay - K3; }
  IP = MyNormalize((JulianDay - 2451550.1) / LC);
  AG = IP*LC;
  phase = 0; 
  
  //Determine the Moon Illumination %
  if ((AG >= 0) && (AG <= LC/2))             //FROM New Moon 0% TO Full Moon 100%
    { phase = (2*AG)/LC; }
  if ((AG > LC/2) && (AG <= LC))             //FROM Full Moon 100% TO New Moon 0%
    { phase = 2*(LC-AG)/LC; }

  //Determine the Lunar Phase
  if ((AG >= 0) && (AG <= 1.85))             //New Moon; ~0-12.5% illuminated
    { LP = "    New Moon   "; 
    //myGLCD.drawBitmap(685,365,52,52,New_Moon,2);
       }
  if ((AG > 1.85) && (AG <= 5.54))           //New Crescent; ~12.5-37.5% illuminated
    { LP = "Waxing Crescent";
    //myGLCD.drawBitmap(685,365,52,52,Waxing_Crescent,2);
       }
  if ((AG > 5.54) && (AG <= 9.23))           //First Quarter; ~37.5-62.5% illuminated
    { LP = " First Quarter ";
    //myGLCD.drawBitmap(685,365,52,52,First_Quarter,2);
       }
  if ((AG > 9.23) && (AG <= 12.92))          //Waxing Gibbous; ~62.5-87.5% illuminated
    { LP = "Waxing Gibbous ";
    //myGLCD.drawBitmap(685,365,52,52,Waxing_Gibbous,2);
       }
  if ((AG > 12.92) && (AG <= 16.61))         //Full Moon; ~87.5-100-87.5% illuminated
    { LP = "   Full Moon   ";
    //myGLCD.drawBitmap(685,365,52,52,Full_Moon,2);
       }    
  if ((AG > 16.61) && (AG <= 20.30))         //Waning Gibbous; ~87.5-62.5% illuminated
    { LP = "Waning Gibbous ";
    //myGLCD.drawBitmap(685,365,52,52,Waning_Gibbous,2);
       }
  if ((AG > 20.30) && (AG <= 23.99))         //Last Quarter; ~62.5-37.5% illuminated
    { LP = " Last Quarter  ";
    //myGLCD.drawBitmap(685,365,52,52,Last_Quarter,2);
       }
  if ((AG > 23.99) && (AG <= 27.68))         //Old Crescent; ~37.5-12.5% illuminated
    { LP = "Waning Crescent";
    //myGLCD.drawBitmap(685,365,52,52,Waning_Crescent,2);
       }
  if ((AG >= 27.68) && (AG <= LC))           //New Moon; ~12.5-0% illuminated
    { LP = "    New Moon   ";
    //myGLCD.drawBitmap(685,365,52,52,New_Moon,2);
      }
        
  return phase; 

}

double MyNormalize(double v) 
{ 
  v = v - floor(v);
  if (v < 0)
    v = v + 1;
  return v;
} 
/**************************** END OF LUNAR PHASE FUNCTION *****************************/

float read_p(){
  float reading  = bme.readPressure() / 100.0F + pressure_offset; 
  Serial.println(reading);
  myGLCD.setFont(Grotesk16x32);
  myGLCD.printNumF(reading, 2, 10, 260);
  myGLCD.setFont(BigFont);
  int altitude    = 74; // in METRES
  temperature = (int)dht1.readTemperature();
  return (double)reading*pow(1-0.0065*(double)altitude/(temperature+0.0065*(double)altitude+273.15),-5.275);
}

// Convert pressure trend to text
String get_trend_text(float trend){
  String trend_str = "Steady"; // Default weather state
  if (trend > 3.5)                          { trend_str = "Rising fast";  }
  else if (trend >   1.5  && trend <= 3.5)  { trend_str = "Rising     ";       }
  else if (trend >   0.25 && trend <= 1.5)  { trend_str = "Rising slow";  }
  else if (trend >  -0.25 && trend <  0.25) { trend_str = "Steady     ";       }
  else if (trend >= -1.5  && trend < -0.25) { trend_str = "Fallin slow"; }
  else if (trend >= -3.5  && trend < -1.5)  { trend_str = "Falling    ";      }
  else if (trend <= -3.5)                   { trend_str = "Fallin fast"; }
  return trend_str;
}

// Convert forecast text to a corresponding image for display together with a record of the current weather
void ForecastToImgTxt(weather_description wx_text){
  if      (wx_text == GoodClearWeather)           {image = sunny_img;       current_wx = sunny;        weather_text = "Good clear weather ";}
  else if (wx_text == BecomingClearer)            {image = mostlysunny_img; current_wx = mostlysunny;  weather_text = "Becoming clearer   ";}
  else if (wx_text == NoChange)                   {image = cloudy_img;      current_wx = cloudy;       weather_text = "No change, clearing";}
  else if (wx_text == ClearSpells)                {image = mostlysunny_img; current_wx = mostlysunny;  weather_text = "Clear spells       ";}
  else if (wx_text == ClearingWithin12hrs)        {image = mostlysunny_img; current_wx = mostlysunny;  weather_text = "Clearing in 12-hrs ";}
  else if (wx_text == ClearingAndColder)          {image = mostlysunny_img; current_wx = mostlysunny;  weather_text = "Clearing and colder";}
  else if (wx_text == GettingWarmer)              {image = mostlysunny_img; current_wx = mostlysunny;  weather_text = "Getting warmer     ";}
  else if (wx_text == WarmerIn2daysRainLikely)    {image = rain_img;        current_wx = rain;         weather_text = "Warmer,rain likely ";}
  else if (wx_text == ExpectRain)                 {image = rain_img;        current_wx = rain;         weather_text = "Expect rain        ";}
  else if (wx_text == WarmerRainWithin36hrs)      {image = rain_img;        current_wx = rain;         weather_text = "Warmer,rain in 36-hrs";}
  else if (wx_text == RainIn18hrs)                {image = rain_img;        current_wx = rain;         weather_text = "Rain in 18-hrs     ";}
  else if (wx_text == RainHighWindsClearAndCool)  {image = rain_img;        current_wx = rain;         weather_text = "Rain, high winds   ";}
  else if (wx_text == GalesHeavyRainSnowInWinter) {image = tstorms_img;     current_wx = tstorms;      weather_text = "Gales, heavy rain  ";}
}

// Convert pressure and trend to a weather description either for 1 or 3 hours with the boolean true/false switch
weather_description get_forecast_text(float pressure_now, float trend, bool range) {
  String trend_str = get_trend_text(trend);
  weather_description wx_text = NoChange; //As a default forecast 
  weather_extra_text = "";
  image = cloudy_img; // Generally when there is 'no change' then cloudy is the conditions
  if (pressure_now >= 1022.68 )                                                          {wx_text = GoodClearWeather;}
  if (pressure_now >= 1022.7  && trend_str  == "Falling fast")                           {wx_text = WarmerRainWithin36hrs;}
  if (pressure_now >= 1013.2  && pressure_now <= 1022.68 && 
     (trend_str == "Steady" || trend_str == "Rising slow"))                              {wx_text = NoChange; (range?wx_history_3hr():wx_history_1hr()); }
  if (pressure_now >= 1013.2 && pressure_now <= 1022.68 &&
     (trend_str == "Rising" || trend_str == "Rising fast"))                              {wx_text = GettingWarmer;}
  if (pressure_now >= 1013.2 && pressure_now <= 1022.68 && trend_str == "Rising slow")   {wx_text = BecomingClearer;}
  if (pressure_now >= 1013.2 && pressure_now <= 1022.68 &&
     (trend_str == "Falling fast" || trend_str == "Falling slow"))                       {wx_text = ExpectRain;}
  if (pressure_now >= 1013.2 && pressure_now <= 1022.68 && trend_str  == "Steady")       {wx_text = ClearSpells; (range?wx_history_3hr():wx_history_1hr());};
  if (pressure_now <= 1013.2 && (trend_str == "Falling slow" || trend_str == "Falling")) {wx_text = RainIn18hrs;}
  if (pressure_now <= 1013.2  &&  trend_str == "Falling fast")                           {wx_text = RainHighWindsClearAndCool;}
  if (pressure_now <= 1013.2  && 
     (trend_str == "Rising" || trend_str=="Rising slow"||trend_str=="Rising fast"))      {wx_text = ClearingWithin12hrs;}
  if (pressure_now <= 1009.14 && trend_str  == "Falling fast")                           {wx_text = GalesHeavyRainSnowInWinter;}
  if (pressure_now <= 1009.14 && trend_str  == "Rising fast")                            {wx_text = ClearingAndColder;}
  return wx_text;
}

// Convert 1-hr weather history to text
void wx_history_1hr() {
  if      (wx_average_1hr >  0) weather_extra_text = "Expect Sun   ";
  else if (wx_average_1hr == 0) weather_extra_text = "Mainly Cloudy";
  else if (wx_average_1hr <  0) weather_extra_text = "Expect Rain  ";
  else weather_extra_text = "";
}

// Convert 3-hr weather history to text
void wx_history_3hr() {
  if      (wx_average_3hr >  0) weather_extra_text = ", expect sun";
  else if (wx_average_3hr == 0) weather_extra_text = ", mainly cloudy";
  else if (wx_average_3hr <  0) weather_extra_text = ", expect rain";
  else weather_extra_text = "";
}
// Forecast basics:
// Look at the pressure change over3 hours
// If pressure is descending, then a low pressure area is approaching 
// If pressure is ascending , then a low is passing or a high pressure is coming
// When pressure is changing rapidly (>6hPa/3 hours), it will be windy (or potentially windy) 
// More detailed:
// Pressure falling slowly (0.5 - 3 hPa in 3h): low is weak, dying or moving slowly. You might get some rain but typically no high winds.
// Pressure falling moderately (3-6 hPa/3h): rapid movement or deepening low. Moderate winds and rain and a warm front.
//                                     :the low is passing fast, the day after tomorrow will typically be fine. 
// Pressure falling fast (6-12 hPa/3h) : Storm conditions highly likely.
// Pressure rises are connected with gradually drier weather
