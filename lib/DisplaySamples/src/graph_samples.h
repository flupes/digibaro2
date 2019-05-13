#ifndef DIGI_GRAPH_SAMPLES_H
#define DIGI_GRAPH_SAMPLES_H

#include "display_samples.h"

class GFXcanvas1;

class GraphSamples : public DisplaySamples {
 public:
  GraphSamples(uint32_t period_in_seconds, uint32_t length = kGraphPxLength)
      : DisplaySamples(period_in_seconds, length) {}

  bool Draw(GFXcanvas1 &canvas);
};

#endif
