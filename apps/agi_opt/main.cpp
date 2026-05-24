#include "common/g16_core.hpp"
#include "common/layers.hpp"
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <string>
#include <vector>

using namespace common;

static bool load_signals(std::string path, Inputs& in) {
  std::ifstream f(path);
  if (!f.good()) return false;
  std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  auto grab = [&](const char* key, double& out) {
    std::regex re(std::string("\"") + key + R"("\s*:\s*([-+]?[0-9]*\.?[0-9]+))");
    std::smatch m;
    if (std::regex_search(s, m, re) && m.size() > 1) {
      out = std::stod(m[1].str());
      return true;
    }
    return false;
  };
  bool ok = true;
  ok &= grab("dx_norm", in.dx_norm);
  ok &= grab("ed_error", in.ed_error);
  ok &= grab("utility", in.utility);
  ok &= grab("stability", in.stability);
  return ok;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: agi_opt signals.json [steps=3] [iters=200]\n";
    return 2;
  }
  Inputs in{};
  if (!load_signals(argv[1], in)) {
    std::cerr << "failed to parse signals from " << argv[1] << "\n";
    return 3;
  }
  int steps = argc >= 3 ? std::stoi(argv[2]) : 3;
  int iters = argc >= 4 ? std::stoi(argv[3]) : 200;

  std::mt19937_64 rng{1234567};
  std::uniform_real_distribution<double> dist(0.5, 1.5);

  auto eval = [&](const std::array<double,16>& w)->double{
    Pipeline pipe;
    pipe.reset(MetaState{});
    Signals sig{in};
    double sum = 0.0;
    // Temporarily set weights via env is not accessible here; instead scale inputs to emulate weight effect across families via utility mix.
    // As a proxy, scale dx_norm by mean weight to test stability of Ω; real per-family weighting is applied via env when running agi_core.
    double meanw = 0.0;
    for (double x : w) meanw += x; meanw /= 16.0;
    Signals sig2 = sig;
    sig2.in.dx_norm *= meanw;
    for (int t = 0; t < steps; ++t) {
      auto out = pipe.step(sig2);
      sum += out.omega;
    }
    return sum / steps;
  };

  std::array<double,16> best{};
  for (double& x : best) x = 1.0;
  double bestScore = eval(best);

  for (int i = 0; i < iters; ++i) {
    std::array<double,16> cand = best;
    int j = i % 16;
    cand[j] *= dist(rng);
    double s = eval(cand);
    if (s < bestScore) { best = cand; bestScore = s; }
  }
  // Print env string for use with agi_core
  std::cout << "Best Ω=" << bestScore << "\nG16_WEIGHTS=";
  for (int i = 0; i < 16; ++i) {
    if (i) std::cout << ",";
    std::cout << best[i];
  }
  std::cout << "\n";
  return 0;
}

