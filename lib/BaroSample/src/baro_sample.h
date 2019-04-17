#ifndef DIGIBARO_BAROSAMPLE_H
#define DIGIBARO_BAROSAMPLE_H

#include <stdint.h>
#include <cstring>

/**
  BaroSample allows to create a packed sample and decode it back.

  Header
    Timestamp
      Scheme: hour twelfth elapsed since choosen epoch (January 1 2019)
      Resolution: 5 minutes (300s)
      Encoding: unsigned integer on 22 bits
      Max value: ~39.9 years

    Type/Reserved
      2 bits to possible stream encode timezone or define
      other types of samples

  Barometric Pressure
    Scheme: count in Pa from 600
    Resolution: 1 Pa (0.01 hPa = 0.01 mBar)
    Encoding: unsigned integer on 16 bits
    Range: 600 hPa --> 1255 hPa (125500 Pa)

  Temperature
    Scheme: centi degree Celcius (0.01 C)
    Resolution: 0.01 deg
    Encoding: signed integer on 14 bits
    Range: -8192 --> 8192 (-81.9 deg C to 81.9 deg C)

  Humidity
    Scheme: relative humidity in per thousands
    Resolution: 0.1 %
    Encoding: unsigned integer on 10 bits
    Range: 0 --> 1000 (0 to 100%, 102.4% really)

  Total
    Full sample thus can be coded on 8 bytes

  Note: the packed sample memory is not stored with this class to avoid the
  extra
  8 bytes necessary since we want to retain a few thousands of BaroInstance in
  RAM.
  This means the packing / unpacking is done on an externally provided buffer.
  This
  buffer should be of type PackedBaroSample to ensure it size.
*/

const uint8_t kBaroSampleSize = 8;
const uint32_t kPressureOffsetPa = 60000;
const uint32_t kSecondsResolution = 60;
// TODO: change to 300 for final program!
const uint32_t k2019epoch = 1546300800;
// epoch is divisible by 500 (5 minutes)

using PackedBaroSample = char[kBaroSampleSize];

class BaroSample {
 public:
  /*
    Construct a sample from timestamp and measurements.
    @param seconds      timestamp in Unix seconds
    @param pressure     compensated pressure in Pa
    @param temperature  compensated temperature in hundreth of degrees
    @param humidity     relative humidity in hundreth of percent
  */
  BaroSample(uint32_t seconds, uint32_t pressure, int32_t temperature,
             uint32_t humidity, int8_t tz = 0, uint8_t extra = 0)
      : timezone_(tz), reserved_(extra) {
    SetTimeStamp(seconds);
    SetPressure(pressure);
    SetTemperature(temperature);
    SetHumidity(humidity);
  }

  void PackSample(char *data) {
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

  /*
    Construct a sample from an encoded packed sample.
    @param data       pointer to the raw data to process
  */
  BaroSample(char *data) {
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
  }

  bool SetTimeStamp(uint32_t seconds) {
    timestamp_ = seconds;
    if (timestamp_ < k2019epoch) {
      hour_twelfth_ = 0;
      return false;
    }
    hour_twelfth_ = (seconds - k2019epoch) / kSecondsResolution;
    hour_twelfth_ &= 0x3FFFFF;  // forget bits of rank larger than 22!
    return true;
  }

  uint32_t GetTimestamp() { return timestamp_; }

  uint32_t GetTimecount() { return hour_twelfth_; }
  
  bool SetPressure(uint32_t pressure) {
    if (pressure > 125500) {
      pressure_pa_off_ = UINT16_MAX;
      return false;
    }
    pressure_pa_off_ = (uint16_t)(pressure - kPressureOffsetPa);
    return true;
  }

  /*
    Set the pressure
    @param pressure   compensated pressure in millibars
  */
  bool SetPressureMilliBar(float pressure) {
    return SetPressure((uint32_t)(pressure * 100.0));
  }

  bool SetTemperature(int32_t temperature) {
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

  bool SetTemeratureDegCelcius(float temperature) {
    return SetTemperature((int32_t)(temperature * 100.0));
  }

  bool SetHumidity(uint32_t humidity) {
    if (humidity > 10200) {
      humidity_deci_percent_ = 0x3FF;
      return false;
    }
    humidity_deci_percent_ = (uint16_t)(humidity / 10);
    return true;
  }

  bool SetHumidityPercent(float humidity) {
    return SetHumidity((uint32_t)(humidity * 100.0));
  }

  uint32_t GetPressure() {
    return kPressureOffsetPa + (uint32_t)(pressure_pa_off_);
  }

  int32_t GetTemperature() { return (int32_t)(temperature_centi_deg_); }

  uint32_t GetHumidity() { return (uint32_t)(humidity_deci_percent_)*10; }

  float PressureMilliBar() {
    return (float)((uint32_t)(pressure_pa_off_) + kPressureOffsetPa) / 100.0;
  }

  float TemperatureDegCelcius() {
    return (float)(temperature_centi_deg_) / 100.0;
  }

  float HumidityPercent() { return (float)(humidity_deci_percent_) / 10.0; }

  void Print();
  void Inspect(char *data);

 private:
  uint32_t timestamp_;              // 4
  uint32_t hour_twelfth_;           // 4
  uint16_t pressure_pa_off_;        // 2
  uint16_t humidity_deci_percent_;  // 2
  int16_t temperature_centi_deg_;   // 2
  int8_t timezone_;                 // 1
  uint8_t reserved_;                // 1
                                    // Total =  16 bytes
};

#endif
