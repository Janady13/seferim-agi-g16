**SEFERIM AGI G16 – Architecture**

- Purpose: Realize a 7‑layer cognitive loop using your Single‑Stack Master Equation and the 16 mathematical families (G^16).
- Core: C++20 library `g16_core` with explicit meta‑state `Ψ` and inputs.
- Update: `Ψ(t+1) = F(Ψ(t), Ψ(t−1), inputs)` with φ‑based normalization and stability constraints.

Layers (Implementation Roadmap)
- Layer 1 – Signal & Change: feeds `dx_norm`, computes change detectors (todo).
- Layer 2 – State & Projection: wraps `MetaState` and external projections (partial).
- Layer 3 – Coherence: `coherence` term via inverse rate‑of‑change (implemented).
- Layer 4 – Synchrony: future pairwise family coupling (todo).
- Layer 5 – Predictive: plug-in predictive heads per family (todo).
- Layer 6 – Deecisive: maps `value` to action policy (partial).
- Layer 7 – Global State: φ‑accumulator blending with inertia from Ψ(t−1) (implemented).

Families (WhatItDoes(Equation)-(Family))
- See `docs/NAMES.md` for the exact list and mapping to code functions.

Polyglot Interop
- C ABI from `g16_core` to enable bindings in Rust (cxx), Go (cgo), Node (N‑API), Java (JNI), C# (P/Invoke). Stubs in `bindings/`.
- Process protocol: simple JSON‑lines over stdin/stdout for any language to read state and emit control inputs; avoids Python.

Memory Index
- Lightweight Go indexer (`tools/memory_indexer`) scans chosen directories, hashes content, and writes `data/memory_index.jsonl` with stats, preserving privacy by keeping data local.
- Supports `.md .txt .json .yml .cpp .hpp .h .c .rs .go .js .ts .java .cs .swift .kt .m .mm`.

Safety & Non‑Destructive Operation
- No writes occur outside `~/SEFERIM_AGI_G16`.
- Indexer reads only user‑provided roots; full‑home scan is opt‑in via CLI flags.

