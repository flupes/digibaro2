# System level tests

This directory contains a set of tests that require multiple libraries at once,
or some tests or utilities not specific to a library in particular.

## DipSwitchSleep

Check that the DIP switch wiring and board configuration does not consume
current in sleep mode.

## ErasePermanentSamples

Simply wipes out the existing "permanent" samples stored in flash, using the
default memory map.

## ExtInterrupt

Shows how a SPDT switch can be wired to consume not current and wake up the
SAMD21 from sleep using an interrupt.

## PackedBitField

Verifies how the compiler packs data structures and bit fields on the 32 bits
SAMD21 processor.

## ReadFlashSamples

Just reads all the "permanent" samples from flash and print them out.

## RecordData

Badly named, obsolete test. Was showing how to reliably read from flash using
a reduncdancy of 2 plus a CRC.

## SleepTest

Demonstrate how to put to sleep the SAMD21 connected to all its devices (RTC,
Display and BME280). The measurements show consumption of ~25uA if all the pins
are correctly configured and all the un-necessary devices disconnected.

## TestBme280Sensor

Simple test of the BME280_Driver library and associated class.

