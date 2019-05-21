/*
  Circuit:
    Slide DPDT switch middle pin connected to input pin 2, outer pins connected 
    to 3.3V and Ground.

            3V3 -----o|
                      |]
    pin 2 <----------o|
                  
            GND -----o

  Compile with:
    pio ci ./test/ExtInterrupt -b zeroUSB -l lib/RTCZero -l lib/SPIMemory
      -O "targets=upload"
*/
#include "RTCZero.h"
#include "SPIMemory.h"

// On board SPI Flash Memory
SPIFlash flash(4);

// SAMD onboard rtc
RTCZero onboard_rtc;

uint8_t wakeup_pin = 2;

uint8_t unused_pins[] = {0,  1,  3,  5,  6,  7,  8,  9,  10, 11, 12, 14,
                         15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
                         32, 33, 34, 35, 36, 37, 38, 39, 40, 41};

volatile bool switch_wakeup = false;

void alarmMatch() { switch_wakeup = false; }

void s1Change() {
  // Somehow, detach interrupt does not work here (maybe not re-entrant?)
  // detachInterrupt(wakeup_pin);

  // We clear the interrupt manually (code from WInterrupts.c)
  // Otherwise, this callback is constantly repeated since the
  // switch stays in the same position...
  EExt_Interrupts in = g_APinDescription[wakeup_pin].ulExtInt;
  uint32_t inMask = 1 << in;
  EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT(inMask);
  switch_wakeup = true;
}

void configureForSleep() {
  uint8_t alarm_seconds = onboard_rtc.getSeconds() + 15;
  if (alarm_seconds > 59) alarm_seconds -= 60;
  onboard_rtc.setAlarmSeconds(alarm_seconds);
  onboard_rtc.enableAlarm(onboard_rtc.MATCH_SS);
  onboard_rtc.attachInterrupt(alarmMatch);

  // CHANGE does not trigger anything because in deep sleep mode the
  // EIC clock is not avaible
  // attachInterrupt(wakeup_pin, s1Change, CHANGE);

  // Manually configure the interrupt to match the next state change
  bool s1 = digitalRead(wakeup_pin);
  if (s1) {
    attachInterrupt(wakeup_pin, s1Change, LOW);
  } else {
    attachInterrupt(wakeup_pin, s1Change, HIGH);
  }
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
  // switch pin (DPDT, so no pull up/down necessary)
  pinMode(wakeup_pin, INPUT);

  // Built in LED ON when awake
  pinMode(LED_BUILTIN, OUTPUT);

  // Let us a chance to reflash!
  delay(5 * 1000);

  // Disable USB
  USBDevice.detach();

}

void loop() {
  // Show that we are awake
  digitalWrite(LED_BUILTIN, HIGH);

  delay(5 * 1000);

  configureForSleep();
  digitalWrite(LED_BUILTIN, LOW);

  // Go to deep sleep
  onboard_rtc.standbyMode();

  onboard_rtc.detachInterrupt();
  // Now we are awake, detach the external interrupt
  // (not done inside the callback)
  detachInterrupt(wakeup_pin);

  if (switch_wakeup) {
    for (uint8_t i = 0; i < 5; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(50);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(600);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
}
