#pragma once
#include "common/g16_core.hpp"
#include <string>

namespace common {

// Thin 7-layer facade that preserves your design but organizes calls.
struct Signals {
  Inputs in;
};

struct LayerOutputs {
  MetaState psi;
  double omega{0.0};
};

class Pipeline {
 public:
  Pipeline() = default;
  void reset(const MetaState& init = MetaState{}) { s_t_ = init; s_tm1_ = MetaState{}; }

  // Steps through all layers; currently delegates math to update_meta_state.
  LayerOutputs step(const Signals& sig) {
    MetaState next = update_meta_state(s_t_, s_tm1_, sig.in);
    LayerOutputs out;
    out.omega = compute_omega(next, s_t_, sig.in);
    out.psi = next;
    s_tm1_ = s_t_;
    s_t_ = next;
    return out;
  }

  const MetaState& state() const { return s_t_; }

 private:
  MetaState s_t_{};
  MetaState s_tm1_{};
};

} // namespace common

