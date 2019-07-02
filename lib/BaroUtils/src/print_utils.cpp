
#include "print_utils.h"

void DEBUG(const char *s) {
  if (Serial) Serial.println(s);
}

void DEBUG(const char *s, int32_t x) {
  if (Serial) {
    Serial.print(s);
    Serial.print(" = ");
    Serial.println(x);
  }
}

void DEBUG(const char *s, const char *x) {
  if (Serial) {
    Serial.print(s);
    Serial.print(" = ");
    Serial.println(x);
  }
}

