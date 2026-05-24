#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build -j"$(sysctl -n hw.ncpu)" >/dev/null

OUTDIR="$ROOT/data"
mkdir -p "$OUTDIR"
SIGNALS=()
while IFS= read -r line || [[ -n "${line:-}" ]]; do
  [[ -z "$line" ]] && continue
  p="${line}"
  tag="$(basename "$p" | tr ' ' '_' | tr '/' '_')"
  idx="$OUTDIR/memory_index_${tag}.jsonl"
  sig="$OUTDIR/signals_${tag}.json"
  if [[ -d "$p" || -f "$p" ]]; then
    echo "Indexing $p ..."
    ./build/memory_indexer -root "$p" -out "$idx" >/dev/null
    ./build/signals_from_index "$idx" "$sig" >/dev/null
    SIGNALS+=("$sig")
  fi
done < "$ROOT/config/roots.txt"

MERGED="$OUTDIR/signals_merged.json"
./build/merge_indices "${SIGNALS[@]}" > "$MERGED"
echo "Merged signals -> $MERGED"
cat "$MERGED"
echo
echo "Running rollout (3 steps):"
./build/agi_step "$MERGED" 3

