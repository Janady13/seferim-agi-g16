C# P/Invoke Stub
----------------

Plan:
- Build `g16_cabi` as a shared library (`.dylib`/`.so`).
- Define `[DllImport]` signatures for `g16_update`.

This keeps the core in C++ while enabling .NET interop without Python.

