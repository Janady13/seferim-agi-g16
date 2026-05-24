SEFERIM AGI G16 – What Was Built
--------------------------------

Dates: Initialized on Dec 17, 2025 (local).

Outcomes
- New non-destructive project at `~/SEFERIM_AGI_G16`.
- C++20 core implementing a minimal Single‑Stack update with explicit G^16 family contributions and φ‑based normalization.
- CLI binaries: `agi_core` (flags) and `agi_step` (reads `signals.json`).
- Go memory indexer that scans your specified folders and writes local JSONL indices (no Python).
- Go signal extractor that converts an index into `signals.json` (dx_norm, ed_error, utility, stability).
- Polyglot pathway via a C ABI and stubs (Rust, Go, Node, Java, C#, Julia, Ruby, Swift). Node N‑API addon scaffold included.
- Shared libraries (`libg16_core_shared`, `libg16_cabi_shared`) + install targets.

Key Files
- `include/common/g16_core.hpp`, `src/common/g16_core.cpp` – core math and families.
- `apps/agi_core/main.cpp` – runnable CLI.
- `apps/agi_step/main.cpp` – steps using `signals.json` and prints Ω.
- `bindings/c/abi.h`, `bindings/c/abi.cpp` – stable C ABI.
- `tools/memory_indexer/` – Go indexer.
- `tools/signals_from_index/` – Go signal converter.
- `bindings/node/` – N‑API stub linking to C ABI.
- `docs/ARCHITECTURE.md`, `docs/NAMES.md` – design + naming.

Scans Performed (local-only)
- `~/seferim-unified` → `data/memory_index_seferim_unified.jsonl`
- `~/EponaCore` → `data/memory_index_eponacore.jsonl`
- `~/dna_v9` → `data/memory_index_dna_v9.jsonl`
- `~/env files 2` → `data/memory_index_env_files_2.jsonl` (no supported files)
- `~/Downloads/files (4)` → `data/memory_index_downloads_files4.jsonl` (images are noted but not parsed)

How Memory Connects Mathematically
- Memory indices are intended to feed Layer‑1 change detectors to set `dx_norm` (magnitude of new/changed “knowledge”), while desiderata deltas map to `ed_error`, and task priority maps to `utility`. This keeps the update loop consistent with your equations while the content stays local and polyglot‑accessible.

Next Integration Steps
- Add a small adapter that maps file deltas → `(dx_norm, ed_error, utility)` per task.
- Expand supported parsers (e.g., Swift/JS/Rust sources) to extract simple statistics for signal shaping.
- Wire an action policy head to drive concrete tasks from `value`.

Build & Run
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(sysctl -n hw.ncpu)`
- `./build/agi_core --dx 0.3 --err 0.1 --util 0.7 --stab 0.8 --steps 3`
- `./build/memory_indexer -root ~/seferim-unified -out data/memory_index.jsonl`
 - `./build/signals_from_index data/memory_index.jsonl data/signals.json && ./build/agi_step data/signals.json 2`

Iterative Upgrade Cycles (Dec 17, 2025)
1) Base scaffold: core C++ library, CLI, Go indexer, docs.
2) Added Ω objective, `agi_step` app, and JSON-based signal path.
3) Node N‑API addon scaffold for JS interop.
4) Extended tests to check Ω non-increase; added shared libraries + install targets.
5) Added weight controls via env `G16_WEIGHTS`; Go `signals_from_index`; end-to-end demo.
