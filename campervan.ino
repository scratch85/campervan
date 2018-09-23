#include <ButtonEvents.h>
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

// button
// tap => on or off
// double tap => if off => on 80%
//            => if on => dimm down 20% (until off)
// hold => if off dimm up until 100%
//         if on dimm down until off

// Sliding Door 1 (top)
#define BUTTON_SD1_PIN 1
ButtonEvents btnSD1;
bool btnSD1Hold = false;
int btnSD1HoldMode = -1;
unsigned long btnSD1Time = 0;
unsigned long btnSD1Interval = 600;

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
int led1Val = 0;
int led1Val2 = led1Val;

#define LED2_PIN 22
int led2Val = 0;
int led2Val2 = led2Val;

#define LED3_PIN 23
int led3Val = 0;
int led3Val2 = led3Val;

// LED Stripes 1, 2 (Driver, Co-Driver)
#define LEDSTRIPE1_PIN 4
#define LEDSTRIPE2_PIN 3

// How many NeoPixels do the sripe have?
#define LEDSTRIPE1_PIXELS 60
#define LEDSTRIPE2_PIXELS 60

// Maximum brightness
#define LEDSTRIPE_BRIGHTNESS 32

// Parameter 1 = number of pixels in stripe
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_GRBW    Pixels are wired for GRBW bitstream (f.e. SK6812)

#define LEDSTRIPES 2
Adafruit_NeoPixel ledstripes_obj[LEDSTRIPES];

String ledstripes1_name = "Stripe1 (Driver)";
unsigned int ledstripes1_effect = 0;
unsigned long ledstripes1_lastUpdate = 0;
unsigned long ledstripes1_updateInterval = 50;
    
String ledstripes2_name = "Stripe2 (Co-Driver)";
unsigned long ledstripes2_effect = 0;
unsigned long ledstripes2_lastUpdate = 0;
unsigned long ledstripes2_updateInterval = 50;

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel. Avoid connecting
// on a live circuit...if you must, connect GND first.

// NOTE: RGBW LEDs draw up to 80mA with all colors + white at full brightness!
// That means that a 60-pixel strip can draw up to 60x 80 = 4800mA, so you
// should use a 5A power supply; for a 144-pixel strip its max 11520mA = 12A!

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

  // Initialize LED Spots
  pinMode(LED1_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED2_PIN, LOW);
  
  pinMode(LED3_PIN, OUTPUT);
  digitalWrite(LED3_PIN, LOW);

  // Initialize LED Stripes
  ledstripes_obj[0] = Adafruit_NeoPixel(LEDSTRIPE1_PIXELS, LEDSTRIPE1_PIN, NEO_GRBW + NEO_KHZ800);
  ledstripes_obj[1] = Adafruit_NeoPixel(LEDSTRIPE2_PIXELS, LEDSTRIPE2_PIN, NEO_GRBW + NEO_KHZ800);
  DEBUG_PRINT("Ledstripes:\n");
  for(unsigned int i = 0; i < LEDSTRIPES; i++) {
    DEBUG_PRINT(i);
    DEBUG_PRINT(" on Pin: ");
    DEBUG_PRINT(ledstripes_obj[i].getPin());
    DEBUG_PRINT("\n");
    ledstripes_obj[i].begin();
    ledstripes_obj[i].setBrightness(LEDSTRIPE_BRIGHTNESS);
    ledstripes_obj[i].show(); // Initialize all pixels to 'off'
  }
}

