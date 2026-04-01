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
- `newton_inner/case_01.dat` … `newton_inner/case_10.dat`
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

## Archived-State Provenance Gap

As of Round 15, the archived simulator artifacts committed under `test/cases/graphene_compression_simulator/`
and `test/cases/graphene_cyclic_crumple/simulator_run/` do not contain the 10 intermediate per-load-step
Newton states requested by the plan for AC-6. The available simulator-side state files are:

- final-state `nano_config.dat` outputs written by `write_config(...)`
- cycle-end `nano_checkpoint.dat` outputs written by `write_checkpoint(...)`

The frozen Fortran sources confirm that `write_checkpoint(...)` runs only at the end of each complete
compression-release cycle, and `write_config(...)` writes only the final state. That means the current
committed oracle repository exposes final or cycle-end `eta` fields, but not a sequence of 10 archived
load-step extracts suitable for direct `newton_inner` fixture generation. Until those intermediate states
are captured from the frozen simulator, the committed `newton_inner/case_01.dat` … `case_10.dat` corpus
remains helper-generated rather than archived-state derived.
