struct label_spec {
  float min_label;
  float max_label;
  float increment;
  int nb_marks;
};

void loose_label(float serie_min, float serie_max, int nb_ticks,
                 label_spec &result);
                 