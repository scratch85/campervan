#include <AceButton.h>
#include "Led.h"
#include <WS2812FX.h>
#include "Color_Definitions.h"
#include <TMP36.h>
using namespace ace_button;

#define SIZE(x) sizeof(x) / sizeof(x[0]);

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

// LED Strips 1, 2 (Driver, Co-Driver)
#define LEDSTRIP1_PIN 4
#define LEDSTRIP2_PIN 3

// How many NeoPixels do the srip have?
#define LEDSTRIP1_PIXELS 10
#define LEDSTRIP2_PIXELS 10

// Brightness & Speed
#define LEDSTRIP_BRIGHTNESS 32
#define LEDSTRIP_SPEED 2000

WS2812FX strip1 = WS2812FX(LEDSTRIP1_PIXELS, LEDSTRIP1_PIN, NEO_GRBW + NEO_KHZ800);
WS2812FX strip2 = WS2812FX(LEDSTRIP2_PIXELS, LEDSTRIP2_PIN, NEO_GRBW + NEO_KHZ800);

struct Effect {
  uint8_t fx;
  uint32_t c;
  uint8_t b;
  uint16_t s;
};

Effect effects1[] = { 
  {FX_MODE_RAINBOW, 0, LEDSTRIP_BRIGHTNESS, 4000}, 
  {FX_MODE_RAINBOW_CYCLE, 0, LEDSTRIP_BRIGHTNESS, 4000}, 
  {FX_MODE_LARSON_SCANNER, GREEN, LEDSTRIP_BRIGHTNESS, LEDSTRIP_SPEED}
};
Effect effects2[] = { 
  {FX_MODE_RAINBOW, 0, LEDSTRIP_BRIGHTNESS, 4000}, 
  {FX_MODE_RAINBOW_CYCLE, 0, LEDSTRIP_BRIGHTNESS, 4000}, 
  {FX_MODE_LARSON_SCANNER, RED, LEDSTRIP_BRIGHTNESS, LEDSTRIP_SPEED}
};

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
  Serial.println(SERIAL_BAUDRATE);
#ifdef DEBUG_BLE
  Serial.print("Serial2 (BLE) started at ");
  Serial.println(SERIAL_BAUDRATE);
#endif
  Serial.print("Sketch:   ");
  Serial.println(__FILE__);
  Serial.print("Uploaded: ");
  Serial.println(__DATE__);
#endif

  // Initialize serial2 - Bluetooth Low Energy (BLE)
#ifdef DEBUG_BLE
  Serial2.begin(SERIAL_BAUDRATE);
  Serial2.setTimeout(SERIAL2_TIMEOUT);
#ifdef DEBUG
  Serial2.print("Serial (console) started at ");
  Serial2.println(SERIAL_BAUDRATE);
#endif
  Serial2.print("Serial2 (BLE) started at ");
  Serial2.println(SERIAL_BAUDRATE);  
  Serial2.print("Sketch:   ");
  Serial2.println(__FILE__);
  Serial2.print("Uploaded: ");
  Serial2.println(__DATE__);
#endif

  // Initialize buttons
  ButtonConfig* systemButtonConfig = ButtonConfig::getSystemButtonConfig();
  systemButtonConfig->setEventHandler(handleSystemEvent);
  systemButtonConfig->setFeature(ButtonConfig::kFeatureClick);
  systemButtonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  systemButtonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  systemButtonConfig->setFeature(ButtonConfig::kFeatureSuppressAll);

  buttonConfigBed.setEventHandler(handleBedEvent);
  buttonConfigBed.setFeature(ButtonConfig::kFeatureClick);
  buttonConfigBed.setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfigBed.setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfigBed.setFeature(ButtonConfig::kFeatureSuppressAll);
  
  buttonConfigStrip.setEventHandler(handleStripEvent);
  buttonConfigStrip.setFeature(ButtonConfig::kFeatureClick);
  buttonConfigStrip.setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfigStrip.setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfigStrip.setFeature(ButtonConfig::kFeatureSuppressAll);

  pinMode(BUTTON_SD1_PIN, INPUT_PULLUP);
  btnSD1.init(BUTTON_SD1_PIN, LOW, 0);
  pinMode(BUTTON_SD2_PIN, INPUT_PULLUP);
  btnSD2.init(BUTTON_SD2_PIN, LOW, 1);
  
  pinMode(BUTTON_DS1_PIN, INPUT_PULLUP);
  btnDS1.init(BUTTON_DS1_PIN, LOW, 0);
  pinMode(BUTTON_CS1_PIN, INPUT_PULLUP);
  btnCS1.init(BUTTON_CS1_PIN, LOW, 1);

  pinMode(BUTTON_DS2_PIN, INPUT_PULLUP);
  btnDS2.init(BUTTON_DS2_PIN, LOW, 0);
  pinMode(BUTTON_CS2_PIN, INPUT_PULLUP);
  btnCS2.init(BUTTON_CS2_PIN, LOW, 1);

   // Initialize LEDs
  led1.attach(LED1_PIN);
  led2.attach(LED2_PIN);
  led3.attach(LED3_PIN);

  // Initialize LED strips
  strip1.init();
  strip1.setBrightness(LEDSTRIP_BRIGHTNESS);
  strip1.setSpeed(LEDSTRIP_SPEED);
  strip1.stop();

  strip2.init();
  strip2.setBrightness(LEDSTRIP_BRIGHTNESS);
  strip2.setSpeed(LEDSTRIP_SPEED);
  strip2.stop();
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

  // The strips
  strip1.service();
  strip2.service();
}

