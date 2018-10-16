#include "Arduino.h"
#include "Led.h"

Led::Led(uint8_t pin) {
  this->pin = pin;
  pinMode(getPin(), OUTPUT);
  off();
}

void Led::on() {
  on(255);
}

void Led::on(uint8_t brightness) {
  setBrightness(brightness);
  analogWrite(getPin(), getBrightness());
}

void Led::off() {
  setBrightness(0);
  analogWrite(getPin(), LOW);
}

void Led::dimUp(uint8_t brightness) {
  setBrightness(min(max(getBrightness() + brightness, 0), 255));
  analogWrite(getPin(), getBrightness());
}

void Led::dimDown(uint8_t brightness) {
  setBrightness(min(max(getBrightness() - brightness, 0), 255));
  analogWrite(getPin(), getBrightness());
}

void Led::toggle() {
  if(isOn()) {
    off();
  } else {
    on();
  }
}

bool Led::isOn() {
  if(getBrightness() > 0) {
    return true;
  }
  return false;
}

byte Led::getPin() {
  return pin;
}

byte Led::getBrightness() {
  return getBrightness();
}

void Led::setBrightness(byte brightness) {
  this->brightness = brightness;
}
