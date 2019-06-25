// monitor with:
// pio device monitor --port COM5 --baud 115200

#include <avr/dtostrf.h>

#include "Adafruit_GFX.h"
#include "Fonts/ClearSans-Medium-10pt7b.h"
#include "Fonts/ClearSans-Medium-12pt7b.h"
#include "Fonts/ClearSans-Medium-24pt7b.h"

#include "baro_sample.h"
#include "digibaro.h"
#include "graph_samples.h"
#include "print_utils.h"
#include "watchdog_timer.h"

// #define KEEP_AWAKE

// canvas to draw on
GFXcanvas1 *canvas;

// Permanent samples written to flash
PermanentSamples permanent_samples(spi_flash);

// Rotating buffer of samples
RotatingSamples rotating_samples(spi_flash);

#ifndef STRESS_TEST
GraphSamples daily_buffer(5 * 60);
GraphSamples weekly_buffer(20 * 60);
#else
GraphSamples daily_buffer(1 * 60);
GraphSamples weekly_buffer(5 * 60);
#endif

int8_t timezone = 0;
int16_t altitude = 0;
uint32_t samples_on_flash = 0;

enum DisplayMode : uint8_t { WEEKLY = 0, DAILY = 1, STATS = 2, INFO = 3 };

uint32_t uptime_seconds = 0;
uint32_t awake_centiseconds = 0;
uint32_t loop_counter = 0;

extern "C" char *sbrk(int i);

int FreeRam() {
  char stack_dummy = 0;
  return &stack_dummy - sbrk(0);
}

void GetSettingsFromDip()
{
  uint8_t dip_switches_state = GetDipState();
  DEBUG("DIP state = ", dip_switches_state);
  uint8_t alt_code = dip_switches_state >> 4;
  uint8_t tz_code = (0x0F & dip_switches_state) >> 1;
  uint8_t dst_mode = 0x01 & dip_switches_state;
  DEBUG("alt_code", alt_code);
  DEBUG("tz_code", tz_code);
  DEBUG("dst_mode", dst_mode);
  altitude = kAltitudes_options[alt_code];
  timezone = kTimezones_offset[tz_code] - dst_mode;
}

void setup() {
  Serial.begin(115200);

  uint8_t count = 0;
  while (!Serial && count < 12) {
    delay(1000);
    count++;
  }
  if (!Serial) {
    USBDevice.detach();
    serial_attached = false;
  }

  PRINTLN("Digibaro Starting...");

  ConfigureDevices();
  GetSettingsFromDip();
  
  ep42_display.ClearFrame();
  PRINT("Free RAM before Canvas allocation: ");
  PRINTLN(FreeRam());
  canvas = new GFXcanvas1(400, 300);
  PRINT("Free RAM after Canvas allocation: ");
  PRINTLN(FreeRam());

#ifndef KEEP_AWAKE
  canvas->setTextColor(0);
  canvas->setTextSize(1);
  canvas->setTextWrap(false);
  canvas->setFont(&ClearSans_Medium24pt7b);

  canvas->fillScreen(1);
  canvas->setCursor(4, 32);
  canvas->print("Waiting 5s");
  ep42_display.DisplayFrame(canvas->getBuffer());

// crucial delay to let us chance to reflash!
// delay(5 * 1000);
#endif

  uint32_t last_index = rotating_samples.begin();
  PRINT("last rotating sample index = ");
  PRINTLN(last_index);

  PRINT("Permanent sample start addr = ");
  PRINTLN(kPermanentSamplesAddrStart);
  samples_on_flash = permanent_samples.begin();
  PRINT("Permanent sample retuned # = ");
  PRINTLN(samples_on_flash);

  // Configure the watchdog to 130s (two full cycles)
  wdt_configure(11);
}

