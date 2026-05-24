#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <array>
#include <algorithm>
#include <numeric>
#include "g16_v2_types.h"
#include "seferim_binary.h"

extern "C" {
  typedef void* seferim_federation_t;
  seferim_federation_t seferim_federation_create();
  void seferim_federation_destroy(seferim_federation_t fed);
  double seferim_federation_coherence(seferim_federation_t fed);
  double seferim_federation_consciousness(seferim_federation_t fed);
  void seferim_federation_tick(seferim_federation_t fed, double dt);
  int seferim_federation_learn(seferim_federation_t fed, const char* item, int family);
  int seferim_federation_save(seferim_federation_t fed, const char* path);
  double seferim_family_coherence(seferim_federation_t fed, int family);
}

// forward decl from allocator.cpp
namespace seferim_alloc {
  std::vector<g16v2::SubstrateState> /*dummy*/;
  std::vector<float> compute_allocation(const std::vector<float>& relevance,
                                        const std::vector<g16v2::SubstrateState>& subs);
}

namespace {

double clamp01(double v) {
  if (v < 0.0) return 0.0;
  if (v > 1.0) return 1.0;
  return v;
}

// Continuous activation parameters (default; can be overridden via env)
static float ALPHA_KAPPA = 3.0f;
static float ALPHA_PSI = 2.0f;
static float THETA = 1.5f;
static float PSI_0 = 1.0f;
static float ETA_MIN = 0.001f;
static float ETA_MAX = 0.1f;
static float ETA_BASE = 0.01f;
static float ETA_GAIN = 0.05f;

inline float Psi_norm(float Psi) { return Psi / (Psi + PSI_0); }
inline float chi(float kappa, float Psi_normalized) {
  float z = ALPHA_KAPPA * kappa + ALPHA_PSI * Psi_normalized - THETA;
  return 1.0f / (1.0f + std::exp(-z));
}
inline float eta_from_kappa(float kappa) {
  float e = ETA_BASE + ETA_GAIN * kappa;
  if (e < ETA_MIN) e = ETA_MIN;
  if (e > ETA_MAX) e = ETA_MAX;
  return e;
}
inline float Gamma_drive(const g16v2::FamilyState& fam, float kappa_item) {
  float Psi_n = Psi_norm(fam.Psi);
  float activation = chi(kappa_item, Psi_n);
  return activation * fam.eta * kappa_item;
}

// Get env or default
static inline float envf(const char* k, float def) {
  const char* v = std::getenv(k);
  if (!v) return def;
  try { return std::stof(v); } catch (...) { return def; }
}

// Initialize params from env once
void init_params_from_env() {
  static bool inited = false;
  if (inited) return;
  inited = true;
  ALPHA_KAPPA = envf("SEFERIM_ALPHA_KAPPA", ALPHA_KAPPA);
  ALPHA_PSI   = envf("SEFERIM_ALPHA_PSI", ALPHA_PSI);
  THETA       = envf("SEFERIM_THETA", THETA);
  PSI_0       = envf("SEFERIM_PSI_0", PSI_0);
  ETA_MIN     = envf("SEFERIM_ETA_MIN", ETA_MIN);
  ETA_MAX     = envf("SEFERIM_ETA_MAX", ETA_MAX);
  ETA_BASE    = envf("SEFERIM_ETA_BASE", ETA_BASE);
  ETA_GAIN    = envf("SEFERIM_ETA_GAIN", ETA_GAIN);
}

using namespace g16v2;

// master update for a single substrate
void learn_to_substrate(AGIState& state, int f, int s, float Gamma, float pi_eff, uint64_t t) {
  auto& fam = state.families[f];
  auto it = fam.active_index.find((uint16_t)s);
  if (it == fam.active_index.end()) {
    fam.active_index[(uint16_t)s] = fam.substrates.size();
    fam.substrates.push_back({0.0f, 0.5f, 1.0f, (uint32_t)t});
    it = fam.active_index.find((uint16_t)s);
  }
  auto& sub = fam.substrates[it->second];
  float delta = (1.0f - sub.phi) * Gamma * pi_eff;
  sub.phi = std::min(1.0f, sub.phi + delta);
  sub.last_tick = (uint32_t)t;
  sub.nu *= 0.99f;
}

// simple content-based hash
static inline uint32_t hash32(const char* s) {
  uint32_t h = 2166136261u;
  for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
    h ^= *p;
    h *= 16777619u;
  }
  return h;
}

// choose a free substrate id, probing if collides
uint16_t find_free_substrate(FamilyState& fam, uint16_t start) {
  uint16_t sid = start % S;
  for (int i = 0; i < S; ++i) {
    uint16_t cand = (uint16_t)((sid + i) % S);
    if (fam.active_index.find(cand) == fam.active_index.end()) return cand;
  }
  // all occupied, return start (will update existing)
  return start % S;
}

// compute global Phi as mean of family means
float family_mean_phi(const FamilyState& fam) {
  if (fam.substrates.empty()) return 0.5f;
  float sum = 0.0f;
  for (const auto& s : fam.substrates) sum += s.phi;
  return sum / (float)fam.substrates.size();
}

float compute_global_phi(const AGIState& st) {
  float sum = 0.0f;
  for (int f = 0; f < F; ++f) sum += family_mean_phi(st.families[f]);
  return sum / (float)F;
}

} // namespace

