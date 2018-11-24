#include <AceButton.h>
#include "Led.h"
#include <Adafruit_NeoPixel.h>
#include "Color_Definitions.h"
#include <TMP36.h>
using namespace ace_button;

// Debugging Serial & Serial2 (BLE) 
#define DEBUG
#define ENABLE_BLE
#define DEBUG_BLE

#if defined DEBUG && defined ENABLE_BLE && defined DEBUG_BLE
  #define DEBUG_PRINT(x) Serial.print(x); Serial2.print(x);
#elif defined DEBUG
  #define DEBUG_PRINT(x) Serial.print(x);
#elif defined ENABLE_BLE && defined DEBUG_BLE
  #define DEBUG_PRINT(x) Serial2.print(x);
#else
  #define DEBUG_PRINT(x)
#endif

// Serial & Seriel2 (Bluetooth Low Energy (BLE))
#define SERIAL_BAUDRATE 9600
#define SERIAL1_TIMEOUT 100
#define SERIAL2_TIMEOUT 100

String strSerial = "";
String strBluetooth = "";

// Buttons
ButtonConfig buttonConfigBed;
ButtonConfig buttonConfigStrip;

#define BUTTON_SD1_PIN 1  // Sliding Door 1 (top)
#define BUTTON_SD2_PIN 2  // Sliding Door 2 (bottom)
AceButton btnSD1, btnSD2;

#define BUTTON_DS1_PIN 12 // Driver Seat 1 (left, top)
#define BUTTON_DS2_PIN 11 // Driver Seat 2 (left, bottom)
AceButton btnDS1(&buttonConfigBed), btnDS2(&buttonConfigStrip);

#define BUTTON_CS1_PIN 8  // Co-Driver Seat 1 (right, top)
#define BUTTON_CS2_PIN 7  // Co-Driver Seat 2 (right, bottom)
AceButton btnCS1(&buttonConfigBed), btnCS2(&buttonConfigStrip);

void handleSystemEvent(AceButton*, uint8_t, uint8_t);
void handleBedEvent(AceButton*, uint8_t, uint8_t);
void handleStripEvent(AceButton*, uint8_t, uint8_t);

// LED Spots 1, 2, 3 (front -> back)
#define LED1_PIN 21
#define LED2_PIN 22
#define LED3_PIN 23
Led led1, led2, led3;

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
  Adafruit_NeoPixel *obj;
};

strip strips[LEDSTRIPS];
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(LEDSTRIP1_PIXELS, LEDSTRIP1_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(LEDSTRIP2_PIXELS, LEDSTRIP2_PIN, NEO_GRBW + NEO_KHZ800);

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
  ButtonConfig* systemButtonConfig = ButtonConfig::getSystemButtonConfig();
  systemButtonConfig->setEventHandler(handleSystemEvent);
  systemButtonConfig->setFeature(ButtonConfig::kFeatureClick);
  systemButtonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  systemButtonConfig->setFeature(ButtonConfig::kFeatureLongPress);

  buttonConfigBed.setEventHandler(handleBedEvent);
  buttonConfigBed.setFeature(ButtonConfig::kFeatureClick);
  buttonConfigBed.setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfigBed.setFeature(ButtonConfig::kFeatureLongPress);
  
  buttonConfigStrip.setEventHandler(handleStripEvent);
  buttonConfigStrip.setFeature(ButtonConfig::kFeatureClick);
  buttonConfigStrip.setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfigStrip.setFeature(ButtonConfig::kFeatureLongPress);

  pinMode(BUTTON_SD1_PIN, INPUT_PULLUP);
  btnSD1.init(BUTTON_SD1_PIN, HIGH, 0);
  pinMode(BUTTON_SD2_PIN, INPUT_PULLUP);
  btnSD2.init(BUTTON_SD2_PIN, HIGH, 1);
  
  pinMode(BUTTON_DS1_PIN, INPUT_PULLUP);
  btnDS1.init(BUTTON_DS1_PIN, HIGH, 0);
  pinMode(BUTTON_CS1_PIN, INPUT_PULLUP);
  btnCS1.init(BUTTON_CS1_PIN, HIGH, 1);

  pinMode(BUTTON_DS2_PIN, INPUT_PULLUP);
  btnDS2.init(BUTTON_DS2_PIN, HIGH, 0);
  pinMode(BUTTON_CS2_PIN, INPUT_PULLUP);
  btnCS2.init(BUTTON_CS2_PIN, HIGH, 1);

   // Initialize LEDs
  led1.attach(LED1_PIN);
  led2.attach(LED2_PIN);
  led3.attach(LED3_PIN);

  // Initialize LED strips
  strips[0].name = "Strip1 (Driver)";
  strips[0].obj = &strip1;
  strips[1].name = "Strip2 (Co-Driver)";
  strips[1].obj = &strip1;
  DEBUG_PRINT("Ledstrips:\n");
  for(unsigned int i = 0; i < LEDSTRIPS; i++) {
    DEBUG_PRINT(i);
    DEBUG_PRINT(" on Pin: ");
    DEBUG_PRINT(strips[i].obj->getPin());
    DEBUG_PRINT("\n");
	  strips[i].effect = 0;
	  strips[i].lastUpdate = 0;
    strips[i].updateInterval = 50;
    strips[i].obj->begin();
    strips[i].obj->setBrightness(LEDSTRIP_BRIGHTNESS);
    strips[i].obj->show(); // Initialize all pixels to 'off'
  }
}

