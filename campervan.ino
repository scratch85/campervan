#include <ButtonEvents.h>
#include "Led.h"
#include <Adafruit_NeoPixel.h>
#include "Color_Definitions.h"
#include <TMP36.h>

// Debugging Serial & Serial2 (BLE) 
//#define DEBUG
#define DEBUG_BLE

#if defined DEBUG && defined DEBUG_BLE
  #define DEBUG_PRINT(x) Serial.print(x); Serial2.print(x);
#elif defined DEBUG
  #define DEBUG_PRINT(x) Serial.print(x);
#elif defined DEBUG_BLE
  #define DEBUG_PRINT(x) Serial2.print(x);
#else
  #define DEBUG_PRINT(x)
#endif

// Serial & Seriel2 (Bluetooth Low Energy (BLE))
#define SERIAL_BAUDRATE 9600
#define SERIAL1_TIMEOUT 100
#define SERIAL2_TIMEOUT 100

// Sliding Door 1 (top)
#define BUTTON_SD1_PIN 1
ButtonEvents btnSD1;

// Sliding Door 2 (bottom)
#define BUTTON_SD2_PIN 2
ButtonEvents btnSD2;

// Driver Seat 1 (left, top)
#define BUTTON_DS1_PIN 12
ButtonEvents btnDS1;

// Driver Seat 2 (left, bottom)
#define BUTTON_DS2_PIN 11
ButtonEvents btnDS2;

// Co-Driver Seat 1 (right, top)
#define BUTTON_CS1_PIN 8
ButtonEvents btnCS1;

// Co-Driver Seat 2 (right, bottom)
#define BUTTON_CS2_PIN 7
ButtonEvents btnCS2;

// LED Spots 1, 2, 3 (front -> back)
#define LED1_PIN 21
Led led1(LED1_PIN);

#define LED2_PIN 22
Led led2(LED2_PIN);

#define LED3_PIN 23
Led led3(LED3_PIN);

// Number of ledstrips
#define LEDSTRIPS 2

// LED Strips 1, 2 (Driver, Co-Driver)
#define LEDSTRIP1_PIN 4
#define LEDSTRIP2_PIN 3

// How many NeoPixels do the srip have?
#define LEDSTRIP1_PIXELS 60
#define LEDSTRIP2_PIXELS 60

// Maximum brightness
#define LEDSTRIP_BRIGHTNESS 32

// The struct
struct strip {
  String name;
	unsigned int effect = 0;
	unsigned long lastUpdate = 0;
  unsigned long updateInterval = 50;
  unsigned int j = 0;
  unsigned int m = 0;
  Adafruit_NeoPixel obj;
};

strip strips[LEDSTRIPS];

// Temperatur TMP36
#define TMP36_PIN A0
#define TMP36_VOLTAGE 3.3
TMP36 tmp36(TMP36_PIN, TMP36_VOLTAGE);

