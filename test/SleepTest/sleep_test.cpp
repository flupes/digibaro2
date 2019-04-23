/*
  Put the SAMD21 in sleep mode and wake up from the
  internal RTC after 20s.
  On wake up, restore I2C comms to external RTC and SPI comms to internal flash.

  The RTC itself consumes quite a bit of current when powered from external
  source (~150uA). So we power it off using the digital pin.

  The serial-USB port is also handled correctly, detach before sleep and 
  re-attached at wake up (console can read data again from host).

  As is, consumption under Vbat = 4V is as follow:
    active (internal LED on) = 10.5mA
    sleep (external RTC off) = 25uA !
*/
#include "SPIMemory.h"

#include "RTCZero.h"
#include "RTClib.h"

#include "flash_config.h"
#include "print_utils.h"

#include "SPI.h"
#include "Wire.h"

// #define BARE_BOARD

// On board SPI Flash Memory
SPIFlash flash(kMiniUltraProOnBoardChipSelectPin);

// SAMD onboard rtc
RTCZero onboard_rtc;

#ifndef BARE_BOARD
// External Real Time Clock
RTC_DS3231 ds3231_rtc;
#endif

// unused pins list
// uint8_t unused_pins[] = {0,  1,  2,  3,  6,  11, 12, 14, 15, 16, 17, 18, 19,
//                          25, 26, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41};

// Pin 4 is internal chip select for the flash memory
// Pin 6 is used to power the external RTC
// Pins 7, 8, 9 and 10 are reserved for the ePaper display
// Pin 13 is built-in LED
// Pins 20 and 21 are primary I2C bus
// Pint 22, 23 and 24 are primary SPI bus

// Somehow, configuring either pin 32 or 33 in INPUT-PULLUP breaks the
// I2C communication with the external RTC !!!

uint8_t unused_pins[] = {0,  1,  2,  3,  5,  11, 12, 14, 15, 16, 17, 18,
                         19, 25, 26, 34, 35, 36, 37, 38, 39, 40, 41};

const uint8_t kRtcPowerPin = 6;

void alarmMatch() {
  digitalWrite(LED_BUILTIN, HIGH);
  onboard_rtc.detachInterrupt();
}

void configureForSleep() {
  PRINT("Going to sleep... ");
#ifndef BARE_BOARD
  // we could get the time from the internal RTC, but
  // this way we check that the external is still responding...
  DateTime utc = ds3231_rtc.now();
  uint8_t alarm_seconds = utc.second() + 20;
  PRINT("Wake up at seconds = ");
  PRINTLN(utc.second());
  if (alarm_seconds > 59) alarm_seconds -= 60;
#else
  uint8_t alarm_seconds = 0;
#endif
  onboard_rtc.setAlarmSeconds(alarm_seconds);
  onboard_rtc.enableAlarm(onboard_rtc.MATCH_SS);
  onboard_rtc.attachInterrupt(alarmMatch);
  digitalWrite(LED_BUILTIN, LOW);
  flash.powerDown();
  Wire.end();  // no direct effect on consumption, but it the I2C continues
  // the RTC will consume a lot of current from the coin battery

  // pinMode(20, INPUT_PULLUP);
  // pinMode(21, INPUT_PULLUP);

  // SPI.end(); --> consume 80uA rather than 20uA if terminating SPI!
  digitalWrite(kRtcPowerPin, LOW);
  USBDevice.detach();
}

void setup() {
  Serial.begin(115200);
  uint8_t count = 0;
  while (!Serial && count < 10) {
    delay(1000);
    count++;
  }

  PRINTLN("TestSleep Starting...");
  PRINTLN("You have 10s before board goes to sleep!");

  onboard_rtc.begin();

  if (flash.begin()) {
    PRINTLN("Serial Flash started.")
  } else {
    PRINTLN("Flash memory initialization error!");
    while (1)
      ;
  }

#ifndef BARE_BOARD
  PRINTLN("Powering the external RTC");
  pinMode(kRtcPowerPin, OUTPUT);
  digitalWrite(kRtcPowerPin, HIGH);
  delay(300);
  ds3231_rtc.begin();

  PRINTLN("Getting time from external clock.");
  DateTime utc = ds3231_rtc.now();
  char buffer[64];
  DateTime local = utc.getLocalTime(-8);
  local.toString(buffer);
  PRINT("current time = ");
  PRINTLN(buffer);

  // Set time of internal clock
  PRINTLN("Set internal clock time.");
  onboard_rtc.setTime(utc.hour(), utc.minute(), utc.second());
  onboard_rtc.setDate(utc.day(), utc.month(), utc.year());

  PRINT("Configure unused pins:");
  // Configure all unused pins as input, enabled with built-in pullup
  for (uint8_t i = 0; i < sizeof(unused_pins); i++) {
    uint8_t pin = unused_pins[i];
    PRINT(" ");
    PRINT(pin);
    pinMode(pin, INPUT_PULLUP);
  }
  PRINTLN();

#else
  onboard_rtc.setTime(10, 10, 01);
  onboard_rtc.setDate(2019, 04, 20);

  uint8_t pinNumber;
  for (pinNumber = 0; pinNumber < 23; pinNumber++) {
    pinMode(pinNumber, INPUT_PULLUP);
  }
  for (pinNumber = 32; pinNumber < 42; pinNumber++) {
    pinMode(pinNumber, INPUT_PULLUP);
  }
  pinMode(25, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
#endif

  // Built in LED ON when awake
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // crucial delay to let us chance to reflash!
  delay(10 * 1000);

  configureForSleep();
  onboard_rtc.standbyMode();
}

void loop() {
  USBDevice.init();
  USBDevice.attach();
  uint8_t count = 0;
  while (!Serial && count < 10) {
    delay(1000);
    count++;
  }
  delay(3000);
  PRINTLN("Just woke up!");

  digitalWrite(kRtcPowerPin, HIGH);
  // SPI.begin();
  flash.powerUp();
  delay(300);
  Wire.begin();

  DateTime utc = ds3231_rtc.now();
  char buffer[64];
  DateTime local = utc.getLocalTime(-8);
  local.toString(buffer);
  PRINT("current time = ");
  PRINTLN(buffer);

  uint32_t number = flash.readULong(kPermanentSamplesSectorStart * KB(4));
  PRINT("Read something from flash: ");
  PRINTLN(number);

  // pretend we are active
  for (uint8_t i = 0; i < 10; i++) {
    PRINT(i);
    PRINT(" ");
    delay(1000);
  }
  PRINTLN();

  PRINTLN("Go to sleep for another 20s...")
  // go to sleep again for another minute
  // for (uint8_t i = 0; i < sizeof(unused_pins); i++) {
  //   pinMode(unused_pins[i], INPUT_PULLUP);
  // }
  configureForSleep();
  onboard_rtc.standbyMode();
}
