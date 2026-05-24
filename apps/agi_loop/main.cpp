#include "common/layers.hpp"
#include <iostream>
#include <sstream>
#include <string>

using namespace common;

int main(int, char**) {
  Pipeline pipe;
  pipe.reset(MetaState{});
  std::string line;
  std::cerr << "agi_loop: send JSON lines like {\"dx_norm\":0.2,\"ed_error\":0.1,\"utility\":0.6,\"stability\":0.9}\n";
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    Signals sig{};
    // naive parse to avoid heavy deps
    auto pick = [&](const char* k, double& dst) {
      auto pos = line.find(k);
      if (pos == std::string::npos) return;
      auto colon = line.find(':', pos);
      if (colon == std::string::npos) return;
      size_t end = colon + 1;
      while (end < line.size() && (line[end]==' '||line[end]=='\"')) ++end;
      std::stringstream ss(line.substr(end));
      ss >> dst;
    };
    pick("dx_norm", sig.in.dx_norm);
    pick("ed_error", sig.in.ed_error);
    pick("utility", sig.in.utility);
    pick("stability", sig.in.stability);
    auto out = pipe.step(sig);
    std::cout << "{"
              << "\"summary\":\"" << summarize(out.psi) << "\","
              << "\"omega\":" << out.omega
              << "}\n";
  }
  return 0;
}

