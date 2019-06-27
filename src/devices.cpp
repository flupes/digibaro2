#include "digibaro.h"
#include "flash_config.h"
#include "print_utils.h"

Bme280Sensor bme(BME280_I2C_ADDR_SEC);

RTC_DS3231 ds3231_rtc;

Epd ep42_display;

RTCZero onboard_rtc;

DateTime boot_utc;

FlashDebug flash_debug;

/**
 * WARNING: the SPIFlash constructor calls pinMode for the
 * CS pin to configure it as OUTPUT. It may cause side effect...
 */
SPIFlash spi_flash(kMiniUltraProOnBoardChipSelectPin);

// TODO : need to check if 19 = A5 should be pulled up (probably not!)
uint8_t pins_to_pullup[] = {0,  1, 5, 7,  8,  9,  10, 14, 22,
                            25, 26, 34, 35, 36, 37, 38, 39, 40, 41};

size_t nb_pins_to_pullup = sizeof(pins_to_pullup);

void ConfigureDevices() {
  if (spi_flash.begin()) {
    PRINTLN("Serial Flash started.")
  } else {
    PRINTLN("Flash memory initialization error!");
    while (1)
      ;
  }

  uint32_t start = flash_debug.SetFlash(&spi_flash);
  Serial.print("debug addr start = ");
  Serial.println(start);

  flash_debug.Message(FlashDebug::BOOT);

  onboard_rtc.begin();

  PRINTLN("Powering the external RTC");
  pinMode(kRtcPowerPin, OUTPUT);
  digitalWrite(kRtcPowerPin, HIGH);
  // spec sheet says the RTC needs 250ms to stabilize
  delay(300);
  ds3231_rtc.begin();

  PRINTLN("Getting time from external clock.");
  boot_utc = ds3231_rtc.now();
  char buffer[64];
  DateTime local = boot_utc.getLocalTime(-8);
  local.toString(buffer);
  PRINT("current time = ");
  PRINTLN(buffer);

  // Set time of internal clock
  PRINT("onboard_rtc.setDate(");
  PRINT(boot_utc.day());
  PRINT(", ");
  PRINT(boot_utc.month());
  PRINT(", ");
  PRINT(boot_utc.year() - 2000);
  PRINTLN(")");
  onboard_rtc.setDate(boot_utc.day(), boot_utc.month(), boot_utc.year() - 2000);
  PRINT("onboard_rtc.setTime(");
  PRINT(boot_utc.hour());
  PRINT(", ");
  PRINT(boot_utc.minute());
  PRINT(", ");
  PRINT(boot_utc.second());
  PRINTLN(")");
  uint8_t minutes = boot_utc.minute();
  uint8_t seconds = boot_utc.second() + 1;
  if (seconds == 60) {
    seconds = 0;
    minutes++;
  }
  onboard_rtc.setTime(boot_utc.hour(), minutes, seconds);

  flash_debug.SetRTC(&onboard_rtc);

  PRINT("Configure unused pins:");
  // Configure all unused pins as input, enabled with built-in pullup
  // WARNING: for now, these pins are configured BEFORE the e-Paper
  // is initialized, otherwise the e-Paper is not responding!
  // I suspect this is because we re-configure a pin wrong...
  for (uint8_t i = 0; i < sizeof(pins_to_pullup); i++) {
    uint8_t pin = pins_to_pullup[i];
    PRINT(" ");
    PRINT(pin);
    pinMode(pin, INPUT_PULLUP);
  }
  PRINTLN();

  // Switches input pins
  for (size_t p = 0; p < sizeof(kSwitchesPin); p++) {
    pinMode(kSwitchesPin[p], INPUT);
  }

  // DIP Switch input pins
  for (size_t p = 0; p < sizeof(kDipPins); p++) {
    pinMode(kDipPins[p], INPUT_PULLDOWN);
  }

  // A5 Battery monitoring pin
  pinMode(kVsensePin, INPUT);

  // Built in LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Start display (need the kRtcPowerPin HIGH to be enabled)
  PRINTLN("Init e-Paper...");
  if (ep42_display.Init() != 0) {
    PRINTLN("e-Paper init failed");
    return;
  }

  if (!bme.Begin()) {
    PRINTLN("Sensor initialization error!");
    while (1)
      ;
  }

  // PRINT("Start address of permanent sample = ");
  // PRINTLN(perm_samples.GetFirstSampleAddr());
  // PRINT("Max number of perm_samples = ");
  // PRINTLN(perm_samples.GetMaxNumberOfSamples());
  // uint32_t index = perm_samples.begin();
  // PRINT("Last retrieved sample index = ");
  // PRINTLN(index);
}

uint8_t GetSwitchesState() {
  uint8_t state = digitalRead(kSwitchesPin[0]);
  state |= digitalRead(kSwitchesPin[1]) << 1;
  return state;
}

uint8_t GetDipState() {
  uint8_t state = 0;
  for (size_t p=0; p<sizeof(kDipPins); p++) {
    state |= digitalRead(kDipPins[p]) << p;
  }
  return state;
}

uint32_t MeasureVbat() {
  const uint32_t kVRef_mV = 3300;
  const uint32_t kRLow = 25;
  const uint32_t kRhigh = 10;
  analogReadResolution(12);
  uint32_t Vsense = analogRead(kVsensePin);
  DEBUG("Vsense", Vsense);
  if (Vsense == 4095) return Vsaturated;
  // Vsense = Vbat * Rlow / (Rhigh + Rlow)
  uint32_t Vbat = Vsense * kVRef_mV * (kRLow + kRhigh) / kRLow;
  // analog read on 12 bits = 4096 ticks
  return Vbat / 4096;
}