void setup() {
  // Wait for serial
#ifdef DEBUG
  while(!Serial) {
    delay(10);
  }
#endif

  // Initialize serial output
#ifdef DEBUG
  Serial.begin(SERIAL_BAUDRATE);
  Serial.setTimeout(SERIAL1_TIMEOUT);
  Serial.print("Serial (console) started at ");
  Serial.print(SERIAL_BAUDRATE);
  Serial.print("\nSketch:   ");
  Serial.print(__FILE__);
  Serial.print("\nUploaded: ");
  Serial.print(__DATE__);
  Serial.print("\n");
#endif

  // Initialize serial2 - Bluetooth Low Energy (BLE)
#ifdef DEBUG_BLE
  Serial2.begin(SERIAL_BAUDRATE);
  Serial2.setTimeout(SERIAL2_TIMEOUT);
  DEBUG_PRINT("Serial2 (BLE) started at ");
  DEBUG_PRINT(SERIAL_BAUDRATE);
  Serial2.print("\nSketch:   ");
  Serial2.print(__FILE__);
  Serial2.print("\nUploaded: ");
  Serial2.print(__DATE__);
  Serial2.print("\n");
#endif

  // Initialize buttons
  pinMode(BUTTON_SD1_PIN, INPUT_PULLUP);
  btnSD1.attach(BUTTON_SD1_PIN);
  btnSD1.activeHigh();
  //btnSD1.debounceTime(15);
  btnSD1.doubleTapTime(200);
  //btnSD1.holdTime(2000);

  pinMode(BUTTON_SD2_PIN, INPUT_PULLUP);
  btnSD2.attach(BUTTON_SD2_PIN);
  btnSD2.activeHigh();
  //btnSD2.debounceTime(15);
  btnSD2.doubleTapTime(200);
  //btnSD2.holdTime(2000);

  pinMode(BUTTON_DS1_PIN, INPUT_PULLUP);
  btnDS1.attach(BUTTON_DS1_PIN);
  btnDS1.activeHigh();
  //btnDS1.debounceTime(15);
  btnDS1.doubleTapTime(200);
  btnDS1.holdTime(2000);

  pinMode(BUTTON_DS2_PIN, INPUT_PULLUP);
  btnDS2.attach(BUTTON_DS2_PIN);
  btnDS2.activeHigh();
  //btnDS2.debounceTime(15);
  btnDS2.doubleTapTime(200);
  //btnDS2.holdTime(2000);

  pinMode(BUTTON_CS1_PIN, INPUT_PULLUP);
  btnCS1.attach(BUTTON_CS1_PIN);
  btnCS1.activeHigh();
  //btnCS1.debounceTime(15);
  btnCS1.doubleTapTime(200);
  btnCS1.holdTime(2000);

  pinMode(BUTTON_CS2_PIN, INPUT_PULLUP);
  btnCS2.attach(BUTTON_CS2_PIN);
  btnCS2.activeHigh();
  //btnCS2.debounceTime(15);
  btnCS2.doubleTapTime(200);
  //btnCS2.holdTime(2000);

  // Initialize LED strips
  strips[0].name = "Strip1 (Driver)";
  strips[0].obj = Adafruit_NeoPixel(LEDSTRIP1_PIXELS, LEDSTRIP1_PIN, NEO_GRBW + NEO_KHZ800);
  strips[1].name = "Strip2 (Co-Driver)";
  strips[1].obj = Adafruit_NeoPixel(LEDSTRIP2_PIXELS, LEDSTRIP2_PIN, NEO_GRBW + NEO_KHZ800);
  DEBUG_PRINT("Ledstrips:\n");
  for(unsigned int i = 0; i < LEDSTRIPS; i++) {
    DEBUG_PRINT(i);
    DEBUG_PRINT(" on Pin: ");
    DEBUG_PRINT(strips[i].obj.getPin());
    DEBUG_PRINT("\n");
	  strips[i].effect = 0;
	  strips[i].lastUpdate = 0;
    strips[i].updateInterval = 50;
    strips[i].obj.begin();
    strips[i].obj.setBrightness(LEDSTRIP_BRIGHTNESS);
    strips[i].obj.show(); // Initialize all pixels to 'off'
  }
}

