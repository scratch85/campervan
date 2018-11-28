# campervan
In early 2018 I started to convert a Ford Transit Custom (SWB) into a campervan. All electronics are controlled by a [teensy 3.2]. This repo maintains the code and circuit diagram used.

## Circuit
![circuit]

## Parts
* 3x [Siegfar SF42049], 3watts, pwm dimmable, DC12V
* 2x [SK6812 RGBWW LED strip], 60Leds/m, IP65, DC5V
* 1x CC2540 CC2541 HM-10 BLE Bluetooth 4.0 Serial Modul AIP (from ebay)
* 1x TMP36 Temperatur sensor
* 6x Metal Push buttons, high head (from AliExpress)
* 7x 220ohm resistor
* 3x IRF3708 LogicLevel Mosfet

## TODOs
* ~~get second strip to work~~
* ~~implement way to synchronize strip effects~~
* ~~process inputs with parameters from serial / ble~~
* ~~helper function for inputs~~
* ~~store values in eeprom~~ (obsolete)
* ~~get more effects ready~~
* ~~rewrite using fastled instead of neopixel lib~~ (obsolete using [WS2812FX lib])
* ~~effects class?~~ (obsolete using [WS2812FX lib])
* ~~effects state machine?~~ (obsolete using [WS2812FX lib])
* consider [OneButton lib]
* effects switcher class
* consider cli lib
* code cleanup

## Even more future improvements
* rework layout on PermaProto
* make it a real pcb?

<!-- Link & Image References -->
[teensy 3.2]: https://www.pjrc.com/teensy/teensy31.html "Teensy 3.2 & 3.1 - Hardware"
[Siegfar SF42049]: http://www.siegfar.de/produkt/sf42049/ "Siegfar SF42049 dimmable 12V leds"
[SK6812 RGBWW LED strip]: http://www.btf-lighting.com/productshow.asp?ArticleID=0&id=164&cid=001
[circuit]: campervan_circuit.png "Camper Van's Circuit made with Fritzing"
[WS2812FX lib]: https://github.com/kitesurfer1404/WS2812FX "WS2812FX - More Blinken for your LEDs!"
[OneButton lib]: https://github.com/mathertel/OneButton