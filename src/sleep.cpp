#include <Arduino.h>

#include "digibaro.h"
#include "print_utils.h"

void alarmMatch() { digitalWrite(LED_BUILTIN, HIGH); }

void switchChange() {
  // We clear the interrupt manually (code from WInterrupts.c)
  // Otherwise, this callback is constantly repeated since the
  // switch stays in the same position...
  for (size_t p = 0; p < 2; p++) {
    EExt_Interrupts in = g_APinDescription[kSwitchesPin[p]].ulExtInt;
    uint32_t inMask = 1 << in;
    EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT(inMask);
  }
}

void configureForSleep() {
  PRINT("Going to sleep... ");
  // we could get the time from the internal RTC, but
  // this way we check that the external is still responding...
  /*
  We should use the external RTC to get on accurate minutes.
  But it is possible to just use the internal RTC to wait a relative
  time.
  DateTime utc = ds3231_rtc.now();
  uint8_t alarm_seconds = utc.second() + 10;
  PRINT("Wake up at seconds = ");
  PRINTLN(utc.second());  */
  uint8_t alarm_seconds = onboard_rtc.getSeconds() + 10;

  if (alarm_seconds > 59) alarm_seconds -= 60;

  ep42_display.Sleep();
  delay(100);

  onboard_rtc.setAlarmSeconds(alarm_seconds);
  onboard_rtc.enableAlarm(onboard_rtc.MATCH_SS);
  onboard_rtc.attachInterrupt(alarmMatch);
  digitalWrite(LED_BUILTIN, LOW);

  for (size_t p = 0; p < 2; p++) {
    uint8_t pin = kSwitchesPin[p];
    bool s = digitalRead(pin);
    if (s) {
      attachInterrupt(pin, switchChange, LOW);
    } else {
      attachInterrupt(pin, switchChange, HIGH);
    }
  }

  // spi_flash.powerDown();

  // Wire.end();  // no direct effect on consumption, but it the I2C continues
  // the RTC will consume a lot of current from the coin battery
  digitalWrite(kRtcPowerPin, LOW);

  USBDevice.detach();
}