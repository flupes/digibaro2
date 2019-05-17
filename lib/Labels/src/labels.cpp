/**
 * Adapted from:
 * 
 * Nice Numbers for Graph Labels
 * by Paul Heckbert
 * from "Graphics Gems", Academic Press, 1990
 */

#include <math.h>
#include "labels.h"

/*
 * nicenum: find a "nice" number approximately equal to x.
 * Round the number if round=1, take ceiling if round=0
 */

float nicenum(float x, bool round)
{
  int expv;  /* exponent of x */
  float f;  /* fractional part of x */
  float nf; /* nice, rounded fraction */

  expv = (int)floorf(log10f(x));
  f = x / powf(10., (float)expv); /* between 1 and 10 */
  if (round)
    if (f < 1.5)
      nf = 1.;
    else if (f < 3.)
      nf = 2.;
    else if (f < 7.)
      nf = 5.;
    else
      nf = 10.;
  else if (f <= 1.)
    nf = 1.;
  else if (f <= 2.)
    nf = 2.;
  else if (f <= 5.)
    nf = 5.;
  else
    nf = 10.;
  return nf * powf(10., (float)expv);
}

/*
 * loose_label: demonstrate loose labeling of data range from min to max.
 */
void loose_label(float min, float max, int nb_ticks, label_spec &result) {
  float range;

  /* we expect min!=max */
  range = nicenum(max - min, false);
  result.increment = nicenum(range / (nb_ticks - 1), true);
  result.min_label = floorf(min / result.increment) * result.increment;
  result.max_label = ceilf(max / result.increment) * result.increment;
}
