/*
 * 3x70 Neon LED Strip Controller - Raspberry Pi Pico W
 * Author: David Orser
 *
 * Drives three 12 V WS2811 "neon" flex strips (70 pixels each, 210 total) from a
 * Raspberry Pi Pico W. The pattern is selected by UDP text messages on port 5005,
 * and the board can be re-flashed over Wi-Fi (Arduino OTA).
 *
 * Formerly 20251113_RPICO_LightsXmas_RedGrn_WifiUDP_v004.ino (Nov 13, 2025).
 * See README.md and docs/ for wiring, commands and known issues.
 */

#include <WiFi.h>
#include <SimpleMDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>

// Wi-Fi and OTA credentials live in secrets.h (git-ignored).
// Copy secrets.h.example to secrets.h and fill in your values.
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #error "Missing secrets.h - copy secrets.h.example to secrets.h and edit it."
#endif

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* ota_dnsName = OTA_HOSTNAME;
const char* ota_password = OTA_PASSWORD;

WiFiUDP Udp;
const unsigned int LOCAL_PORT = 5005;
char rxBuf[1024] = "rainbow";

#define PIN 16   // Bottom LEFT
#define PIN2 15  // Bottom RIGHT
#define NUMPIXELS 70

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

// Wiring Order and Organization of LED strips
// Strip2 --Controller-- Strip1a - Strip1b
// Strip1a  BRG
// Strip1b  BRG
// Strip2  GBR
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS * 2, PIN2, NEO_BRG + NEO_KHZ800); //WS2812B);   
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GBR + NEO_KHZ800); //  WS2812B_BRG); 
Adafruit_NeoPixel strip3 = Adafruit_NeoPixel(1, 18, NEO_BRG + NEO_KHZ400);              // NEO_BRG + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setPixel_decodedStripLoc(int loc, int r, int g, int b);
void setPixel_decodedStripLoc(uint16_t n, uint32_t c);
const int DECODE_NUM_PIXELS = 209;
void rainbow(uint8_t wait);
uint32_t Wheel(byte WheelPos);
void setupWifi();
void udpHandler();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting");
  setupWifi();

  if (Udp.begin(LOCAL_PORT)) {
    Serial.print("UDP listening on port "); Serial.println(LOCAL_PORT);
  } else {
    Serial.println("UDP begin failed!");
  }
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  strip.begin();
  strip.setBrightness(255);
  strip.clear();
  strip.show(); // Initialize all pixels to 'off'
  strip2.begin();
  strip2.setBrightness(255);
  strip2.clear();
  strip2.show(); // Initialize all pixels to 'off'
  strip3.begin();
  strip3.setPixelColor(0,64,64,64);
  strip3.setPixelColor(1,64,64,64);
  strip3.setPixelColor(2,64,64,64);
  strip3.show();
}

