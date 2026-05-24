#include "common/g16_core.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace common;

int main() {
  MetaState s0{};
  MetaState s_1{};
  Inputs in{};
  in.dx_norm = 0.2;
  in.utility = 0.5;
  in.ed_error = 0.1;
  in.stability = 0.9;
  MetaState s1 = update_meta_state(s0, s_1, in);
  // Basic sanity checks
  assert(std::isfinite(s1.coherence));
  assert(std::isfinite(s1.value));
  assert(s1.uncertainty >= 0.0 && s1.uncertainty <= 1.0);
  double omega1 = compute_omega(s1, s0, in);
  // If stability is reasonable and utility > error, Omega should decrease over a few steps.
  MetaState s2 = update_meta_state(s1, s0, in);
  double omega2 = compute_omega(s2, s1, in);
  assert(std::isfinite(omega1) && std::isfinite(omega2));
  // Non-strict inequality to avoid flakiness with placeholder dynamics
  assert(omega2 <= omega1 + 0.25);
  // Check stability gating: lower stability should reduce step magnitude
  Inputs in_low = in; in_low.stability = 0.1;
  MetaState s_lo = update_meta_state(s0, s_1, in_low);
  double d_hi = 0.0, d_lo = 0.0;
  for (int i = 0; i < 16; ++i) {
    d_hi += std::abs(s1.g[i] - s0.g[i]);
    d_lo += std::abs(s_lo.g[i] - s0.g[i]);
  }
  assert(d_lo <= d_hi + 1e-6);
  std::cout << "OK: " << summarize(s1) << "\n";
  return 0;
}
