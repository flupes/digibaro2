#include "baro_sample.h"
#include "print_utils.h"

bool BaroSample::operator==(BaroSample &s) {
  if (timestamp_ != s.timestamp_) return false;
  if (hour_twelfth_ != s.hour_twelfth_) return false;
  if (pressure_pa_off_ != s.pressure_pa_off_) return false;
  if (humidity_deci_percent_ != s.humidity_deci_percent_) return false;
  if (temperature_centi_deg_ != s.temperature_centi_deg_) return false;
  if (timezone_ != s.timezone_) return false;
  if (reserved_ != s.reserved_) return false;
  return true;
}

bool BaroSample::operator!=(BaroSample &s) { return !(*this == s); }

void BaroSample::PackSample(char *data) {
  uint16_t utemp = 0;
  if (temperature_centi_deg_ < 0) {
    utemp = 0x3FFF & (uint16_t)(16384 + temperature_centi_deg_);
    // utemp = ~(temperature_centi_deg_) + 1;
    // Serial.print("utemp = ");
    // Serial.println(utemp, HEX);
  } else {
    utemp = temperature_centi_deg_;
  }
  uint8_t temp_msb = (uint8_t)(utemp >> 6);
  uint8_t temp_lsb = (uint8_t)(utemp << 2);
  uint8_t humi_msb = (uint8_t)(humidity_deci_percent_ >> 2);
  uint8_t humi_lsb = (uint8_t)(0x0003 & humidity_deci_percent_);
  uint32_t shifted_ts = hour_twelfth_ << 2;
  memcpy(data, &shifted_ts, 3);
  // Serial.print("PACKING: hour_twelfth = ");
  // Serial.print(hour_twelfth_, HEX);
  // Serial.print(" --> shifted_ts = ");
  // Serial.println(shifted_ts, HEX);
  memcpy(data + 3, &pressure_pa_off_, 2);
  data[5] = temp_msb;
  data[6] = humi_msb;
  data[7] = temp_lsb | humi_lsb;
}

BaroSample::BaroSample(char *data, bool packed) {
  if (packed) {
    uint32_t shifted_ts = 0;
    memcpy(&shifted_ts, data, 3);
    hour_twelfth_ = shifted_ts >> 2;
    // Serial.print("UNPACKING: shifted_ts = ");
    // Serial.print(shifted_ts, HEX);
    // Serial.print(" --> hour_twelfth = ");
    // Serial.println(hour_twelfth_, HEX);
    // hour_twelfth_ &= 0x3FFFFF; // should not be necessary...
    memcpy(&pressure_pa_off_, data + 3, 2);
    uint8_t temp_msb = data[5];
    uint8_t temp_lsb = data[7] & 0xFC;
    uint8_t humi_msb = data[6];
    uint8_t humi_lsb = data[7] & 0x03;
    // Serial.print("temp_msb = ");
    // Serial.print(temp_msb, HEX);
    // Serial.print(" << 6 ");
    // Serial.println((uint16_t)(temp_msb) << 6, HEX);
    // Serial.print("temp_lsb = ");
    // Serial.print(temp_lsb, HEX);
    // Serial.print(" >> 2 ");
    // Serial.println((uint16_t)(temp_lsb >> 2), HEX);
    uint16_t utemp = ((uint16_t)(temp_msb) << 6) | (uint16_t)(temp_lsb >> 2);
    // Serial.print("utemp = ");
    // Serial.println(utemp, HEX);
    if (utemp < 8192) {
      temperature_centi_deg_ = (int16_t)(utemp);
    } else {
      temperature_centi_deg_ = (int16_t)(0xC000 | utemp);
    }
    humidity_deci_percent_ = ((uint16_t)(humi_msb) << 2) | (uint16_t)(humi_lsb);
    timestamp_ = kSecondsResolution * (uint32_t)(hour_twelfth_) + k2019epoch;
  } else {
    memcpy(&timestamp_, data, 4);
    memcpy(&hour_twelfth_, data + 4, 4);
    memcpy(&pressure_pa_off_, data + 8, 2);
    memcpy(&humidity_deci_percent_, data + 10, 2);
    memcpy(&temperature_centi_deg_, data + 12, 2);
    memcpy(&timezone_, data + 14, 1);
    memcpy(&reserved_, data + 15, 1);
  }
}