void loop() {
  ArduinoOTA.handle();  // must be called each loop and at least once every ~20ms 
  udpHandler();

  static long unsigned int nextRunTime1 = 0; // all times in milliseconds
  long unsigned int loopStartTime = millis();
  static unsigned long int loopFinishTime = 0;

  int red = 255;
  int green = 55;
  int blue = 15;
  // Serial.println(rxBuf);
  // Serial.println(strcmp(rxBuf, "hi"));

  if (loopStartTime > nextRunTime1) {
    nextRunTime1 = loopStartTime + 100; // wait 100ms before updating LEDs again.

  if(strcmp(rxBuf, "redgrn")==0) {
    int Cred = strip.Color(100,0,0);
    int Cgreen = strip.Color(0,100,0);
    for (int i = 0; i <= DECODE_NUM_PIXELS; i=i+2) {
      setPixel_decodedStripLoc(i, Cred);
      setPixel_decodedStripLoc(i+1, Cgreen);
    }
  } else if (strcmp(rxBuf, "purple")==0) {
    red = 255;
    green = 0;
    blue = 255;
    for (int i = 0; i <= 209; i=i+1) {
      setPixel_decodedStripLoc(i, red, green, blue);
    }
  } else if (strcmp(rxBuf, "rainbow")==0) {
    rainbowNF();
    } else if (strncmp(rxBuf, "cchk", 4)==0) {
      int temp_red, temp_grn, temp_blu;
      if(rxBuf[ 4] != '\0') {
        temp_red = (rxBuf[ 4] - '0')*100 + (rxBuf[ 5] - '0')*10 + (rxBuf[ 6] - '0')*1; 
        temp_grn = (rxBuf[ 7] - '0')*100 + (rxBuf[ 8] - '0')*10 + (rxBuf[ 9] - '0')*1; 
        temp_blu = (rxBuf[10] - '0')*100 + (rxBuf[11] - '0')*10 + (rxBuf[12] - '0')*1; 
      } else {
        temp_red = 90; 
        temp_grn = 5; 
        temp_blu = 10;
      }
      uint32_t Cchk = strip.Color(temp_red, temp_grn, temp_blu);

      for (int i = 0; i <= DECODE_NUM_PIXELS; i=i+1) {
        setPixel_decodedStripLoc(i, Cchk);
      }
  } else if (strcmp(rxBuf, "redGrnChase")==0) {
    redGrnChaseNF();
  } else if (strcmp(rxBuf, "redGrnChase2")==0) {
    redGrnChaseSinNF();
  } else if (strcmp(rxBuf, "dim")==0){
    red = 1;
    green = 1;
    blue = 1;
    for (int i = 0; i <= DECODE_NUM_PIXELS; i=i+1) {
      setPixel_decodedStripLoc(i, red, green, blue);
    }
  } else if (strcmp(rxBuf, "warm")==0) { // warm white
    red = 255;
    green = 55;
    blue = 15;
    for (int i = 0; i <= DECODE_NUM_PIXELS; i=i+1) {
      setPixel_decodedStripLoc(i, red, green, blue);
    }
  } else if (strcmp(rxBuf, "off")==0) {
    for (int i = 0; i <= DECODE_NUM_PIXELS; i++) {
      setPixel_decodedStripLoc(i, 0, 0, 0);
    }
  }

    strip.show();
    strip2.show();
  } // end nextRunTime1 process

  //Heart Beat LED
  if(millis() % 1000 > 500) {
    digitalWrite(LED_BUILTIN, HIGH);  
    //Serial.print("loop time = ");
    //Serial.println(loopFinishTime - loopStartTime);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  loopFinishTime = millis();
} // end loop()


void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    rp2040.restart();
  }

  // Port defaults to 2040
  // ArduinoOTA.setPort(2040);

  // Hostname defaults to pico-[ChipID]
  ArduinoOTA.setHostname(ota_dnsName);

  // No authentication by default
  ArduinoOTA.setPassword(ota_password);

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setPixel_decodedStripLoc(uint16_t n, uint32_t c) {
  uint8_t r = (uint8_t)(c >> 16);
  uint8_t g = (uint8_t)(c >>  8);
  uint8_t b = (uint8_t)(c      );
  setPixel_decodedStripLoc(n, r, g, b);
}

void setPixel_decodedStripLoc(int loc, int r, int g, int b) {
  // old idea (for loop around table)
  // 0-69 --> 0-69 strip
  // 70-139 --> 70-139 strip
  // 140-209 --> 69-0 strip2
  // new idea (one long strip with box in middle)
  // 0-69 --> 69 - 0 strip
  // 70-139 --> 0-69 strip2
  // 140-209 --> 70-139 strip2
  // DECODE_NUM_PIXELS = 209
  if ((loc >= 0) && (loc <= 69))  {
    // strip.setPixelColor(0, 100, 0, 0);  //strip from 0 to 69
    strip2.setPixelColor(69 - loc, r, g, b);  //strip from 0 to 69
  }
  else if ((loc >= 70) && (loc <= 139))  {
    strip.setPixelColor(loc-70, r, g, b);  //strip from 70 to 139
    // strip2.setPixelColor(loc - 70, r, g, b); 
  }
  else if ((loc >= 140) && (loc <= 209)) {
    // old strip2.setPixelColor(68 - (loc - 140), r, g, b); 
    strip.setPixelColor(loc-70, r, g, b);
  }
  else  {                                          
    // ERROR: We should never reach hear
  }
}

void rainbowNF() {
  static uint16_t j=0; 
  for (uint16_t i = 0; i <= DECODE_NUM_PIXELS; i++) {
    setPixel_decodedStripLoc(i, Wheel((i + j) & 255));
  }
  j = j + 1;
  if (j>=256) {j=0;}
}

