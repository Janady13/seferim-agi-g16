#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build -j"$(sysctl -n hw.ncpu)" >/dev/null

echo "Running agi_core demo..."
./build/agi_core --dx 0.3 --err 0.1 --util 0.7 --stab 0.8 --steps 2

echo "Indexing seferim-unified (sample)..."
./build/memory_indexer -root "$HOME/seferim-unified" -out data/memory_index_demo.jsonl
echo "Done. Output -> data/memory_index_demo.jsonl"

