// monitor with:
// pio device monitor --port COM5 --baud 115200

#include <avr/dtostrf.h>

#include "Adafruit_GFX.h"
#include "Fonts/ClearSans-Medium-12pt7b.h"
#include "Fonts/ClearSans-Medium-16pt7b.h"
#include "Fonts/ClearSans-Medium-18pt7b.h"
#include "Fonts/ClearSans-Medium-32pt7b.h"

#include "baro_sample.h"
#include "digibaro.h"
#include "display_samples.h"
#include "graph_samples.h"
#include "print_utils.h"
#include "vai_silouhette.h"
#include "watchdog_timer.h"

// #define KEEP_AWAKE

// #define STRESS_TEST

// canvas to draw on
GFXcanvas1 *canvas;

// Permanent samples written to flash
PermanentSamples permanent_samples(spi_flash);

// Rotating buffer of samples
RotatingSamples rotating_samples(spi_flash);

#ifndef STRESS_TEST
#define DAILY_PERIOD_MINUTE 5
#define WEEKLY_PERIOD_MINUTE 20
#else
#define DAILY_PERIOD_MINUTE 2
#define WEEKLY_PERIOD_MINUTE 10
#endif

GraphSamples daily_buffer(DAILY_PERIOD_MINUTE * 60);
GraphSamples weekly_buffer(WEEKLY_PERIOD_MINUTE * 60);

uint32_t uptime_seconds = 0;
uint32_t awake_centiseconds = 0;
uint32_t loop_counter = 0;

uint32_t samples_on_flash = 0;
uint32_t vbat_mv = 0;
int16_t current_line = 0;
int16_t altitude = 0;
int8_t timezone = 0;

enum DisplayMode : uint8_t { STATS = 0, INFO = 1, DAILY = 2, WEEKLY = 3 };

extern "C" char *sbrk(int i);

int FreeRam() {
  char stack_dummy = 0;
  return &stack_dummy - sbrk(0);
}

void ConfigureSettingsFromDip() {
  uint8_t dip_switches_state = GetDipState();
  DEBUG("DIP state = ", dip_switches_state);
  uint8_t alt_code = dip_switches_state >> 4;
  uint8_t tz_code = (0x0F & dip_switches_state) >> 1;
  uint8_t dst_mode = 0x01 & dip_switches_state;
  DEBUG("alt_code", alt_code);
  DEBUG("tz_code", tz_code);
  DEBUG("dst_mode", dst_mode);
  altitude = kAltitudes_options[alt_code];
  timezone = kTimezones_offset[tz_code] + dst_mode;
}

void SyncOnboardRTC(DateTime &utc) {
  if (loop_counter % 60 == 0) {
    // re-sync the onboard RTC every hour
    onboard_rtc.setTime(utc.hour(), utc.minute(), utc.second());
    onboard_rtc.setDate(utc.day(), utc.month(), uint8_t(utc.year() - 2000));
  }
}

