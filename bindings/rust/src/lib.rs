#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct G16MetaState {
    pub g: [f64; 17],
    pub coherence: f64,
    pub value: f64,
    pub uncertainty: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct G16Inputs {
    pub dx_norm: f64,
    pub ed_error: f64,
    pub utility: f64,
    pub stability: f64,
}

extern "C" {
    fn g16_update(
        s_t: *const G16MetaState,
        s_tm1: *const G16MetaState,
        input: *const G16Inputs,
        out: *mut G16MetaState,
    );
}

pub fn update(s_t: &G16MetaState, s_tm1: &G16MetaState, input: &G16Inputs) -> G16MetaState {
    let mut out = G16MetaState::default();
    unsafe {
        g16_update(s_t as *const _, s_tm1 as *const _, input as *const _, &mut out as *mut _);
    }
    out
}

