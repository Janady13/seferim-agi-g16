Node Addon (N‑API) Stub
-----------------------

This folder will host a minimal N‑API addon that calls the `g16_cabi` static library.

Planned files (not required to build core):
- `binding.gyp`
- `index.js`
- `src/addon.cc`

Usage pattern
- Build C++ core with CMake to produce `libg16_cabi.a`.
- Compile the addon via `node-gyp` pointing to the static library.
- Expose `update(s_t, s_tm1, input) -> state`.

