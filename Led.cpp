#include "Arduino.h"
#include "Led.h"

Led::Led() {
  pin = -1;
}

void Led::attach(uint8_t pin) {
  this->pin = pin;
  pinMode(getPin(), OUTPUT);
  off();
}

void Led::on() {
  on(255);
}

void Led::on(uint8_t brightness) {
  if(isAttached()) {
    setBrightness(brightness);
    analogWrite(getPin(), getBrightness());
  }
}

void Led::off() {
  if(isAttached()) {
    setBrightness(0);
    analogWrite(getPin(), LOW);
  }
}

void Led::dimUp(uint8_t brightness) {
  if(isAttached()) {
    setBrightness(min(max(getBrightness() + brightness, 0), 255));
    analogWrite(getPin(), getBrightness());
  }
}

void Led::dimDown(uint8_t brightness) {
  if(isAttached()) {
    setBrightness(min(max(getBrightness() - brightness, 0), 255));
    analogWrite(getPin(), getBrightness());
  }
}

void Led::toggle() {
  if(isOn()) {
    off();
  } else {
    on();
  }
}

bool Led::isAttached() {
  if(getPin() > -1) {
    return true;
  }
  return false;
}

bool Led::isOn() {
  if(getBrightness() > 0) {
    return true;
  }
  return false;
}

uint8_t Led::getPin() {
  return pin;
}

uint8_t Led::getBrightness() {
  return brightness;
}

void Led::setBrightness(uint8_t brightness) {
  this->brightness = brightness;
}
