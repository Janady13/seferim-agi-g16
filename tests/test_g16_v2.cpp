#include "g16_v2_core.h"
#include <cassert>
#include <iostream>
using namespace g16v2;

int main() {
  std::cout << "G16 v2 basic tests\n";
  // Fibonacci timing spot-checks
  assert(is_fibonacci_tick(1));
  assert(is_fibonacci_tick(2));
  assert(is_fibonacci_tick(4));
  assert(!is_fibonacci_tick(3));
  // Spiral directions
  Config cfg; cfg.use_semantic_planes = false;
  Psi d0 = get_spiral_direction(0, cfg);
  Psi d1 = get_spiral_direction(1, cfg);
  assert(std::abs(d0.x[0] - 1.0) < 1e-9);
  assert(std::abs(d1.x[0] - 1.0) > 1e-6);
  // Update boundedness
  CouplingMatrix J; build_golden_coupling_matrix(J, cfg);
  SystemState S; init_state(S);
  Signals sig{0.01, 0.2, 0.9, 0.99};
  for (int i = 0; i < 100; ++i) update_psi(S, sig, J, cfg);
  assert(S.psi.norm() < 1000.0);
  std::cout << "OK\n";
  return 0;
}

