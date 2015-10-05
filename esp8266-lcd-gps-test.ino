//This is working!
//
//Connect
//
//#define TFT_CS     15
//#define TFT_RST    12
//#define TFT_DC     16
//
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
//
//#define TFT_SCLK 14
//#define TFT_MOSI 13
// The Adafruit_ST7735 fork from https://github.com/nzmichaelh/Adafruit-ST7735-Library is used.
// and the HWSPI check is inverted, not sure why yet.

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

// The TinyGPS++ object
TinyGPSPlus gps;

// For the breakout, you can use any 2 or 3 pins
// These pins will also work for the 1.8" TFT shield
#define TFT_CS     15
#define TFT_RST    0  // you can also connect this to the Arduino reset
// in which case, set this #define pin to 0!
#define TFT_DC     16

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

// Option 2: use any pins but a little slower!
#define TFT_SCLK 14   // set these to be whatever pins you like!
#define TFT_MOSI 13   // set these to be whatever pins you like!
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

SoftwareSerial ss(5, 4);  // Rx/Tx

// Setup mag sensor
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

bool magPresent = true;
float headingDegrees;
const char *headingCardinal;

void setup() {
  // Debug data on hardware serial
  Serial.begin(115200);

  // GPS data from software serial
  ss.begin(9600);

  // Start i2c
  Wire.begin(2,12);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  // Use this initializer (uncomment) if you're using a 1.44" TFT
  tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, black tab
  Serial.println("Initialized");

  // large block of text
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(2); 

  // Start mag sensoring
  if(!mag.begin())
  {
    /* There was a problem detecting the HMC5883 ... check your connections */
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    magPresent = false;
    //while(1);
  }
  
 
}

void loop() {

  if (magPresent) {
    /* Get a new sensor event */ 
    sensors_event_t event; 
    mag.getEvent(&event);
  
    float heading = atan2(event.magnetic.y, event.magnetic.x);
    
    // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your location.
    // Find yours here: http://www.magnetic-declination.com/
    // Mine is: -13* 2' W, which is ~13 Degrees, or (which we need) 0.22 radians
    // If you cannot find your Declination, comment out these two lines, your compass will be slightly off.
    float declinationAngle = 0.22;
    heading += declinationAngle;
    
    // Correct for when signs are reversed.
    if(heading < 0)
      heading += 2*PI;
      
    // Check for wrap due to addition of declination.
    if(heading > 2*PI)
      heading -= 2*PI;
     
    // Convert radians to degrees for readability.
    headingDegrees = heading * 180/M_PI; 
    
    Serial.print("Heading (degrees): "); Serial.println(headingDegrees);
  }

  printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
  printInt(gps.hdop.value(), gps.hdop.isValid(), 5);
  printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
  printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
  printInt(gps.location.age(), gps.location.isValid(), 5);
//  printDateTime(gps.date, gps.time);
  printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
  printFloat(gps.course.deg(), gps.course.isValid(), 7, 2);
  printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
  printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.value()) : "*** ", 6);
  Serial.println();

  tft.setCursor(1,1);
  tft.fillRect(0,0, 128, 9, ST7735_WHITE);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
  tft.print("GPS Data  ");
  getDate(gps.date);  
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setCursor(1, 10);
  tft.print("Sat: ");
  tft.println(gps.satellites.value());
  tft.setCursor(1, 18);
  tft.print("Lat: ");
  tft.println(gps.location.lat(), 6);
  tft.setCursor(1, 26);
  tft.print("Lng: ");
  tft.println(gps.location.lng(), 6);
  tft.setCursor(1, 34);
  tft.print("Alt: ");
  tft.println(gps.altitude.meters(), 1);

  if (magPresent) {
    headingCardinal = TinyGPSPlus::cardinal(headingDegrees);
    printStr(gps.location.isValid() ? headingCardinal : "*** ",1);
    tft.setCursor(1, 42);
    tft.print("Hdg: ");
    int len  = strlen(headingCardinal);
    tft.print(headingCardinal);
    for (int i=len; i<4; ++i)
      tft.print(' ');
  }
  smartDelay(1000);
}

void testdrawtext(const char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color, ST7735_BLACK);
  tft.setTextWrap(true);
  tft.print(text);
}


static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  smartDelay(0);
}

static void getDate(TinyGPSDate &d)
{
  if (!d.isValid())
  {
    Serial.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
//    Serial.print(sz);
    tft.print(sz);
  }
  smartDelay(0);
}

static void printTime(TinyGPSTime &t)
{
  if (!t.isValid())
  {
    Serial.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    Serial.print(sz);
  }
  smartDelay(0);
}

static char printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartDelay(0);
}

  void receiveData(int byteCount) {
}

void sendData() {
}

