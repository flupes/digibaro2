#include <Arduino.h>

#include "baro_sample.h"

// We use a Zero compatible board
#define Serial SerialUSB

void BaroSample::Print() {
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

void BaroSample::Inspect(char *data) {
    Serial.print("hourtwelfth = ");
    Serial.println(hour_twelfth_, HEX);
    Serial.print("pressure    = ");
    Serial.println(pressure_pa_off_, HEX);
    Serial.print("temperature = ");
    Serial.println(temperature_centi_deg_, HEX);
    Serial.print("humidity    = ");
    Serial.println(humidity_deci_percent_, HEX);
    Serial.print("  -->  data = ");
    for (uint8_t i=0; i<kBaroSampleSize; i++) {
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}