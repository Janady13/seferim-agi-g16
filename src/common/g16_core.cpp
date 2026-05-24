#include "common/g16_core.hpp"
#include "common/weights.hpp"
#include "common/params.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace common {

// Helpers
static inline double clamp01(double x) { return std::max(0.0, std::min(1.0, x)); }
static inline double softsign(double x) { return x / (1.0 + std::abs(x)); }
static inline double smooth_norm(double a, double b) { return std::sqrt(a*a + b*b); }

// Family definitions (placeholders with interpretable math)
// Each family maps an activation 'x' to a contribution, conditioned by inputs.
// Names follow: What it does (equation)-(family)
std::vector<FamilyFn> build_families() {
  std::vector<FamilyFn> f;
  f.reserve(16);
  // f1: Continuous Dynamics: x' = F(x,t)
  f.push_back({"ContinuousDynamics(x' = F(x,t))-f1", [](double x, const Inputs& in){
    return x + 0.1 * (in.dx_norm - x);
  }});
  // f2: Discrete Dynamics: x_{t+1} = F(x_t, u_t^e)
  f.push_back({"DiscreteDynamics(x_{t+1}=F(x_t,u))-f2", [](double x, const Inputs& in){
    return 0.9 * x + 0.1 * in.utility;
  }});
  // f3: Reaction-Diffusion: 6δ/δt = D∇^2x + f(x)
  f.push_back({"ReactionDiffusion(D∇^2x+f(x))-f3", [](double x, const Inputs& in){
    return x + 0.05 * (softsign(in.dx_norm) - x);
  }});
  // f4: Optimization: x* = arg max U(x)
  f.push_back({"Optimization(argmax U(x))-f4", [](double x, const Inputs& in){
    return x + 0.2 * softsign(in.utility - x);
  }});
  // f5: Bayesian Inference: P(θ|D) ∝ P(D|θ)P(θ)
  f.push_back({"BayesianInference(P(θ|D)∝...)-f5", [](double x, const Inputs& in){
    double post = (x + in.utility) / (1.0 + std::abs(in.utility));
    return clamp01(post);
  }});
  // f6: Learning / Weight Update: W_{t+1} = W_t + ΔW
  f.push_back({"LearningWeightUpdate(ΔW)-f6", [](double x, const Inputs& in){
    return x + 0.1 * in.dx_norm;
  }});
  // f7: Mutual Information: I(X;Y)
  f.push_back({"MutualInformation(I(X;Y))-f7", [](double x, const Inputs& in){
    double s = 1.0 / (1.0 + std::exp(-in.dx_norm));
    return 0.5 * (x + s);
  }});
  // f8: Rate-Distortion: min I(X;Z) - β I(Z;Y)
  f.push_back({"RateDistortion(min I-βI)-f8", [](double x, const Inputs& in){
    return x - 0.05 * in.ed_error + 0.05 * in.utility;
  }});
  // f9: Consensus / Averaging: x_i(t+1)=x_i(t)+Σ(x_j-x_i)
  f.push_back({"ConsensusAveraging(Σ(xj-xi))-f9", [](double x, const Inputs& in){
    return 0.95 * x + 0.05 * in.dx_norm;
  }});
  // f10: Swarm Interaction Force: x_i = Σ f(x_j - x_i)
  f.push_back({"SwarmInteractionForce(Σf)-f10", [](double x, const Inputs& in){
    return x + 0.05 * softsign(in.dx_norm - x);
  }});
  // f11: Field-Based Accumulator: φ_t = (1-ρ)φ_t + Δφ
  f.push_back({"FieldAccumulator(φ_t=(1-ρ)φ_t+Δφ)-f11", [](double x, const Inputs& in){
    double rho = 1.0 / (1.0 + phi());
    return (1.0 - rho) * x + rho * in.dx_norm;
  }});
  // f12: Variational / Free Energy: F = E[ln q(z)-ln p(x,z)]
  f.push_back({"VariationalFreeEnergy(F)-f12", [](double x, const Inputs& in){
    return x - 0.05 * std::abs(in.dx_norm) + 0.05 * in.utility;
  }});
  // f13: Compression / Rate Control (proxy)
  f.push_back({"CompressionControl(C)-f13", [](double x, const Inputs& in){
    return x - 0.05 * std::abs(in.dx_norm);
  }});
  // f14: Memory Trace / Zecher: x'_i = Σ f_i : dΩ
  f.push_back({"MemoryTrace(Σ f_i dΩ)-f14", [](double x, const Inputs& in){
    return x + 0.02 * (in.dx_norm + in.utility);
  }});
  // f15: Deecisive Action / Policy
  f.push_back({"DeecisiveAction(Policy)-f15", [](double x, const Inputs& in){
    return x + 0.15 * softsign(in.utility) - 0.05 * in.ed_error;
  }});
  // f16: Global Coherence / Kehillah φ-accumulator
  f.push_back({"GlobalCoherence(φ-accumulator)-f16", [](double x, const Inputs& in){
    return x + (phi() - 1.0) * 0.01 * (in.stability - x);
  }});
  return f;
}

