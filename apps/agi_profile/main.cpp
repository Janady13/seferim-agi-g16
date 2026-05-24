#include "common/g16_core.hpp"
#include <cstdlib>
#include <iostream>
#include <random>

using namespace common;

int main(int argc, char** argv) {
  int steps = argc > 1 ? std::atoi(argv[1]) : 50;
  int trials = argc > 2 ? std::atoi(argv[2]) : 100;
  std::mt19937_64 rng{42};
  std::uniform_real_distribution<double> u(0.0, 1.0);
  double sum = 0.0;
  for (int k = 0; k < trials; ++k) {
    MetaState s0{}, s_1{};
    for (int t = 0; t < steps; ++t) {
      Inputs in{u(rng), u(rng), u(rng), u(rng)};
      auto s1 = update_meta_state(s0, s_1, in);
      sum += compute_omega(s1, s0, in);
      s_1 = s0; s0 = s1;
    }
  }
  std::cout << "avg_Omega=" << (sum / (steps * trials)) << "\n";
  return 0;
}

