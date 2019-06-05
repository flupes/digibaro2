
// monitor with:
// pio device monitor --port COM5 --baud 115200

#include "Adafruit_GFX.h"
#include "Fonts/ClearSans-Medium-24pt7b.h"

#include "baro_sample.h"
#include "digibaro.h"
#include "permanent_samples.h"
#include "print_utils.h"
#include "rotating_samples.h"

#define WAIT_FOR_SERIAL

// #define USE_SWITCH

// canvas to draw on
GFXcanvas1 *canvas;

// Permanent samples written to flash
PermanentSamples perm_samples(spi_flash);

uint32_t spi_addr;

// Rotating buffer of samples
RotatingSamples ring_samples(spi_flash);

const int8_t kTimeZone = -8;
const int16_t kAltitude = 220;
DateTime startup;

extern "C" char *sbrk(int i);

int FreeRam() {
  char stack_dummy = 0;
  return &stack_dummy - sbrk(0);
}

#define COLORED 0
#define UNCOLORED 1

void switchChange() {
  for (size_t i = 0; i < 2; i++) {
    EExt_Interrupts in = g_APinDescription[kSwitchesPin[i]].ulExtInt;
    uint32_t inMask = 1 << in;
    EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT(inMask);
  }
}

void setup() {
  Serial.begin(115200);

#ifdef WAIT_FOR_SERIAL
  uint8_t count = 0;
  while (!Serial && count < 12) {
    delay(1000);
    count++;
  }
#endif

  PRINTLN("Digibaro Starting...");

  configureDevices();

  ep42_display.ClearFrame();
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
  canvas->print("Waiting 5s");
  ep42_display.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
  ep42_display.DisplayFrame();
  PRINTLN("frame should be displayed...");
  // crucial delay to let us chance to reflash!
  delay(5 * 1000);

  PRINT("Permanent sample start addr = ");
  PRINTLN(kPermanentSamplesAddrStart);
  uint32_t nb_samples = perm_samples.begin();
  spi_addr =
      nb_samples * kPermanentSampleBytesLength + kPermanentSamplesAddrStart;
  PRINT("Permanent sample retuned # = ");
  PRINTLN(nb_samples);
  PRINT("Flash address start =  ");
  PRINTLN(spi_addr);
}

void collectSample(DateTime &dt) {
  bme.PerformMeasurement();

  BaroSample sample(dt.unixtime(), bme.GetPressure() / 100,
                    bme.GetTemperature(), bme.GetHumidity() / 10, kAltitude,
                    kTimeZone);

  if (Serial) {
    char buffer[64];
    DateTime local = dt.getLocalTime(kTimeZone);
    local.toString(buffer);
    Serial.print("collect_sample at time = ");
    Serial.print(buffer);
    Serial.print(" : press = ");
    Serial.print(bme.GetPressure());
    Serial.print("  | temp = ");
    Serial.print(bme.GetTemperature());
    Serial.print("  | humi = ");
    Serial.print(bme.GetHumidity());
    Serial.println();
    Serial.print("--> sample created at elevation = ");
    Serial.print(kAltitude);
    Serial.print(" with seconds = ");
    Serial.print(sample.GetTimestamp());
    Serial.print(" : press = ");
    Serial.print(sample.PressureMilliBar());
    Serial.print(" | temp = ");
    Serial.print(sample.TemperatureDegCelcius());
    Serial.print(" | humi = ");
    Serial.print(sample.HumidityPercent());
    Serial.println();
  }
  char buffer[64];
  sprintf(buffer, "pressure: %d", (int)sample.PressureMilliBar());
  canvas->setCursor(4, 140);
  canvas->print(buffer);
  sprintf(buffer, "temperature: %d", (int)sample.TemperatureDegCelcius());
  canvas->setCursor(4, 190);
  canvas->print(buffer);
  sprintf(buffer, "humitidy: %d", (int)sample.HumidityPercent());
  canvas->setCursor(4, 240);
  canvas->print(buffer);

  // uint32_t count = perm_samples.AddSample(sample);
  // if (Serial) {
  //   Serial.print("Sample #  ");
  //   Serial.print(count);
  //   Serial.print(" written at addr = ");
  //   Serial.println(perm_samples.GetLastSampleAddr());
  // }
}

void loop() {
  static uint32_t counter = 0;
  static uint32_t awake_ms = 0;
  static uint32_t after_awake = 0;

  if (!after_awake) after_awake = millis();

  counter++;
  spi_flash.writeULong(spi_addr, counter);
  spi_addr += 4;

  DateTime utc = ds3231_rtc.now();
  spi_flash.writeULong(spi_addr, utc.unixtime());
  spi_addr += 4;
  char buffer[64];
  DateTime local = utc.getLocalTime(-8);
  local.toString(buffer);
  PRINT("current time = ");
  PRINTLN(buffer);

  spi_flash.writeByte(spi_addr++, 1);
  // Display something to prove we are alive
  // epd.ClearFrame();
  canvas->fillScreen(UNCOLORED);
  sprintf(buffer, "Date: %04d-%02d-%02d", local.year(), local.month(),
          local.day());
  canvas->setCursor(4, 40);
  canvas->print(buffer);

  sprintf(buffer, "Time: %02d:%02d:%02d", local.hour(), local.minute(),
          local.second());
  canvas->setCursor(4, 90);
  canvas->print(buffer);

  sprintf(buffer, "%lu %lus %luh", counter, awake_ms / 1000,
          (utc.secondstime() - boot_utc.secondstime()) / 3600);
  canvas->setCursor(5, 290);
  canvas->print(buffer);

  spi_flash.writeByte(spi_addr++, 2);
  collectSample(utc);
  spi_flash.writeByte(spi_addr++, 3);

  ep42_display.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
  ep42_display.DisplayFrame();

  spi_flash.writeByte(spi_addr++, 4);

  PRINTLN("Go to sleep for another 15s...")
  configureForSleep();

  spi_flash.writeByte(spi_addr++, 5);

  awake_ms += (millis() - after_awake);

  onboard_rtc.standbyMode();
  // now we are awake again!
  onboard_rtc.detachInterrupt();
  after_awake = millis();

  // SPI.begin();
  // spi_flash.powerUp();
  spi_flash.writeByte(spi_addr++, 6);

  USBDevice.init();
  USBDevice.attach();
#ifdef WAIT_FOR_SERIAL
  uint8_t count = 0;
  while (!Serial && count < 10) {
    delay(1000);
    count++;
  }
  delay(2000);
#endif

  spi_flash.writeByte(spi_addr++, 7);
  PRINTLN("Just woke up!");

  // Enable power to the external RTC and display
  digitalWrite(kRtcPowerPin, HIGH);
  // It is necessary to re-enable the I2C bus (why?)
  // Wire.begin();
  // Wait at least 250ms for the RTC to get up to speed
  delay(300);

  PRINTLN("Restart e-Paper...");
  if (ep42_display.Init() != 0) {
    PRINTLN("e-Paper init failed");
    while (1)
      ;
  }
  spi_flash.writeByte(spi_addr++, 8);
}
