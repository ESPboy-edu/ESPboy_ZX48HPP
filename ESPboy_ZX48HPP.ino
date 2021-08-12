//v1.7 12.08.2021 LittleFS again, numorous small optimizations (Romans)
//v1.6 30.05.2021 SPIFFS returned back, some INLINE to CACHE and int8_t to int32_t optimisations (Romans)
//v1.5 25.04.2021 del OTA, change SPIFFS to Little FS, fixed render (border) fixed file list navigation (RomanS)
//v1.4 29.04.2020 new OTA for ESPboy App store, reading the game from PROGMEM if no one on SPIFFS, sound noise bugfix (RomanS)
//v1.3 12.03.2020 core rewritten to c++, memory and speed optimizations, ESPboy OTA v1.0 added by Plague(DmitryL) plague@gmx.co.uk
//v1.2 06.01.2020 bug fixes, onscreen keyboard added, keyboard module support
//v1.1 23.12.2019  z80 format v3 support, improved frameskip, screen and controls config files
//v1.0 20.12.2019 initial version, with sound

//by Shiru
//shiru@mail.ru
//https://www.patreon.com/shiru8bit
//uses Z80 core by Ketmar


//#include "User_Setup.h"
//#define USER_SETUP_LOADED

#include <arduino.h>

extern void zx_setup();
extern void zx_loop();

void setup() {
		zx_setup();
}

void loop(){
		zx_loop();
	}
