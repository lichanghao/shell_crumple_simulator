# Constitutive Oracle Provenance

## Oracle Repository

- **Path**: `../finite_crystal_elasticity/`
- **Frozen commit**: `7d3f77fab2378d675d14ebeac3d8e65d94221a4f`
- **Commit message**: "Document Modules 1–3 in build_and_run_notes.md"

## Fixture Scope

The committed files under `brenner/` and `newton_inner/` are fixed-output constitutive fixtures
used by the C++ unit tests for the translated Brenner potential and inner Newton relaxation path.

- `brenner/case_01.dat` … `brenner/case_10.dat`
  - Inputs: six-component `pe = [a1, a2, a3, theta23, theta31, theta12]`
  - Outputs: Fortran `W`, `dW/dpe`, and a finite-difference Hessian row set derived from the
    frozen `Inner_Brenner` implementation
- `newton_inner/case_01.dat` … `newton_inner/case_04.dat`
  - Inputs: `C_elem`, `curvppal`, `vppal`, initial `eta`, `crit`, `maxn`
  - Outputs: Fortran `newton_inner` iteration count, fail mode, converged `eta`, final
    `Hyper_pot_inner` energy, gradient, curvature Hessian, and `dW/dpe`

## Reproduction Helper

The fixtures are regenerated in this repository with:

```bash
mkdir -p /tmp/constitutive_mods /tmp/constitutive_obj
gfortran -std=legacy -O0 -J /tmp/constitutive_mods -c \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/headers.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/Taylor.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner2.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/morse.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/mm3.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/Hyper_pot_inner_alg.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/newton_inner.f90 \
  test/cases/tools/dump_constitutive_oracle.f90
mv *.o /tmp/constitutive_obj/
gfortran -std=legacy -O0 -J /tmp/constitutive_mods \
  -o /tmp/dump_constitutive_oracle /tmp/constitutive_obj/*.o
/tmp/dump_constitutive_oracle test/cases/constitutive_oracle
```

The helper's material constants and input states are intentionally committed in
`test/cases/tools/dump_constitutive_oracle.f90` so the C++ fixtures stay reproducible and stable.
