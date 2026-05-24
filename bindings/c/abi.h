#pragma once
#include "common/g16_core.hpp"

extern "C" {
  typedef struct {
    double g[17];
    double coherence;
    double value;
    double uncertainty;
  } g16_meta_state_c;

  typedef struct {
    double dx_norm;
    double ed_error;
    double utility;
    double stability;
  } g16_inputs_c;

  void g16_update(const g16_meta_state_c* s_t,
                  const g16_meta_state_c* s_tm1,
                  const g16_inputs_c* in,
                  g16_meta_state_c* out);
}

