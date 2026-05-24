#include "g16_v2_core.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

// seferim C API (implemented in src/seferim/seferim_federation.cpp)
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

using namespace g16v2;

struct IndexItem {
  std::string path;
  std::string type;
  std::string content;
  int family = -1;
};

struct FamilyQueues {
  std::vector<IndexItem> q[NUM_FAMILIES];
  size_t borrowed = 0;
  std::array<uint64_t, NUM_FAMILIES> served{};
  FamilyQueues() { for (auto& s : served) s = 0; }
};

static int detect_family(const std::string& type, const std::string& path) {
  if (type == "c" || type == "cpp" || type == "h" ||
      type == "js" || type == "ts" || type == "go" ||
      type == "rs" || type == "java" || type == "swift" ||
      type == "kt" || type == "m" || type == "mm") {
    return 10; // Yad
  }
  if (type == "txt" || type == "md" || type == "json" ||
      type == "yaml" || type == "xml") {
    return 11; // Lashon
  }
  if (type == "png" || type == "jpg" || type == "jpeg") {
    return 1;  // Iris
  }
  // path hints
  if (path.find("/code/") != std::string::npos) return 10;
  if (path.find("/lang/") != std::string::npos) return 11;
  return -1;
}

static std::vector<IndexItem> load_index(const char* path) {
  std::vector<IndexItem> items;
  std::ifstream f(path);
  if (!f) {
    std::cerr << "Index not found: " << path << "\n";
    return items;
  }
  std::string line;
  while (std::getline(f, line)) {
    // very light JSONL extraction (expects {"path":"..","type":"..","content":"..","family":N})
    IndexItem it;
    auto grab = [&](const std::string& key)->std::string{
      std::string pat = "\"" + key + "\"";
      size_t p = line.find(pat);
      if (p == std::string::npos) return "";
      p = line.find(':', p);
      if (p == std::string::npos) return "";
      // if quoted value
      size_t q1 = line.find('"', p+1);
      size_t q2 = std::string::npos;
      if (q1 != std::string::npos) q2 = line.find('"', q1+1);
      if (q1 != std::string::npos && q2 != std::string::npos) {
        return line.substr(q1+1, q2-q1-1);
      }
      return "";
    };
    it.path = grab("path");
    it.type = grab("type");
    it.content = grab("content");
    // optional explicit family
    size_t pf = line.find("\"family\"");
    if (pf != std::string::npos) {
      pf = line.find(':', pf);
      if (pf != std::string::npos) {
        std::string tail = line.substr(pf+1);
        it.family = std::atoi(tail.c_str());
      }
    }
    if (it.family < 0) it.family = detect_family(it.type, it.path);
    items.push_back(std::move(it));
  }
  return items;
}

static void build_family_queues(const std::vector<IndexItem>& items, FamilyQueues& fq) {
  for (const auto& it : items) {
    int fam = it.family >= 0 ? it.family : detect_family(it.type, it.path);
    if (fam < 0 || fam >= NUM_FAMILIES) continue;
    fq.q[fam].push_back(it);
  }
}

static Config load_config() {
  Config cfg;
  const char* w_env = std::getenv("G16_WEIGHTS");
  if (w_env) {
    std::istringstream ss(w_env);
    std::string tok;
    int i = 0;
    while (std::getline(ss, tok, ',') && i < NUM_FAMILIES) cfg.w[i++] = std::stod(tok);
  } else {
    cfg.w = {8.11856, 7.82319, 7.53323, 3.9886, 7.70958, 8.36018, 3.94711,
             4.3281, 6.95926, 3.68361, 3.90775, 3.04808, 3.30263, 8.4151,
             4.64597, 2.18988};
  }
  if (auto* v = std::getenv("G16_GAMMA_MIN")) cfg.gamma_min = std::stod(v);
  if (auto* v = std::getenv("G16_LAMBDA")) cfg.lambda = std::stod(v);
  if (auto* v = std::getenv("SEFERIM_TARGET_COH")) cfg.target_coh = std::stod(v);
  if (auto* v = std::getenv("SEFERIM_LR_BASE")) cfg.lr_base = std::stod(v);
  if (auto* v = std::getenv("SEFERIM_LR_GAIN")) cfg.lr_gain = std::stod(v);
  if (auto* v = std::getenv("AGI_INGEST_GATE")) cfg.ingest_gate = std::stod(v);
  if (auto* v = std::getenv("AGI_INGEST_MAX")) cfg.ingest_max = std::stoi(v);
  if (std::getenv("G16_NO_FIBONACCI")) cfg.use_fibonacci_timing = false;
  if (std::getenv("G16_NO_SPIRAL")) cfg.use_golden_spiral = false;
  if (std::getenv("G16_NO_RESONANCE")) cfg.use_cross_family_resonance = false;
  if (std::getenv("G16_SEQUENTIAL_PLANES")) cfg.use_semantic_planes = false;
  return cfg;
}

