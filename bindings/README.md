Bindings Overview (No Python)
-----------------------------

This project exposes a small C++ ABI from `g16_core` and defines a simple line‑oriented protocol so that components in many languages can interoperate without rewriting the math core.

Languages with planned stubs in this repo:
- Rust (`bindings/rust/`): crate wrapping C ABI using `cxx` or `bindgen`.
- Go (`bindings/go/`): `cgo` wrapper.
- Node (`bindings/node/`): N‑API addon.
- Java (`bindings/java/`): JNI loader.
- C# (`bindings/csharp/`): P/Invoke wrapper.
- Julia (`bindings/julia/`): `ccall` wrapper.
- Ruby (`bindings/ruby/`): `ffi` wrapper.
- Swift (`bindings/swift/`): SwiftPM system library target.

Note
- Python is intentionally excluded per request.
- Stubs are minimal to avoid forcing all toolchains on first build.

