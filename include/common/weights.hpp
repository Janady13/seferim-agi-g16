#pragma once
#include <array>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace common {

inline std::array<double,16> get_weights() {
  std::array<double,16> w{};
  for (auto& x : w) x = 1.0;
  const char* env = std::getenv("G16_WEIGHTS");
  if (!env) return w;
  std::string s(env);
  size_t start = 0;
  for (int i = 0; i < 16 && start < s.size(); ++i) {
    size_t comma = s.find(',', start);
    std::string tok = s.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
    try { w[i] = std::stod(tok); } catch (...) {}
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return w;
}

} // namespace common

