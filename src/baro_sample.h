#include <stdint.h>
#include <cstring>

/*
  Coding of weather samples:

  Timestamp
    Scheme: minutes elapsed since choosen epoch (January 1 2019)
    Resolution: minute (60s)
    Encoding: unsigned integer on 24 bits
    Max value: ~31.9 years

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
*/

const uint8_t kBaroSampleSize = 8;
const uint32_t k2019epoch = 1546300800;
const uint32_t kPressureOffsetPa = 60000;

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
  BaroSample(uint32_t seconds, uint32_t pressure, uint32_t temperature,
             uint32_t humidity) {
    SetTimeStamp(seconds);
    SetPressure(pressure);
    SetTemperature(temperature);
    SetHumidity(humidity);
    PackSample();
  }

  /*
    Construct a sample from an encoded packed sample.
    @param data       pointer to the raw data to process
  */
  BaroSample(char *data) {
    memcpy(data_, data, kBaroSampleSize);
    UnpackSample();
  }

  /*
    Fill the given buffer with the raw data of the encoded sample.
    Mostly for debug purpose.
    The method does NOT allocate the buffer, the caller is responsible for this.
    @param buffer   Pointer to a character buffer of at least size kBaroSampleSize
  */
  void GetRawData(char *buffer) {
    memcpy(buffer, data_, kBaroSampleSize);
  }

  bool SetTimeStamp(uint32_t seconds) {
    timestamp_ = seconds;
    if ( timestamp_ < k2019epoch ) {
      minutes_ = 0;
      return false;
    }
    minutes_ = (seconds - k2019epoch) / 60;
    return true;
  }

  bool SetPressure(uint32_t pressure) {
    if (pressure > 125500 ) {
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
    return SetPressure( (uint32_t)(pressure*100.0) );
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
    return SetTemperature( (int32_t)(temperature * 100.0) );
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
    return SetHumidity( (uint32_t)(humidity * 100.0) );
  }

  void PackSample() {
    uint16_t utemp;
    if (temperature_centi_deg_ < 0) {
      utemp = 0x3FFF & (uint16_t)(16384 + temperature_centi_deg_);
      // utemp = ~(temperature_centi_deg_) + 1;
      // Serial.print("utemp = ");
      // Serial.println(utemp, HEX);
    }
    else {
      utemp = temperature_centi_deg_;
    }
    uint8_t temp_msb = (uint8_t)(utemp >> 6);
    uint8_t temp_lsb = (uint8_t)(utemp << 2);
    uint8_t humi_msb = (uint8_t)(humidity_deci_percent_ >> 2);
    uint8_t humi_lsb = (uint8_t)(0x0003 & humidity_deci_percent_);
    memcpy(data_, &minutes_, 3);
    memcpy(data_ + 3, &pressure_pa_off_, 2);
    data_[5] = temp_msb;
    data_[6] = humi_msb;
    data_[7] = temp_lsb | humi_lsb;
  }

  void UnpackSample() {
    minutes_ = 0;
    memcpy(&minutes_, data_, 3);
    memcpy(&pressure_pa_off_, data_ + 3, 2);
    uint8_t temp_msb = data_[5];
    uint8_t temp_lsb = data_[7] & 0xFC;
    uint8_t humi_msb = data_[6];
    uint8_t humi_lsb = data_[7] & 0x03;
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
    if ( utemp < 8192 ) {  
      temperature_centi_deg_ = (int16_t)(utemp);
    }
    else {
      temperature_centi_deg_ = (int16_t)(0xC000 | utemp);
    }
    humidity_deci_percent_ = ((uint16_t)(humi_msb) << 2) | (uint16_t)(humi_lsb);
    timestamp_ = 60 * (uint32_t)(minutes_) + k2019epoch;
  }

  uint32_t GetPressure() {
    return kPressureOffsetPa + (uint32_t)(pressure_pa_off_);
  }

  int32_t GetTemperature() {
    return (int32_t)(temperature_centi_deg_);
  }

  uint32_t GetHumidity() {
    return (uint32_t)(humidity_deci_percent_)*10;
  }

  uint32_t GetTimestamp() {
    return timestamp_;
  }

  float PressureMilliBar() {
    return (float)( (uint32_t)(pressure_pa_off_)+kPressureOffsetPa ) / 100.0;
  }

  float TemperatureDegCelcius() {
    return (float)(temperature_centi_deg_) / 100.0;
  }

  float HumidityPercent() {
    return (float)(humidity_deci_percent_) / 10.0;
  }

  void Print();
  void Inspect();

 private:
  uint32_t timestamp_;
  uint32_t minutes_;
  uint16_t pressure_pa_off_;
  uint16_t humidity_deci_percent_;
  int16_t temperature_centi_deg_;

  PackedBaroSample data_;

};