void processSerialInput(String data) {
  // process everything in lowercase
  data.toLowerCase();
  DEBUG_PRINT("processSerialInput: ");
  DEBUG_PRINT(data);
  DEBUG_PRINT("\n");

  if(data.equals("help")) {
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
  v1.toLowerCase();
  v2.toLowerCase();
  
  DEBUG_PRINT("processStatus(");
  DEBUG_PRINT(v1);
  DEBUG_PRINT(", ");
  DEBUG_PRINT(v2);
  DEBUG_PRINT(")\n");

  if(v1.equals("led")) {
    if(v2.equals("1") || v2.equals("2") || v2.equals("3")) {
      DEBUG_PRINT("led");
      DEBUG_PRINT(v2);
      DEBUG_PRINT(": ");
      if(v2.equals("1")) {
        DEBUG_PRINT(led1.getBrightness());
      } else if(v2.equals("2")) {
        DEBUG_PRINT(led2.getBrightness());
      } else if(v2.equals("3")) {
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
    if(v2.equals("1") || v2.equals("2")) {
      DEBUG_PRINT("strip");
      DEBUG_PRINT(v2);
      DEBUG_PRINT(": ");
      if(v2.equals("1")) {
        DEBUG_PRINT(strip1.isRunning());
      } else if(v2.equals("2")) {
        DEBUG_PRINT(strip2.isRunning());
      } else {
        DEBUG_PRINT(0);
      }
    } else {
      DEBUG_PRINT("strip1: ");
      DEBUG_PRINT(strip1.isRunning());
      DEBUG_PRINT("\n");
      DEBUG_PRINT("strip2: ");
      DEBUG_PRINT(strip2.isRunning());
    }
    DEBUG_PRINT("\n");
  } else if(v1.equals("tmp")) {
    DEBUG_PRINT("temperature: ");
    if(v2.equals("F")) {
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
    DEBUG_PRINT(strip1.isRunning());
    DEBUG_PRINT("\n");
    DEBUG_PRINT("strip2: ");
    DEBUG_PRINT(strip2.isRunning());
    DEBUG_PRINT("\n");
    DEBUG_PRINT("temperature: ");
    DEBUG_PRINT(String(tmp36.getTempC()));
    DEBUG_PRINT("\n");
  }
}

void processLed(String v1, String v2) {
  v1.toLowerCase();
  v2.toLowerCase();
  
  DEBUG_PRINT("processLed(");
  DEBUG_PRINT(v1);
  DEBUG_PRINT(", ");
  DEBUG_PRINT(v2);
  DEBUG_PRINT(")\n");

  uint32_t i1;
  if(v2.equals("on")) {
    i1 = 255;
  } else if(v2.equals("off")) {
    i1 = 0;
  } else if(v2.endsWith("%")) {
    i1 = map(sanitizeValue(v2.remove(v2.length() - 1), 0, 0, 100), 0, 100, 0, 255);
  } else {
    i1 = sanitizeValue(v2, 255, 0, 255);
  }
  if(v1.equals("1") || v1.equals("2") || v1.equals("3")) {
    if(v1.equals("1")) {
      led1.on(i1);
    } else if(v1.equals("2")) {
      led2.on(i1);
    } else if(v1.equals("3")) {
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
  v1.toLowerCase();
  v2.toLowerCase();
  v3.toLowerCase();
  v4.toLowerCase();
  
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
  if(v2.equals("on")) {
    i1 = FX_MODE_STATIC;
    i2 = WHITE_LED;
  } else if(v2.equals("off")) {
    i1 = FX_MODE_STATIC;
    i2 = BLACK;
  } else if(v2.equals("effect") || v2.equals("e")) {
    if(v3.equals("rainbow") || v3.equals("rb")) {
      i1 = FX_MODE_RAINBOW;
      i2 = WHITE_LED;
    } else if(v3.equals("rainbowcycle") || v3.equals("rbc")) {
      i1 = FX_MODE_RAINBOW_CYCLE;
      i2 = WHITE_LED;
    } else if(v3.equals("knightrider") || v3.equals("kr") || v3.equals("larsonscanner") || v3.equals("ls")) {
      i1 = FX_MODE_LARSON_SCANNER;
      i2 = getColorFromString(v4, RED);
    } else {
      i1 = sanitizeValue(v3, 0, 0, strip1.getModeCount());
      i2 = getColorFromString(v4, RED);
    }
  } else {
    i1 = FX_MODE_STATIC;
    i2 = getColorFromString(v2, 0);
  }

  if(v1.equals("1") || v1.equals("2")) {
    if(v1.equals("1")) {
      if(i2 == BLACK) {
        strip1.stop();
      } else {
        strip1.setColor(i2);
        strip1.setMode(i1);
        strip1.start();
      }
    } else if(v1.equals("2")) {
      if(i2 == BLACK) {
        strip2.stop();
      } else {
        strip2.setColor(i2);
        strip2.setMode(i1);
        strip2.start();
      }
    }
  } else {
    if(i2 == BLACK) {
      strip1.stop();
      strip2.stop();
    } else {
      strip1.setColor(i2);
      strip2.setColor(i2);
      strip1.setMode(i1);
      strip2.setMode(i1);
      strip1.start();
      strip2.start();
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

uint8_t getIndex(Effect e[], uint8_t s, uint8_t value) {
  for(uint8_t i = 0; i < s; i++) {
    if(e[i].fx == value) return i + 1;
  }
  return 0;
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

uint32_t getColorFromString(String c, uint32_t def) {
  c.toLowerCase();
  if(c.equals("black") || c.equals("bl")) return BLACK;
  if(c.equals("blue")  || c.equals("b"))  return BLUE;
  if(c.equals("green") || c.equals("g"))  return GREEN;
  if(c.equals("red")   || c.equals("r"))  return RED;
  if(c.equals("white") || c.equals("w"))  return WHITE_LED;
  return sanitizeValue(c, def, 0, 4294967295);
}

void handleSystemEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  /*
  DEBUG_PRINT(F("handleSystemEvent(): eventType: "));
  DEBUG_PRINT(eventType);
  DEBUG_PRINT("\n");
  DEBUG_PRINT(F("; buttonState: "));
  DEBUG_PRINT(buttonState);
  DEBUG_PRINT("\n");
  */

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
  /*
  DEBUG_PRINT(F("handleBedEvent( buttonId: "));
  DEBUG_PRINT(button->getId());
  DEBUG_PRINT(F(", eventType: "));
  DEBUG_PRINT(eventType);
  DEBUG_PRINT(F(", buttonState: "));
  DEBUG_PRINT(buttonState);
  DEBUG_PRINT(F("\n"));
  */

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
  /*
  DEBUG_PRINT(F("handleStripEvent( buttonId: "));
  DEBUG_PRINT(button->getId());
  DEBUG_PRINT(F(", eventType: "));
  DEBUG_PRINT(eventType);
  DEBUG_PRINT(F(", buttonState: "));
  DEBUG_PRINT(buttonState);
  DEBUG_PRINT(F("\n"));
  */

  uint8_t e, s;

  switch (eventType) {
    case AceButton::kEventClicked:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventClicked event detected\n");
      switch (button->getId()) {
        case 0:
          if(strip1.isRunning()) {
            strip1.stop();
          } else {
            strip1.setColor(WHITE_LED);
            strip1.setMode(FX_MODE_STATIC);
            strip1.start();
          }
          break;
        case 1:
          if(strip2.isRunning()) {
            strip2.stop();
          } else {
            strip2.setColor(WHITE_LED);
            strip2.setMode(FX_MODE_STATIC);
            strip2.start();
          }
          break;
      }
      break;
    case AceButton::kEventDoubleClicked:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventDoubleClicked event detected\n");
      switch (button->getId()) {
        case 0:
          s = SIZE(effects1);
          e = getIndex(effects1, s, strip1.getMode());
          if(e >= s) {
            strip1.setMode(0);
            strip1.stop();
          } else {
            strip1.setMode(effects1[e].fx);
            strip1.setColor(effects1[e].c);
            strip1.setBrightness(effects1[e].b);
            strip1.setSpeed(effects1[e].s);
            strip1.start();
          }
          break;
        case 1:
          s = SIZE(effects2);
          e = getIndex(effects2, s, strip2.getMode());
          if(e >= s) {
            strip2.setMode(0);
            strip2.stop();
          } else {
            strip2.setMode(effects1[e].fx);
            strip2.setColor(effects1[e].c);
            strip2.setBrightness(effects1[e].b);
            strip2.setSpeed(effects1[e].s);
            strip2.start();
          }
          break;
      }
      break;
    case AceButton::kEventLongPressed:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINT(": kEventLongPressed event detected\n");
      switch (button->getId()) {
        case 0:
          // Sync 1 => 2
          strip2.setMode(strip1.getMode());
          strip2.setColor(strip1.getColor());
          strip2.setBrightness(strip1.getBrightness());
          strip2.setSpeed(strip1.getSpeed());
          strip1.resetSegmentRuntimes();
          strip2.resetSegmentRuntimes();
          strip1.start();
          strip2.start();
          break;
        case 1:
          // Sync 2 => 1
          strip1.setMode(strip2.getMode());
          strip1.setColor(strip2.getColor());
          strip1.setBrightness(strip2.getBrightness());
          strip1.setSpeed(strip2.getSpeed());
          strip1.resetSegmentRuntimes();
          strip2.resetSegmentRuntimes();
          strip1.start();
          strip2.start();
          break;
      }
      break;
  }
}
