#ifndef DIGIBARO_PRINT_UTILS_H
#define DIGIBARO_PRINT_UTILS_H

#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#define PRINT(x) \
  if (Serial) Serial.print(x);

#define PRINTLN(x) \
  if (Serial) Serial.println(x);

#endif
