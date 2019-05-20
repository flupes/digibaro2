/*
  Compile with:
    pio ci ./test/ExtInterrupt -b zeroUSB -l lib/RTCZero -l lib/SPIMemory
      -O "targets=upload"
*/
#include "RTCZero.h"

// SAMD onboard rtc
RTCZero onboard_rtc;

uint8_t unused_pins[] = {0,  1,  3,  5,  6,  7,  8,  9,  10, 11, 12, 14,
                         15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
                         32, 33, 34, 35, 36, 37, 38, 39, 40, 41};

void alarmMatch() { onboard_rtc.detachInterrupt(); }

void s1Change() {
  detachInterrupt(2);
  onboard_rtc.detachInterrupt();
}

void configureForSleep() {
  uint8_t alarm_seconds = onboard_rtc.getSeconds() + 20;
  if (alarm_seconds > 59) alarm_seconds -= 60;
  onboard_rtc.setAlarmSeconds(alarm_seconds);
  onboard_rtc.enableAlarm(onboard_rtc.MATCH_SS);
  onboard_rtc.attachInterrupt(alarmMatch);
  
  // CHANGE does not trigger anything!
  // attachInterrupt(2, s1Change, CHANGE);
  bool s1 = digitalRead(2);
  if (s1) {
    attachInterrupt(2, s1Change, LOW);
  } else {
    attachInterrupt(2, s1Change, HIGH);
  }
}

void setup() {
  onboard_rtc.begin();
  onboard_rtc.setTime(10, 10, 01);
  onboard_rtc.setDate(19, 05, 19);

  // Configure all unused pins as input, enabled with built-in pullup
  for (uint8_t i = 0; i < sizeof(unused_pins); i++) {
    pinMode(unused_pins[i], INPUT_PULLUP);
  }
  // switch pin (DPDT, so no pull up/down necessary)
  pinMode(2, INPUT);

  // Built in LED ON when awake
  pinMode(LED_BUILTIN, OUTPUT);

  // Let us a chance to reflash!
  delay(10 * 1000);

  // Disable USB
  USBDevice.detach();
}

void loop() {
  // Show that we are awake
  digitalWrite(LED_BUILTIN, HIGH);

  delay(5 * 1000);

  configureForSleep();
  digitalWrite(LED_BUILTIN, LOW);
  onboard_rtc.standbyMode();
}
