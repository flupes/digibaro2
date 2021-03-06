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

const uint8_t kBaroPackedSampleSize = 8;
const uint8_t kBaroObjectSampleSize = 16;
const uint32_t kPressureOffsetPa = 60000;
const uint32_t kMaxRecordablePressure = 125500;
const uint32_t kSecondsResolution =
    60;  // Resolution of the compacted timestamps
const uint32_t kPermanentPeriodSeconds =
    300;  // Interval in seconds between two permanent samples stored in flash

// TODO: change for final program:
//   - kSecondsResolution to 300
//   - kPermanentPeriodSeconds to 3600

const uint32_t k2019epoch = 1546300800;  // Offset for compacted timestamps
// epoch is divisible by 500 (5 minutes)

using PackedBaroSample = char[kBaroPackedSampleSize];
using SerializedBaroSample = char[kBaroObjectSampleSize];

class BaroSample {
 public:
  /*
    Construct a sample from timestamp and measurements.
    @param seconds      timestamp in Unix seconds
    @param pressure     compensated pressure in Pa
    @param temperature  compensated temperature in hundreth of degrees
    @param humidity     relative humidity in hundreth of percent

    The units of the input parameters match the units used by the
    BME280 sensor. However, internally, different units are used
    to optimize the data structure size.
  */
  BaroSample(uint32_t seconds, uint32_t pressure, int32_t temperature,
             uint32_t humidity, int16_t elevation = 0, int8_t tz = 0,
             uint8_t extra = 0) {
    Set(seconds, pressure, temperature, humidity, elevation, tz, extra);
  }

  /*
    Default constructor: build a dummy sample
    */
  BaroSample() { Set(k2019epoch, 0, -8192, 0, 0, 0, 0xFF); }

  void Set(uint32_t seconds, uint32_t pressure, int32_t temperature,
           uint32_t humidity, int16_t elevation = 0, int8_t tz = 0,
           uint8_t extra = 0) {
    SetTimeStamp(seconds);
    SetPressure(pressure);
    SetTemperature(temperature);
    SetHumidity(humidity);
    if (elevation != 0) {
      SetElevation(elevation);
    }
    timezone_ = tz;
    reserved_ = extra;
  }

  /*
    Serialiaze a baro sample as is (16 bytes)
  */
  void SerializeSample(char *data);

  /*
    Pack a baro sample on 8 bytes only
  */
  void PackSample(char *data);

  /*
    Construct a sample from either an encoded packed sample or just serialized
    data members
    @param data       pointer to the raw data to process
    @param packed     does the data contain a packed or serialized sample
  */
  BaroSample(char *data, bool packed = true);

  bool operator==(BaroSample &right);

  bool operator!=(BaroSample &right);

  bool SetTimeStamp(uint32_t seconds);

  bool SetPressure(uint32_t pressure);

  bool SetTemperature(int32_t temperature);

  bool SetHumidity(uint32_t humidity);

  /**
   * Modify the internally stored barometric pressure to normalize it at mean
   * sea level in function of the given elevation (in meters).
  */
  uint32_t SetElevation(int16_t elevation) {
    SetPressure(
        SeaLevelPressure(GetPressure(), elevation, temperature_centi_deg_));
    return GetPressure();
  }

  /**
   * Modify the internally stored barometric pressure with the given offset
   * (in hPa).
   */
  uint32_t SetCalibration(int16_t pressure_calibration);

  uint32_t GetTimestamp() { return timestamp_; }

  uint32_t GetTimecount() { return hour_twelfth_; }

  /**
   * Set the pressure
   * @param pressure    compensated pressure in millibars
  */
  bool SetPressureMilliBar(float pressure) {
    return SetPressure((uint32_t)(pressure * 100.0));
  }

  /**
   * Set the temperature
   * @param temperature   temperature in degrees celcius
   */
  bool SetTemeratureDegCelcius(float temperature) {
    return SetTemperature((int32_t)(temperature * 100.0));
  }

  /**
   * Set the humidity
   * @param humidity      relative humidity as a percentage (0..1)
   */
  bool SetHumidityPercent(float humidity) {
    return SetHumidity((uint32_t)(humidity * 100.0));
  }

  /** Returns the pressure in Pa. */
  uint32_t GetPressure() {
    return kPressureOffsetPa + (uint32_t)(pressure_pa_off_);
  }

  /** Returns the temperature in hundreds of degree celcius. */
  int32_t GetTemperature() { return (int32_t)(temperature_centi_deg_); }

  /** Return the humidity in hundreds of percent. */
  uint32_t GetHumidity() { return (uint32_t)(humidity_deci_percent_)*10; }

  float PressureMilliBar() {
    return (float)((uint32_t)(pressure_pa_off_) + kPressureOffsetPa) / 100.0;
  }

  float TemperatureDegCelcius() {
    return (float)(temperature_centi_deg_) / 100.0;
  }

  float HumidityPercent() { return (float)(humidity_deci_percent_) / 10.0; }

  void Print();
  void PrettyPrint();
  void Inspect(char *data);

  /**
   * Computes the mean sea level pressure (in hPa) from the given station
   * pressure (in hPa), elevation (in meters) and temperature (1/100 degress)
   */
  static uint32_t SeaLevelPressure(uint32_t station_pressure, int16_t elevation,
                                   int16_t temperature);

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
