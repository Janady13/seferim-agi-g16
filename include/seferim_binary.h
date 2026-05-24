#pragma once
#include <cstdint>
#include <vector>
#include <fstream>
#include <cstring>
#include "g16_v2_types.h"

namespace seferim_bin {

using namespace g16v2;

constexpr uint32_t SEFG_MAGIC = 0x53454647; // 'SEFG'

#pragma pack(push, 1)
struct BinaryHeader {
  uint32_t magic;       // 0x53454647
  uint32_t tick;
  uint16_t active_count;
  uint8_t phi_global;   // [0,255] -> [0,1]
  uint8_t omega;        // [0,255] scaled
  uint16_t checksum;
  uint16_t version;
};

struct BinaryFamily {
  uint8_t psi;          // quantized pressure
  uint8_t eta;          // quantized plasticity
};

struct BinarySubstrate {
  uint8_t fam_sub_hi;   // bits 0-3: family, bits 4-7: substrate[12:9]
  uint8_t sub_mid;      // substrate[8:1]
  uint8_t sub_lo_phi;   // bit 7: substrate[0], bits 0-6: φ (7-bit)
  uint8_t kappa_nu;     // bits 0-3: κ_real (4-bit), bits 4-7: ν (4-bit)
};
#pragma pack(pop)

inline uint8_t q8(float v) { if (v < 0) v = 0; if (v > 1) v = 1; return (uint8_t)(v * 255.0f + 0.5f); }
inline float dq8(uint8_t v) { return v / 255.0f; }
inline uint8_t q7(float v) { if (v < 0) v = 0; if (v > 1) v = 1; return (uint8_t)(v * 127.0f + 0.5f); }
inline float dq7(uint8_t v) { return v / 127.0f; }
inline uint8_t q4(float v) { if (v < 0) v = 0; if (v > 1) v = 1; return (uint8_t)(v * 15.0f + 0.5f); }
inline float dq4(uint8_t v) { return v / 15.0f; }

class BinaryStateIO {
public:
  static std::vector<uint8_t> serialize(const AGIState& state) {
    std::vector<uint8_t> buf;
    uint16_t active = 0;
    for (int f = 0; f < F; ++f) active += (uint16_t)state.families[f].substrates.size();
    BinaryHeader hdr{SEFG_MAGIC, (uint32_t)state.tick, active,
                     q8(state.Phi_global), q8(std::min(1.0f, state.Omega / 100.0f)),
                     0, 1};
    buf.resize(sizeof(BinaryHeader));
    std::memcpy(buf.data(), &hdr, sizeof(hdr));
    for (int f = 0; f < F; ++f) {
      BinaryFamily bf{ q8(state.families[f].Psi), q8(state.families[f].eta) };
      auto p = (const uint8_t*)&bf;
      buf.insert(buf.end(), p, p + sizeof(bf));
    }
    for (int f = 0; f < F; ++f) {
      for (const auto& kv : state.families[f].active_index) {
        uint16_t sid = kv.first;
        const auto& sub = state.families[f].substrates[kv.second];
        BinarySubstrate bs;
        bs.fam_sub_hi = (uint8_t)((f & 0xF) | ((sid >> 9) << 4));
        bs.sub_mid = (uint8_t)((sid >> 1) & 0xFF);
        bs.sub_lo_phi = (uint8_t)(((sid & 1) << 7) | (q7(sub.phi) & 0x7F));
        bs.kappa_nu = (uint8_t)((q4(sub.kappa_real) & 0xF) | (q4(sub.nu) << 4));
        auto p = (const uint8_t*)&bs;
        buf.insert(buf.end(), p, p + sizeof(bs));
      }
    }
    uint16_t sum = 0;
    for (auto b : buf) sum += b;
    std::memcpy(buf.data() + offsetof(BinaryHeader, checksum), &sum, 2);
    return buf;
  }

  static bool deserialize(const std::vector<uint8_t>& buf, AGIState& state) {
    if (buf.size() < sizeof(BinaryHeader) + F * sizeof(BinaryFamily)) return false;
    const uint8_t* p = buf.data();
    BinaryHeader hdr{};
    std::memcpy(&hdr, p, sizeof(hdr));
    if (hdr.magic != SEFG_MAGIC) return false;
    p += sizeof(hdr);
    state.tick = hdr.tick;
    state.Phi_global = dq8(hdr.phi_global);
    state.Omega = dq8(hdr.omega) * 100.0f;
    for (int f = 0; f < F; ++f) {
      BinaryFamily bf{};
      std::memcpy(&bf, p, sizeof(bf));
      p += sizeof(bf);
      state.families[f].Psi = dq8(bf.psi);
      state.families[f].eta = dq8(bf.eta);
      state.families[f].substrates.clear();
      state.families[f].active_index.clear();
    }
    for (uint16_t i = 0; i < hdr.active_count; ++i) {
      BinarySubstrate bs{};
      std::memcpy(&bs, p, sizeof(bs));
      p += sizeof(bs);
      int f = bs.fam_sub_hi & 0xF;
      uint16_t s = (uint16_t)(((bs.fam_sub_hi >> 4) << 9) | (bs.sub_mid << 1) | (bs.sub_lo_phi >> 7));
      float phi = dq7(bs.sub_lo_phi & 0x7F);
      float kr = dq4(bs.kappa_nu & 0xF);
      float nu = dq4(bs.kappa_nu >> 4);
      auto& fam = state.families[f];
      fam.active_index[s] = fam.substrates.size();
      fam.substrates.push_back({phi, kr, nu, (uint32_t)state.tick});
    }
    return true;
  }

  static void save(const AGIState& state, const char* path) {
    auto buf = serialize(state);
    std::ofstream f(path, std::ios::binary);
    f.write((const char*)buf.data(), (std::streamsize)buf.size());
  }

  static bool load(const char* path, AGIState& state) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    std::streamsize sz = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> buf((size_t)sz);
    f.read((char*)buf.data(), sz);
    return deserialize(buf, state);
  }
};

} // namespace seferim_bin

