/* Compile with:
  pio ci .\test --board=zeroUSB -l src -l ../BaroUtils -O "targets=upload"
*/

#include "labels.h"

#include "print_utils.h"

const int NBTESTS = 12;

float inputs[NBTESTS][2] = {{7., 17.},      {-17., -7.},   {-3., 16.},
                            {-3., 18.},     {961., 977.},  {1023., 1027.},
                            {1023., 1033.}, {999., 1010.}, {999., 1011.},
                            {999., 1019.},  {999., 1021.}, {989., 1041.}};

float outputs[NBTESTS][3] = {
    {6., 18., 2.},      {-18., -6., 2.},    {-5., 20., 5.},
    {-10., 20., 10.},   {960., 980., 5.},   {1023., 1027., 1.},
    {1022., 1034., 2.}, {995., 1010., 5.},  {995., 1015., 5.},
    {995., 1020., 5.}, {990., 1030., 10.}, {980., 1060., 20.}};

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Starting labels_test");
  int errors = 0;
  const float EPSILON = 1E-6;
  label_spec result;
  for (int i = 0; i < NBTESTS; i++) {
    loose_label(inputs[i][0], inputs[i][1], 5, result);
    if (fabs(result.min_label - outputs[i][0]) > EPSILON ||
        fabs(result.max_label - outputs[i][1]) > EPSILON ||
        fabs(result.increment - outputs[i][2]) > EPSILON) {
      Serial.print("Error for test #");
      Serial.print(i+1);
      Serial.print(" serie_min=");
      Serial.print(inputs[i][0]);
      Serial.print(" , serie_max=");
      Serial.print(inputs[i][1]);
      Serial.print(" --> min_label=");
      Serial.print(outputs[i][0]);
      Serial.print(", max_label=");
      Serial.print(outputs[i][1]);
      Serial.print(", increment=");
      Serial.println(outputs[i][2]);
      errors++;
    }
  }
  if (errors > 0) {
    Serial.print("FAILED (errors=");
    Serial.print(errors);
    Serial.println(")!");
  } else {
    Serial.println("PASSED.");
  }
}

void loop() {}