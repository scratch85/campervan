#include <AceButton.h>
#include "Led.h"
#include "WS2812FX.h"
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
  #define DEBUG_PRINTF(x, y) Serial.print(x, y); Serial2.print(x, y);
  #define DEBUG_PRINTLN(x) Serial.println(x); Serial2.println(x);
#elif defined DEBUG
  #define DEBUG_PRINT(x) Serial.print(x);
  #define DEBUG_PRINTF(x, y) Serial.print(x, y);
  #define DEBUG_PRINTLN(x) Serial.println(x);
#elif defined ENABLE_BLE && defined DEBUG_BLE
  #define DEBUG_PRINT(x) Serial2.print(x);
  #define DEBUG_PRINTF(x, y) Serial2.print(x, y);
  #define DEBUG_PRINTLN(x) Serial2.println(x);
#else
  #define DEBUG_PRINT(x);
  #define DEBUG_PRINTF(x);
  #define DEBUG_PRINTLN(x);
#endif

// Serial & Seriel2 (Bluetooth Low Energy (BLE))
#define SERIAL_BAUDRATE 9600
#define SERIAL1_TIMEOUT 100
#define SERIAL2_TIMEOUT 100

char serial[20];
uint8_t si = 0;
char bluetooth[20];
uint8_t bi = 0;

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
#define NUM_LEDS 3
#define LED1_PIN 21
#define LED2_PIN 22
#define LED3_PIN 23
Led led1, led2, led3;

// LED Strips 1, 2 (Driver, Co-Driver)
#define NUM_STRIPS 2
#define LEDSTRIP1_PIN 4
#define LEDSTRIP1_PIXELS 60
#define LEDSTRIP2_PIN 3
#define LEDSTRIP2_PIXELS 60

#define LEDSTRIP_BRIGHTNESS 32
#define LEDSTRIP_SPEED 4000

WS2812FX strip1 = WS2812FX(LEDSTRIP1_PIXELS, LEDSTRIP1_PIN, NEO_GRBW + NEO_KHZ800);
WS2812FX strip2 = WS2812FX(LEDSTRIP2_PIXELS, LEDSTRIP2_PIN, NEO_GRBW + NEO_KHZ800);

struct Effect {
  uint8_t fx;
  uint32_t c[NUM_COLORS];
  uint8_t b;
  uint16_t s;
};

Effect effects1[] = { 
  {FX_MODE_RAINBOW, {0, 0, 0}, LEDSTRIP_BRIGHTNESS, 8000}, 
  {FX_MODE_RAINBOW_CYCLE, {0, 0, 0}, LEDSTRIP_BRIGHTNESS, 8000}, 
  {FX_MODE_LARSON_SCANNER, {GREEN, GREEN, GREEN}, LEDSTRIP_BRIGHTNESS, LEDSTRIP_SPEED}
};

