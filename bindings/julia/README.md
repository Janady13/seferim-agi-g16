Julia ccall Stub
----------------

Example usage once a shared library is built:

```julia
# struct layouts must match C ABI
struct G16MetaState
    g::NTuple{17, Cdouble}
    coherence::Cdouble
    value::Cdouble
    uncertainty::Cdouble
end
struct G16Inputs
    dx_norm::Cdouble
    ed_error::Cdouble
    utility::Cdouble
    stability::Cdouble
end
ccall((:g16_update, "libg16_cabi"), Cvoid,
      (Ref{G16MetaState}, Ref{G16MetaState}, Ref{G16Inputs}, Ref{G16MetaState}),
      s_t, s_tm1, inp, out)
```

