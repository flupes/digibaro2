#include <stdint.h>

struct SampleHeader {
  uint32_t timestamp;  // 4 bytes
  uint8_t type;        // 1 byte
  uint8_t size;        // 1 byte
  uint8_t crc;         // 1 byte
};                     // 7 bytes

struct BaroData {
  SampleHeader header;        // 7 bytes
  uint16_t pressure;   // 2 bytes  0.02 hPa (data*50 --> hPa)
  int8_t temperature;  // 1 byte 0.01 degree C, signed (coded on 14 bits total)
  uint8_t humidity;    // 1 byte   0.1 % (coded on 10 bits total)
  uint8_t extra;  // 1 byte       6 bits for temperature + 2 bits for humidity
};                // 12 bytes

class BaroSample {
 public:
  BaroSample(uint32_t pressure, uint32_t temperature, uint32_t humidity) {
    SetPressure(pressure);
    SetTemperature(temperature);
    SetHumidity(humidity);
  }

  BaroSample(const BaroData &data) {
      comp_pressure_ = 50 * (uint32_t)(data.pressure);
      comp_temperature_ = (int16_t)(data.temperature) << 6;
      comp_temperature_ &= (int16_t)(data.extra >> 2);
      rel_humidity_ = (uint16_t)(data.humidity) << 2;
      rel_humidity_ &= (uint16_t)(data.extra & 0x03);
      rel_humidity_ *= 10;
  }

  bool SetPressure(uint32_t pressure) {
    comp_pressure_ = pressure;
    if (pressure > 120000) {
      sample_.pressure = UINT16_MAX;
      return false;
    }
    sample_.pressure = (uint16_t)(pressure / 50);
    return true;
  }

  bool SetTemperature(int32_t temperature) {
    comp_temperature_ = temperature;
    int16_t reduced_temp;
    bool overflow = false;
    if (temperature < -8100) {
      reduced_temp = 0xFFFF;
      overflow = true;
    }
    if (temperature > 8100) {
      // TODO Figure out where the sign bit goes to actually
      // construct a max negative value on 14 bits!
      reduced_temp = 0xFFFF;
      overflow = true;
    }
    reduced_temp = (int16_t)(temperature);
    sample_.temperature = (reduced_temp >> 6);
    sample_.extra &= 0x03;
    sample_.extra |= (uint8_t)(reduced_temp << 2);
    return !overflow;
  }

  bool SetHumidity(uint32_t humidity) {
    rel_humidity_ = humidity;
    if (humidity > 10000) {
      sample_.humidity = 0xFF;
      sample_.extra &= 0xFC;
      return false;
    }
    uint16_t reduced_hum = (uint16_t)(humidity / 10);
    sample_.humidity = (uint8_t)(reduced_hum >> 6);
    sample_.extra &= 0xFC;
    sample_.extra |= (uint8_t)(reduced_hum)&0x03;
    return true;
  }

 private:
  uint32_t comp_pressure_;
  int32_t comp_temperature_;
  uint32_t rel_humidity_;
  BaroData sample_;
};