extern "C" {

seferim_federation_t seferim_federation_create() {
  init_params_from_env();
  auto* st = new g16v2::AGIState();
  for (int f = 0; f < g16v2::F; ++f) {
    st->families[f].Psi = 0.5f;
    st->families[f].eta = ETA_BASE;
  }
  st->Phi_global = 0.1f;
  st->Omega = 0.0f;
  st->tick = 0;
  return reinterpret_cast<seferim_federation_t>(st);
}

void seferim_federation_destroy(seferim_federation_t fed) {
  auto* st = reinterpret_cast<g16v2::AGIState*>(fed);
  delete st;
}

double seferim_federation_coherence(seferim_federation_t fed) {
  auto* st = reinterpret_cast<g16v2::AGIState*>(fed);
  return (double)compute_global_phi(*st);
}

double seferim_federation_consciousness(seferim_federation_t fed) {
  auto* st = reinterpret_cast<g16v2::AGIState*>(fed);
  // mean penalized by family variance
  float means[g16v2::F];
  for (int f = 0; f < g16v2::F; ++f) means[f] = family_mean_phi(st->families[f]);
  float mu = 0.0f;
  for (float v : means) mu += v;
  mu /= (float)g16v2::F;
  float var = 0.0f;
  for (float v : means) { float d = v - mu; var += d * d; }
  var /= (float)g16v2::F;
  float cons = mu * (1.0f - std::min(1.0f, var));
  if (cons < 0.0f) cons = 0.0f;
  if (cons > 1.0f) cons = 1.0f;
  return (double)cons;
}

void seferim_federation_tick(seferim_federation_t fed, double dt) {
  (void)dt;
  auto* st = reinterpret_cast<g16v2::AGIState*>(fed);
  st->tick += 1;
  // drift novelty down slightly, plasticity toward base
  for (int f = 0; f < g16v2::F; ++f) {
    auto& fam = st->families[f];
    for (auto& s : fam.substrates) s.nu = std::max(0.0f, s.nu * 0.999f);
    fam.eta = fam.eta * 0.99f + 0.01f * ETA_BASE;
  }
}

int seferim_federation_learn(seferim_federation_t fed, const char* item, int family) {
  auto* st = reinterpret_cast<g16v2::AGIState*>(fed);
  if (family < 0 || family >= g16v2::F) return -1;
  const char* txt = item ? item : "";
  size_t len = std::strlen(txt);
  float kappa_item = std::tanh((float)len / 4000.0f);

  auto& fam = st->families[family];
  fam.eta = eta_from_kappa(kappa_item);

  // Candidate set: hashed target + possibly a new free slot
  uint16_t hsid = (uint16_t)(hash32(txt) % g16v2::S);
  std::vector<uint16_t> candidates;
  candidates.push_back(hsid);
  if (fam.substrates.size() < (size_t)g16v2::S) {
    candidates.push_back(find_free_substrate(fam, (uint16_t)((hsid + 13) % g16v2::S)));
  }
  // Build relevance and substrate snapshots
  std::vector<float> relevance;
  std::vector<g16v2::SubstrateState> subs;
  for (uint16_t sid : candidates) {
    auto it = fam.active_index.find(sid);
    SubstrateState snap{0.0f, 0.5f, 1.0f, (uint32_t)st->tick};
    if (it != fam.active_index.end()) snap = fam.substrates[it->second];
    // simple relevance: focus on hashed sid, slight preference for higher phi
    float rel = (sid == hsid ? 1.0f : 0.2f) + 0.2f * snap.phi;
    relevance.push_back(rel);
    subs.push_back(snap);
  }
  auto pi = seferim_alloc::compute_allocation(relevance, subs);
  float G = Gamma_drive(fam, kappa_item);
  for (size_t i = 0; i < candidates.size(); ++i) {
    if (pi[i] > 1e-6f) learn_to_substrate(*st, family, candidates[i], G, pi[i], st->tick);
  }
  st->Phi_global = compute_global_phi(*st);
  fam.Psi = fam.Psi * 0.99f + (1.0f - st->Phi_global) * 0.1f;
  return 0;
}

int seferim_federation_save(seferim_federation_t fed, const char* path) {
  auto* st = reinterpret_cast<g16v2::AGIState*>(fed);
  std::string binpath(path);
  if (binpath.size() < 4 || binpath.substr(binpath.size()-4) != ".bin") {
    binpath += ".bin";
  }
  seferim_bin::BinaryStateIO::save(*st, binpath.c_str());
  return 0;
}

int seferim_federation_load(seferim_federation_t fed, const char* path) {
  auto* st = reinterpret_cast<g16v2::AGIState*>(fed);
  std::string binpath(path);
  if (binpath.size() < 4 || binpath.substr(binpath.size()-4) != ".bin") {
    binpath += ".bin";
  }
  if (seferim_bin::BinaryStateIO::load(binpath.c_str(), *st)) {
    st->Phi_global = compute_global_phi(*st);
    return 0;
  }
  return -1;
}

double seferim_family_coherence(seferim_federation_t fed, int family) {
  auto* st = reinterpret_cast<g16v2::AGIState*>(fed);
  if (family < 0 || family >= g16v2::F) return 0.0;
  return (double)family_mean_phi(st->families[family]);
}

} // extern "C"
