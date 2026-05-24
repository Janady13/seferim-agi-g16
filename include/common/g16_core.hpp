// SEFERIM AGI G16 - Core interfaces
// Naming convention: WhatItDoes(Equation)-(Family)
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace common {

// Golden ratio constant φ
constexpr double phi() { return 1.6180339887498948482; }

// Ψ(t): compact meta-state vector. Keep small and explicit.
struct MetaState {
  // 16 family contributions + bias
  std::array<double, 17> g; // g[0..15] families, g[16] bias/normalizer
  // coherence + value + uncertainty signals
  double coherence{0.0};
  double value{0.0};
  double uncertainty{1.0};
};

// Inputs at time t (observations, control, external context)
struct Inputs {
  double dx_norm{0.0};     // normalized change magnitude
  double ed_error{0.0};    // desiderata error ||E_D(t) - E_D(t-1)||
  double utility{0.0};     // task utility signal
  double stability{1.0};   // stability constraint (>=0)
};

// Family function interface: f_i(x) -> contribution
struct FamilyFn {
  std::string name;        // e.g. "ContinuousDynamics(x' = F(x,t)) - f1"
  double (*fn)(double x, const Inputs& in); // minimal scalar form
};

// Produce the 16 family functions (lightweight placeholders with clear math labels)
std::vector<FamilyFn> build_families();

// Single-Stack Master Update: Ψ(t+1) = F(Ψ(t), Ψ(t-1), inputs)
MetaState update_meta_state(const MetaState& psi_t,
                            const MetaState& psi_tm1,
                            const Inputs& in);

// Objective: Ω = Surprise + Uncertainty − Value + penalty(stability)
double compute_omega(const MetaState& next,
                     const MetaState& prev,
                     const Inputs& in);

// Encode a short textual tag for logging or bindings.
std::string summarize(const MetaState& s);

} // namespace common
