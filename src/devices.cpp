#include "digibaro.h"
#include "flash_config.h"
#include "print_utils.h"

Bme280Sensor bme(BME280_I2C_ADDR_SEC);

RTC_DS3231 ds3231_rtc;

Epd ep42_display;

RTCZero onboard_rtc;

/**
 * WARNING: spi_flash needs to be defined AFTER the other devices!
 * It makes not sense, but if the allocation comes before the other
 * devices, then standby mode consumes 2.3mA rather than 23uA!
 * It could be due that the SPIFlash constructor calls pinMode for the
 * CS pin to configure it as OUTPUT.
 * Still, more investigation are required to fully undertand this 
 * ordering sensitivity!
 */
SPIFlash spi_flash(kMiniUltraProOnBoardChipSelectPin);

uint8_t pins_to_pullup[] = {0,  1,  2,  3,  5,  7,  8,  9,  10, 11,
                            12, 14, 15, 16, 17, 18, 19, 22, 25, 26,
                            34, 35, 36, 37, 38, 39, 40, 41};

size_t nb_pins_to_pullup = sizeof(pins_to_pullup);


void configureDevices()
{

  onboard_rtc.begin();

  if (spi_flash.begin()) {
    PRINTLN("Serial Flash started.")
  } else {
    PRINTLN("Flash memory initialization error!");
    while (1)
      ;
  }

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
  for (uint8_t i = 0; i < sizeof(pins_to_pullup); i++) {
    uint8_t pin = pins_to_pullup[i];
    PRINT(" ");
    PRINT(pin);
    pinMode(pin, INPUT_PULLUP);
  }
  PRINTLN();

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