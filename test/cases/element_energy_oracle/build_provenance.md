# Element Energy Oracle Provenance

## Oracle Repository

- **Path**: `../finite_crystal_elasticity/`
- **Frozen commit**: `7d3f77fab2378d675d14ebeac3d8e65d94221a4f`
- **Commit message**: "Document Modules 1–3 in build_and_run_notes.md"

## Fixture Scope

`archived_compression_np1/case_01.dat` is the canonical Fortran reference for the
`ElementEnergy.FElemMatchesFortranOracle` C++ test.

- **Element**: 83 (1-based), the first interior element in the compressed graphene mesh
- **ngauss**: 2
- **Path**: analytical stresses (`flag_num_diff=false`): `def_bonds` + `Stresses` +
  `Hyper_Pot` (inline Morse wrapper, nCode_Pot=1)
- **Initial condition**: `eta=0` per Gauss point (same as `compute_element_energy` test)
- **Source**: `nano_final_config.dat` (deformed state, same source as the constitutive oracle)

**Outputs stored**:
- `W_elem`: total element energy (sum over Gauss points of `W * weight`)
- `f_elem(12, 3)`: force accumulation for all 12 neighbor nodes × 3 directions

**flag_num_diff note**: Element 83 has non-trivial curvature (`|curv0_elem[0]| ≈ 0.043`),
so principal curvatures are distinct and `flag_num_diff=false`. The oracle uses the
analytical path. The C++ `flag_num_diff=true` path is tested separately in
`FlagNumDiffPathProducesFiniteEnergyAndForces` (smoke test, no Fortran oracle needed
because this path fires only for near-flat geometry absent from the archived simulator run).

**S_m note**: In the canonical `ener_elem.f90` (lines 76-84), the bending-stress S_m loop
is identical to the membrane-stress S_n loop: both perturb `C_elem_`. This is a probable
copy-paste defect in the Fortran (for bending stress, perturbing `curv0_elem_` is more
physical), but since the oracle uses the analytical path (`flag_num_diff=false`), the S_m
loop in `ener_elem.f90` is never reached for element 83. The C++ implementation was
updated in Round 25 to match the Fortran exactly in the `flag_num_diff` branch.

## Reproduction

```bash
mkdir -p /tmp/elem_energy_mods
gfortran -std=legacy -O0 -J /tmp/elem_energy_mods -c \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/headers.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/Taylor.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/BSpline.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/gauss.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/geometry.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/principal.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/exponential.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner2.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/morse.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/mm3.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/Hyper_pot_inner_alg.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/newton_inner.f90 \
  test/cases/tools/dump_element_energy_oracle.f90
gfortran -std=legacy -O0 -J /tmp/elem_energy_mods \
  -o /tmp/dump_element_energy_oracle *.o
rm -f *.o
/tmp/dump_element_energy_oracle \
  test/cases/graphene_compression_simulator/np1 \
  test/cases/element_energy_oracle/archived_compression_np1
```

Note: `energy.f90` (which contains `Hyper_Pot` and `Stresses` but also an MPI include) is
intentionally excluded. The oracle implements `My_Hyper_Pot` (Morse wrapper) and
`My_Stresses` as contained subroutines, matching the canonical `Hyper_Pot` and `Stresses`
semantics exactly.