void CheckAndWaitForSerial() {
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

void setup() {
  Serial.begin(115200);

#ifdef SERIAL_DEBUG
  uint8_t count = 0;
  while (!Serial && count < 12) {
    delay(1000);
    count++;
  }
  if (!Serial) {
    USBDevice.detach();
    serial_attached = false;
  }
#endif

  PRINTLN("Digibaro Starting...");

  ConfigureDevices();
  ConfigureSettingsFromDip();
  flash_debug.Message(FlashDebug::INIT, 0, 0);

  vbat_mv = MeasureVbat();

  ep42_display.ClearFrame();
  PRINT("Free RAM before Canvas allocation: ");
  PRINTLN(FreeRam());
  canvas = new GFXcanvas1(400, 300);
  PRINT("Free RAM after Canvas allocation: ");
  PRINTLN(FreeRam());

#ifndef KEEP_AWAKE

  canvas->fillScreen(0);
  canvas->drawBitmap(0, 0, vai_silouhette.data, vai_silouhette.width,
                     vai_silouhette.height, 1);
  canvas->setTextColor(0);
  canvas->setTextSize(1);
  canvas->setTextWrap(false);
  canvas->setFont(&ClearSans_Medium18pt7b);
  canvas->setCursor(185, 115);
  canvas->print("VAI");
  canvas->setCursor(185, 180);
  canvas->print("DigiBaro");
  ep42_display.DisplayFrame(canvas->getBuffer());

#ifdef SERIAL_DEBUG
  // crucial delay to let us chance to reflash!
  delay(5 * 1000);
#endif
#endif

#ifndef SERIAL_DEBUG
  uint8_t count = 0;
  while (!Serial && count < 20) {
    delay(1000);
    count++;
  }
  if (!Serial) {
    USBDevice.detach();
    serial_attached = false;
  }
#endif

  uint32_t last_index = rotating_samples.begin();
  PRINT("last rotating sample index = ");
  PRINTLN(last_index);

  PRINT("Permanent sample start addr = ");
  PRINTLN(kPermanentSamplesAddrStart);
  samples_on_flash = permanent_samples.begin();
  PRINT("Permanent sample retuned # = ");
  PRINTLN(samples_on_flash);

  // Initialized the buffers
  daily_buffer.Fill(rotating_samples, boot_utc.unixtime());
  weekly_buffer.Fill(rotating_samples, boot_utc.unixtime());

  // Configure the watchdog to 130s (two full cycles)
  wdt_configure(11);
}

BaroSample CollectSample(DateTime &dt, DateTime &local) {
  bme.PerformMeasurement();

  BaroSample sample(dt.unixtime(), bme.GetPressure() / 100,
                    bme.GetTemperature(), bme.GetHumidity() / 10, altitude,
                    timezone);

  if (Serial) {
    char buffer[56];
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

  if (dt.minute() % DAILY_PERIOD_MINUTE == 0) {
    rotating_samples.AddSample(sample);
  }
  if (dt.minute() == 0) {
    samples_on_flash = permanent_samples.AddSample(sample);
    flash_debug.Message(FlashDebug::STEP, 11, awake_centiseconds / (100 * 60));
  }
  return sample;
}

void DisplayLine(const char *buffer, int16_t spacing, int16_t x_offset = 2) {
  current_line += spacing;
  canvas->setCursor(x_offset, current_line);
  canvas->print(buffer);
}

void DisplayStats(DateTime &local, BaroSample &current) {
  char buffer[32];

  const size_t kPeriodHours = 3;
  const size_t kNbSamples = 11;
  DisplaySamples pressure_3h(kPeriodHours * 3600, kNbSamples);
  DisplaySamples temp_3h(kPeriodHours * 3600, kNbSamples);

  BaroSample last =
      rotating_samples.GetSampleAtIndex(rotating_samples.GetLastSampleIndex());

  last.PrettyPrint();
  pressure_3h.Fill(rotating_samples, last.GetTimestamp());
  pressure_3h.Print();
  temp_3h.Fill(rotating_samples, last.GetTimestamp(), TEMPERATURE);
  temp_3h.Print();

  canvas->setFont(&ClearSans_Medium18pt7b);
  current_line = 0;

  sprintf(buffer, "%04d-%02d-%02d %02d:%02d UTC%+d", local.year(),
          local.month(), local.day(), local.hour(), local.minute(), timezone);
  DisplayLine(buffer, 20, 16);

  char pressure_str[8];
  char temperature_str[8];
  char humidity_str[8];
  dtostrf(last.PressureMilliBar(), 5, 1, pressure_str);
  dtostrf(last.TemperatureDegCelcius(), 4, 1, temperature_str);
  dtostrf(last.HumidityPercent(), 4, 1, humidity_str);
  sprintf(buffer, "%s mb | %s C | %s%%", pressure_str, temperature_str,
          humidity_str);
  DisplayLine(buffer, 26);

  canvas->setFont(&ClearSans_Medium16pt7b);
  current_line += 6;

  char delta_str[8];
  // There is a bug: the first sample of the serie is invalid!
  // So: 1) the serie should be 1 element longer and 2) we want one extra
  // element for the first delta --> serie length = 9 + 2 !
  int16_t last_pressure = pressure_3h.Data(1);
  int16_t hours = -(int16_t)kPeriodHours * ((int16_t)kNbSamples - 3);
  for (size_t i = 2; i < kNbSamples; i++) {
    int16_t pressure = pressure_3h.Data(i);
    if (pressure == INT16_MIN) {
      sprintf(pressure_str, "----");
    } else {
      dtostrf((float)pressure / 10.0, 5, 1, pressure_str);
    }
    if (last_pressure == INT16_MIN || pressure == INT16_MIN) {
      sprintf(delta_str, "+#.#");
    } else {
      float delta = (float)(pressure - last_pressure) / 10.0;
      if (delta < 0) {
        delta_str[0] = '-';
        dtostrf(-delta, 3, 1, delta_str + 1);
      } else {
        delta_str[0] = '+';
        dtostrf(delta, 3, 1, delta_str + 1);
      }
    }
    int16_t temperature = temp_3h.Data(i);
    if (temperature == INT16_MIN) {
      sprintf(temperature_str, "---");
    } else {
      dtostrf(temperature / 100.0, 3, 1, temperature_str);
    }
    sprintf(buffer, "-%02d: %s ~ %s @ %sC", -hours, pressure_str, delta_str,
            temperature_str);
    DisplayLine(buffer, 24, 16);
    hours += kPeriodHours;
    last_pressure = pressure;
  }

  current_line += 2;
  dtostrf((float)pressure_3h.SerieMin() / 10.0, 5, 1, pressure_str);
  dtostrf((float)pressure_3h.SerieMax() / 10.0, 5, 1, humidity_str);
  sprintf(buffer, "min=%s / max=%s", pressure_str, humidity_str);
  DisplayLine(buffer, 24, 24);
}

void DisplayInfo(DateTime &local, BaroSample &last) {
  char buffer[56];
  char pressure_str[8];
  char temperature_str[8];
  char humidity_str[8];
  char other_str[8];

  canvas->setFont(&ClearSans_Medium12pt7b);
  current_line = 0;

  DateTime utc = DateTime(last.GetTimestamp());
  sprintf(buffer, "UTC : %04d-%02d-%02d %02d:%02d:%02d | Alt=%dm", utc.year(),
          utc.month(), utc.day(), utc.hour(), utc.minute(), utc.second(),
          altitude);
  DisplayLine(buffer, 15);

  sprintf(buffer, "Local: %04d-%02d-%02d %02d:%02d:%02d | TZ=%+d", local.year(),
          local.month(), local.day(), local.hour(), local.minute(),
          local.second(), timezone);
  DisplayLine(buffer, 22);

  dtostrf(last.PressureMilliBar(), 5, 1, pressure_str);
  dtostrf(last.TemperatureDegCelcius(), 4, 1, temperature_str);
  dtostrf(last.HumidityPercent(), 4, 1, humidity_str);
  sprintf(buffer, "Last: p=%s mb | t=%s C | h=%s %%", pressure_str,
          temperature_str, humidity_str);
  DisplayLine(buffer, 22);

  dtostrf((float)daily_buffer.SerieMin() / 10.0, 5, 1, pressure_str);
  dtostrf((float)weekly_buffer.SerieMin() / 10.0, 5, 1, other_str);
  sprintf(buffer, "Min p: 28h=%smb / 4.7d=%smb", pressure_str,
          other_str);
  DisplayLine(buffer, 26);

  dtostrf((float)daily_buffer.SerieMax() / 10.0, 5, 1, pressure_str);
  dtostrf((float)weekly_buffer.SerieMax() / 10.0, 5, 1, other_str);
  sprintf(buffer, "Max p: 28h=%smb / 4.7d=%smb", pressure_str, other_str);
  DisplayLine(buffer, 22);

#ifdef VMEASURE_WORKS
  if (vbat_mv == Vsaturated) {
    sprintf(buffer, "Vbat > 4600mV");
  } else {
    sprintf(buffer, "Vbat = %lu mV", vbat_mv);
  }
#else
  sprintf(buffer, "Vbat = N/A");
#endif
  DisplayLine(buffer, 26);

  sprintf(buffer, "# samples stored on flash = %lu", samples_on_flash);
  DisplayLine(buffer, 26);

  sprintf(buffer, "Min/Max recorded pressure on flash:");
  DisplayLine(buffer, 22);
  DateTime tlow = DateTime(permanent_samples.min_pressure_.GetTimestamp());
  dtostrf(permanent_samples.min_pressure_.PressureMilliBar(), 5, 1,
          pressure_str);
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d (UTC) -> %s mb", tlow.year(),
          tlow.month(), tlow.day(), tlow.hour(), tlow.minute(), tlow.second(),
          pressure_str);
  DisplayLine(buffer, 22);
  DateTime thigh = DateTime(permanent_samples.max_pressure_.GetTimestamp());
  dtostrf(permanent_samples.max_pressure_.PressureMilliBar(), 5, 1,
          pressure_str);
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d (UTC) -> %s mb", thigh.year(),
          thigh.month(), thigh.day(), thigh.hour(), thigh.minute(),
          thigh.second(), pressure_str);
  DisplayLine(buffer, 22);

  uptime_seconds = utc.secondstime() - boot_utc.secondstime();
  TimeSpan up = TimeSpan(uptime_seconds);
  sprintf(buffer, "loop counter=%ld | awake=%lds", loop_counter,
          awake_centiseconds / 100);
  DisplayLine(buffer, 22);
  sprintf(buffer, "uptime: %dd %dh %dm (%lus)", up.days(), up.hours(),
          up.minutes(), uptime_seconds);
  DisplayLine(buffer, 22);
  sprintf(buffer, "build: %s (%s)", DIGIBARO_VERSION, __DATE__);
  DisplayLine(buffer, 22);
}

