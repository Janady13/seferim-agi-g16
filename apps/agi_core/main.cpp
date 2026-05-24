#include "common/g16_core.hpp"
#include <iostream>
#include <string>

using namespace common;

int main(int argc, char** argv) {
  MetaState s0{}; // zero-init
  MetaState s_1{}; // previous
  Inputs in{};

  // Simple CLI flags
  int steps = 1;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--dx" && i + 1 < argc) in.dx_norm = std::stod(argv[++i]);
    else if (a == "--err" && i + 1 < argc) in.ed_error = std::stod(argv[++i]);
    else if (a == "--util" && i + 1 < argc) in.utility = std::stod(argv[++i]);
    else if (a == "--stab" && i + 1 < argc) in.stability = std::stod(argv[++i]);
    else if (a == "--steps" && i + 1 < argc) steps = std::stoi(argv[++i]);
    else if (a == "--help") {
      std::cout << "agi_core --dx <v> --err <v> --util <v> --stab <v> --steps <n>\n";
      return 0;
    }
  }

  for (int t = 0; t < steps; ++t) {
    MetaState s1 = update_meta_state(s0, s_1, in);
    std::cout << "t=" << t << " " << summarize(s1) << "\n";
    s_1 = s0;
    s0 = s1;
  }
  return 0;
}

