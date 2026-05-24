Ruby FFI Stub
-------------

Use the `ffi` gem to map `g16_update`:

```ruby
require 'ffi'
module G16
  extend FFI::Library
  ffi_lib 'libg16_cabi'
  class MetaState < FFI::Struct
    layout :g, [:double, 17], :coherence, :double, :value, :double, :uncertainty, :double
  end
  class Inputs < FFI::Struct
    layout :dx_norm, :double, :ed_error, :double, :utility, :double, :stability, :double
  end
  attach_function :g16_update, [:pointer, :pointer, :pointer, :pointer], :void
end
```

