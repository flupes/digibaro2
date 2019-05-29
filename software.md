# Digibaro Software Overview

All libraries used by digibaro2 are included with the repo, either as simple
directories (if the library is only relevant to digibaro), or as a git submodule
if the library has other application (this is the case for all the external
dependencies).

## Dependencies (External Libraries)

### Adafruit-GFX-Library

Used for all the drawing primitives and the canvas on which to draw.

### epd42

Waveshare code to drive their 4.2in black and white display. The drawing is done
on the Adafruit canvas, that is then pushed to the display.

### BME280_driver

This is the low level code provided by BOSH to interface with the BME280. Added
to this code is also a thin wrapper class to use the sensor Arduino style.

### FastCRC

Provides CRC functionalities

### RTCZero

SAMD21 internal RTC and power management.

### SPIMemory

Access the on-board flash.

## Internal Code and Libraries

### BaroUtils

Some utilities used by most of the code, like a printf dependent on the USB
avaibility.

### BaroSample
Management of data collected from the BME280 sensor (timestamp, serializing,
packing, etc.).

### PermanentSamples

Stores (and read) samples permanently in flash (just a linear succession of
samples).

### RobustFlashIndexes

Indexes on flash with redundancy and CRC checks.

### RotatingSamples

Rotating buffer on of sample on flash. Relies on RobustFlashIndexes.

### DisplaySamples

Creates RAM based timeseries of samples from flash.

### GraphSamples

Draw a graph from the DisplaySamples

### Labels

Compute nice axis label (from Numerical Recipes)

### RTClib

Manage the external DS3231 RTC (custom fork)

