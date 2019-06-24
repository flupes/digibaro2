#include <Arduino.h>

#include "digibaro.h"
#include "print_utils.h"

volatile bool timer_wakeup = false;

bool serial_attached = false;

void alarmMatch() { timer_wakeup = true; }

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

void ConfigureForSleep() {
  ep42_display.Sleep();

  timer_wakeup = false;
  
  // Configure wake up for the next minute
  onboard_rtc.setAlarmSeconds(0);
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

  if (serial_attached) USBDevice.detach();
}