void loop() {
  // Read from Seriel2 (Bluetooth Low Energy (BLE) module) ...
  String strBluetooth = "";
  if(Serial2.available() > 0) {
    strBluetooth = Serial2.readString();
  }
  // ... and process inputs
  strBluetooth = strBluetooth.trim();
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
  strSerial = strSerial.trim();
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
        if(led1Val > 0) {
          DEBUG_PRINT("btnSD1: tap event detected: ON ");
          DEBUG_PRINT(round(led1Val * 100 / 254));
          DEBUG_PRINT("% (");
          DEBUG_PRINT(led1Val);
          DEBUG_PRINT(") => OFF\n");
          led1Val = 0;
        } else {
          led1Val = 254;
          DEBUG_PRINT("btnSD1: tap event detected: OFF => ON 100% (");
          DEBUG_PRINT(led1Val);
          DEBUG_PRINT(")\n");
        }
        break;
      case (doubleTap):
        if(led1Val > 0) {
          DEBUG_PRINT("btnSD1: doubleTap event detected: ON ");
          DEBUG_PRINT(round(led1Val * 100 / 254));
          DEBUG_PRINT("% (");
          DEBUG_PRINT(led1Val);
          DEBUG_PRINT(") => ON ");
          led1Val = max(0,round(led1Val - (254 * 0.2)));
          DEBUG_PRINT(round(led1Val * 100 / 254));
          DEBUG_PRINT("% (");
          DEBUG_PRINT(led1Val);
          DEBUG_PRINT(")\n");
        } else {
          led1Val = round(254 - (254 * 0.2));
          DEBUG_PRINT("btnSD1: doubleTap event detected: OFF => ON 80% (");
          DEBUG_PRINT(led1Val);
          DEBUG_PRINT(")\n");
        }
        break;
      case (hold):
        btnSD1Hold = true;
        if(led1Val > 0) {
          btnSD1HoldMode = 0;
        } else {
          btnSD1HoldMode = 1;
        }
        btnSD1Time = millis();
        break;
      case (none):
        break;
    }
  }
  if(btnSD1Hold && btnSD1.read() == HIGH) {
    if(millis() - btnSD1Time > btnSD1Interval) {
      btnSD1Time = millis();

      DEBUG_PRINT("btnSD1: hold event detected: ");
      if(led1Val > 0) {
        DEBUG_PRINT("ON ");
        DEBUG_PRINT(round(led1Val * 100 / 254));
        DEBUG_PRINT("% (");
        DEBUG_PRINT(led1Val);
        DEBUG_PRINT(")");
      } else {
        DEBUG_PRINT("OFF");
      }
      if(btnSD1HoldMode == 0) {
        led1Val = round(led1Val - (254 * 0.1));
        if(led1Val <= 0) {
          led1Val = 0;
          btnSD1HoldMode = 1;
        }
      } else {
        led1Val = round(led1Val + (254 * 0.1));
        if(led1Val >= 254) {
          led1Val = 254;
          btnSD1HoldMode = 0;
        }
      }
      if(led1Val > 0) {
        DEBUG_PRINT(" => ON ");
        DEBUG_PRINT(round(led1Val * 100 / 254));
        DEBUG_PRINT("% (");
        DEBUG_PRINT(led1Val);
        DEBUG_PRINT(")\n");
      } else {
        DEBUG_PRINT(" => OFF\n");
      }
    }
  } else {
    btnSD1Hold = false;
  }
  // - - - - - END # Button Sliding Door 1 - - - - - - - - - -

  // - - - - - BEGIN # Button Driver Seat 1 - - - - - - - - - -
  if(btnDS1.update() == true) {
    switch(btnDS1.event()) {
      case (tap):
        if(led3Val > 0) {
          DEBUG_PRINT("btnDS1: tap event detected: ON ");
          DEBUG_PRINT(round(led3Val * 100 / 254));
          DEBUG_PRINT("% (");
          DEBUG_PRINT(led3Val);
          DEBUG_PRINT(") => OFF\n");
          led3Val = 0;
        } else {
          led3Val = 254;
          DEBUG_PRINT("btnDS1: tap event detected: OFF => ON 100% (");
          DEBUG_PRINT(led3Val);
          DEBUG_PRINT(")\n");
        }
        break;
      case (doubleTap):
        if(led3Val > 0) {
          DEBUG_PRINT("btnDS1: doubleTap event detected: ON ");
          DEBUG_PRINT(round(led3Val * 100 / 254));
          DEBUG_PRINT("% (");
          DEBUG_PRINT(led3Val);
          DEBUG_PRINT(") => ON ");
          led3Val = max(0,round(led3Val - (254 * 0.2)));
          DEBUG_PRINT(round(led3Val * 100 / 254));
          DEBUG_PRINT("% (");
          DEBUG_PRINT(led3Val);
          DEBUG_PRINT(")\n");
        } else {
          led3Val = round(254 - (254 * 0.2));
          DEBUG_PRINT("btnDS1: doubleTap event detected: OFF => ON 80% (");
          DEBUG_PRINT(led3Val);
          DEBUG_PRINT(")\n");
        }
        break;
      case (hold):
        DEBUG_PRINT("btnDS1: hold event detected: ALL OFF\n");
        led1Val = 0;
        led2Val = 0;
        led3Val = 0;
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
        if(ledstripes1_effect > 0 || !stripeGetState(ledstripes1_name, ledstripes_obj[0])) {
          stripeSetColor(ledstripes1_name, ledstripes_obj[0], WHITE_LED);
          ledstripes1_effect = 0;
        } else {
          stripeSetColor(ledstripes1_name, ledstripes_obj[0], BLACK);
          ledstripes1_effect = 0;
        }
        break;
      case (doubleTap):
        DEBUG_PRINT("btnDS2: doubleTap event detected\n");
        ledstripes1_effect++;
        if(ledstripes1_effect > 2) {
          stripeSetColor(ledstripes1_name, ledstripes_obj[0], BLACK);
          ledstripes1_effect = 0;
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
        if(led3Val > 0) {
          DEBUG_PRINT("btnCS1: tap event detected: ON ");
          DEBUG_PRINT(round(led3Val * 100 / 254));
          DEBUG_PRINT("% (");
          DEBUG_PRINT(led3Val);
          DEBUG_PRINT(") => OFF\n");
          led3Val = 0;
        } else {
          led3Val = 254;
          DEBUG_PRINT("btnCS1: tap event detected: OFF => ON 100% (");
          DEBUG_PRINT(led3Val);
          DEBUG_PRINT(")\n");
        }
        break;
      case (doubleTap):
        if(led3Val > 0) {
          DEBUG_PRINT("btnCS1: doubleTap event detected: ON ");
          DEBUG_PRINT(round(led3Val * 100 / 254));
          DEBUG_PRINT("% (");
          DEBUG_PRINT(led3Val);
          DEBUG_PRINT(") => ON ");
          led3Val = max(0,round(led3Val - (254 * 0.2)));
          DEBUG_PRINT(round(led3Val * 100 / 254));
          DEBUG_PRINT("% (");
          DEBUG_PRINT(led3Val);
          DEBUG_PRINT(")\n");
        } else {
          led3Val = round(254 - (254 * 0.2));
          DEBUG_PRINT("btnCS1: doubleTap event detected: OFF => ON 80% (");
          DEBUG_PRINT(led3Val);
          DEBUG_PRINT(")\n");
        }
        break;
      case (hold):
        DEBUG_PRINT("btnCS1: hold event detected: ALL OFF\n");
        led1Val = 0;
        led2Val = 0;
        led3Val = 0;
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
        /*if(ledstripes2_effect > 0 || !stripeGetState(ledstripes2_name, ledstripes_obj[1])) {
          stripeSetColor(ledstripes2_name, ledstripes_obj[1], WHITE_LED, 50);
          ledstripes2_effect = 0;
        } else {
          stripeSetColor(ledstripes2_name, ledstripes_obj[1], BLACK, 50);
          ledstripes2_effect = 0;
        }*/
        if(ledstripes1_effect > 0 || !stripeGetState(ledstripes1_name, ledstripes_obj[0])) {
          stripeSetColor(ledstripes1_name, ledstripes_obj[0], WHITE_LED);
          ledstripes1_effect = 0;
        } else {
          stripeSetColor(ledstripes1_name, ledstripes_obj[0], BLACK);
          ledstripes1_effect = 0;
        }
        break;
      case (doubleTap):
        DEBUG_PRINT("btnCS2: doubleTap event detected\n");
        /*ledstripes2_effect++;
        if(ledstripes2_effect > 2) {
          stripeSetColor(ledstripes2_name, ledstripes_obj[1], BLACK);
          ledstripes2_effect = 0;
        }*/
        ledstripes1_effect++;
        if(ledstripes1_effect > 2) {
          stripeSetColor(ledstripes1_name, ledstripes_obj[0], BLACK);
          ledstripes1_effect = 0;
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

  // Handle LED1
  if(led1Val != led1Val2) {
    if(led1Val == 0) {
      // Turn off the LED
      //digitalWrite(LED1_PIN, LOW);
      analogWrite(LED1_PIN, LOW);
    } else {
      // Turn on the LED and dimm
      analogWrite(LED1_PIN, led1Val);
    }
  }
  led1Val2 = led1Val;

  // Handle LED2
  if(led2Val != led2Val2) {
    if(led2Val == 0) {
      // Turn off the LED
      //digitalWrite(LED2_PIN, LOW);
      analogWrite(LED2_PIN, LOW);
    } else {
      // Turn on the LED and dimm
      analogWrite(LED2_PIN, led2Val);
    }
  }
  led2Val2 = led2Val;

  // Handle LED3
  if(led3Val != led3Val2) {
    if(led3Val == 0) {
      // Turn off the LED
      //digitalWrite(LED3_PIN, LOW);
      analogWrite(LED3_PIN, LOW);
    } else {
      // Turn on the LED and dimm
      analogWrite(LED3_PIN, led3Val);
    }
  }
  led3Val2 = led3Val;

  // Handle LED Stripes
  if(millis() - ledstripes1_lastUpdate >= ledstripes1_updateInterval) {
    switch(ledstripes1_effect) {
      case 0:
        // do nothing
        break;
      case 1:
        stripeRainbow(ledstripes1_name, ledstripes_obj[0]);
        break;
      case 2:
        stripeRainbowCycle(ledstripes1_name, ledstripes_obj[0]);
        break;
      case 3:
        stripeKnightRider(ledstripes1_name, ledstripes_obj[0], RED);
        break;
    }
    ledstripes1_lastUpdate = millis();
  }
  /*
  if(millis() - ledstripes2_lastUpdate >= ledstripes2_updateInterval) {
    switch(ledstripes2_effect) {
      case 0:
        // do nothing
        break;
      case 1:
        stripeRainbow(ledstripes2_name, ledstripes_obj[1]);
        break;
      case 2:
        stripeRainbowCycle(ledstripes2_name, ledstripes_obj[1]);
        break;
    }
    ledstripes2_lastUpdate = millis();
  }
  */
  /*
  for(unsigned int i = 0; i < LEDSTRIPES; i++) {
    // Check if an update is needed
    if(millis() - ledstripes_lastUpdate[i] >= ledstripes_updateInterval[i]) {
      switch(ledstripes_effect[i]) {
        case 0:
          // do nothing
          break;
        case 1:
          stripeRainbow(ledstripes_name[i], ledstripes_obj[i]);
          break;
        case 2:
          stripeRainbowCycle(ledstripes_name[i], ledstripes_obj[i]);
          break;
      }
      ledstripes_lastUpdate[i] = millis();
    }
  }
  */
  delay(10);
}

void processSerialInput(String data) {
  if(data.equalsIgnoreCase("status")) {
    DEBUG_PRINT("Status\n");
    DEBUG_PRINT("---------------\n");
    DEBUG_PRINT("Temp.: ");
    DEBUG_PRINT(String(tmp36.getTempC()));
    DEBUG_PRINT("°C\n");
    DEBUG_PRINT("---------------\n");
  } else if(data.equalsIgnoreCase("l1")) {
    led1Val = 128;
  } else if(data.equalsIgnoreCase("l1-")) {
    led1Val = max(0,led1Val - (254 * 0.1));
  } else if(data.equalsIgnoreCase("l2")) {
    led2Val = 128;
  } else if(data.equalsIgnoreCase("l2-")) {
    led2Val = max(0,led2Val - (254 * 0.1));
  } else if(data.equalsIgnoreCase("l3")) {
    led3Val = 128;
  } else if(data.equalsIgnoreCase("l3-")) {
    led3Val = max(0,led3Val - (254 * 0.1));
  } else if(data.equalsIgnoreCase("on")) {
    led1Val = 128;
    led2Val = 128;
    led3Val = 128;
  } else if(data.equalsIgnoreCase("off")) {
    led1Val = 0;
    led2Val = 0;
    led3Val = 0;
  } else if(data.equalsIgnoreCase("s1w")) {
    stripeSetColor(ledstripes1_name, ledstripes_obj[0], WHITE_LED);
    ledstripes1_effect = 0;
  } else if(data.equalsIgnoreCase("s1r")) {
    stripeSetColor(ledstripes1_name, ledstripes_obj[0], RED);
    ledstripes1_effect = 0;
  } else if(data.equalsIgnoreCase("s1g")) {
    stripeSetColor(ledstripes1_name, ledstripes_obj[0], GREEN);
    ledstripes1_effect = 0;
  } else if(data.equalsIgnoreCase("s1b")) {
    stripeSetColor(ledstripes1_name, ledstripes_obj[0], BLUE);
    ledstripes1_effect = 0;
  } else if(data.equalsIgnoreCase("s1off")) {
    stripeSetColor(ledstripes1_name, ledstripes_obj[0], BLACK);
    ledstripes1_effect = 0;
  } else if(data.equalsIgnoreCase("s1rb")) {
    ledstripes1_effect = 1;
  } else if(data.equalsIgnoreCase("s1rbc")) {
    ledstripes1_effect = 2;
  } else if(data.equalsIgnoreCase("s1kr")) {
    ledstripes1_effect = 3;
  } else if(data.equalsIgnoreCase("s1+")) {
    ledstripes1_updateInterval = max(10,ledstripes1_updateInterval - 10);
    DEBUG_PRINT("ledstripes1_updateInterval: ");
    DEBUG_PRINT(ledstripes1_updateInterval);
    DEBUG_PRINT("\n");
  } else if(data.equalsIgnoreCase("s1-")) {
    ledstripes1_updateInterval = min(1000,ledstripes1_updateInterval + 10);
    DEBUG_PRINT("ledstripes1_updateInterval: ");
    DEBUG_PRINT(ledstripes1_updateInterval);
    DEBUG_PRINT("\n");
  } else if(data.equalsIgnoreCase("s1status")) {
    stripeGetState(ledstripes1_name, ledstripes_obj[0]);
  }
}

// Set all LEDs of the stripe to the same color
void stripeSetColor(String &name, Adafruit_NeoPixel &stripe, uint32_t c) {
  DEBUG_PRINT("stripeSetColor (");
  DEBUG_PRINT(name);
  DEBUG_PRINT(", ");
  DEBUG_PRINT(c);
  DEBUG_PRINT(")");
  for(uint16_t i = 0; i < stripe.numPixels(); i++) {
    DEBUG_PRINT(i);
    DEBUG_PRINT(", ");
    stripe.setPixelColor(i, c);
  }
  stripe.show();
  DEBUG_PRINT("stripeSetColor: done\n");
}

void stripeRainbow(String &name, Adafruit_NeoPixel &stripe) {
  static uint16_t j = 0;

  //DEBUG_PRINT("stripeRainbow(");
  //DEBUG_PRINT(name);
  //DEBUG_PRINT(")\n");
  for(uint16_t i = 0; i < stripe.numPixels(); i++) {
    stripe.setPixelColor(i, Wheel(stripe,(i + j) & 255));
  }
  stripe.show();
  j++;
  if(j >= 256) {
    j = 0;
  }
  //DEBUG_PRINT("stripeRainbow: done\n");
}

// Slightly different, this makes the rainbow equally distributed throughout
void stripeRainbowCycle(String &name, Adafruit_NeoPixel &stripe) {
  static uint16_t j = 0;

  //DEBUG_PRINT("stripeRainbowCycle(");
  //DEBUG_PRINT(name);
  //DEBUG_PRINT(")\n");
  for(uint16_t i = 0; i < stripe.numPixels(); i++) {
    stripe.setPixelColor(i, Wheel(stripe,((i * 256 / stripe.numPixels()) + j) & 255));
  }
  stripe.show();
  j++;
  //if(j >= 256 * 5) {
  if(j >= 256) {
    j = 0;
  }
  //DEBUG_PRINT("stripeRainbowCycle: done\n");
}

void stripeKnightRider(String &name, Adafruit_NeoPixel &stripe, uint32_t c) {
  static uint16_t j = 0;
  uint16_t x = 2;
  uint16_t m = 0;

  //DEBUG_PRINT("stripeKnightRider(");
  //DEBUG_PRINT(name);
  //DEBUG_PRINT(", ");
  //DEBUG_PRINT(c);
  //DEBUG_PRINT(")\n");
  for(uint16_t i = 0; i < stripe.numPixels(); i++) {
    if(i >= j - x && i <= j + x) {
      stripe.setPixelColor(i, c);
    } else {
      stripe.setPixelColor(i, BLACK);
    }
  }
  stripe.show();
  if(m == 0) {
    j++;
  } else {
    j--;
  }
  if(j <= 0) {
    m = 0;
  }
  if(j >= stripe.numPixels()) {
    m = 1;
  }
  //DEBUG_PRINT("stripeKnightRider: done\n");
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(Adafruit_NeoPixel &stripe, byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return stripe.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    return stripe.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return stripe.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

// Returns true if at least one of the LEDs of the stipe is glowing
bool stripeGetState(String &name, Adafruit_NeoPixel &stripe) {
  DEBUG_PRINT("stripeGetState(");
  DEBUG_PRINT(name);
  DEBUG_PRINT(")\n");
  bool state = false;
  uint16_t i = 0;
  while(!state && i < stripe.numPixels()) {
    if(stripe.getPixelColor(i) == 0) {
      i++;
    } else {
      DEBUG_PRINT("stripeGetState (");
      DEBUG_PRINT(i);
      DEBUG_PRINT("): ");
      DEBUG_PRINT(stripe.getPixelColor(i));
      DEBUG_PRINT("\n");
      state = true;
    }
  }
  DEBUG_PRINT("stripeGetColor: state = ");
  if(state) {
    DEBUG_PRINT("true\n");
  } else {
    DEBUG_PRINT("false\n");
  }
  DEBUG_PRINT("stripeGetColor: done\n");
  return state;
}