void DisplayHeader(DateTime &local, BaroSample &sample) {
  char buffer[16];
  canvas->setFont(&ClearSans_Medium32pt7b);
  int16_t u, l, x, y;
  uint16_t w, h;

  sprintf(buffer, "%02d:%02d", local.hour(), local.minute());
  canvas->getTextBounds(buffer, 0, 44, &u, &l, &w, &h);
  x = w;
  y = h;
  canvas->setCursor(0, y);
  canvas->print(buffer);

  dtostrf(sample.PressureMilliBar(), 5, 1, buffer);
  canvas->getTextBounds(buffer, 0, y, &u, &l, &w, &h);
  canvas->setCursor(390 - w, y);
  canvas->print(buffer);

  canvas->setFont(&ClearSans_Medium12pt7b);
  sprintf(buffer, "TZ=%+d", timezone);
  canvas->setCursor(x + 16, 16);
  canvas->print(buffer);

  sprintf(buffer, "%dm", altitude);
  canvas->setCursor(x + 16, y);
  canvas->print(buffer);
}

void Display(DateTime &local, BaroSample &sample, uint8_t mode) {
  uint32_t count;

  // Display was powered OFF: need to wake up (with reset)
  if (ep42_display.Init() != 0) {
    flash_debug.Message(FlashDebug::STEP, DEVICE_EPD42_ID, -1);
    while (1)
      ;
  }

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
      if (count > 0) {
        daily_buffer.Draw(*canvas, 1, 0);
      }
      DisplayHeader(local, sample);
      break;
    case DisplayMode::WEEKLY:
      count = weekly_buffer.Fill(rotating_samples, sample.GetTimestamp());
      if (count > 0) {
        weekly_buffer.Draw(*canvas, 1, 0);
      }
      DisplayHeader(local, sample);
      break;
    default:
      break;
  }

  ep42_display.SendNewBufferAndRefresh(canvas->getBuffer());
  ep42_display.Sleep();
}

