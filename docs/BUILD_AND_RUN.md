Quick Build & Run
-----------------

Prereqs
- macOS toolchain with Apple Clang.
- Go 1.21+ for the indexer (no Python used).

Build
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build -j$(sysctl -n hw.ncpu)`

Run Core
- `./build/agi_core --dx 0.3 --err 0.1 --util 0.7 --stab 0.8 --steps 3`

Auto Pipeline
- Set roots in `config/roots.txt`, then `./tools/auto_run.sh`
- Outputs `data/signals_merged.json` and runs a 3‑step rollout.

Index Memory (selected roots)
- `./build/memory_indexer -root $HOME/seferim-unified -out data/memory_index.jsonl`
- Repeat with other roots as needed.

Scan Entire Home (opt‑in; can be slow)
- `./build/memory_indexer -root $HOME -out data/memory_index_full.jsonl`
  - Optional skips: `-skip \"/Library,/System,/Applications,/proc,/dev,/Volumes,/node_modules,/.git,/.cargo,/.cache\"`

Where Things Go
- Indices: `data/*.jsonl`
- Core library: `build/libg16_core.a`
- C ABI: `build/libg16_cabi.a`

Optimize Weights (minimize Ω)
- `./build/agi_opt data/signals_merged.json 3 200` → prints `G16_WEIGHTS=...`
- Use with: `G16_WEIGHTS=... ./build/agi_core ...`
