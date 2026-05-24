#include "common/g16_core.hpp"
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

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
    std::cerr << "usage: agi_step signals.json [steps]\n";
    return 2;
  }
  Inputs in{};
  if (!load_signals(argv[1], in)) {
    std::cerr << "failed to parse signals from " << argv[1] << "\n";
    return 3;
  }
  int steps = 1;
  if (argc >= 3) steps = std::stoi(argv[2]);
  MetaState s0{}, s_1{};
  for (int t = 0; t < steps; ++t) {
    MetaState s1 = update_meta_state(s0, s_1, in);
    double omega = compute_omega(s1, s0, in);
    std::cout << "t=" << t << " " << summarize(s1) << " Ω=" << omega << "\n";
    s_1 = s0; s0 = s1;
  }
  return 0;
}

