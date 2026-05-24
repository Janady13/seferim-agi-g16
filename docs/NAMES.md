Module Names – WhatItDoes(Equation)-(Family)
-------------------------------------------

- ContinuousDynamics(x' = F(x,t))-f1
- DiscreteDynamics(x_{t+1}=F(x_t,u))-f2
- ReactionDiffusion(D∇^2x+f(x))-f3
- Optimization(argmax U(x))-f4
- BayesianInference(P(θ|D)∝...)-f5
- LearningWeightUpdate(ΔW)-f6
- MutualInformation(I(X;Y))-f7
- RateDistortion(min I-βI)-f8
- ConsensusAveraging(Σ(xj-xi))-f9
- SwarmInteractionForce(Σf)-f10
- FieldAccumulator(φ_t=(1-ρ)φ_t+Δφ)-f11
- VariationalFreeEnergy(F)-f12
- CompressionControl(C)-f13
- MemoryTrace(Σ f_i dΩ)-f14
- DeecisiveAction(Policy)-f15
- GlobalCoherence(φ-accumulator)-f16

Each label maps 1‑to‑1 to functions in `include/common/g16_core.hpp` via `build_families()`.

