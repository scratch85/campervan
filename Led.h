#ifndef Led_h
#define Led_h

class Led {
  public:
    Led();
    void attach(uint8_t pin);
    void on();
    void on(uint8_t);
    void off();
    void dimUp(uint8_t);
    void dimDown(uint8_t);
    void toggle();
    bool isOn();
    bool isAttached();
    uint8_t getPin();
    uint8_t getBrightness();
  private:
    uint8_t pin;
    uint8_t brightness;
    bool debug;
    void setBrightness(uint8_t);
};

#endif
