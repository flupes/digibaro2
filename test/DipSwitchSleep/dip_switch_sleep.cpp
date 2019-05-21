/*
  Circuit:
    Three switches connected to A1..A3 inputs with internal pulldown.
    High side of the switches controlled by output pin 6.
                     __
    pin 4 >----+----o  o------> A1
               |     __
               +----o  o------> A2
               |     __
               +----o  o------> A3

  Compile with:
    pio ci ./test/DipSwitchSleep -b zeroUSB -l lib/RTCZero -l lib/SPIMemory
      -O "targets=upload"
*/

#include "RTCZero.h"
#include "SPIMemory.h"

// On board SPI Flash Memory
SPIFlash flash(4);

// SAMD onboard rtc
RTCZero onboard_rtc;

uint8_t vctrl_pin = 6;

uint8_t unused_pins[] = {0,  1,  2,  3,  5,  7,  8,  9,  10, 11, 12,
                         14, 18, 19, 20, 21, 22, 23, 24, 25, 26, 32,
                         33, 34, 35, 36, 37, 38, 39, 40, 41};

void alarmMatch() {
  onboard_rtc.detachInterrupt();
  digitalWrite(vctrl_pin, HIGH);
}

void configureForSleep() {
  uint8_t alarm_seconds = onboard_rtc.getSeconds() + 10;
  if (alarm_seconds > 59) alarm_seconds -= 60;
  onboard_rtc.setAlarmSeconds(alarm_seconds);
  onboard_rtc.enableAlarm(onboard_rtc.MATCH_SS);
  onboard_rtc.attachInterrupt(alarmMatch);
  digitalWrite(vctrl_pin, LOW);
}

void setup() {
  onboard_rtc.begin();
  onboard_rtc.setTime(10, 10, 01);
  onboard_rtc.setDate(19, 05, 19);

  flash.begin();
  // Flash seems to consume 16uA (if not powered down) when the board sleep!
  flash.powerDown();

  // Configure all unused pins as input, enabled with built-in pullup
  for (uint8_t i = 0; i < sizeof(unused_pins); i++) {
    pinMode(unused_pins[i], INPUT_PULLUP);
  }
  // Vhigh pin
  pinMode(vctrl_pin, OUTPUT);

  // Input pins with interal pull down
  pinMode(15, INPUT_PULLDOWN);
  pinMode(16, INPUT_PULLDOWN);
  pinMode(17, INPUT_PULLDOWN);

  // Built in LED ON when awake
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Let us a chance to reflash!
  delay(8 * 1000);

  // Disable USB
  USBDevice.detach();
}

void loop() {
  configureForSleep();
  digitalWrite(LED_BUILTIN, LOW);
  // Go to deep sleep
  onboard_rtc.standbyMode();

  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  uint8_t b1 = digitalRead(15);
  uint8_t b2 = digitalRead(16);
  uint8_t b3 = digitalRead(17);
  uint8_t num = b1 << 2 | b2 << 1 | b3;
  delay(1800);

  for (uint8_t i = 0; i < num; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
  }
}
