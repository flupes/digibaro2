#ifndef DIGIBARO_DEFS_H
#define DIGIBARO_DEFS_H

#include <stdint.h>

#include <RTCZero.h>
#include <RTClib.h>
#include <SPIMemory.h>
#include <bme280_sensor.h>
#include <epd4in2.h>

#include "flash_debug.h"
#include "permanent_samples.h"
#include "rotating_samples.h"

constexpr uint8_t kRtcPowerPin = 6;

constexpr uint8_t kSwitchesPin[2] = {2, 3};

constexpr uint8_t kDipPins[6] = {PIN_A1, PIN_A2, PIN_A3, PIN_A4, 12, 11};

constexpr uint8_t kVsensePin = PIN_A5;

constexpr uint32_t Vsaturated = 10000;

extern void ConfigureForSleep();

extern void ConfigureDevices();

extern uint8_t GetSwitchesState();

extern uint8_t GetDipState();

extern uint32_t MeasureVbat();

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

extern DateTime boot_utc;

extern FlashDebug flash_debug;

// Specicy if the wakeup was due to the alarm or not (meaning switch change)
extern volatile bool timer_wakeup;

// Use USB Serial or not (depends on the boot state)
extern bool serial_attached;

// TODO : these should move into settings stored on flash!
constexpr int8_t kTimezones_offset[] = {1, 0, -1, -4, -5, -6, -7, -8};

constexpr int16_t kAltitudes_options[] = {0, 222, 445, 1620};

// Some ID to facilitate error logging to flash
#define DEVICE_SPI_FLASH_ID 1
#define DEVICE_ONBOARD_RTC_ID 2
#define DEVICE_RTC_DS3231_ID 3
#define DEVICE_EPD42_ID 4
#define DEVICe_BME280_ID 5

#endif
