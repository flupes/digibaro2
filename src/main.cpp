

// monitor with:
// pio device monitor --port COM5 --baud 115200

#include "Adafruit_GFX.h"
#include "Fonts/ClearSans-Medium-24pt7b.h"
#include "RTCZero.h"
#include "RTClib.h"
#include "SPIMemory.h"
#include "baro_sample.h"
#include "bme280_sensor.h"
#include "digibaro.h"
#include "epd4in2.h"
#include "permanent_samples.h"
#include "print_utils.h"
#include "rotating_samples.h"

#define WAIT_FOR_SERIAL

// #define USE_SWITCH

// Global variables:

// On board SPI Flash Memory
SPIFlash spi_flash(kMiniUltraProOnBoardChipSelectPin);

// SAMD onboard rtc
RTCZero onboard_rtc;

// Real Time Clock
RTC_DS3231 ds3231_rtc;

// e-paper display
Epd ep42_display;

// canvas to draw on
GFXcanvas1 *canvas;

// BME 280 Pressure/Temperature/Humidity sensor
Bme280Sensor bme(BME280_I2C_ADDR_SEC);

// Permanent samples written to flash
PermanentSamples perm_samples(spi_flash);

// Rotating buffer of samples
RotatingSamples ring_samples(spi_flash);

const int8_t kTimeZone = -8;
const int16_t kAltitude = 220;

extern "C" char *sbrk(int i);

int FreeRam() {
  char stack_dummy = 0;
  return &stack_dummy - sbrk(0);
}

#define COLORED 0
#define UNCOLORED 1

void alarmMatch() {
  onboard_rtc.detachInterrupt();
  // nothing, just wakeup
}

void switchChange() {
  for (size_t i = 0; i < 2; i++) {
    EExt_Interrupts in = g_APinDescription[kSwitchesPin[i]].ulExtInt;
    uint32_t inMask = 1 << in;
    EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT(inMask);
  }
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
  digitalWrite(kRtcPowerPin, LOW);

  USBDevice.detach();
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

  onboard_rtc.begin();

  if (!spi_flash.begin()) {
    PRINTLN("Flash memory initialization error!");
    while (1)
      ;
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
  for (uint8_t i = 0; i < sizeof(unused_pins); i++) {
    uint8_t pin = unused_pins[i];
    PRINT(" ");
    PRINT(pin);
    pinMode(pin, INPUT_PULLUP);
  }
  PRINTLN();

  // Built in LED ON when awake
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

#ifdef USE_SWITCH
  // Switch pins
  for (size_t i = 0; i < 2; i++) {
    pinMode(kSwitchesPin[i], INPUT);
  }
#endif
  // Start display (need the kRtcPowerPin HIGH to be enabled)
  PRINTLN("Init e-Paper...");

  if (ep42_display.Init() != 0) {
    PRINTLN("e-Paper init failed");
    return;
  }
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
  canvas->print("Going to sleep in 10s");
  ep42_display.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
  ep42_display.DisplayFrame();

  // crucial delay to let us chance to reflash!
  delay(10 * 1000);

  configureForSleep();
  onboard_rtc.standbyMode();
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
  canvas->setCursor(4, 150);
  canvas->print(buffer);
  sprintf(buffer, "temperature: %d", (int)sample.TemperatureDegCelcius());
  canvas->setCursor(4, 200);
  canvas->print(buffer);
  sprintf(buffer, "humitidy: %d", (int)sample.HumidityPercent());
  canvas->setCursor(4, 250);
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
  USBDevice.init();
  USBDevice.attach();
#ifdef WAIT_FOR_SERIAL
  uint8_t count = 0;
  while (!Serial && count < 10) {
    delay(1000);
    count++;
  }
  delay(3000);
#else
// It looks like the e-Paper needs some time to power up?
// delay(1000);
#endif

  PRINTLN("Just woke up!");

  SPI.begin();

  spi_flash.powerUp();

  // Enable power to the external RTC and display
  digitalWrite(kRtcPowerPin, HIGH);
  // It is necessary to re-enable the I2C bus (why?)
  // Wire.begin();
  // Wait at least 250ms for the RTC to get up to speed
  delay(300);

  DateTime utc = ds3231_rtc.now();
  char buffer[64];
  DateTime local = utc.getLocalTime(-8);
  local.toString(buffer);
  PRINT("current time = ");
  PRINTLN(buffer);

  PRINTLN("Restart e-Paper...");
  if (ep42_display.Init() != 0) {
    PRINTLN("e-Paper init failed");
    while (1)
      ;
  }

  // Display something to prove we are alive
  // epd.ClearFrame();
  canvas->fillScreen(UNCOLORED);
  sprintf(buffer, "Date: %04d-%02d-%02d", local.year(), local.month(),
          local.day());
  canvas->setCursor(4, 50);
  canvas->print(buffer);

  sprintf(buffer, "Time: %02d:%02d:%02d", local.hour(), local.minute(),
          local.second());
  canvas->setCursor(4, 100);
  canvas->print(buffer);

  collectSample(utc);

  ep42_display.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
  ep42_display.DisplayFrame();

  PRINTLN("Go to sleep for another 15s...")
  configureForSleep();
  onboard_rtc.standbyMode();
}
