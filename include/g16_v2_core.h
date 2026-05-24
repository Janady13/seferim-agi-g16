#pragma once
#include "g16_v2_types.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

namespace g16v2 {

// Fibonacci utilities
inline bool is_fibonacci_tick(uint64_t t) {
  for (size_t i = 0; i < FIB_CUMULATIVE.size(); ++i) {
    if (FIB_CUMULATIVE[i] == t) return true;
    if (FIB_CUMULATIVE[i] > t) return false;
  }
  return false;
}
inline size_t get_fibonacci_index(uint64_t t) {
  for (size_t i = 0; i < FIB_CUMULATIVE.size(); ++i) {
    if (FIB_CUMULATIVE[i] >= t) return i;
  }
  return FIB_CUMULATIVE.size() - 1;
}
inline uint64_t ticks_until_next_fib(uint64_t t) {
  for (size_t i = 0; i < FIB_CUMULATIVE.size(); ++i) {
    if (FIB_CUMULATIVE[i] > t) return FIB_CUMULATIVE[i] - t;
  }
  return 0;
}

// Golden spiral
inline void init_rotation(GoldenRotation& R) { R.init(); }

inline void apply_spiral_rotation(
  Psi& psi,
  const GoldenRotation& /*R*/,
  uint64_t t,
  const Config& cfg
) {
  const auto& planes = cfg.use_semantic_planes ? SEMANTIC_PLANES : PLANE_FAMILIES;
  for (int k = 0; k < NUM_PLANES; ++k) {
    int f1 = planes[k].first;
    int f2 = planes[k].second;
    double theta = OMEGA[k] * static_cast<double>(t);
    double c = std::cos(theta);
    double s = std::sin(theta);
    double x1 = psi.x[f1];
    double x2 = psi.x[f2];
    psi.x[f1] = c * x1 - s * x2;
    psi.x[f2] = s * x1 + c * x2;
  }
}

inline Psi get_spiral_direction(uint64_t t, const Config& cfg) {
  Psi dir;
  for (int i = 0; i < NUM_FAMILIES; ++i) dir.x[i] = 0.0;
  dir.x[0] = 1.0;
  GoldenRotation R;
  init_rotation(R);
  apply_spiral_rotation(dir, R, t, cfg);
  return dir;
}

// Coupling
inline void build_golden_coupling_matrix(CouplingMatrix& J, const Config& cfg) {
  for (int f = 0; f < NUM_FAMILIES; ++f)
    for (int g = 0; g < NUM_FAMILIES; ++g)
      J.J[f][g] = 0.0;
  const auto& planes = cfg.use_semantic_planes ? SEMANTIC_PLANES : PLANE_FAMILIES;
  for (int k = 0; k < NUM_PLANES; ++k) {
    int f1 = planes[k].first;
    int f2 = planes[k].second;
    double lam = LAMBDA[k];
    J.J[f1][f1] += lam * 0.5;
    J.J[f2][f2] += lam * 0.5;
    J.J[f1][f2] += lam;
    J.J[f2][f1] += lam;
  }
  double max_val = 0.0;
  for (int f = 0; f < NUM_FAMILIES; ++f)
    for (int g = 0; g < NUM_FAMILIES; ++g)
      max_val = std::max(max_val, std::abs(J.J[f][g]));
  if (max_val > 0) {
    double scale = 1.0 / max_val;
    for (int f = 0; f < NUM_FAMILIES; ++f)
      for (int g = 0; g < NUM_FAMILIES; ++g)
        J.J[f][g] *= scale;
  }
}

inline double coupling_force(int f, const Phi& phi, const CouplingMatrix& J) {
  double sum = 0.0;
  for (int g = 0; g < NUM_FAMILIES; ++g) sum += J.get(f, g) * phi.phi[g];
  return sum;
}

// Norm, gating, uncertainty
inline double compute_b(const Psi& psi) {
  return 1.0 / (1.0 + psi.norm_sq() / PHI);
}
inline double compute_gamma(double stability, double gamma_min) {
  return gamma_min + (1.0 - gamma_min) * stability;
}
inline double compute_uncertainty(double b) { return 1.0 - b * b; }

// Objective
inline Omega compute_omega(
  const Psi& psi,
  const Psi& psi_prev,
  const Signals& signals,
  const std::array<double, NUM_FAMILIES>& w,
  double b,
  const Config& cfg
) {
  Omega omega{};
  Psi delta{};
  for (int i = 0; i < NUM_FAMILIES; ++i) delta.x[i] = psi.x[i] - psi_prev.x[i];
  double l1 = 0.0;
  for (int i = 0; i < NUM_FAMILIES; ++i) l1 += std::abs(delta.x[i]);
  double l2_sq = delta.norm_sq();
  omega.surprise = cfg.alpha1 * l1 + cfg.alpha2 * l2_sq;
  omega.uncertainty = compute_uncertainty(b);
  double w_dot_psi = 0.0;
  for (int i = 0; i < NUM_FAMILIES; ++i) w_dot_psi += w[i] * psi.x[i];
  omega.value = signals.utility * w_dot_psi;
  omega.penalty = cfg.lambda * signals.dx_norm * signals.dx_norm;
  omega.total = omega.surprise + omega.uncertainty - omega.value + omega.penalty;
  return omega;
}

inline double compute_kappa(
  const Psi& psi,
  const Signals& signals,
  const std::array<double, NUM_FAMILIES>& w,
  double b
) {
  double w_dot_psi = 0.0;
  for (int i = 0; i < NUM_FAMILIES; ++i) w_dot_psi += w[i] * psi.x[i];
  double value = signals.utility * w_dot_psi;
  double uncertainty = 1.0 - b * b;
  return value - uncertainty;
}

inline void compute_learning_rates(
  std::array<double, NUM_FAMILIES>& eta,
  double kappa,
  const std::array<double, NUM_FAMILIES>& w,
  const Phi& phi,
  const CouplingMatrix& J,
  const Config& cfg
) {
  double kappa_pos = std::max(0.0, kappa);
  for (int f = 0; f < NUM_FAMILIES; ++f) {
    double coupling = coupling_force(f, phi, J);
    double effective_weight = w[f] + coupling;
    double e = cfg.lr_base + cfg.lr_gain * kappa_pos * effective_weight;
    if (e < 0.001) e = 0.001;
    if (e > 1.0) e = 1.0;
    eta[f] = e;
  }
}

inline void update_psi(
  SystemState& state,
  const Signals& signals,
  const CouplingMatrix& J,
  const Config& cfg
) {
  state.psi_prev = state.psi;
  state.b = compute_b(state.psi);
  state.gamma = compute_gamma(signals.stability, cfg.gamma_min);

  Psi u{};
  for (int f = 0; f < NUM_FAMILIES; ++f) {
    u.x[f] = cfg.w[f] * (signals.utility * 0.4 +
                         (1.0 - signals.ed_error) * 0.3 +
                         signals.stability * 0.3);
    if (cfg.use_cross_family_resonance) {
      u.x[f] += coupling_force(f, state.phi, J) * 0.1;
    }
  }
  double u_norm = u.norm();
  if (u_norm < 1e-10) u_norm = 1e-10;

  Psi update_dir{};
  if (cfg.use_golden_spiral) {
    Psi spiral_dir = get_spiral_direction(state.tick, cfg);
    for (int f = 0; f < NUM_FAMILIES; ++f) update_dir.x[f] = u_norm * spiral_dir.x[f];
  } else {
    update_dir = u;
  }

  for (int f = 0; f < NUM_FAMILIES; ++f) {
    state.psi.x[f] = state.b * (state.gamma * state.psi.x[f] +
                                (1.0 - state.gamma) * update_dir.x[f]);
  }

  state.signals = signals;
  state.omega = compute_omega(state.psi, state.psi_prev, signals, cfg.w, state.b, cfg);
  state.kappa = compute_kappa(state.psi, signals, cfg.w, state.b);
  state.phi.global = 1.0 / (1.0 + std::abs(state.omega.total));
  compute_learning_rates(state.learning_rates, state.kappa, cfg.w, state.phi, J, cfg);
  state.tick++;
}

inline bool should_learn(const SystemState& state, const Config& cfg) {
  if (state.kappa < cfg.ingest_gate) return false;
  if (state.items_ingested >= static_cast<uint64_t>(cfg.ingest_max)) return false;
  if (cfg.use_fibonacci_timing) {
    return is_fibonacci_tick(state.tick);
  }
  return true;
}

inline void update_phi_from_federation(
  Phi& phi,
  const std::array<double, NUM_FAMILIES>& family_coherences
) {
  for (int f = 0; f < NUM_FAMILIES; ++f) phi.phi[f] = family_coherences[f];
}

inline void init_state(SystemState& state) {
  for (int f = 0; f < NUM_FAMILIES; ++f) {
    state.psi.x[f] = 0.0;
    state.psi_prev.x[f] = 0.0;
    state.phi.phi[f] = 0.5;
    state.learning_rates[f] = 0.05;
  }
  state.phi.global = 0.5;
  state.b = 1.0;
  state.kappa = 0.0;
  state.gamma = 0.5;
  state.tick = 0;
  state.fib_index = 0;
  state.items_ingested = 0;
  state.signals = {0.0, 0.5, 0.5, 1.0};
  state.omega = {0.0, 0.0, 0.0, 0.0, 0.0};
}

} // namespace g16v2