void loop() {
  char in;

  // Read from Seriel2 (Bluetooth Low Energy (BLE) module) ...
#ifdef ENABLE_BLE
  if(Serial2.available() > 0) {
    in = Serial2.read();
    if(in == char(10) || in == char(13) || in == NULL) {
      // ... and process inputs
      strBluetooth.trim();
      if(strBluetooth.length() > 0) {
        DEBUG_PRINT("Bluetooth: ");
        DEBUG_PRINT(strBluetooth);
        DEBUG_PRINT("\n");
        processSerialInput(strBluetooth);
        strBluetooth = "";
      }
    } else {
      strBluetooth += in;
    }
  }
#endif

  // Read from Serial ...
#ifdef DEBUG
  if(Serial.available() > 0) {
    in = Serial.read();
    if(in == char(10) || in == char(13) || in == NULL) {
      // ... and process inputs
      strSerial.trim();
      if(strSerial.length() > 0) {
        DEBUG_PRINT("Serial: ");
        DEBUG_PRINT(strSerial);
        DEBUG_PRINT("\n");
        processSerialInput(strSerial);
        strSerial = "";
      }
    } else {
      strSerial += in;
    }
  }
#endif

  // The buttons
  btnSD1.check();
  btnSD2.check();
  btnDS1.check();
  btnDS2.check();
  btnCS1.check();
  btnCS2.check();

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
  // process everything in lowercase
  data.toLowerCase();
  DEBUG_PRINT("processSerialInput: ");
  DEBUG_PRINT(data);
  DEBUG_PRINT("\n");

  if(data.equalsIgnoreCase("help")) {
    processHelp();
  } else if(data.startsWith("status")) {
    processStatus(split(data,' ',1), split(data,' ',2));
  } else if(data.startsWith("led")) {
    processLed(split(data,' ',1), split(data,' ',2));
  } else if(data.startsWith("strip")) {
    processStrip(split(data,' ',1), split(data,' ',2), split(data,' ',3), split(data,' ',4));
  }
}

void processHelp() {
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
}

