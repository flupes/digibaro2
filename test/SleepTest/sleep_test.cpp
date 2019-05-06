/*
  Put the SAMD21 in sleep mode and wake up from the internal RTC after 15s.

  On wake up, restore I2C comms to external RTC, SPI comms to internal flash,
  SPI comms with the e-paper display, and USB comms with the host computer.

  The RTC itself consumes quite a bit of current when powered from external
  source (~150uA). So we power it off using the digital pin.

  The same digital pin used to power the external RTC is also used to enable
  the voltage regulator on-board the e-Paper display: when sleeping, the
  regulator is disabled. It saves ~60uA of idle current, but more importantly,
  also disables the level converted that is on the board (we do not need it
  since the Zero is already at 3.3V, but it would be too hard to remove
  bypass the chip). If not disabled, the level converter somehow draws a lot
  of current on each pin (probably pull-ups doing their jobs).
  See https://www.waveshare.com/w/upload/9/97/4.2inch_e-Paper_Schematic.pdf for
  the wiring of the regulator and level converter.

  The serial-USB port is also handled correctly, detach before sleep and
  re-attached at wake up (console can read data again from host).

  As is, consumption under Vbat = 4V is as follow:
    active but no display (internal LED on) = 10.5mA
    active, with display inactive =           11.5mA
    active, with display refreshing =         14mA
    sleep (external RTC off) =                25uA !

  Compile with:
    pio ci ./test/SleepTest -b zeroUSB -l lib/BaroUtils -l lib/RTClib
      -l lib/RTCZero -l lib/SPIMemory -l lib/epd42 -l lib/Adafruit-GFX-Library
      -O "targets=upload"
*/
#include "SPIMemory.h"

#include "RTCZero.h"
#include "RTClib.h"

#include "flash_config.h"
#include "print_utils.h"

#include "SPI.h"
#include "Wire.h"

// Uncomment to test without any peripherals
// #define BARE_BOARD

// Let a chance to the user to connect to the serial port
// #define WAIT_FOR_SERIAL

// Comment out to disable the display
#define USE_DISPLAY

#ifdef USE_DISPLAY
#include "Adafruit_GFX.h"
#include "Fonts/ClearSans-Medium-24pt7b.h"
#include "epd4in2.h"

// e-paper display
Epd epd;

// canvas to draw on
GFXcanvas1 *canvas;
#endif

#define COLORED 0
#define UNCOLORED 1

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

uint8_t unused_pins[] = {0,  1,  2,  3,  5,  7,  8,  9,  10, 11, 12, 14, 15, 16,
                         17, 18, 19, 25, 26, 34, 35, 36, 37, 38, 39, 40, 41};

const uint8_t kRtcPowerPin = 6;

extern "C" char *sbrk(int i);

int FreeRam() {
  char stack_dummy = 0;
  return &stack_dummy - sbrk(0);
}

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
  uint8_t alarm_seconds = utc.second() + 15;
  PRINT("Wake up at seconds = ");
  PRINTLN(utc.second());
  if (alarm_seconds > 59) alarm_seconds -= 60;
#else
  uint8_t alarm_seconds = 0;
#endif

#ifdef USE_DISPLAY
  epd.Sleep();
  delay(100);
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

#ifndef WAIT_FOR_SERIAL
  uint8_t count = 0;
  while (!Serial && count < 10) {
    delay(1000);
    count++;
  }
#endif

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
  // spec sheet says the RTC needs 250ms to stabilize
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
  onboard_rtc.setDate(2019, 04, 30);

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

#ifdef USE_DISPLAY
  // Start display (need the kRtcPowerPin HIGH to be enabled)
  PRINTLN("Init e-Paper...");

  if (epd.Init() != 0) {
    PRINTLN("e-Paper init failed");
    return;
  }
  epd.ClearFrame();

  PRINT("Free RAM before Canvas allocation: ");
  PRINTLN(FreeRam());

  canvas = new GFXcanvas1(400, 300);

  PRINT("Free RAM after Canvas allocation: ");
  PRINTLN(FreeRam());

  canvas->setTextColor(COLORED);
  canvas->setTextSize(1);
  canvas->setTextWrap(false);
  canvas->setFont(&ClearSans_Medium24pt7b);

  canvas->fillScreen(UNCOLORED);
  canvas->setCursor(4, 32);
  canvas->print("Going to sleep in 10s");
  epd.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
  epd.DisplayFrame();
#endif

  // crucial delay to let us chance to reflash!
  delay(10 * 1000);

  configureForSleep();
  onboard_rtc.standbyMode();
}

void loop() {
  // Enable power to the external RTC and display
  digitalWrite(kRtcPowerPin, HIGH);

  USBDevice.init();
  USBDevice.attach();
#ifdef WAIT_FOR_SERIAL
  uint8_t count = 0;
  while (!Serial && count < 10) {
    delay(1000);
    count++;
  }
  delay(3000);
#else
  // It looks like the e-Paper needs some time to power up?
  // delay(1000);
#endif

  PRINTLN("Just woke up!");

  // It looks like the SPI works automagically after waking up
  // SPI.begin();
  flash.powerUp();
 
  // Wait at least 250ms for the RTC to get up to speed
  delay(300);
 
  // It is necessary to re-enable the I2C bus (why?)
  Wire.begin();

  DateTime utc = ds3231_rtc.now();
  char buffer[64];
  DateTime local = utc.getLocalTime(-8);
  local.toString(buffer);
  PRINT("current time = ");
  PRINTLN(buffer);

#ifdef USE_DISPLAY
  PRINTLN("Restart e-Paper...");
  if (epd.Init() != 0) {
    PRINTLN("e-Paper init failed");
    while (1);
  }

  // Display something to prove we are alive
  // epd.ClearFrame();
  canvas->fillScreen(UNCOLORED);
  sprintf(buffer, "Date: %04d-%02d-%02d", local.year(), local.month(),
          local.day());
  canvas->setCursor(4, 60);
  canvas->print(buffer);
 
  sprintf(buffer, "Time: %02d:%02d:%02d", local.hour(), local.minute(),
          local.second());
  canvas->setCursor(4, 120);
  canvas->print(buffer);

  epd.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
  epd.DisplayFrame();
#endif

  // Try to access SPI Flash, just to make sure it is working.
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

  PRINTLN("Go to sleep for another 15s...")
  configureForSleep();
  onboard_rtc.standbyMode();
}
