#pragma once
#include <cstdlib>

namespace common {
struct Params {
  double coh_gain{1.0};
  double val_gain{1.0};
  double unc_gain{1.0};
  double gamma_min{0.5}; // stability gate lower bound
  double phi_override{0.0}; // 0 => use default
  // Omega components
  double alpha1{1.0};   // L1 weight
  double alpha2{1.0};   // L2^2 weight
  double lambda{1.0};   // stability penalty weight
};

inline double parse_env_double(const char* k, double def) {
  const char* v = std::getenv(k);
  if (!v) return def;
  try {
    return std::stod(v);
  } catch (...) {
    return def;
  }
}

inline Params get_params() {
  Params p;
  p.coh_gain = parse_env_double("G16_COH_GAIN", 1.0);
  p.val_gain = parse_env_double("G16_VAL_GAIN", 1.0);
  p.unc_gain = parse_env_double("G16_UNC_GAIN", 1.0);
  p.gamma_min = parse_env_double("G16_GAMMA_MIN", 0.5);
  p.phi_override = parse_env_double("G16_PHI", 0.0);
  p.alpha1 = parse_env_double("G16_OMEGA_ALPHA1", 1.0);
  p.alpha2 = parse_env_double("G16_OMEGA_ALPHA2", 1.0);
  p.lambda = parse_env_double("G16_OMEGA_LAMBDA", 1.0);
  return p;
}
} // namespace common
