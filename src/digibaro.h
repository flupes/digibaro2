#ifndef DIGIBARO_DEFS_H
#define DIGIBARO_DEFS_H

#include <stdint.h>

#include <RTCZero.h>
#include <RTClib.h>
#include <SPIMemory.h>
#include <bme280_sensor.h>
#include <epd4in2.h>

constexpr uint8_t kRtcPowerPin = 6;

constexpr uint8_t kSwitchesPin[2] = {2, 3};

extern void configureForSleep();

extern void configureDevices();

// Global variables defined in devices.c

// On board SPI Flash Memory
extern SPIFlash spi_flash;

// SAMD onboard rtc
extern RTCZero onboard_rtc;

// Real Time Clock
extern RTC_DS3231 ds3231_rtc;

// e-paper display
extern Epd ep42_display;

// BME 280 Pressure/Temperature/Humidity sensor
extern Bme280Sensor bme;

// unused pins list
extern uint8_t pins_to_pullup[];
extern size_t nb_pins_to_pullup;

#endif
