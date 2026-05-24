#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <unordered_map>

namespace g16v2 {

// Golden ratio constants
constexpr double PHI = 1.6180339887498948482;
constexpr double PSI = 0.6180339887498948482;
constexpr double SQRT5 = 2.2360679774997896964;
constexpr double PI = 3.1415926535897932385;

// Dimension constants
constexpr int NUM_FAMILIES = 16;
constexpr int NUM_PLANES = 8;  // 16D = 8 complex planes
constexpr int DNA_DIM = 256;

// 5000-substrate upgrade sizes
constexpr int F = 16;          // families
constexpr int S = 5000;        // substrates per family (upper bound)

// Precomputed Fibonacci numbers (first 50)
constexpr std::array<uint64_t, 50> FIB = {
  0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597,
  2584, 4181, 6765, 10946, 17711, 28657, 46368, 75025, 121393, 196418,
  317811, 514229, 832040, 1346269, 2178309, 3524578, 5702887, 9227465,
  14930352, 24157817, 39088169, 63245986, 102334155, 165580141, 267914296,
  433494437, 701408733, 1134903170, 1836311903, 2971215073ULL, 4807526976ULL,
  7778742049ULL
};

// Precomputed cumulative Fibonacci (learning tick indices)
// T[n] = F[n+2] - 1
constexpr std::array<uint64_t, 45> FIB_CUMULATIVE = {
  1, 2, 4, 7, 12, 20, 33, 54, 88, 143, 232, 376, 609, 986, 1596, 2583,
  4180, 6764, 10945, 17710, 28656, 46367, 75024, 121392, 196417, 317810,
  514228, 832039, 1346268, 2178308, 3524577, 5702886, 9227464, 14930351,
  24157816, 39088168, 63245985, 102334154, 165580140, 267914295, 433494436,
  701408732, 1134903169, 1836311902, 2971215072ULL
};

// Precomputed rotation frequencies ω[k] = 2π / φ^k
constexpr std::array<double, NUM_PLANES> OMEGA = {
  3.8832220774509332, 2.3999631939115080, 1.4832588835394252,
  0.9167043103720828, 0.5665545731673424, 0.3501497372047404,
  0.2164048359626020, 0.1337449012421384
};

// Precomputed coupling eigenvalues λ[k] = (2π / φ^k)²
constexpr std::array<double, NUM_PLANES> LAMBDA = {
  15.0794190748, 5.7598232438, 2.2000053977, 0.8403468544,
  0.3209780697, 0.1226047960, 0.0468310987, 0.0178877485
};

// Meta-state vector (16D)
struct Psi {
  std::array<double, NUM_FAMILIES> x{};