Effect effects2[] = { 
  {FX_MODE_RAINBOW, {0, 0, 0}, LEDSTRIP_BRIGHTNESS, 8000}, 
  {FX_MODE_RAINBOW_CYCLE, {0, 0, 0}, LEDSTRIP_BRIGHTNESS, 8000}, 
  {FX_MODE_TWINKLE_FADE, {WHITE, WHITE, WHITE}, LEDSTRIP_BRIGHTNESS, LEDSTRIP_SPEED}, 
  {FX_MODE_TWINKLE_FADE_RANDOM, {0, 0, 0}, LEDSTRIP_BRIGHTNESS, LEDSTRIP_SPEED}, 
  {FX_MODE_LARSON_SCANNER, {RED, RED, RED}, LEDSTRIP_BRIGHTNESS, LEDSTRIP_SPEED},
  {FX_MODE_COLOR_WIPE_RANDOM, {0, 0, 0}, LEDSTRIP_BRIGHTNESS, LEDSTRIP_SPEED}, 
  {FX_MODE_COLOR_SWEEP_RANDOM, {0, 0, 0}, LEDSTRIP_BRIGHTNESS, LEDSTRIP_SPEED}
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
  Serial.print(F("Serial (console) started at "));
  Serial.println(SERIAL_BAUDRATE);
#ifdef DEBUG_BLE
  Serial.print(F("Serial2 (ble) started at "));
  Serial.println(SERIAL_BAUDRATE);
#endif
  Serial.print(F("Sketch: "));
  Serial.println(__FILE__);
  Serial.print(F("Uploaded: "));
  Serial.println(__DATE__);
#endif

  // Initialize serial2 - Bluetooth Low Energy (BLE)
#ifdef DEBUG_BLE
  Serial2.begin(SERIAL_BAUDRATE);
  Serial2.setTimeout(SERIAL2_TIMEOUT);
#ifdef DEBUG
  Serial2.print(F("Serial (console) started at "));
  Serial2.println(SERIAL_BAUDRATE);
#endif
  Serial2.print(F("Serial2 (ble) started at "));
  Serial2.println(SERIAL_BAUDRATE);  
  Serial2.print(F("Sketch: "));
  Serial2.println(__FILE__);
  Serial2.print(F("Uploaded: "));
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
  while(Serial2.available() > 0) {
    in = Serial2.read();
    if(bi > 0 && bluetooth[bi - 1] == char(13) && in == char(10)) {
      // ... and process inputs
      DEBUG_PRINT(F("Bluetooth: "));
      DEBUG_PRINTLN(bluetooth);
      processSerialInput(bluetooth, sizeof(bluetooth));
      memset(bluetooth, '\0', sizeof(bluetooth));
      bi = 0;
    } else {
      bluetooth[bi++] = in;
    }
  }
#endif

  // Read from Serial ...
#ifdef DEBUG
  while(Serial.available() > 0) {
    in = Serial.read();
    if(si > 0 && serial[si - 1] == char(13) && in == char(10)) {
      // ... and process inputs
      DEBUG_PRINT(F("Serial: "));
      DEBUG_PRINTLN(serial);
      processSerialInput(serial, sizeof(serial));
      memset(serial, '\0', sizeof(serial));
      si = 0;
    } else {
      serial[si++] = in;
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

void processSerialInput(char str[], uint8_t s) {
  uint8_t r = 0;

  if(strncmp(str, "h", 1) == 0) {
    processHelp();
  //} else if(data.startsWith("status")) {
    //processStatus(split(data,' ',1), split(data,' ',2));
  }
  else if(strncmp(str, "l", 1) == 0) {
    r = processLed(str, s);
    if(r != 0) {
      DEBUG_PRINT(F("processLed failed at "));
      DEBUG_PRINTLN(r);
      DEBUG_PRINTLN(str);
    }
  } else if(strncmp(str, "s", 1) == 0) {
    r = processStrip(str, s);
    if(r != 0) {
      DEBUG_PRINT(F("processStrip failed at "));
      DEBUG_PRINTLN(r);
      DEBUG_PRINTLN(str);
    }
  }
}

void processHelp() {
  uint8_t cs = SIZE(c);
  DEBUG_PRINTLN(F("Leds"));
  DEBUG_PRINTLN(F("l<0-3>   - number indicates which led to control, 0 means all"));
  DEBUG_PRINTLN(F("b<0-255> - brigthness control, from darkest (0) to brightest (255), default is 255 if not specified"));
  DEBUG_PRINTLN();
  DEBUG_PRINTLN(F("Strips"));
  DEBUG_PRINTLN(F("s<0-2>   - number indicates which strip to control, 0 means all"));
  DEBUG_PRINT(F("b<0-255> - brigthness control, from darkest (0) to brightest (255), default is "));
  DEBUG_PRINT(LEDSTRIP_BRIGHTNESS);
  DEBUG_PRINTLN(F(" if not specified"));
  DEBUG_PRINT(F("p<1-255> - speed control, from slowest (1) to fastest (255), default is "));
  DEBUG_PRINT(LEDSTRIP_SPEED);
  DEBUG_PRINTLN(F(" if not specified"));
  DEBUG_PRINT(F("e<0-"));
  DEBUG_PRINT(MODE_COUNT);
  DEBUG_PRINTLN(F(">  - effect control, default is 0"));
  DEBUG_PRINT(F("c<0-"));
  DEBUG_PRINT(cs);
  DEBUG_PRINTLN(F(">  - color control, specify up to 3 colors"));
  DEBUG_PRINTLN(F("y        - sync settings to other strip"));
  DEBUG_PRINTLN();
  DEBUG_PRINTLN(F("Colors"));
  for(uint8_t i = 1; i < cs; i++) {
    DEBUG_PRINT(c[i - 1].n);
    DEBUG_PRINT(F(" ("));
    DEBUG_PRINT(i - 1);
    DEBUG_PRINT(F(")"));
    if(i % 3 == 0) {
      DEBUG_PRINTLN(F(","));
    } else {
      DEBUG_PRINT(F("\t"));
    }
  }
  DEBUG_PRINTLN();
}

/*
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
*/

uint8_t processLed(char str[], uint8_t s) {
  uint8_t i = 0;
  uint8_t v;
  bool a[NUM_LEDS] = {false, false, false};
  uint8_t b = 255;
  while(i < s) {
    switch(str[i]) {
      case 'l': // leds
        if(i + 1 >= s) return i;
        v = str[i + 1];
        if(v == 0 || v == '0') {
          a[0] = true;
          a[1] = true;
          a[2] = true;
        } else if(v == 1 || v == '1') {
          a[0] = true;
        } else if(v == 2 || v == '2') {
          a[1] = true;
        } else if(v == 3 || v == '3') {
          a[2] = true;
        } else {
          return i;
        }
        i = i + 2;
      break;
      case 'b': // brigthness
        if(i + 1 >= s) return i;
        b = str[i + 1];
        i = i + 2;
      break;
      default:
        i++;
      break;
    }
  }
  if(a[0]) led1.on(b);
  if(a[1]) led2.on(b);
  if(a[2]) led3.on(b);
  return 0;
}

uint8_t processStrip(char str[], uint8_t s) {
  uint8_t i = 0;
  uint8_t v;
  bool a[NUM_STRIPS] = {false, false};
  bool y = false; // sync
  uint8_t e = 0;  // effect
  uint8_t b = LEDSTRIP_BRIGHTNESS; // brightness
  uint8_t p = 0;  // speed
  uint32_t ca[NUM_COLORS] = {0, 0, 0};
  uint8_t ci = 0; // color index
  uint8_t cs = SIZE(c);
  while(i < s) {
    switch(str[i]) {
      case 's': // strips
        if(i + 1 >= s) return i;
        v = str[i + 1];
        if(v == 0 || v == '0') {
          a[0] = true;
          a[1] = true;
        } else if(v == 1 || v == '1') {
          a[0] = true;
        } else if(v == 2 || v == '2') {
          a[1] = true;
        } else {
          return i;
        }
        i = i + 2;
      break;
      case 'b': // brigthness
        if(i + 1 >= s) return i;
        b = str[i + 1];
        i = i + 2;
      break;
      case 'c': // color
        if(ci >= NUM_COLORS) return i;
        //if(i + 4 >= s) {
          if(i + 1 >= s) {
            return i;
          } else {
            ca[ci++] = c[(uint8_t) str[i + 1] % cs].col;
            i = i + 1;
          }
        //} else {
        //  ca[ci++] = (str[i + 1] << 24) | (str[i + 2] << 16) | (str[i + 3] << 8) | str[i + 4];
        //  i = i + 5;
        //}
      break;
      case 'e': // effect
        if(i + 1 >= s) return i;
        e = str[i + 1];
        if(e >= MODE_COUNT) return i;
        i = i + 2;
      break;
      case 'p': // speed
        if(i + 1 >= s) return i;
        p = str[i + 1];
        i = i + 2;
      break;
      case 'y': // sync
        y = true;
        i++;
      break;
      default:
        i++;
      break;
    }
  }
  if(a[0]) {
    strip1.setBrightness(b);
    strip1.setColors(0, ca);
    if(p > 0) strip1.setSpeed(map(p, 0, 255, SPEED_MAX, SPEED_MIN));
    strip1.setMode(e);
    strip1.start();
    if(y && !a[1]) syncStrips(&strip1, &strip2);
  }
  if(a[1]) {
    strip2.setBrightness(b);
    strip2.setColors(0, ca);
    if(p > 0) strip2.setSpeed(map(p, 0, 255, SPEED_MAX, SPEED_MIN));
    DEBUG_PRINTLN(e);
    strip2.setMode(e);
    strip2.start();
    if(y && !a[0]) syncStrips(&strip2, &strip1);
  }
  return 0;
}

uint8_t getNextEffect(Effect e[], uint8_t s, uint8_t val) {
  for(uint8_t i = 0; i < s; i++) {
    if(e[i].fx == val) return i + 1;
  }
  return 0;
}

void syncStrips(WS2812FX *s1, WS2812FX *s2) {
  s2->setMode(s1->getMode());
  s2->setBrightness(s1->getBrightness());
  s2->setColors(0, s1->getColors(0));
  s2->setSpeed(s1->getSpeed());
  s2->setSegmentRuntime(s1->getSegmentRuntime());
  s2->resume();
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
      DEBUG_PRINTLN(F(": kEventClicked event detected"));
      led1.toggle();
      break;
    case AceButton::kEventDoubleClicked:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINTLN(F(": kEventDoubleClicked event detected"));
      if(led1.isOn()) {
        led1.dimDown(round(255 * 0.2)); // dim down 20%
      } else {
       led1.on(round(255 * 0.8)); // switch on @ 80%
      }
      break;
    case AceButton::kEventLongPressed:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINTLN(F(": kEventLongPressed event detected"));
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
      DEBUG_PRINTLN(F(": kEventClicked event detected"));
      led3.toggle();
      break;
    case AceButton::kEventDoubleClicked:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINTLN(F(": kEventDoubleClicked event detected"));
      if(led3.isOn()) {
        led3.dimDown(round(255 * 0.2)); // dim down 20%
      } else {
        led3.on(round(255 * 0.8)); // switch on @ 80%
      }
      break;
    case AceButton::kEventLongPressed:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINTLN(F(": kEventLongPressed event detected"));
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
      DEBUG_PRINTLN(F(": kEventClicked event detected"));
      switch (button->getId()) {
        case 0:
          if(strip1.isRunning()) {
            strip1.stop();
          } else {
            strip1.setColor(WHITE);
            strip1.setMode(FX_MODE_STATIC);
            strip1.start();
          }
          break;
        case 1:
          if(strip2.isRunning()) {
            strip2.stop();
          } else {
            strip2.setColor(WHITE);
            strip2.setMode(FX_MODE_STATIC);
            strip2.start();
          }
          break;
      }
      break;
    case AceButton::kEventDoubleClicked:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINTLN(F(": kEventDoubleClicked event detected"));
      switch (button->getId()) {
        case 0:
          s = SIZE(effects1);
          e = getNextEffect(effects1, s, strip1.getMode());
          if(e >= s) {
            strip1.setMode(0);
            strip1.stop();
          } else {
            strip1.setMode(effects1[e].fx);
            strip1.setColors(0, effects1[e].c);
            strip1.setBrightness(effects1[e].b);
            strip1.setSpeed(effects1[e].s);
            strip1.start();
          }
          break;
        case 1:
          s = SIZE(effects2);
          e = getNextEffect(effects2, s, strip2.getMode());
          if(e >= s) {
            strip2.setMode(0);
            strip2.stop();
          } else {
            strip2.setMode(effects2[e].fx);
            strip2.setColors(0, effects2[e].c);
            strip2.setBrightness(effects2[e].b);
            strip2.setSpeed(effects2[e].s);
            strip2.start();
          }
          break;
      }
      break;
    case AceButton::kEventLongPressed:
      DEBUG_PRINT(button->getId());
      DEBUG_PRINTLN(F(": kEventLongPressed event detected"));
      switch (button->getId()) {
        case 0:
          // Sync 1 => 2
          syncStrips(&strip1, &strip2);
          break;
        case 1:
          // Sync 2 => 1
          syncStrips(&strip2, &strip1);
          break;
      }
      break;
  }
}