void BaroSample::SerializeSample(char *data) {
  memcpy(data, &timestamp_, 4);
  memcpy(data + 4, &hour_twelfth_, 4);
  memcpy(data + 8, &pressure_pa_off_, 2);
  memcpy(data + 10, &humidity_deci_percent_, 2);
  memcpy(data + 12, &temperature_centi_deg_, 2);
  memcpy(data + 14, &timezone_, 1);
  memcpy(data + 15, &reserved_, 1);
}

bool BaroSample::SetTimeStamp(uint32_t seconds) {
  timestamp_ = seconds;
  if (timestamp_ < k2019epoch) {
    hour_twelfth_ = 0;
    return false;
  }
  hour_twelfth_ = (seconds - k2019epoch) / kSecondsResolution;
  hour_twelfth_ &= 0x3FFFFF;  // forget bits of rank larger than 22!
  return true;
}

bool BaroSample::SetPressure(uint32_t pressure) {
  if (pressure > 125500) {
    pressure_pa_off_ = UINT16_MAX;
    return false;
  }
  pressure_pa_off_ = (uint16_t)(pressure - kPressureOffsetPa);
  return true;
}

bool BaroSample::SetTemperature(int32_t temperature) {
  bool overflow = false;
  if (temperature < -8192) {
    temperature = -8192;
    overflow = true;
  }
  if (temperature > 8191) {
    temperature = 8191;
    overflow = true;
  }
  temperature_centi_deg_ = (int16_t)(temperature);
  return !overflow;
}

bool BaroSample::SetHumidity(uint32_t humidity) {
  if (humidity > 10200) {
    humidity_deci_percent_ = 0x3FF;
    return false;
  }
  humidity_deci_percent_ = (uint16_t)(humidity / 10);
  return true;
}

uint32_t BaroSample::SeaLevelPressure(uint32_t station_pressure,
                                      int16_t elevation, int16_t temperature) {
  // See https://keisan.casio.com/exec/system/1224575267 for equation
  float correction =
      (1.0 -
       0.0065 * (float)elevation /
           ((float)temperature/100.0 + 0.0065 * (float)elevation + 273.15));
  float factor = powf(correction, -5.257);
  return (uint32_t)((float)station_pressure*factor);
}

void BaroSample::Print() {
  if (Serial) {
    Serial.print("timestamp   = ");
    Serial.println(timestamp_);
    Serial.print("hourtwelfth = ");
    Serial.println(hour_twelfth_);
    Serial.print("pressure    = ");
    Serial.println(pressure_pa_off_);
    Serial.print("temperature = ");
    Serial.println(temperature_centi_deg_);
    Serial.print("humidity    = ");
    Serial.println(humidity_deci_percent_);
  }
}

void BaroSample::PrettyPrint() {
  if (Serial) {
    Serial.print("UTC unix seconds = ");
    Serial.print(GetTimestamp());
    Serial.print(" s | press = ");
    Serial.print(PressureMilliBar());
    Serial.print(" mBar | temp = ");
    Serial.print(TemperatureDegCelcius());
    Serial.print(" deg C | humi = ");
    Serial.print(HumidityPercent());
    Serial.println(" %");
  }
}
void BaroSample::Inspect(char *data) {
  if (Serial) {
    Serial.print("hourtwelfth = ");
    Serial.println(hour_twelfth_, HEX);
    Serial.print("pressure    = ");
    Serial.println(pressure_pa_off_, HEX);
    Serial.print("temperature = ");
    Serial.println(temperature_centi_deg_, HEX);
    Serial.print("humidity    = ");
    Serial.println(humidity_deci_percent_, HEX);
    Serial.print("  -->  data = ");
    for (uint8_t i = 0; i < kBaroPackedSampleSize; i++) {
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}