BaroSample CollectSample(DateTime &dt) {
  bme.PerformMeasurement();

  BaroSample sample(dt.unixtime(), bme.GetPressure() / 100,
                    bme.GetTemperature(), bme.GetHumidity() / 10, altitude,
                    timezone);

  if (Serial) {
    char buffer[64];
    DateTime local = dt.getLocalTime(timezone);
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
    Serial.print(altitude);
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

#ifndef STRESS_TEST
  if (dt.minute() % 5 == 0) {
    rotating_samples.AddSample(sample);
  }
  if (dt.minute() == 0) {
    permanent_samples.AddSample(sample);
    flash_debug.Message(FlashDebug::STEP, 11, awake_centiseconds / (100 * 60));
  }
#else
  uint32_t index = rotating_samples.AddSample(sample);
  DEBUG("new rotating sample index", index);
  if (dt.minute() % 15 == 0) {
    samples_on_flash = permanent_samples.AddSample(sample);

    flash_debug.Message(FlashDebug::STEP, 11, awake_centiseconds / (100 * 60));

    if (Serial) {
      Serial.print("Sample #  ");
      Serial.print(samples_on_flash);
      Serial.print(" written at addr = ");
      Serial.println(permanent_samples.GetLastSampleAddr());
    }
  }
#endif
  return sample;
}

void DisplayStats(DateTime &local, BaroSample &last) {
  char buffer[32];

  canvas->setFont(&ClearSans_Medium24pt7b);

  sprintf(buffer, "Date: %04d-%02d-%02d", local.year(), local.month(),
          local.day());
  canvas->setCursor(4, 40);
  canvas->print(buffer);

  sprintf(buffer, "Time: %02d:%02d:%02d", local.hour(), local.minute(),
          local.second());
  canvas->setCursor(4, 90);
  canvas->print(buffer);

  sprintf(buffer, "pressure: %d", (int)last.PressureMilliBar());
  canvas->setCursor(4, 140);
  canvas->print(buffer);
  sprintf(buffer, "temperature: %d", (int)last.TemperatureDegCelcius());
  canvas->setCursor(4, 190);
  canvas->print(buffer);
  sprintf(buffer, "humitidy: %d", (int)last.HumidityPercent());
  canvas->setCursor(4, 240);
  canvas->print(buffer);
}

void DisplayInfo(DateTime &local, BaroSample &last) {
  char buffer[56];
  char pressure_str[8];
  char temperature_str[8];
  char humidity_str[8];

  canvas->setFont(&ClearSans_Medium12pt7b);

  DateTime utc = DateTime(last.GetTimestamp());
  sprintf(buffer, "UTC : %04d-%02d-%02d %02d:%02d:%02d | Alt=%dm", utc.year(),
          utc.month(), utc.day(), utc.hour(), utc.minute(), utc.second(),
          altitude);
  canvas->setCursor(4, 15);
  canvas->print(buffer);

  sprintf(buffer, "Local: %04d-%02d-%02d %02d:%02d:%02d | TZ=%d", local.year(),
          local.month(), local.day(), local.hour(), local.minute(),
          local.second(), timezone);
  canvas->setCursor(4, 35);
  canvas->print(buffer);

  dtostrf(last.PressureMilliBar(), 5, 1, pressure_str);
  dtostrf(last.TemperatureDegCelcius(), 4, 1, temperature_str);
  dtostrf(last.HumidityPercent(), 4, 1, humidity_str);
  sprintf(buffer, "Last: p=%s mb | t=%s C | h=%s %%", pressure_str,
          temperature_str, humidity_str);
  canvas->setCursor(4, 55);
  canvas->print(buffer);

  sprintf(buffer, "# samples stored on flash = %lu", samples_on_flash);
  canvas->setCursor(4, 75);
  canvas->print(buffer);
  uptime_seconds = utc.secondstime() - boot_utc.secondstime();
  TimeSpan up = TimeSpan(uptime_seconds);
  sprintf(buffer, "loop counter=%ld | awake=%lds", loop_counter,
          awake_centiseconds / 100);
  canvas->setCursor(4, 95);
  canvas->print(buffer);
  sprintf(buffer, "uptime: %dd %dh %dm (%lus)", up.days(), up.hours(),
          up.minutes(), uptime_seconds);
  canvas->setCursor(4, 115);
  canvas->print(buffer);
  sprintf(buffer, "build: %s (%s)", DIGIBARO_VERSION, __DATE__);
  canvas->setCursor(4, 135);
  canvas->print(buffer);
}

void Display(DateTime &local, BaroSample &sample, uint8_t mode) {
  uint32_t count;

  ep42_display.ConfigAndSendOldBuffer(canvas->getBuffer());

  canvas->fillScreen(1);
  DEBUG("display mode", mode);

  switch (mode) {
    case DisplayMode::INFO:
      DisplayInfo(local, sample);
      break;
    case DisplayMode::STATS:
      DisplayStats(local, sample);
      break;
    case DisplayMode::DAILY:
      count = daily_buffer.Fill(rotating_samples, sample.GetTimestamp());
      DEBUG("daily count", count);
      if (count > 0) {
        daily_buffer.Draw(*canvas, 1, 0);
      }
      break;
    case DisplayMode::WEEKLY:
      weekly_buffer.Fill(rotating_samples, sample.GetTimestamp());
      weekly_buffer.Draw(*canvas, 1, 0);
      break;
    default:
      break;
  }

  ep42_display.SendNewBufferAndRefresh(canvas->getBuffer());
  ep42_display.Sleep();
}

void loop() {
  static uint32_t after_awake = 0;
  wdt_reset();
  loop_counter++;
  if (!after_awake) after_awake = millis();

  // Enable power to the external RTC and display
  digitalWrite(kRtcPowerPin, HIGH);
  delay(100);

  DateTime utc = ds3231_rtc.now();
  if (loop_counter % 60 == 0) {
    // re-sync the onboard RTC every hour
    onboard_rtc.setTime(utc.hour(), utc.minute(), utc.second());
    onboard_rtc.setDate(utc.day(), utc.month(), uint8_t(utc.year() - 2000));
  }

  char buffer[64];
  DateTime local = utc.getLocalTime(timezone);
  local.toString(buffer);
  PRINT("current time = ");
  PRINTLN(buffer);

  uint8_t wake_switch_state = GetSwitchesState();
  GetSettingsFromDip();

  BaroSample last_measurement = CollectSample(utc);

#ifdef STRESS_TEST
  uint8_t period = 1;
#else
  uint8_t period = 15;
#endif
  if (!timer_wakeup || utc.minute() % period == 0) {
    // Update display only to the period in minute defined above
    // Or if awakened by external switch input
    if (ep42_display.Init() != 0) {
      PRINTLN("e-Paper init failed");
      while (1)
        ;
    }

    Display(local, last_measurement, wake_switch_state);
    // ep42_display.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
    // ep42_display.DisplayFrame();
  }

  uint8_t new_switch_state = GetSwitchesState();
#ifndef KEEP_AWAKE
  if (new_switch_state == wake_switch_state) {
    // Only go in standby mode if switches were not modified
    // Setup for standby mode
    ConfigureForSleep();
    // flash_debug.Message(FlashDebug::STANDBY, 1, loop_counter);
    awake_centiseconds += ((millis() - after_awake)) / 10;

    // Go in standby mode for 1 minute!
    onboard_rtc.standbyMode();

    // now we are awake again!
    onboard_rtc.detachInterrupt();
    for (size_t p = 0; p < 2; p++) {
      detachInterrupt(kSwitchesPin[p]);
    }
    after_awake = millis();
    digitalWrite(LED_BUILTIN, HIGH);
    // flash_debug.Message(FlashDebug::WAKEUP, 1, awake_centiseconds /
    // (100*60));
    PRINTLN("Just woke up!");
    if (serial_attached) {
      USBDevice.init();
      USBDevice.attach();
      uint8_t count = 0;
      while (!Serial && count < 9) {
        delay(1000);
        count++;
      }
    }
  }
#else
  delay(10 * 1000);
#endif
}
