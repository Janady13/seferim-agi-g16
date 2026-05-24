#include <algorithm>
#include <numeric>
#include <vector>
#include <cmath>
#include "g16_v2_types.h"

using namespace g16v2;

namespace seferim_alloc {

static constexpr float ALPHA_ENTMAX = 1.5f;
static constexpr float ALPHA_MARGIN = 2.0f;
static constexpr float ALPHA_NU = 1.0f;
static constexpr float GAMMA_SHARP = 2.0f;

// Sparse entmax (approx)
static std::vector<float> entmax(const std::vector<float>& r, float alpha) {
  std::vector<float> sorted = r;
  std::sort(sorted.begin(), sorted.end(), std::greater<float>());
  float tau = sorted.front();
  // simple threshold search
  for (size_t k = 1; k < sorted.size(); ++k) {
    float acc = 0.0f;
    for (size_t i = 0; i <= k; ++i) acc += sorted[i];
    float t = (acc - 1.0f) / (float)(k + 1);
    if (k + 1 == sorted.size() || t >= sorted[k]) { tau = t; break; }
  }
  std::vector<float> p(r.size());
  float sum = 0.0f;
  for (size_t i = 0; i < r.size(); ++i) {
    float v = std::max(0.0f, r[i] - tau);
    p[i] = std::pow(v, 1.0f / (alpha - 1.0f));
    sum += p[i];
  }
  float inv = 1.0f / (sum + 1e-9f);
  for (auto& v : p) v *= inv;
  return p;
}

// Full allocator pipeline
std::vector<float> compute_allocation(
    const std::vector<float>& relevance,
    const std::vector<SubstrateState>& subs
) {
  const size_t N = relevance.size();
  if (N == 0) return {};

  std::vector<float> pi = entmax(relevance, ALPHA_ENTMAX); // stage 0

  // stage 1: margin emphasis (approx using global max)
  float r_max = *std::max_element(relevance.begin(), relevance.end());
  for (size_t i = 0; i < N; ++i) {
    float r_second = 0.0f;
    for (size_t j = 0; j < N; ++j) if (j != i) r_second = std::max(r_second, relevance[j]);
    float margin = relevance[i] - r_second;
    float gate = 1.0f / (1.0f + std::exp(-ALPHA_MARGIN * margin));
    pi[i] *= gate;
  }

  // stage 2: truth modulation + novelty suppression
  for (size_t i = 0; i < N; ++i) {
    pi[i] *= subs[i].kappa_real;
    pi[i] *= std::exp(-ALPHA_NU * subs[i].nu);
  }

  // stage 3: consolidation gravity
  float phi_sum = 0.0f;
  for (const auto& s : subs) phi_sum += s.phi;
  for (size_t i = 0; i < N; ++i) {
    float c = subs[i].phi / (phi_sum + 1e-9f);
    pi[i] *= (c + 0.01f);
  }

  // stage 4: sharpening + renorm
  float sumg = 0.0f;
  for (auto& v : pi) { v = std::pow(v, GAMMA_SHARP); sumg += v; }
  float invg = 1.0f / (sumg + 1e-9f);
  for (auto& v : pi) v *= invg;
  return pi;
}

} // namespace seferim_alloc