void loop() {
  // Read from Seriel2 (Bluetooth Low Energy (BLE) module) ...
  String strBluetooth = "";
  if(Serial2.available() > 0) {
    strBluetooth = Serial2.readString();
  }
  // ... and process inputs
  strBluetooth.trim();
  if(strBluetooth.length() > 0) {
    DEBUG_PRINT("Bluetooth: ");
    DEBUG_PRINT(strBluetooth);
    DEBUG_PRINT("\n");
    processSerialInput(strBluetooth);
  }

  // Read from Serial ...
#ifdef DEBUG
  String strSerial = "";
  if(Serial.available() > 0) {
    strSerial = Serial.readString();
  }
  // ... and process inputs
  strSerial.trim();
  if(strSerial.length() > 0) {
    DEBUG_PRINT("Serial: ");
    DEBUG_PRINT(strSerial);
    DEBUG_PRINT("\n");
    processSerialInput(strSerial);
  }
#endif

  // - - - - - BEGIN # Button Sliding Door 1 - - - - - - - - - -
  if(btnSD1.update() == true) {
    switch(btnSD1.event()) {
      case (tap):
        DEBUG_PRINT("btnSD1: tap event detected");
        led1.toggle();
        break;
      case (doubleTap):
        DEBUG_PRINT("btnSD1: doubleTap event detected");
        if(led1.isOn()) {
          led1.dimDown(round(255 * 0.2)); // dim down 20%
        } else {
          led1.on(round(255 * 0.8)); // switch on @ 80%
        }
        break;
      case (hold):
        DEBUG_PRINT("btnSD1: hold event detected");
        led1.on();
        led2.on();
        led3.on();
        break;
      case (none):
        break;
    }
  }
  // - - - - - END # Button Sliding Door 1 - - - - - - - - - -

  // - - - - - BEGIN # Button Driver Seat 1 - - - - - - - - - -
  if(btnDS1.update() == true) {
    switch(btnDS1.event()) {
      case (tap):
        DEBUG_PRINT("btnDS1: tap event detected");
        led3.toggle();
        break;
      case (doubleTap):
        DEBUG_PRINT("btnDS1: doubleTap event detected");
        if(led3.isOn()) {
          led3.dimDown(round(255 * 0.2)); // dim down 20%
        } else {
          led3.on(round(255 * 0.8)); // switch on @ 80%
        }
        break;
      case (hold):
        DEBUG_PRINT("btnDS1: hold event detected");
        led1.off();
        led2.off();
        led3.off();
        break;
      case (none):
        break;
    }
  }
  // - - - - - END # Button Driver Seat 1 - - - - - - - - - -

  // - - - - - BEGIN # Button Driver Seat 2 - - - - - - - - - -
  if(btnDS2.update() == true) {
    switch(btnDS2.event()) {
      case (tap):
        DEBUG_PRINT("btnDS2: tap event detected\n");
        if(strips[0].effect > 0 || !stripGetState(&strips[0])) {
          stripSetColor(&strips[0], WHITE_LED);
          strips[0].effect = 0;
        } else {
          stripSetColor(&strips[0], BLACK);
          strips[0].effect = 0;
        }
        break;
      case (doubleTap):
        DEBUG_PRINT("btnDS2: doubleTap event detected\n");
        strips[0].effect++;
        if(strips[0].effect > 2) {
          stripSetColor(&strips[0], BLACK);
          strips[0].effect = 0;
        }
        break;
      case (hold):
        DEBUG_PRINT("btnDS2: hold event detected\n");
        break;
      case (none):
        break;
    }
  }
  // - - - - - END # Button Driver Seat 2 - - - - - - - - - -

  // - - - - - BEGIN # Button Co-Driver Seat 1 - - - - - - - - - -
  if(btnCS1.update() == true) {
    switch(btnCS1.event()) {
      case (tap):
        DEBUG_PRINT("btnCS1: tap event detected");
        led3.toggle();
        break;
      case (doubleTap):
        DEBUG_PRINT("btnCS1: doubleTap event detected");
        if(led3.isOn()) {
          led3.dimDown(round(255 * 0.2)); // dim down 20%
        } else {
          led3.on(round(255 * 0.8)); // switch on @ 80%
        }
        break;
      case (hold):
        DEBUG_PRINT("btnCS1: hold event detected: ALL OFF\n");
        led1.off();
        led2.off();
        led3.off();
        break;
      case (none):
        break;
    }
  }
  // - - - - - END # Button Co-Driver Seat 1 - - - - - - - - - -

  // - - - - - BEGIN # Button Co-Driver Seat 2 - - - - - - - - - -
  if(btnCS2.update() == true) {
    switch(btnCS2.event()) {
      case (tap):
        DEBUG_PRINT("btnCS2: tap event detected\n");
        if(strips[0].effect > 0 || !stripGetState(&strips[0])) {
          stripSetColor(&strips[0], WHITE_LED);
          strips[0].effect = 0;
        } else {
          stripSetColor(&strips[0], BLACK);
          strips[0].effect = 0;
        }
        break;
      case (doubleTap):
        DEBUG_PRINT("btnCS2: doubleTap event detected\n");
        strips[0].effect++;
        if(strips[0].effect > 2) {
          stripSetColor(&strips[0], BLACK);
          strips[0].effect = 0;
        }
        break;
      case (hold):
        DEBUG_PRINT("btnCS2: hold event detected\n");
        break;
      case (none):
        break;
    }
  }
  // - - - - - END # Button Co-Driver Seat 2 - - - - - - - - - -

  // Handle LED strips
  for(unsigned int i = 0; i < LEDSTRIPS; i++) {
    // Check if an update is needed
    if(millis() - strips[i].lastUpdate >= strips[i].updateInterval) {
      switch(strips[i].effect) {
        case 0:
          // do nothing
          break;
        case 1:
          stripRainbow(&strips[i]);
          break;
        case 2:
          stripRainbowCycle(&strips[i]);
          break;
        case 3:
          stripKnightRider(&strips[i], RED);
          break;
      }
      strips[i].lastUpdate = millis();
    }
  }
  delay(10);
}

