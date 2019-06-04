#include <Arduino.h>

#include "digibaro.h"
#include "print_utils.h"

void alarmMatch() {
  digitalWrite(LED_BUILTIN, HIGH);
  onboard_rtc.detachInterrupt();
}

void configureForSleep() {
  PRINT("Going to sleep... ");
  // we could get the time from the internal RTC, but
  // this way we check that the external is still responding...
  DateTime utc = ds3231_rtc.now();
  uint8_t alarm_seconds = utc.second() + 15;
  PRINT("Wake up at seconds = ");
  PRINTLN(utc.second());
  if (alarm_seconds > 59) alarm_seconds -= 60;

  ep42_display.Sleep();
  delay(100);

  onboard_rtc.setAlarmSeconds(alarm_seconds);
  onboard_rtc.enableAlarm(onboard_rtc.MATCH_SS);
  onboard_rtc.attachInterrupt(alarmMatch);
  digitalWrite(LED_BUILTIN, LOW);
  spi_flash.powerDown();
  // Wire.end();  // no direct effect on consumption, but it the I2C continues
  // the RTC will consume a lot of current from the coin battery
  digitalWrite(kRtcPowerPin, LOW);

  USBDevice.detach();
}