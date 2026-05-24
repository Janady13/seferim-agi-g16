SwiftPM Stub
------------

Define a system library target that links to `libg16_cabi` and exposes a Swift wrapper struct.

Sketch:
- `Package.swift` with `.systemLibrary(name: "G16CABI")`
- `Sources/G16CABI/module.modulemap`
- `Sources/G16/G16.swift` bridging to C symbols.

