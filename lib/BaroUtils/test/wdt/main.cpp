#include <Arduino.h>
#include "RTCZero.h"

#include "watchdog_timer.h"

// pio ci test\wdt -b zeroUSB -l .\src -l ..\RTCZero -O "targets=upload"

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

RTCZero onboard_rtc;

void rtcAlarm() {
  onboard_rtc.detachInterrupt();
  digitalWrite(LED_BUILTIN, HIGH);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  onboard_rtc.begin();

  // Serial.begin(115200);
  // while (!Serial)
  //   ;
  // Serial.println("watchdog_timer test");
  USBDevice.detach();

  // Watchdog is defined with a clock period ticking at 8ms
  // Time Out Period
  // 0 --> 8 * ms = 0.064s
  // 1 --> 16 * 8ms = 0.128s
  // 2 --> 32 * 8ms = 0.256s
  // 3 --> 64 * 8ms = 0.512s
  // 4 --> 128 * 8ms = 1.024s
  // 5 --> 256 * 8ms = 2.048s
  // 6 --> 512 * 8ms = 4.096s
  // 7 --> 1024 * 8ms = 8.192s
  // 8 --> 2048 * 8ms = 16.384s
  // 9 --> 4096 * 8ms = 32.768s
  // 10 --> 8192 * 8ms = 65.536s
  // 11 --> 16384 * 8ms = 131.072s
  wdt_configure(10);
  // configure with 9 will trigger a uproc reset
  // wdt_configure(9);
}

void loop() {
  uint8_t alarm_seconds = onboard_rtc.getSeconds();

  // Nominal case: standby 1 minute + delay 3s < 65s --> no reboot
  delay(3000);

  // uint8_t alarm_seconds = onboard_rtc.getSeconds() + 10;
  // if (alarm_seconds > 59) alarm_seconds -= 60;
  onboard_rtc.setAlarmSeconds(alarm_seconds);
  onboard_rtc.enableAlarm(onboard_rtc.MATCH_SS);
  onboard_rtc.attachInterrupt(rtcAlarm);
  digitalWrite(LED_BUILTIN, LOW);
  onboard_rtc.standbyMode();

  wdt_reset();
}
