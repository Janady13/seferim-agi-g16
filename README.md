# SEFERIM AGI G16

> A research scaffold for a 7-layer cognitive loop grounded in the G^16 family of mathematical operators.

[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/build-CMake-064f8c.svg)](https://cmake.org/)

A non-destructive scaffold implementing a mathematically grounded core for a 7-layer cognitive loop using the G^16 family set. Part of the [MEGAMIND](https://feedthejoe.com) distributed AGI research program at [ThatDeveloperGuy](https://thatdeveloperguy.com).

## Highlights

- **C++20 core** (`src/common`) — minimal, testable state update of the form `Ψ(t+1) = F(Ψ(t), inputs)`.
- **G^16 family mapping** — clear association of each family (continuous, discrete, optimization, Bayesian, mutual information, …) to a code path.
- **Standalone app binaries** in `apps/`:
  - `agi_core` — single step or short rollout
  - `agi_loop` — full cognitive loop
  - `agi_opt` — optimization variant
  - `agi_profile` — performance profiling
  - `agi_step` — step-by-step inspection
  - `agi_bridge_v2` — streaming bridge with the G^16 v2 core
- **Memory indexer in Go** (`tools/memory_indexer`) — lightweight knowledge index from opt-in directory paths. No Python dependency.
- **Polyglot bindings** — C ABI plus stubs for C#, Java, Julia, Node, Ruby, Rust, and Swift, each in `bindings/<lang>`.
- **Vite + React dashboard** in `dashboard/` — real-time state visualization.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
./build/apps/agi_core --help
```

## Architecture

See `docs/ARCHITECTURE.md` for the full architectural overview. Key documents:

- `docs/ARCHITECTURE.md` — overall system architecture
- `docs/BLUEPRINT_EXECUTION.md` — execution sequence walkthrough
- `docs/BUILD_AND_RUN.md` — detailed build + run instructions
- `AGENTS.md` — orchestrator agent specification

## Non-destructive promise

This codebase only operates on directories it owns or directories you explicitly list in `config/roots.txt`. The memory indexer is read-only on external paths.

## Configuration

Edit `config/roots.txt` to list directories you want the memory indexer to read. Each path becomes an opt-in source of knowledge for the index. The file ships with placeholder examples — replace with your own absolute paths before running the indexer.

## Bindings

Each language binding lives under `bindings/<lang>/` with its own README. The Rust and Node bindings have working stub implementations; the others are protocol-level stubs ready to be filled in.

## What this is part of

SEFERIM AGI G16 is a sub-project of the broader **MEGAMIND** federated AGI research program. It can be used standalone (the build above is fully self-contained) or as a component of a larger federation of cognitive nodes.

- **Studio**: [ThatDevPro](https://www.thatdevpro.com)
- **Parent agency**: [ThatDeveloperGuy](https://thatdeveloperguy.com)
- **Research hub**: [feedthejoe.com](https://feedthejoe.com)
- **Lead researcher**: Joseph W. Anady ([ORCID 0009-0008-8625-949X](https://orcid.org/0009-0008-8625-949X), [Wikidata Q139901957](https://www.wikidata.org/wiki/Q139901957))

## License

MIT © 2026 Joseph W. Anady. See [LICENSE](LICENSE).
