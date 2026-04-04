# Constitutive Oracle Provenance

## Oracle Repository

- **Path**: `../finite_crystal_elasticity/`
- **Frozen commit**: `7d3f77fab2378d675d14ebeac3d8e65d94221a4f`
- **Commit message**: "Document Modules 1–3 in build_and_run_notes.md"

## Fixture Scope

The committed files under `brenner/`, `newton_inner/`, and `archived_compression_np1/` are
fixed-output constitutive fixtures used by the C++ unit tests for the translated Brenner potential,
canonical prepared-bond path, and inner Newton relaxation path.

- `brenner/case_01.dat` … `brenner/case_10.dat`
  - Inputs: six-component `pe = [a1, a2, a3, theta23, theta31, theta12]`
  - Outputs: Fortran `W`, `dW/dpe`, and a finite-difference Hessian row set derived from the
    frozen `Inner_Brenner` implementation
- `newton_inner/case_01.dat` … `newton_inner/case_10.dat`
  - Inputs: `C_elem`, `curvppal`, `vppal`, initial `eta`, `crit`, `maxn`
  - Outputs: Fortran `newton_inner` iteration count, fail mode, converged `eta`, final
    `Hyper_pot_inner` energy, gradient, curvature Hessian, and `dW/dpe`
- `archived_compression_np1/case_01.dat` … `archived_compression_np1/case_10.dat`
  - Inputs: archived `(element, gauss)` selectors from the frozen serial compression simulator
    output under `test/cases/graphene_compression_simulator/np1/`
  - Outputs: Fortran `metric`, `curv`, `principal`, archived prepared-bond scalars
    (`A_norm`, `Ei`, `pe`), plus `newton_inner` and `Hyper_pot_inner` outputs evaluated on those
    archived simulator states

## Reproduction Helper

The synthetic Brenner and Newton fixtures are regenerated in this repository with:

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

The archived compression fixtures are regenerated in this repository with:

```bash
mkdir -p /tmp/archived_constitutive_mods /tmp/archived_constitutive_obj
rm -f /tmp/archived_constitutive_obj/*.o
gfortran -std=legacy -O0 -J /tmp/archived_constitutive_mods -c \
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
  test/cases/tools/dump_archived_constitutive_oracle.f90
mv *.o /tmp/archived_constitutive_obj/
gfortran -std=legacy -O0 -J /tmp/archived_constitutive_mods \
  -o /tmp/dump_archived_constitutive_oracle /tmp/archived_constitutive_obj/*.o
/tmp/dump_archived_constitutive_oracle \
  test/cases/graphene_compression_simulator/np1 \
  test/cases/constitutive_oracle/archived_compression_np1
```

## Archived Compression Fixture Provenance

`test/cases/tools/dump_archived_constitutive_oracle.f90` reads the committed serial compression oracle under
`test/cases/graphene_compression_simulator/np1/` and emits ten archived constitutive cases from the frozen
final-state simulator outputs.

- **Node positions** come from `nano_final_config.dat` — the fully deformed state after 50 load steps.
  This produces non-trivial `C_elem` and non-zero `curv0_elem` (bending curvature), unlike the flat initial
  state in `nano_config.dat` which yields identity metrics and zero curvatures.
- **Inner displacements** (`eta`): `nano_final_config.dat` stores zero eta (the Fortran simulator resets eta
  after each accepted Newton step and does not write the per-Gauss-point converged eta back to the final
  config file). The helper therefore starts Newton from `eta=0` on the deformed geometry, which is the
  same initial condition used inside the frozen simulator's element loop.
- **Mesh connectivity** comes from `nano_Mesh.dat`; **reference deformation gradients** from `nano_zero.dat`.
- The helper selects the first five elements whose 12-node `neigh_vert` patch stays entirely within the
  real-node range, then emits both Gauss points for each selected element.
- The committed selectors are element IDs `83` through `87` with Gauss points `1` and `2`.
- For each case the helper evaluates frozen Fortran `metric`, `curv`, `principal`, and `def_bonds_`
  on the deformed simulator coordinates, then runs `newton_inner` and `Hyper_pot_inner` with `crit=1e-8`
  and `max_iter=100`.

**Non-triviality guarantee**: the committed fixtures have `|curv0_elem[0]| > 1e-4` (element 83 yields
`curv0_elem[0] ≈ -0.043`), confirming that the geometry state is genuinely deformed, not the flat
undeformed initial configuration. The C++ regression asserts this property explicitly.