void processStatus(String v1, String v2) {
  DEBUG_PRINT("processStatus(");
  DEBUG_PRINT(v1);
  DEBUG_PRINT(", ");
  DEBUG_PRINT(v2);
  DEBUG_PRINT(")\n");

  if(v1.equals("led")) {
    if(v2.equalsIgnoreCase("1") || v2.equalsIgnoreCase("2") || v2.equalsIgnoreCase("3")) {
      DEBUG_PRINT("led");
      DEBUG_PRINT(v2);
      DEBUG_PRINT(": ");
      if(v2.equalsIgnoreCase("1")) {
        DEBUG_PRINT(led1.getBrightness());
      } else if(v2.equalsIgnoreCase("2")) {
        DEBUG_PRINT(led2.getBrightness());
      } else if(v2.equalsIgnoreCase("3")) {
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
      DEBUG_PRINT("\n");
    }
    DEBUG_PRINT("\n");
  } else if(v1.equals("strip")) {
    if(v2.equalsIgnoreCase("1") || v2.equalsIgnoreCase("2")) {
      DEBUG_PRINT("strip");
      DEBUG_PRINT(v2);
      DEBUG_PRINT(": ");
      if(v2.equalsIgnoreCase("1")) {
        DEBUG_PRINT(stripGetState(&strips[0]));
      } else if(v2.equalsIgnoreCase("2")) {
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
    DEBUG_PRINT("temperature: ");
    if(v2.equalsIgnoreCase("F")) {
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
}

void processLed(String v1, String v2) {
  DEBUG_PRINT("processLed(");
  DEBUG_PRINT(v1);
  DEBUG_PRINT(", ");
  DEBUG_PRINT(v2);
  DEBUG_PRINT(")\n");

  uint32_t i1;
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
}

void processStrip(String v1, String v2, String v3, String v4) {
  DEBUG_PRINT("processStrip(");
  DEBUG_PRINT(v1);
  DEBUG_PRINT(", ");
  DEBUG_PRINT(v2);
  DEBUG_PRINT(", ");
  DEBUG_PRINT(v3);
  DEBUG_PRINT(", ");
  DEBUG_PRINT(v4);
  DEBUG_PRINT(")\n");

  uint32_t i1, i2;
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

void handleSystemEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  // Print out a message for all events.
  DEBUG_PRINT(F("handleSystemEvent(): eventType: "));
  DEBUG_PRINT(eventType);
  DEBUG_PRINT("\n");
  DEBUG_PRINT(F("; buttonState: "));
  DEBUG_PRINT(buttonState);
  DEBUG_PRINT("\n");

  switch (eventType) {
    case AceButton::kEventClicked:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventClicked event detected\n");
      led1.toggle();
      break;
    case AceButton::kEventDoubleClicked:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventDoubleClicked event detected\n");
      if(led1.isOn()) {
        led1.dimDown(round(255 * 0.2)); // dim down 20%
      } else {
       led1.on(round(255 * 0.8)); // switch on @ 80%
      }
      break;
    case AceButton::kEventLongPressed:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventLongPressed event detected\n");
      if(led1.isOn() || led2.isOn() || led3.isOn()) {
        led1.off();
        led2.off();
        led3.off();
      } else {
        led1.on();
        led2.on();
        led3.on();
      }
      break;
  }
}

void handleBedEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  // Print out a message for all events.
  DEBUG_PRINT(F("handleBedEvent(): eventType: "));
  DEBUG_PRINT(eventType);
  DEBUG_PRINT("\n");
  DEBUG_PRINT(F("; buttonState: "));
  DEBUG_PRINT(buttonState);
  DEBUG_PRINT("\n");

  switch (eventType) {
    case AceButton::kEventClicked:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventClicked event detected\n");
      led3.toggle();
      break;
    case AceButton::kEventDoubleClicked:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventDoubleClicked event detected\n");
      if(led3.isOn()) {
        led3.dimDown(round(255 * 0.2)); // dim down 20%
      } else {
        led3.on(round(255 * 0.8)); // switch on @ 80%
      }
      break;
    case AceButton::kEventLongPressed:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventLongPressed event detected\n");
      if(led1.isOn() || led2.isOn() || led3.isOn()) {
        led1.off();
        led2.off();
        led3.off();
      } else {
        led1.on();
        led2.on();
        led3.on();
      }
      break;
  }
}

void handleStripEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  // Print out a message for all events.
  DEBUG_PRINT(F("handleStripEvent(): eventType: "));
  DEBUG_PRINT(eventType);
  DEBUG_PRINT("\n");
  DEBUG_PRINT(F("; buttonState: "));
  DEBUG_PRINT(buttonState);
  DEBUG_PRINT("\n");

  switch (eventType) {
    case AceButton::kEventClicked:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventClicked event detected\n");
      if(strips[button->getId()].effect > 0 || !stripGetState(&strips[button->getId()])) {
        stripSetColor(&strips[button->getId()], WHITE_LED);
        strips[button->getId()].effect = 0;
      } else {
        stripSetColor(&strips[button->getId()], BLACK);
        strips[button->getId()].effect = 0;
      }
      break;
    case AceButton::kEventDoubleClicked:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventDoubleClicked event detected\n");
      strips[button->getId()].effect++;
      if(strips[button->getId()].effect > 2) {
        stripSetColor(&strips[button->getId()], BLACK);
        strips[button->getId()].effect = 0;
      }
      break;
    case AceButton::kEventLongPressed:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventLongPressed event detected: ALL OFF\n");
      stripSetColor(&strips[0], BLACK);
      strips[0].effect = 0;
      stripSetColor(&strips[1], BLACK);
      strips[1].effect = 0;
      break;
  }
}

// Set all LEDs of the strip to the same color
void stripSetColor(strip *s, uint32_t c) {
  //DEBUG_PRINT("stripSetColor (");
  //DEBUG_PRINT(s->name);
  //DEBUG_PRINT(", ");
  //DEBUG_PRINT(c);
  //DEBUG_PRINT(")");
  for(uint16_t i = 0; i < s->obj->numPixels(); i++) {
    //DEBUG_PRINT(i);
    //DEBUG_PRINT(", ");
    s->obj->setPixelColor(i, c);
  }
  s->obj->show();
  //DEBUG_PRINT("stripSetColor: done\n");
}

void stripRainbow(strip *s) {
  //DEBUG_PRINT("stripRainbow(");
  //DEBUG_PRINT(s->name);
  //DEBUG_PRINT(")\n");
  for(uint16_t i = 0; i < s->obj->numPixels(); i++) {
    s->obj->setPixelColor(i, Wheel(s->obj,(i + s->j) & 255));
  }
  s->obj->show();
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
  for(uint16_t i = 0; i < s->obj->numPixels(); i++) {
    s->obj->setPixelColor(i, Wheel(s->obj,((i * 256 / s->obj->numPixels()) + s->j) & 255));
  }
  s->obj->show();
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
  for(uint16_t i = 0; i < s->obj->numPixels(); i++) {
    if(i >= s->j - x && i <= s->j + x) {
      s->obj->setPixelColor(i, c);
    } else {
      s->obj->setPixelColor(i, BLACK);
    }
  }
  s->obj->show();
  if(s->m == 0) {
    s->j++;
  } else {
    s->j--;
  }
  if(s->j <= 0) {
    s->m = 0;
  }
  if(s->j >= s->obj->numPixels()) {
    s->m = 1;
  }
  //DEBUG_PRINT("stripKnightRider: done\n");
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(Adafruit_NeoPixel *s, byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return s->Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    return s->Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return s->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

// Returns true if at least one of the LEDs of the stip is glowing
bool stripGetState(strip *s) {
  //DEBUG_PRINT("stripGetState(");
  //DEBUG_PRINT(s->name);
  //DEBUG_PRINT(")\n");
  bool state = false;
  uint16_t i = 0;
  while(!state && i < s->obj->numPixels()) {
    if(s->obj->getPixelColor(i) == 0) {
      i++;
    } else {
      //DEBUG_PRINT("stripGetState (");
      //DEBUG_PRINT(i);
      //DEBUG_PRINT("): ");
      //DEBUG_PRINT(s->obj->getPixelColor(i));
      //DEBUG_PRINT("\n");
      state = true;
    }
  }
  //DEBUG_PRINT("stripGetColor: state = ");
  //if(state) { DEBUG_PRINT("true\n"); } else { DEBUG_PRINT("false\n"); }
  //DEBUG_PRINT("stripGetColor: done\n");
  return state;
}