void loop() {
  static uint32_t after_awake = millis();
  wdt_reset();  // Tell the watchdog timer everything is fine
  loop_counter++;

  // Enable power to the external RTC and display
  digitalWrite(kRtcPowerPin, HIGH);
  delay(50);

  DateTime utc = ds3231_rtc.now();
  SyncOnboardRTC(utc);
  DateTime local = utc.getLocalTime(timezone);

  // Get display mode from rocker switches state
  uint8_t wake_switch_state = GetSwitchesState();
  ConfigureSettingsFromDip();
  BaroSample last_measurement = CollectSample(utc, local);
  vbat_mv = MeasureVbat();

  if (!timer_wakeup || utc.minute() % DAILY_PERIOD_MINUTE == 0) {
    // Update display only to the period in minute defined above
    // or if awakened by external switch input.
    Display(local, last_measurement, wake_switch_state);
  }

#ifndef KEEP_AWAKE
  uint8_t new_switch_state = GetSwitchesState();
  if (new_switch_state == wake_switch_state) {
    // Only go in standby mode if switches were not modified
    ConfigureForSleep();
    awake_centiseconds += ((millis() - after_awake)) / 10;

    // Go in standby mode for 1 minute!
    // flash_debug.Message(FlashDebug::STANDBY, 1, loop_counter);
    onboard_rtc.standbyMode();

    // nNw we are awake again! --> detach the interrupts
    onboard_rtc.detachInterrupt();
    for (size_t p = 0; p < 2; p++) {
      detachInterrupt(kSwitchesPin[p]);
    }
    after_awake = millis();
    digitalWrite(LED_BUILTIN, HIGH);
    // flash_debug.Message(FlashDebug::WAKEUP, 1, awake_centiseconds /
    // (100*60));

    CheckAndWaitForSerial();
  }
#else
  delay(10 * 1000);
#endif
}
