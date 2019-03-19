#include <stdint.h>
#include <cstring>

const uint8_t kBaroSampleSize = 8;
const uint32_t k2019epoch = 1546300800;

/*
struct SampleHeader {
  uint32_t timestamp;  // 4 bytes
  uint8_t type;        // 1 byte
  uint8_t size;        // 1 byte
  uint8_t crc;         // 1 byte
};                     // 7 bytes

struct BaroData {
  SampleHeader header;  // 7 bytes
  uint16_t pressure;    // 2 bytes  0.02 hPa (data*50 --> hPa)
  int8_t temperature;   // 1 byte 0.01 degree C, signed (coded on 14 bits total)
  uint8_t humidity;     // 1 byte   0.1 % (coded on 10 bits total)
  uint8_t extra;  // 1 byte       6 bits for temperature + 2 bits for humidity
};                // 12 bytes
*/

using PackedBaroSample = char[8];

class BaroSample {
 public:
  BaroSample(uint32_t seconds, uint32_t pressure, uint32_t temperature,
             uint32_t humidity) {
    SetTimeStamp(seconds);
    SetPressure(pressure);
    SetTemperature(temperature);
    SetHumidity(humidity);
    PackSample();
  }

  BaroSample(char *data) {
      memcpy(data_, data, kBaroSampleSize);
      UnpackSample();
  }

  bool SetTimeStamp(uint32_t seconds) {
    timestamp_ = seconds;
    minutes_ = (seconds - k2019epoch) / 60;
  }

  bool SetPressure(uint32_t pressure) {
    if (pressure > 131000) {
      pressure_0_02_hpa_ = UINT16_MAX;
      return false;
    }
    pressure_0_02_hpa_ = (uint16_t)(pressure / 50);
    return true;
  }

  bool SetTemperature(int32_t temperature) {
    bool overflow = false;
    if (temperature < -8190) {
      temperature = -8190;
      overflow = true;
    }
    if (temperature > 8190) {
      temperature = 8190;
      overflow = true;
    }
    temperature_centi_deg_ = (uint16_t)(temperature);
    return !overflow;
  }

  bool SetHumidity(uint32_t humidity) {
    if (humidity > 10200) {
      humidity_deci_percent_ = 0x3FF;
      return false;
    }
    humidity_deci_percent_ = (uint16_t)(humidity / 10);
    return true;
  }

  void PackSample() {
      uint8_t temp_msb = (uint8_t)(temperature_centi_deg_ >> 6);
      uint8_t temp_lsb = (uint8_t)(temperature_centi_deg_ << 2);
      uint8_t humi_msb = (uint8_t)(humidity_deci_percent_ >> 2);
      uint8_t humi_lsb = (uint8_t)(0x0003 & humidity_deci_percent_);
      memcpy(data_, &minutes_, 3);
      memcpy(data_+3, &pressure_0_02_hpa_, 2);
      data_[5] = temp_msb;
      data_[6] = humi_msb;
      data_[7] = temp_lsb | humi_lsb;
  }

  void UnpackSample() {
      minutes_ = 0;
      memcpy(&minutes_, data_, 3);
      memcpy(&pressure_0_02_hpa_, data_+3, 2);
      uint8_t temp_msb = data_[5];
      uint8_t temp_lsb = data_[7] & 0xFC;
      uint8_t humi_msb = data_[6];
      uint8_t humi_lsb = data_[7] & 0x03;
      Serial.print("temp_msb = ");
      Serial.print(temp_msb, HEX);
      Serial.print(" << 6 ");
      Serial.println((uint16_t)(temp_msb) << 6, HEX);
      Serial.print("temp_lsb = ");
      Serial.print(temp_lsb, HEX);
      Serial.print(" >> 2 ");
      Serial.println((uint16_t)(temp_lsb >> 2), HEX);
      temperature_centi_deg_ = (uint16_t)( ((uint16_t)(temp_msb) << 6) | (uint16_t)(temp_lsb >> 2) );
      humidity_deci_percent_ = ((uint16_t)(humi_msb) << 2) | (uint16_t)(humi_lsb);
      timestamp_ = 60 * (uint32_t)(minutes_) + k2019epoch;
  }

    void Print();
    void Inspect();

  PackedBaroSample data_;
 
 private:
  uint32_t timestamp_;
  uint32_t minutes_;
  uint16_t pressure_0_02_hpa_;
  uint16_t humidity_deci_percent_;
  int16_t temperature_centi_deg_;


//   BaroData sample_;
};