int cheapSin(int x, int max) {
  x = x % (max*2);
  if(x<=max) {
    return x;
  } else {
    return 2*max-x;
  }
}

void redGrnChaseSinNF() {
  static uint16_t time=0;
  const int blank = 5;
  for(int pos=0; pos < 48; pos++) {
    int val = 4*cheapSin(pos+time,24);
    setPixel_decodedStripLoc(pos,val,0,0);
  }
  for(int pos=49+blank; pos < (48*2); pos++) {
    int val = 4*cheapSin(pos+time,24);
    setPixel_decodedStripLoc(pos,0,val,0);
  }
  time++;
  if(time > 48*100) { time = 0; }
}

void redGrnChaseNF() {
  static uint16_t j=0;
  int pRed = 100;
  int pGrn = 100;
  int pBlu = 0;
  int len = DECODE_NUM_PIXELS+1;
  for(int i = 0; i < len; i=i+30) {
    setPixel_decodedStripLoc((i+j+0)%len , pRed/16, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+1)%len , pRed/12, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+2)%len , pRed/8, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+3)%len , pRed/6, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+4)%len , pRed/4, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+5)%len , pRed/3, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+6)%len , pRed/2, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+7)%len , pRed/1, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+8)%len , pRed/2, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+9)%len , pRed/3, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+10)%len , pRed/4, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+11)%len , pRed/6, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+12)%len , pRed/8, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+13)%len , pRed/12, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+14)%len , pRed/16, 0     , pBlu);
    setPixel_decodedStripLoc((i+j+15)%len , 0     , pGrn/16, pBlu);
    setPixel_decodedStripLoc((i+j+16)%len , 0     , pGrn/12, pBlu);
    setPixel_decodedStripLoc((i+j+17)%len, 0     , pGrn/8, pBlu);
    setPixel_decodedStripLoc((i+j+18)%len, 0     , pGrn/6, pBlu);
    setPixel_decodedStripLoc((i+j+19)%len, 0     , pGrn/4, pBlu);
    setPixel_decodedStripLoc((i+j+20)%len, 0     , pGrn/3, pBlu);
    setPixel_decodedStripLoc((i+j+21)%len, 0     , pGrn/2, pBlu);
    setPixel_decodedStripLoc((i+j+22)%len, 0     , pGrn/1, pBlu);
    setPixel_decodedStripLoc((i+j+23)%len, 0     , pGrn/2, pBlu);
    setPixel_decodedStripLoc((i+j+24)%len, 0     , pGrn/3, pBlu);
    setPixel_decodedStripLoc((i+j+25)%len, 0     , pGrn/4, pBlu);
    setPixel_decodedStripLoc((i+j+26)%len, 0     , pGrn/6, pBlu);
    setPixel_decodedStripLoc((i+j+27)%len, 0     , pGrn/8, pBlu);
    setPixel_decodedStripLoc((i+j+28)%len, 0     , pGrn/12, pBlu);
    setPixel_decodedStripLoc((i+j+29)%len, 0     , pGrn/16, pBlu);
  }
  j = j + 1;
  if (j>=len) {j=0;}
}

void rainbow(uint8_t wait)
{
  uint16_t i, j;

  for (j = 0; j < 256; j++)
  {
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel((i + j) & 255));

    }

    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)

  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void udpHandler() {
  int packetSize = Udp.parsePacket();
  if (packetSize <= 0) return;

  int len = Udp.read(rxBuf, sizeof(rxBuf) - 1);
  if (len < 0) return;
  rxBuf[len] = '\0';

  IPAddress rip = Udp.remoteIP();
  uint16_t rport = Udp.remotePort();

  Serial.print("RX "); Serial.print(packetSize);
  Serial.print("B from "); Serial.print(rip);
  Serial.print(":"); Serial.println(rport);
  Serial.print("Data: "); Serial.println(rxBuf);

  // Confirmation reply (echo with a small prefix)
  Udp.beginPacket(rip, rport);
  Udp.print("OK: ");
  Udp.write((const uint8_t*)rxBuf, len);
  Udp.endPacket();
}

