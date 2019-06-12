#include <stdint.h>

// Configure the watchdog timer with a given period
extern void wdt_configure(uint8_t period);

// Disable the WDT
extern void wdt_disable();

// Reset the watchdog timer (avoid system reset)
extern void wdt_reset();