  double norm() const {
    double sum = 0.0;
    for (int i = 0; i < NUM_FAMILIES; ++i) sum += x[i] * x[i];
    return std::sqrt(sum);
  }
  double norm_sq() const {
    double sum = 0.0;
    for (int i = 0; i < NUM_FAMILIES; ++i) sum += x[i] * x[i];
    return sum;
  }
};

// Per-family coherence
struct Phi {
  std::array<double, NUM_FAMILIES> phi{};  // φ_f for each family
  double global = 0.5;                     // Φ_global = 1 / (1 + |Ω|)
};

// Signal vector (extracted from federation)
struct Signals {
  double dx_norm = 0.0;   // |coh(t) - coh(t-1)|
  double ed_error = 0.5;  // |coh* - coh(t)|
  double utility = 0.5;   // 0.5 * cons + 0.5 * coh
  double stability = 1.0; // 1 - dx_norm
};

// Objective decomposition
struct Omega {
  double surprise = 0.0;     // S = α₁‖ΔΨ‖₁ + α₂‖ΔΨ‖₂²
  double uncertainty = 0.0;  // U = 1 - b²
  double value = 0.0;        // V = u · ⟨w, Ψ⟩
  double penalty = 0.0;      // P = λ · ‖Δcoh‖²
  double total = 0.0;        // Ω = S + U - V + P
};

// Rotation matrix helper (values are recomputed per tick)
struct GoldenRotation {
  std::array<double, NUM_PLANES> cos_theta{};
  std::array<double, NUM_PLANES> sin_theta{};
  void init() {
    for (int k = 0; k < NUM_PLANES; ++k) {
      cos_theta[k] = std::cos(OMEGA[k]);
      sin_theta[k] = std::sin(OMEGA[k]);
    }
  }
};

// Coupling matrix J (symmetric 16x16)
struct CouplingMatrix {
  std::array<std::array<double, NUM_FAMILIES>, NUM_FAMILIES> J{};
  double get(int f, int g) const { return J[f][g]; }
  void set(int f, int g, double val) { J[f][g] = val; J[g][f] = val; }
};

// Family mapping metadata
struct FamilyInfo {
  int index;
  const char* name;
  const char* domain;
};

constexpr std::array<FamilyInfo, NUM_FAMILIES> FAMILIES = {{
  {0,  "Athena",     "Strategy"},
  {1,  "Iris",       "Vision"},
  {2,  "Hermes",     "Communication"},
  {3,  "Prometheus", "Creation"},
  {4,  "Apollo",     "Harmony"},
  {5,  "Hephaestus", "Craft"},
  {6,  "Demeter",    "Growth"},
  {7,  "Poseidon",   "Flow"},
  {8,  "Hera",       "Governance"},
  {9,  "Ares",       "Competition"},
  {10, "Yad",        "Programming"},
  {11, "Lashon",     "Language"},
  {12, "Chronos",    "Time"},
  {13, "Mnemosyne",  "Memory"},
  {14, "Hypnos",     "Rest"},
  {15, "Thanatos",   "Pruning"}
}};

// Plane mapping
constexpr std::array<std::pair<int, int>, NUM_PLANES> PLANE_FAMILIES = {{
  {0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}, {10, 11}, {12, 13}, {14, 15}
}};

constexpr std::array<std::pair<int, int>, NUM_PLANES> SEMANTIC_PLANES = {{
  {0, 8},   // Athena-Hera
  {10, 11}, // Yad-Lashon
  {5, 10},  // Hephaestus-Yad
  {13, 14}, // Mnemosyne-Hypnos
  {3, 4},   // Prometheus-Apollo
  {2, 7},   // Hermes-Poseidon
  {6, 15},  // Demeter-Thanatos
  {1, 9}    // Iris-Ares
}};

// Full system state
struct SystemState {
  Psi psi;
  Psi psi_prev;
  Phi phi;
  Signals signals;
  Omega omega;
  double b = 1.0;
  double kappa = 0.0;
  double gamma = 0.5;
  uint64_t tick = 0;
  uint64_t fib_index = 0;
  uint64_t items_ingested = 0;
  std::array<double, NUM_FAMILIES> learning_rates{};
};

// Configuration
struct Config {
  // Weights (calibrated)
  std::array<double, NUM_FAMILIES> w{};

  // Controller parameters
  double gamma_min = 0.1;
  double alpha1 = 0.5;
  double alpha2 = 0.5;
  double lambda = 0.1;

  // Federation parameters
  double target_coh = 0.8;
  double lr_base = 0.05;
  double lr_gain = 0.2;

  // Ingestion parameters
  double ingest_gate = 0.2;
  int ingest_max = 200;

  // Extension flags
  bool use_fibonacci_timing = true;
  bool use_golden_spiral = true;
  bool use_cross_family_resonance = true;
  bool use_semantic_planes = true;
};

// ---------------------------------------------------------------------------
// 5000-substrate upgrade: per-substrate state and AGI state
// ---------------------------------------------------------------------------
struct SubstrateState {
  float phi = 0.0f;        // Coherence [0,1]
  float kappa_real = 0.5f; // Verification confidence [0,1]
  float nu = 1.0f;         // Novelty (decays over time)
  uint32_t last_tick = 0;  // Last update tick
};

struct FamilyState {
  std::vector<SubstrateState> substrates;                 // sparse
  std::unordered_map<uint16_t, size_t> active_index;      // substrate_id -> vec index
  float Psi = 0.5f;                                       // Pressure
  float eta = 0.01f;                                      // Plasticity
  uint32_t served = 0;                                    // Fairness counter
};

struct AGIState {
  uint32_t id = 0;
  std::array<FamilyState, F> families{};
  float Phi_global = 0.1f;   // Global coherence
  float Omega = 0.0f;        // Cumulative regret
  uint64_t tick = 0;
  uint64_t knowledge_bits = 0;
};

} // namespace g16v2