int main(int argc, char** argv) {
  uint64_t max_ticks = 1000;
  if (argc > 1) max_ticks = std::stoull(argv[1]);

  Config cfg = load_config();
  SystemState state;
  init_state(state);
  CouplingMatrix J;
  build_golden_coupling_matrix(J, cfg);
  GoldenRotation R;
  init_rotation(R);

  seferim_federation_t fed = seferim_federation_create();
  if (!fed) {
    std::cerr << "Failed to create federation\n";
    return 1;
  }

  std::vector<IndexItem> index;
  if (const char* idx_path = std::getenv("AGI_INGEST_INDEX")) {
    index = load_index(idx_path);
  }
  FamilyQueues fq;
  build_family_queues(index, fq);
  size_t fib_events = 0;
  double prev_coh = seferim_federation_coherence(fed);

  std::cout << std::fixed << std::setprecision(6);
  for (uint64_t t = 0; t < max_ticks; ++t) {
    seferim_federation_tick(fed, 0.01);
    double coh = seferim_federation_coherence(fed);
    double cons = seferim_federation_consciousness(fed);

    Signals signals;
    signals.dx_norm = std::abs(coh - prev_coh);
    signals.ed_error = std::abs(cfg.target_coh - coh);
    signals.utility = 0.5 * cons + 0.5 * coh;
    signals.stability = 1.0 - signals.dx_norm;

    for (int f = 0; f < NUM_FAMILIES; ++f) {
      state.phi.phi[f] = seferim_family_coherence(fed, f);
    }

    update_psi(state, signals, J, cfg); // controller dynamics remain

    // Continuous learning each tick (no Fibonacci gate)
    {
      fib_events++; // service quanta for fairness proportions
      // Select family with largest deficit: s_f * fib_events - served[f]
      int select_f = -1;
      double best_deficit = -1e9;
      double s_f = 1.0 / NUM_FAMILIES;
      for (int f = 0; f < NUM_FAMILIES; ++f) {
        double deficit = s_f * static_cast<double>(fib_events) - static_cast<double>(fq.served[f]);
        if (deficit > best_deficit) {
          best_deficit = deficit;
          select_f = f;
        }
      }
      // Try to serve from select_f; if empty, borrow from neighbor (simple ring)
      IndexItem item{};
      bool have = false;
      if (select_f >= 0 && !fq.q[select_f].empty()) {
        item = fq.q[select_f].back();
        fq.q[select_f].pop_back();
        have = true;
      } else {
        // borrow
        for (int k = 1; k < NUM_FAMILIES; ++k) {
          int left = (select_f - k + NUM_FAMILIES) % NUM_FAMILIES;
          int right = (select_f + k) % NUM_FAMILIES;
          if (!fq.q[left].empty()) {
            item = fq.q[left].back();
            fq.q[left].pop_back();
            have = true;
            fq.borrowed++;
            break;
          }
          if (!fq.q[right].empty()) {
            item = fq.q[right].back();
            fq.q[right].pop_back();
            have = true;
            fq.borrowed++;
            break;
          }
        }
      }
      if (have && !item.content.empty()) {
        int fam = item.family >= 0 ? item.family : detect_family(item.type, item.path);
        seferim_federation_learn(fed, item.content.c_str(), fam);
        fq.served[fam] += 1;
        state.items_ingested++;
      }
    }

    std::cout << "t=" << t
              << " coh=" << coh
              << " cons=" << cons
              << " dx=" << signals.dx_norm
              << " err=" << signals.ed_error
              << " stab=" << signals.stability
              << " | Ω=" << state.omega.total
              << " κ=" << state.kappa
              << " b=" << state.b
              << " Φ=" << state.phi.global
              << " ing=" << state.items_ingested
              << "\n";
    prev_coh = coh;
  }

  if (const char* save_dir = std::getenv("SEFERIM_SAVE_DIR")) {
    std::string path = std::string(save_dir) + "/federation_state.bin";
    seferim_federation_save(fed, path.c_str());
    std::cout << "Saved snapshot to " << path << "\n";
  }

  seferim_federation_destroy(fed);
  return 0;
}
