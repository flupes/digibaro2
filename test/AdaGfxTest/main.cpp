// Get the board definitions...
#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

#include "Adafruit_GFX.h"

// #define SET_ADA
// #define SET_FREE_CLEAR_HACK
#define SET_CLEAR_HACK

#ifdef SET_ADA
#include "Fonts/FreeMono12pt7b.h"
#include "Fonts/FreeMono18pt7b.h"
#include "Fonts/FreeMono24pt7b.h"
#include "Fonts/FreeMono9pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSans24pt7b.h"
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include "Fonts/FreeSansBold18pt7b.h"
#include "Fonts/FreeSansBold24pt7b.h"
#include "Fonts/FreeSansBold9pt7b.h"
#include "Fonts/Org_01.h"
#include "Fonts/Picopixel.h"
#endif

#ifdef SET_FREE_CLEAR_HACK
#include "Fonts/FreeSans-Regular-9pt7b.h"
#include "Fonts/FreeSans-Regular-12pt7b.h"
#include "Fonts/FreeSans-Regular-18pt7b.h"
#include "Fonts/FreeSans-Bold-9pt7b.h"
#include "Fonts/FreeSans-Bold-12pt7b.h"
#include "Fonts/FreeSans-Bold-18pt7b.h"
#include "Fonts/FreeSans-Italic-9pt7b.h"
#include "Fonts/FreeSans-Italic-12pt7b.h"
#include "Fonts/FreeSans-Italic-18pt7b.h"
#include "Fonts/ClearSans-Regular-9pt7b.h"
#include "Fonts/ClearSans-Regular-12pt7b.h"
#include "Fonts/ClearSans-Regular-18pt7b.h"
#include "Fonts/ClearSans-Bold-9pt7b.h"
#include "Fonts/ClearSans-Bold-12pt7b.h"
#include "Fonts/ClearSans-Bold-18pt7b.h"
#include "Fonts/ClearSans-Italic-9pt7b.h"
#include "Fonts/ClearSans-Italic-12pt7b.h"
#include "Fonts/ClearSans-Italic-18pt7b.h"
#include "Fonts/Hack-Regular-9pt7b.h"
#include "Fonts/Hack-Regular-12pt7b.h"
#include "Fonts/Hack-Regular-18pt7b.h"
#include "Fonts/Hack-Bold-9pt7b.h"
#include "Fonts/Hack-Bold-12pt7b.h"
#include "Fonts/Hack-Bold-18pt7b.h"
#include "Fonts/Hack-Italic-9pt7b.h"
#include "Fonts/Hack-Italic-12pt7b.h"
#include "Fonts/Hack-Italic-18pt7b.h"
#include "Fonts/ClearSans-Medium-9pt7b.h"
#include "Fonts/ClearSans-Medium-12pt7b.h"
#include "Fonts/ClearSans-Medium-18pt7b.h"
#endif

#ifdef SET_CLEAR_HACK
#include "Fonts/ClearSans-Medium-9pt7b.h"
#include "Fonts/ClearSans-Medium-10pt7b.h"
#include "Fonts/ClearSans-Medium-12pt7b.h"
#include "Fonts/ClearSans-Medium-18pt7b.h"
#include "Fonts/ClearSans-Medium-24pt7b.h"
#include "Fonts/ClearSans-Medium-26pt7b.h"
#include "Fonts/Hack-Bold-9pt7b.h"
#include "Fonts/Hack-Bold-12pt7b.h"
#include "Fonts/Hack-Bold-18pt7b.h"
#include "Fonts/Hack-Bold-24pt7b.h"
#endif

#include "epd4in2.h"

#define COLORED 0
#define UNCOLORED 1

Epd epd;
GFXcanvas1 *canvas;

char short_str[] = "-0123456789.:+";
char long_str[] = "-0123456789. abcdef:+ AOWXYZ (,)";

extern "C" char *sbrk(int i);

int FreeRam() {
  char stack_dummy = 0;
  return &stack_dummy - sbrk(0);
}

