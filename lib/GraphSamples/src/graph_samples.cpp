#include "graph_samples.h"

#include "Adafruit_GFX.h"
#include "Fonts/ClearSans-Medium-10pt7b.h"

#include "labels.h"
#include "print_utils.h"

#define USE_LINE

#define FIXED_SCALE 1

#define OUTLIER_MIN 8800
#define OUTLIER_MAX 11000
#define PAPER_MIN 9900
#define PAPER_MAX 10300
#define PAPER_INCR 50

void GraphSamples::Draw(GFXcanvas1 &canvas, uint8_t bg_color, uint8_t fg_color) {
  canvas.fillScreen(bg_color);

  // canvas.drawRect(graph_xstart, graph_ystart, graph_width, -graph_height,
  //                 COLORED);
  // canvas.drawFastVLine(graph_xstart+graph_width, graph_ystart, -graph_height,
  // COLORED);
  // draw the left vertical line of the graph boundary.
  // (all the others boundaries will be draw together with the tick marks)
  canvas.drawFastVLine(graph_xstart, graph_ystart, -graph_height, fg_color);

  label_spec lbl;
  div_t qmin = div(min_, PAPER_INCR);
  uint32_t min = PAPER_INCR*qmin.quot;
  div_t qmax = div(max_+PAPER_INCR, PAPER_INCR);
  uint32_t max = PAPER_INCR*qmax.quot;

#ifndef FIXED_SCALE
  // WARNING: this will only work for PRESSURE (that are multiplied by 10)
  if (max_ - min_  < 40 ) {
    min -= 20;
    max += 20;
  }
  loose_label(min, max, 5, lbl);
#else
  if (min >= PAPER_MIN || min < OUTLIER_MIN) {
    min = PAPER_MIN;
  }
  if (max <= PAPER_MAX || max > OUTLIER_MAX) {
    max = PAPER_MAX;
  }
  lbl.increment = PAPER_INCR;
  lbl.min_label = min;
  lbl.max_label = max;
  lbl.nb_marks = (max-min)/PAPER_INCR + 1;
#endif

  // DEBUG("serie_min", min_);
  // DEBUG("serie_max", max_);
  // DEBUG("set min", min);
  // DEBUG("set max", max);
  // DEBUG("label_min", lbl.min_label);
  // DEBUG("label_max", lbl.max_label);
  // DEBUG("increment", lbl.increment);
  // DEBUG("nb_marks", lbl.nb_marks);

  // uint32_t start = millis();
  char buffer[8];
  canvas.setTextColor(fg_color);
  canvas.setTextSize(1);
  canvas.setTextWrap(false);
  canvas.setFont(&ClearSans_Medium10pt7b);
  int16_t y = graph_ystart;
  int16_t step = graph_height / (lbl.nb_marks - 1);
  int16_t tick = lbl.min_label;
  int16_t x_label = graph_xstart + graph_width;
  for (int16_t i = 0; i < lbl.nb_marks; i++) {
    canvas.drawFastHLine(graph_xstart, y, graph_width + 4, fg_color);
    // canvas.drawFastHLine(x_label, y-1, 4, fg_color);
    sprintf(buffer, "%d", tick / 10);
    // int16_t tx, ty;
    // uint16_t tw, th;
    // canvas.getTextBounds(buffer, 0, 100, &tx, &ty, &tw, &th);
    // PRINTLN(buffer);
    // DEBUG("tx", tx);
    // DEBUG("ty", ty);
    // DEBUG("tw", tw);
    // DEBUG("th", th);
    canvas.setCursor(x_label + 6, y + 4);
    canvas.print(buffer);
    y -= step;
    tick += lbl.increment;
  }
  // uint32_t stop = millis();
  // PRINT("graph marks elapsed (ms) : ");
  // PRINTLN(stop - start);

  uint32_t series_hours = (size_ * period_) / 3600;
  uint8_t major_hours;
  uint8_t nb_ticks;
  uint8_t hours_intervals[] = {1, 3, 6, 12, 24, 36, 48};
  for (uint8_t k = 0; k < sizeof(hours_intervals); k++) {
    major_hours = hours_intervals[k];
    nb_ticks = series_hours / major_hours;
    if (nb_ticks < 6) break;
  }
  int16_t mark_hours = 0;
  int16_t mark_px = graph_xstart + graph_width;
  int16_t period_px = 3600 * major_hours * graph_width / (size_ * period_);
  for (uint8_t t = 0; t <= nb_ticks; t++) {
    canvas.drawFastVLine(mark_px, graph_ystart + 4, -(graph_height + 4),
                         fg_color);
    if (mark_hours == 0) {
      sprintf(buffer, "0");
    } else {
      if (major_hours < 24) {
        sprintf(buffer, "%dh", mark_hours);
      } else {
        sprintf(buffer, "%dd", mark_hours / 24);
      }
    }
    // int16_t tx, ty;
    // uint16_t tw, th;
    // canvas.getTextBounds(buffer, 0, 100, &tx, &ty, &tw, &th);
    canvas.setCursor(mark_px - 6, screen_height - 2);
    canvas.print(buffer);
    mark_hours -= major_hours;
    mark_px -= period_px;
  }

  int16_t y1 = 0;
  for (size_t i = 0; i < size_; i++) {
    int16_t data = Data(i);
#ifdef USE_LINE
    int16_t y2 =
        graph_ystart -
        graph_height * (data - lbl.min_label) / (lbl.max_label - lbl.min_label);
    if (y1 != 0 && data != INT16_MIN) {
      if (y1 < y2) {
        canvas.drawFastVLine(i + graph_xstart, y1 - 1, 2 + (y2 - y1), fg_color);
      } else {
        canvas.drawFastVLine(i + graph_xstart, y2 - 1, 2 + (y1 - y2), fg_color);
      }
    }
    if (data == INT16_MIN) {
      y1 = 0;
    } else {
      y1 = y2;
    }
#else
    if (data != INT16_MIN) {
      int16_t y = 300 - 300 * (data - min) / (max - min);
      canvas.drawFastVLine(i + 50, y, 300, fg_color);
    }
#endif
  }
}
