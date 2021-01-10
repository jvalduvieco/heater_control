# Temperature sensor 

* Power using Vcc at 5V. If powering directly to 3.3v input board burns on connecting a USB cable.
* Does not allow OTA due insufficient memory. Need to investigate.
* Add board: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
* Board ESP32 Arduino -> ESP32 dev module
* Temperature sensor DS18B20
* https://www.az-delivery.de/en/products/esp32-developmentboard?_pos=27&_sid=e42edec95&_ss=r
* Libraries: DallasTemperature
* Virtual COM port driver: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
* A pullup on GPIO 2 or GPIO 12 during boot disable programming mode 
