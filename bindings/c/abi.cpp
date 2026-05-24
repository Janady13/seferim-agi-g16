#include "abi.h"
using namespace common;

extern "C" {

void g16_update(const g16_meta_state_c* s_t,
                const g16_meta_state_c* s_tm1,
                const g16_inputs_c* in_c,
                g16_meta_state_c* out) {
  MetaState st{}, stm1{};
  for (int i = 0; i < 17; ++i) {
    st.g[i] = s_t->g[i];
    stm1.g[i] = s_tm1->g[i];
  }
  st.coherence = s_t->coherence;
  st.value = s_t->value;
  st.uncertainty = s_t->uncertainty;
  stm1.coherence = s_tm1->coherence;
  stm1.value = s_tm1->value;
  stm1.uncertainty = s_tm1->uncertainty;

  Inputs in{};
  in.dx_norm = in_c->dx_norm;
  in.ed_error = in_c->ed_error;
  in.utility = in_c->utility;
  in.stability = in_c->stability;

  MetaState res = update_meta_state(st, stm1, in);
  for (int i = 0; i < 17; ++i) out->g[i] = res.g[i];
  out->coherence = res.coherence;
  out->value = res.value;
  out->uncertainty = res.uncertainty;
}

}
