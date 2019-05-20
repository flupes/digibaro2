#include "graph_samples.h"

#include "Adafruit_GFX.h"
#include "Fonts/ClearSans-Medium-10pt7b.h"

#include "labels.h"
#include "print_utils.h"

#define COLORED 0
#define UNCOLORED 1

#define USE_LINE

bool GraphSamples::Draw(GFXcanvas1 &canvas) {
  canvas.fillScreen(UNCOLORED);

  canvas.drawRect(graph_xstart, graph_ystart, graph_width, -graph_height,
                  COLORED);

  label_spec lbl;
  loose_label(min_, max_, 5, lbl);
  // DEBUG("serie_min", min_);
  // DEBUG("serie_max", max_);
  // DEBUG("label_min", lbl.min_label);
  // DEBUG("label_max", lbl.max_label);
  // DEBUG("increment", lbl.increment);
  // DEBUG("nb_marks", lbl.nb_marks);

  uint32_t start = millis();
  char buffer[8];
  canvas.setTextColor(COLORED);
  canvas.setTextSize(1);
  canvas.setTextWrap(false);
  canvas.setFont(&ClearSans_Medium10pt7b);
  int16_t y = graph_ystart;
  int16_t step = graph_height / (lbl.nb_marks - 1);
  int16_t tick = lbl.min_label;
  for (int16_t i = 0; i < lbl.nb_marks; i++) {
    canvas.drawFastHLine(graph_xstart - 4, y, 4, COLORED);
    sprintf(buffer, "%d", tick / 10);
    int16_t tx, ty;
    uint16_t tw, th;
    canvas.getTextBounds(buffer, 0, 100, &tx, &ty, &tw, &th);
    // PRINTLN(buffer);
    // DEBUG("tx", tx);
    // DEBUG("ty", ty);
    // DEBUG("tw", tw);
    // DEBUG("th", th);
    canvas.setCursor(graph_xstart - 8 - tw, y + th / 2);
    canvas.print(buffer);
    y -= step;
    tick += lbl.increment;
  }
  uint32_t stop = millis();
  PRINT("graph marks elapsed (ms) : ");
  PRINTLN(stop-start);

  int16_t y1 = 0;
  for (size_t i = 0; i < size_; i++) {
    int16_t data = Data(i);
#ifdef USE_LINE
    int16_t y2 =
        graph_ystart -
        graph_height * (data - lbl.min_label) / (lbl.max_label - lbl.min_label);
    if (y1 != 0 && data != INT16_MIN) {
      if (y1 < y2) {
        canvas.drawFastVLine(i + graph_xstart, y1 - 1, 2 + (y2 - y1), COLORED);
      } else {
        canvas.drawFastVLine(i + graph_xstart, y2 - 1, 2 + (y1 - y2), COLORED);
      }
    }
    if (data == INT16_MIN) {
      y1 = 0;
    } else {
      y1 = y2;
    }
#else
    if (data != INT16_MIN) {
      int16_t y = 300 - 300 * (data - min_) / (max_ - min_);
      canvas.drawFastVLine(i + 50, y, 300, COLORED);
    }
#endif
  }
}