uint16_t printText(const GFXfont *font, const char *str, bool restart = false) {
  static int16_t line = 0;

  int16_t x, y;
  uint16_t w, h;

  if ( restart ) {
    line = 0;
  }

  canvas->setFont(font);
  canvas->getTextBounds(str, 0, 100, &x, &y, &w, &h);
  Serial.print("Font Height = ");
  Serial.print(h);
  Serial.print(" / String length = ");
  Serial.println(w);
  line += h + 2;
  canvas->setCursor(4, line);
  canvas->print(str);
  line += 2;
  canvas->drawFastHLine(0, line, 400, COLORED);
  return line;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Init e-Paper...");

  if (epd.Init() != 0) {
    Serial.println("e-Paper init failed");
    return;
  }
  epd.ClearFrame();

  Serial.print("Free RAM before Canvas allocation: ");
  Serial.println(FreeRam());

  canvas = new GFXcanvas1(400, 300);

  Serial.print("Free RAM after Canvas allocation: ");
  Serial.println(FreeRam());

  canvas->fillScreen(UNCOLORED);

  canvas->setTextColor(COLORED);
  canvas->setTextSize(1);
  canvas->setTextWrap(false);

#ifdef SET_ADA
  printText(&FreeMono9pt7b, long_str);
  printText(&FreeMono12pt7b, long_str);
  printText(&FreeMono18pt7b, short_str);
  // printText(&FreeMono24pt7b, short_str);

  printText(&FreeSans9pt7b, long_str);
  printText(&FreeSans12pt7b, long_str);
  printText(&FreeSans18pt7b, short_str);
  printText(&FreeSans24pt7b, short_str);

  printText(&FreeSansBold9pt7b, long_str);
  printText(&FreeSansBold12pt7b, long_str);
  printText(&FreeSansBold18pt7b, short_str);
  // printText(&FreeSansBold24pt7b, short_str);

  printText(&Hack_Bold12pt7b, long_str);

  uint16_t line = printText(&Org_01, long_str);
  canvas->setFont(&Picopixel);
  canvas->setCursor(200, line-1);
  canvas->print(long_str);

  canvas->drawRect(0, 0, 400, 300, COLORED);
  epd.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
  epd.DisplayFrame();

  while (1)
    ;
#endif

#ifdef SET_CLEAR_HACK
  printText(&ClearSans_Medium9pt7b, long_str);
  printText(&ClearSans_Medium10pt7b, long_str);
  printText(&ClearSans_Medium12pt7b, long_str);
  printText(&ClearSans_Medium18pt7b, short_str);
  printText(&ClearSans_Medium24pt7b, short_str);
  printText(&ClearSans_Medium26pt7b, short_str);

  printText(&Hack_Bold9pt7b, long_str);
  printText(&Hack_Bold12pt7b, long_str);
  printText(&Hack_Bold18pt7b, short_str);
  printText(&Hack_Bold24pt7b, short_str);

  // Best selection...
  printText(&ClearSans_Medium10pt7b, "1024-");

  canvas->drawRect(0, 0, 400, 300, COLORED);
  epd.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
  epd.DisplayFrame();

  while (1)
    ;
#endif

}

uint16_t counter = 0;
char str[8];

void loop() {
#ifdef SET_FREE_CLEAR_HACK
  canvas->fillScreen(UNCOLORED);

  printText(&FreeSans_Bold9pt7b, long_str, true);
  printText(&FreeSans_Bold12pt7b, long_str);
  printText(&FreeSans_Bold18pt7b, short_str);

  printText(&ClearSans_Bold9pt7b, long_str);
  printText(&ClearSans_Bold12pt7b, long_str);
  printText(&ClearSans_Bold18pt7b, short_str);

  printText(&ClearSans_Medium9pt7b, long_str);
  printText(&ClearSans_Medium18pt7b, short_str);

  printText(&Hack_Bold9pt7b, long_str);
  printText(&Hack_Bold12pt7b, long_str);
  printText(&Hack_Bold18pt7b, short_str);

  canvas->drawRect(0, 0, 400, 300, COLORED);
  epd.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
  epd.DisplayFrame();
  delay(20 * 1000);

  canvas->fillScreen(UNCOLORED);

  printText(&FreeSans_Regular9pt7b, long_str, true);
  printText(&FreeSans_Regular12pt7b, long_str);
  printText(&FreeSans_Regular18pt7b, short_str);

  printText(&ClearSans_Regular9pt7b, long_str);
  printText(&ClearSans_Regular12pt7b, long_str);
  printText(&ClearSans_Regular18pt7b, short_str);
 
  printText(&ClearSans_Medium9pt7b, long_str);
  printText(&ClearSans_Medium18pt7b, short_str);

  printText(&Hack_Regular9pt7b, long_str);
  printText(&Hack_Regular12pt7b, long_str);
  printText(&Hack_Regular18pt7b, short_str);

  canvas->drawRect(0, 0, 400, 300, COLORED);
  epd.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
  epd.DisplayFrame();
  delay(20 * 1000);

  canvas->fillScreen(UNCOLORED);

  printText(&FreeSans_Italic9pt7b, long_str, true);
  printText(&FreeSans_Italic12pt7b, long_str);
  printText(&FreeSans_Italic18pt7b, short_str);

  printText(&ClearSans_Italic9pt7b, long_str);
  printText(&ClearSans_Italic12pt7b, long_str);
  printText(&ClearSans_Italic18pt7b, short_str);

  printText(&ClearSans_Medium9pt7b, long_str);
  printText(&ClearSans_Medium18pt7b, short_str);

  printText(&Hack_Italic9pt7b, long_str);
  printText(&Hack_Italic12pt7b, long_str);
  printText(&Hack_Italic18pt7b, short_str);

  canvas->drawRect(0, 0, 400, 300, COLORED);
  epd.SetPartialWindow(canvas->getBuffer(), 0, 0, 400, 300);
  epd.DisplayFrame();
  delay(20 * 1000);
#endif

  // sprintf(str, "%04d", counter);
  // Serial.print("Start drawing... ");
  // paint.Clear(UNCOLORED);
  // paint.DrawStringAt(0, 0, str, &Font24, COLORED);
  // epd.SetPartialWindow(paint.GetImage(), 8, 8, paint.GetWidth(),
  //                      paint.GetHeight());
  // epd.DisplayFrame();
  // Serial.println("Frame completed.");

  // counter++;
  // delay(30 * 1000);
}