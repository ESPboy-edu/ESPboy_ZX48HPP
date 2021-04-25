/*
ESPboy_Init class
for www.ESPboy.com project by RomanS
https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun
v1.0
*/

#include "ESPboyInit.h"

ESPboyInit::ESPboyInit(){};

void ESPboyInit::begin(const char *appName) {
  //Serial.begin(115200); //serial init
  WiFi.mode(WIFI_OFF); // to safe battery power

//DAC init and backlit off
  dac.begin(MCP4725address);
   dac.setVoltage(0, true);

//mcp23017 init for buttons, LED LOCK and TFT Chip Select pins
  mcp.begin(MCP23017address);
  delay(100);
  for (int i=0;i<8;i++){  
     mcp.pinMode(i, INPUT);
     mcp.pullUp(i, HIGH);}

//LED init
  myLED.begin(&this->mcp);
  myLED.setRGB(0,0,0);

//sound init and test
  pinMode(SOUNDPIN, OUTPUT);
  //playTone(200, 100); 
  //delay(100);
  //playTone(100, 100);
  //delay(100);
  //noPlayTone();
  
//LCD TFT init
  mcp.pinMode(CSTFTPIN, OUTPUT);
  mcp.digitalWrite(CSTFTPIN, LOW);
  tft.begin();
  tft.setSwapBytes(true);
  delay(100);
  //tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

//clear TFT and backlit on high
  dac.setVoltage(4095, true);
};


void ESPboyInit::playTone(uint16_t frq, uint16_t dur) { tone(SOUNDPIN, frq, dur); }
void ESPboyInit::playTone(uint16_t frq) { tone(SOUNDPIN, frq); }

void ESPboyInit::noPlayTone() { noTone(SOUNDPIN); }

uint8_t ESPboyInit::getKeys() { return (~mcp.readGPIOAB() & 255); }
