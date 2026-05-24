#include "g16_v2_core.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <sys/stat.h>
#include <csignal>

// seferim C API
extern "C" {
  typedef void* seferim_federation_t;
  seferim_federation_t seferim_federation_create();
  void seferim_federation_destroy(seferim_federation_t fed);
  double seferim_federation_coherence(seferim_federation_t fed);
  double seferim_federation_consciousness(seferim_federation_t fed);
  void seferim_federation_tick(seferim_federation_t fed, double dt);
  int seferim_federation_learn(seferim_federation_t fed, const char* item, int family);
  int seferim_federation_save(seferim_federation_t fed, const char* path);
  int seferim_federation_load(seferim_federation_t fed, const char* path);
  double seferim_family_coherence(seferim_federation_t fed, int family);
}

// Global for signal handling
static std::atomic<bool> g_running{true};
static seferim_federation_t g_fed = nullptr;
static std::string g_save_path;

void signal_handler(int sig) {
  std::cerr << "\nReceived signal " << sig << ", saving state and shutting down...\n";
  g_running = false;
}

using namespace g16v2;

struct IndexItem {
  std::string path;
  std::string type;
  std::string content;
  int family = -1;
};

struct StreamingQueue {
  std::vector<IndexItem> items;
  std::mutex mtx;
  std::atomic<size_t> total_added{0};
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
  if (path.find("/code/") != std::string::npos) return 10;
  if (path.find("/lang/") != std::string::npos) return 11;
  return 11; // Default to Lashon for text
}

static IndexItem parse_jsonl_line(const std::string& line) {
  IndexItem it;
  auto grab = [&](const std::string& key)->std::string{
    std::string pat = "\"" + key + "\"";
    size_t p = line.find(pat);
    if (p == std::string::npos) return "";
    p = line.find(':', p);
    if (p == std::string::npos) return "";
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
  size_t pf = line.find("\"family\"");
  if (pf != std::string::npos) {
    pf = line.find(':', pf);
    if (pf != std::string::npos) {
      std::string tail = line.substr(pf+1);
      it.family = std::atoi(tail.c_str());
    }
  }
  if (it.family < 0) it.family = detect_family(it.type, it.path);
  return it;
}

// Watch file for new lines and add to queue
static void file_watcher_thread(const char* path, StreamingQueue& queue, std::atomic<bool>& running) {
  std::ifstream f;
  size_t last_pos = 0;
  time_t last_mtime = 0;

  while (running) {
    struct stat st;
    if (stat(path, &st) == 0) {
      if (st.st_mtime != last_mtime || !f.is_open()) {
        last_mtime = st.st_mtime;

        if (!f.is_open()) {
          f.open(path);
          if (f) {
            // Read all existing lines first
            std::string line;
            while (std::getline(f, line)) {
              if (!line.empty()) {
                IndexItem item = parse_jsonl_line(line);
                if (!item.content.empty()) {
                  std::lock_guard<std::mutex> lock(queue.mtx);
                  queue.items.push_back(std::move(item));
                  queue.total_added++;
                }
              }
            }
            f.clear();
            last_pos = f.tellg();
          }
        } else {
          // Check for new content
          f.clear();
          f.seekg(0, std::ios::end);
          size_t end_pos = f.tellg();

          if (end_pos > last_pos) {
            f.seekg(last_pos);
            std::string line;
            while (std::getline(f, line)) {
              if (!line.empty()) {
                IndexItem item = parse_jsonl_line(line);
                if (!item.content.empty()) {
                  std::lock_guard<std::mutex> lock(queue.mtx);
                  queue.items.push_back(std::move(item));
                  queue.total_added++;
                }
              }
            }
            f.clear();
            last_pos = f.tellg();
          }
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
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
  return cfg;
}

int main(int argc, char** argv) {
  std::cerr << "SEFERIM Streaming Bridge v2.2 (Persistent Memory)\n";
  std::cerr << "Continuous learning mode - watches index file for new items\n\n";

  // Setup signal handlers for graceful shutdown
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

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
  g_fed = fed;

  // Setup save directory
  const char* save_dir = std::getenv("SEFERIM_SAVE_DIR");
  std::string state_path;
  if (save_dir) {
    // Create directory if needed
    mkdir(save_dir, 0755);
    state_path = std::string(save_dir) + "/federation_state.bin";
    g_save_path = state_path;

    // Try to load existing state for persistence
    struct stat st;
    if (stat(state_path.c_str(), &st) == 0) {
      std::cerr << "Loading existing state from: " << state_path << "\n";
      if (seferim_federation_load(fed, state_path.c_str()) == 0) {
        std::cerr << "State loaded successfully - resuming learning\n";
      } else {
        std::cerr << "Could not load state - starting fresh\n";
      }
    } else {
      std::cerr << "No existing state found - starting fresh\n";
    }
  }

  StreamingQueue queue;
  std::atomic<bool> running{true};
  std::thread watcher;

  const char* idx_path = std::getenv("AGI_INGEST_INDEX");
  if (idx_path) {
    std::cerr << "Watching index: " << idx_path << "\n";
    watcher = std::thread(file_watcher_thread, idx_path, std::ref(queue), std::ref(running));
  } else {
    std::cerr << "Warning: AGI_INGEST_INDEX not set, no learning will occur\n";
  }

  std::cout << std::fixed << std::setprecision(6);
  double prev_coh = seferim_federation_coherence(fed);
  uint64_t t = 0;
  uint64_t last_save = 0;
  auto last_save_time = std::chrono::steady_clock::now();

  // Run indefinitely (or until killed)
  while (g_running && running) {
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

    update_psi(state, signals, J, cfg);

    // Learn from queue if items available
    {
      std::lock_guard<std::mutex> lock(queue.mtx);
      if (!queue.items.empty()) {
        IndexItem item = std::move(queue.items.back());
        queue.items.pop_back();

        if (!item.content.empty()) {
          int fam = item.family >= 0 ? item.family : detect_family(item.type, item.path);
          seferim_federation_learn(fed, item.content.c_str(), fam);
          state.items_ingested++;
        }
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
              << " q=" << queue.total_added.load()
              << "\n";

    prev_coh = coh;
    t++;

    // Save every 30 seconds or 5000 ticks (whichever comes first)
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_save_time).count();
    if (!state_path.empty() && (t - last_save >= 5000 || elapsed >= 30)) {
      seferim_federation_save(fed, state_path.c_str());
      last_save = t;
      last_save_time = now;
      std::cerr << "[Save] State saved at tick " << t << " (ing=" << state.items_ingested << ")\n";
    }

    // Throttle to ~100 ticks/sec so dashboard can keep up
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Graceful shutdown - save final state
  running = false;
  if (!state_path.empty()) {
    std::cerr << "Saving final state to: " << state_path << "\n";
    seferim_federation_save(fed, state_path.c_str());
    std::cerr << "Final state saved (tick=" << t << ", ing=" << state.items_ingested << ")\n";
  }

  if (watcher.joinable()) watcher.join();
  seferim_federation_destroy(fed);
  std::cerr << "Shutdown complete.\n";
  return 0;
}