void processSerialInput(String data) {
  String v1, v2, v3, v4;
  uint32_t i1, i2;

  // process everything in lowercase
  data.toLowerCase();
  DEBUG_PRINT("processSerialInput: ");
  DEBUG_PRINT(data);
  DEBUG_PRINT("\n");

  // debug split
  i1 = 0;
  v1 = split(data,' ',0);
  DEBUG_PRINT("split_");
  DEBUG_PRINT(i1);
  DEBUG_PRINT(": ");
  DEBUG_PRINT(v1);
  DEBUG_PRINT("\n");
  while(v1.length() > 0 && i1 < 10) {
    v1 = split(data,' ',i1++);
    DEBUG_PRINT("split_");
    DEBUG_PRINT(i1);
    DEBUG_PRINT(": ");
    DEBUG_PRINT(v1);
    DEBUG_PRINT("\n");
  }

  if(data.equalsIgnoreCase("help")) {
    DEBUG_PRINT("usage: status [led <1-3>|strip <1-2>|tmp [C|F]]\n");
    DEBUG_PRINT("   or: led <1-3|all> [on|off|0-255|0-100%]\n");
    DEBUG_PRINT("   or: strip <1-2|all> <on|off|color|effect [id|name] [...]>\n");
    DEBUG_PRINT("\n");
    DEBUG_PRINT("Common commands:\n");
    DEBUG_PRINT("  status - get the status of leds, strips and sensors\n");
    DEBUG_PRINT("  led    - control led lights\n");
    DEBUG_PRINT("  strip  - control led strips\n");
    DEBUG_PRINT("\n");
    DEBUG_PRINT("Generic options:\n");
    DEBUG_PRINT("  <1-3>  - select the device id you want to control\n");
    DEBUG_PRINT("  all    - select to control all led or strip devices\n");
    DEBUG_PRINT("\n");
    DEBUG_PRINT("Specific options:\n");
    DEBUG_PRINT("  on     - switch device on\n");
    DEBUG_PRINT("  off    - switch device off\n");
    DEBUG_PRINT("  0-255  - select brightness as byte\n");
    DEBUG_PRINT("  0-100% - switch brightness in percent\n");
    DEBUG_PRINT("  color  - select a specific color (see \"Colors\")\n");
    DEBUG_PRINT("  effect - select a specific effect by id or name (see \"Effects\")\n");
    DEBUG_PRINT("\n");
    DEBUG_PRINT("Colors:\n");
    DEBUG_PRINT("  black, blue, green, red, white\n");
    DEBUG_PRINT("\n");
    DEBUG_PRINT("Effects:\n");
    DEBUG_PRINT("  1: rainbow (rb)       - rainbow colors slowly changing\n");
    DEBUG_PRINT("  2: rainbowcycle (rbc) - all rainbow colors cycling at once\n");
    DEBUG_PRINT("  3: knightrider (kr)   - moving red light\n");
    DEBUG_PRINT("\n");
  } else if(data.startsWith("status")) {
    v1 = split(data,' ',1);
    if(v1.equals("led")) {
      v1 = split(data,' ',2);
      if(v1.equalsIgnoreCase("1") || v1.equalsIgnoreCase("2") || v1.equalsIgnoreCase("3")) {
        DEBUG_PRINT("led");
        DEBUG_PRINT(v1);
        DEBUG_PRINT(": ");
        if(v1.equalsIgnoreCase("1")) {
          DEBUG_PRINT(led1.getBrightness());
        } else if(v1.equalsIgnoreCase("2")) {
          DEBUG_PRINT(led2.getBrightness());
        } else if(v1.equalsIgnoreCase("3")) {
          DEBUG_PRINT(led3.getBrightness());
        } else {
          DEBUG_PRINT(0);
        }
      } else {
        DEBUG_PRINT("led1: ");
        DEBUG_PRINT(led1.getBrightness());
        DEBUG_PRINT("\n");
        DEBUG_PRINT("led2: ");
        DEBUG_PRINT(led2.getBrightness());
        DEBUG_PRINT("\n");
        DEBUG_PRINT("led3: ");
        DEBUG_PRINT(led3.getBrightness());
      }
      DEBUG_PRINT("\n");
    } else if(v1.equals("strip")) {
      v1 = split(data,' ',2);
      if(v1.equalsIgnoreCase("1") || v1.equalsIgnoreCase("2")) {
        DEBUG_PRINT("strip");
        DEBUG_PRINT(v1);
        DEBUG_PRINT(": ");
        if(v1.equalsIgnoreCase("1")) {
          DEBUG_PRINT(stripGetState(&strips[0]));
        } else if(v1.equalsIgnoreCase("2")) {
          DEBUG_PRINT(stripGetState(&strips[1]));
        } else {
          DEBUG_PRINT(0);
        }
      } else {
        DEBUG_PRINT("strip1: ");
        DEBUG_PRINT(stripGetState(&strips[0]));
        DEBUG_PRINT("\n");
        DEBUG_PRINT("strip2: ");
        DEBUG_PRINT(stripGetState(&strips[1]));
      }
      DEBUG_PRINT("\n");
    } else if(v1.equals("tmp")) {
      v1 = split(data,' ',2);
      DEBUG_PRINT("temperature: ");
      if(v1.equalsIgnoreCase("F")) {
        DEBUG_PRINT(String(tmp36.getTempF()));
      } else {
        DEBUG_PRINT(String(tmp36.getTempC()));
      }
      DEBUG_PRINT("\n");
    } else {
      DEBUG_PRINT("led1: ");
      DEBUG_PRINT(led1.getBrightness());
      DEBUG_PRINT("\n");
      DEBUG_PRINT("led2: ");
      DEBUG_PRINT(led2.getBrightness());
      DEBUG_PRINT("\n");
      DEBUG_PRINT("led3: ");
      DEBUG_PRINT(led3.getBrightness());
      DEBUG_PRINT("\n");
      DEBUG_PRINT("strip1: ");
      DEBUG_PRINT(stripGetState(&strips[0]));
      DEBUG_PRINT("\n");
      DEBUG_PRINT("strip2: ");
      DEBUG_PRINT(stripGetState(&strips[1]));
      DEBUG_PRINT("\n");
      DEBUG_PRINT("temperature: ");
      DEBUG_PRINT(String(tmp36.getTempC()));
      DEBUG_PRINT("\n");
    }
  } else if(data.startsWith("led")) {
    v1 = split(data,' ',1);
    v2 = split(data,' ',2);
    if(v2.equalsIgnoreCase("on")) {
      i1 = 255;
    } else if(v2.equalsIgnoreCase("off")) {
      i1 = 0;
    } else if(v2.endsWith("%")) {
      i1 = map(sanitizeValue(v2.remove(v2.length() - 1), 0, 0, 100), 0, 100, 0, 255);
    } else {
      i1 = sanitizeValue(v2, 255, 0, 255);
    }
    if(v1.equalsIgnoreCase("1") || v1.equalsIgnoreCase("2") || v1.equalsIgnoreCase("3")) {
      if(v1.equalsIgnoreCase("1")) {
        led1.on(i1);
      } else if(v1.equalsIgnoreCase("2")) {
        led2.on(i1);
      } else if(v1.equalsIgnoreCase("3")) {
        led3.on(i1);
      } else {
        // nothing
      }
    } else {
      led1.on(i1);
      led2.on(i1);
      led3.on(i1);
    }
  } else if(data.equals("strip")) {
    v1 = split(data,' ',1);
    v2 = split(data,' ',2);
    v3 = split(data,' ',3);
    v4 = split(data,' ',4);
    if(v2.equalsIgnoreCase("on")) {
      i1 = WHITE_LED;
    } else if(v2.equalsIgnoreCase("off")) {
      i1 = BLACK;
    } else if(v2.equalsIgnoreCase("effect") || v2.equalsIgnoreCase("e")) {
      v2 = "e";
      if(v3.equalsIgnoreCase("rainbow") || v3.equalsIgnoreCase("rb") || v3.equalsIgnoreCase("1")) {
        i1 = 1;
      } else if(v3.equalsIgnoreCase("rainbowcycle") || v3.equalsIgnoreCase("rbc") || v3.equalsIgnoreCase("2")) {
        i1 = 2;
      } else if(v3.equalsIgnoreCase("knightrider") || v3.equalsIgnoreCase("kr") || v3.equalsIgnoreCase("3")) {
        i1 = 3;
        i2 = getColorFromString(v4);
      } else {
        i1 = 0;
      }
    } else {
      i1 = getColorFromString(v2);
    }
    if(v1.equalsIgnoreCase("1") || v1.equalsIgnoreCase("2")) {
      if(v1.equalsIgnoreCase("1")) {
        if(v2.equalsIgnoreCase("e")) {
          strips[0].effect = i1;
        } else {
          stripSetColor(&strips[0], i1);
          strips[0].effect = 0;
        }
      } else if(v1.equalsIgnoreCase("2")) {
        if(v2.equalsIgnoreCase("e")) {
          strips[1].effect = i1;
        } else {
          stripSetColor(&strips[1], i1);
          strips[1].effect = 0;
        }
      } else {
        // nothing
      }
    } else {
      if(v2.equalsIgnoreCase("e")) {
        strips[0].effect = i1;
        strips[1].effect = i1;
      } else {
        stripSetColor(&strips[0], i1);
        strips[0].effect = 0;
        stripSetColor(&strips[1], i1);
        strips[1].effect = 0;
      }
    }
  }
}

