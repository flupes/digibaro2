## Platform

The code in this repo is specifically designed for an Arduino "zeroUSB"
compatible using a **ARM Cortex M0+**. The board used for my project is a
[Mini Ultra Pro v2](https://www.rocketscream.com/blog/docs-item/mini-ultra-pro-hookup-guide/)
from "Rocket Scream". The board was choosen because it has been designed for low
power application, and it already has a 2MB on-board flash. The Cortex M0 uses a
`SAMD21` which is a 32-bit microcontroller with 32KB or RAM. So there is very
little optimization to use types smaller than the native **word** `uint32_t` or
`int32_t`.

## Code organization

Most of the code resides in individuals libraries under `lib`. This allows for
much easier unit testing, at the cost of a lot of folders to deal with.

## General concepts

Class `BaroSample` holds information collected from the BME280 sensor, in a
slightly more compact form, using uint16_t rather than uint32_t. The class
also allows serialization and de-serialization to flash memory.

BaroSamples are written two flash using two classes:

  - `PermanentSamples` stores low frequency (typically hourly) samples, until
    the flash memory is full (~29 years of data!). These samples are not
    supposed to be deleted or written over.
  - `RotatingSamples` holds higher frequency samples (typically every 5
    minutes). This is a circular buffer, with samples being overwritten when the
    buffer is full. The length of this circular buffer ensure that the flash
    memory will not be subject to too many cycles.
    This class is supported by `RobustFlashIndexes` which maintains which is
    the last index of the circular buffer. The use of this separate circular
    buffer allows extra safety in maintaining the indexes because of the 
    redundancy and CRC offered by RobustFlashIndexes.

## Tests