MetaState update_meta_state(const MetaState& psi_t,
                            const MetaState& psi_tm1,
                            const Inputs& in) {
  MetaState next = psi_t;
  static auto params = get_params();

  // Aggregate G^16 = Σ f_i
  static auto fam = build_families();
  static auto w = get_weights();
  for (size_t i = 0; i < 16; ++i) {
    double x = psi_t.g[i];
    next.g[i] = w[i] * fam[i].fn(x, in);
  }

  // Normalize with b(t) from Ψ norm; uncertainty from b
  double norm2 = 0.0;
  for (size_t i = 0; i < 16; ++i) norm2 += next.g[i] * next.g[i];
  double phi_used = params.phi_override > 0.0 ? params.phi_override : phi();
  double b = 1.0 / (1.0 + norm2 / phi_used);
  next.g[16] = b;

  // Coherence: inverse of (uncertainty + rate of change surrogate via dx_norm)
  double roc = smooth_norm(in.dx_norm, 0.0);
  next.coherence = params.coh_gain * (1.0 / (1.0 + roc));
  // Value proxy (kept for logging; Ω uses exact form)
  next.value = clamp01(params.val_gain * (0.5 + 0.5 * softsign(in.utility - in.ed_error)));
  // Uncertainty tied to b
  next.uncertainty = clamp01(params.unc_gain * (1.0 - b * b));

  // Small φ-correction using Ψ(t-1) for “single-stack” inertia
  for (size_t i = 0; i < 16; ++i) {
    next.g[i] = 0.5 * next.g[i] + 0.5 * (psi_tm1.g[i] + (phi_used-1.0) * (psi_t.g[i] - psi_tm1.g[i]));
  }
  // Stability gating: damp updates when stability is low (enforces constraint)
  double gamma = params.gamma_min + (1.0 - params.gamma_min) * clamp01(in.stability);
  for (size_t i = 0; i < 16; ++i) {
    double d = next.g[i] - psi_t.g[i];
    next.g[i] = psi_t.g[i] + gamma * d;
  }
  // Apply outer normalization scaling by b
  for (size_t i = 0; i < 16; ++i) next.g[i] *= next.g[16];
  return next;
}

std::string summarize(const MetaState& s) {
  std::ostringstream oss;
  oss << "G16={";
  for (size_t i = 0; i < 16; ++i) {
    if (i) oss << ",";
    oss << s.g[i];
  }
  oss << "}|b=" << s.g[16]
      << " coh=" << s.coherence
      << " val=" << s.value
      << " unc=" << s.uncertainty;
  return oss.str();
}

double compute_omega(const MetaState& next,
                     const MetaState& prev,
                     const Inputs& in) {
  auto params = get_params();
  static auto w = get_weights();
  // Surprise components from ΔΨ
  double l1 = 0.0, l2sq = 0.0;
  for (int i = 0; i < 16; ++i) {
    double d = next.g[i] - prev.g[i];
    l1 += std::abs(d);
    l2sq += d * d;
  }
  l1 /= 16.0;
  double surprise = params.alpha1 * l1 + params.alpha2 * l2sq;
  // Uncertainty from b = next.g[16]
  double b = next.g[16];
  double uncertainty = 1.0 - b * b;
  // Value = s3 * <w, Ψ>
  double dot = 0.0;
  for (int i = 0; i < 16; ++i) dot += w[i] * next.g[i];
  double value = in.utility * dot;
  // Penalty = λ * s1^2 (s1 = dx_norm)
  double penalty = params.lambda * (in.dx_norm * in.dx_norm);
  double omega = surprise + uncertainty - value + penalty;
  return omega;
}

} // namespace common