String split(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  for(int i = 0; i <= maxIndex && found <= index; i++) {
    if(data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

uint32_t sanitizeValue(String data, uint32_t def, uint32_t vmin, uint32_t vmax) {
  uint32_t tmp;
  if(data.length() > 0) {
    tmp = data.toInt();
    tmp = min(max(tmp, vmin), vmax);
  } else {
    tmp = def;
  }
  return tmp;
}

uint32_t getColorFromString(String c) {
  if(c.equalsIgnoreCase("black") || c.equalsIgnoreCase("bl")) return BLACK;
  if(c.equalsIgnoreCase("blue")  || c.equalsIgnoreCase("b"))  return BLUE;
  if(c.equalsIgnoreCase("green") || c.equalsIgnoreCase("g"))  return GREEN;
  if(c.equalsIgnoreCase("red")   || c.equalsIgnoreCase("r"))  return RED;
  if(c.equalsIgnoreCase("white") || c.equalsIgnoreCase("w"))  return WHITE_LED;
  return sanitizeValue(c, 0, 0, 4294967295);
} 

// Set all LEDs of the strip to the same color
void stripSetColor(strip *s, uint32_t c) {
  DEBUG_PRINT("stripSetColor (");
  DEBUG_PRINT(s->name);
  DEBUG_PRINT(", ");
  DEBUG_PRINT(c);
  DEBUG_PRINT(")");
  for(uint16_t i = 0; i < s->obj.numPixels(); i++) {
    DEBUG_PRINT(i);
    DEBUG_PRINT(", ");
    s->obj.setPixelColor(i, c);
  }
  s->obj.show();
  DEBUG_PRINT("stripSetColor: done\n");
}

void stripRainbow(strip *s) {
  //DEBUG_PRINT("stripRainbow(");
  //DEBUG_PRINT(s->name);
  //DEBUG_PRINT(")\n");
  for(uint16_t i = 0; i < s->obj.numPixels(); i++) {
    s->obj.setPixelColor(i, Wheel(s->obj,(i + s->j) & 255));
  }
  s->obj.show();
  s->j++;
  if(s->j >= 256) {
    s->j = 0;
  }
  //DEBUG_PRINT("stripRainbow: done\n");
}

// Slightly different, this makes the rainbow equally distributed throughout
void stripRainbowCycle(strip *s) {
  //DEBUG_PRINT("stripRainbowCycle(");
  //DEBUG_PRINT(s->name);
  //DEBUG_PRINT(")\n");
  for(uint16_t i = 0; i < s->obj.numPixels(); i++) {
    s->obj.setPixelColor(i, Wheel(s->obj,((i * 256 / s->obj.numPixels()) + s->j) & 255));
  }
  s->obj.show();
  s->j++;
  if(s->j >= 256) {
    s->j = 0;
  }
  //DEBUG_PRINT("stripRainbowCycle: done\n");
}

void stripKnightRider(strip *s, uint32_t c) {
  uint16_t x = 2;

  //DEBUG_PRINT("stripKnightRider(");
  //DEBUG_PRINT(s->name);
  //DEBUG_PRINT(", ");
  //DEBUG_PRINT(c);
  //DEBUG_PRINT(")\n");
  for(uint16_t i = 0; i < s->obj.numPixels(); i++) {
    if(i >= s->j - x && i <= s->j + x) {
      s->obj.setPixelColor(i, c);
    } else {
      s->obj.setPixelColor(i, BLACK);
    }
  }
  s->obj.show();
  if(s->m == 0) {
    s->j++;
  } else {
    s->j--;
  }
  if(s->j <= 0) {
    s->m = 0;
  }
  if(s->j >= s->obj.numPixels()) {
    s->m = 1;
  }
  //DEBUG_PRINT("stripKnightRider: done\n");
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(Adafruit_NeoPixel &strip, byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

// Returns true if at least one of the LEDs of the stip is glowing
bool stripGetState(strip *s) {
  DEBUG_PRINT("stripGetState(");
  DEBUG_PRINT(s->name);
  DEBUG_PRINT(")\n");
  bool state = false;
  uint16_t i = 0;
  while(!state && i < s->obj.numPixels()) {
    if(s->obj.getPixelColor(i) == 0) {
      i++;
    } else {
      DEBUG_PRINT("stripGetState (");
      DEBUG_PRINT(i);
      DEBUG_PRINT("): ");
      DEBUG_PRINT(s->obj.getPixelColor(i));
      DEBUG_PRINT("\n");
      state = true;
    }
  }
  DEBUG_PRINT("stripGetColor: state = ");
  if(state) {
    DEBUG_PRINT("true\n");
  } else {
    DEBUG_PRINT("false\n");
  }
  DEBUG_PRINT("stripGetColor: done\n");
  return state;
}
