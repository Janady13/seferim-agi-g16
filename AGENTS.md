# Agents Guidelines (Project Scope)

This scaffold mirrors your C++20 conventions:
- Sources: `src/`, headers: `include/common` and `include/agents`.
- Executables: `apps/` (add new binaries here).
- Out-of-tree builds: `build/`.
- Tests: `tests/` (use CTest; keep them small and fast).
- Style: 2-space indent, braces on same line, `#pragma once`.
- Namespaces: `common::` and `agents::`.
- Logging: `spdlog` (not required to build this minimal scaffold; can be added later).

Naming convention mapping
- Per your request, components are labeled “What it does (equation)-(family)” in docs and module mappings (see `docs/NAMES.md`). Filenames are kept POSIX-friendly and short, while preserving the mapping in comments and docs to avoid fragile paths containing spaces/parentheses.

Safety
- Importers and indexers default to read-only scanning of user-provided directories. Full disk scans are opt-in and documented.

