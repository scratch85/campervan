#ifndef Led_h
#define Led_h

class Led {
  public:
    Led(uint8_t);
    void on();
    void on(uint8_t);
    void off();
    void dimUp(uint8_t);
    void dimDown(uint8_t);
    void toggle();
    bool isOn();
    byte getPin();
    byte getBrightness();
  private:
    uint8_t pin;
    uint8_t brightness;
    bool debug;
    void setBrightness(uint8_t);
};

#endif
