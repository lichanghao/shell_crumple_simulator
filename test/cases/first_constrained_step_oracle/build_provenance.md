# First Constrained-Step Oracle Provenance

## Scope

`element83_expected.dat` captures the authoritative same-trace Fortran oracle used to diagnose the
AC-7 blocker on the very first constrained-step energy evaluation.
`element83_state.dat` archives the reconstructed 12-node element-83 patch coordinates from that same
authoritative Fortran replay so unit tests can exercise the kernel directly without rerunning `crunch_it`.
`element83_full_oracle.dat` archives the fuller analytical Fortran surface on that same authoritative state:
`C_elem`, `curv0_elem`, `curvppal`, `vppal`, prepared `pe`, converged `eta`, `W`, `ddWdeta`,
element `W_elem`, and `f_elem`.
`reconstructed_case/` archives the reconstructed simulator inputs used to regenerate
`element83_full_oracle.dat` from committed source without relying on ad-hoc `/tmp` state.

- Element: `83` (1-based)
- Gauss points: `2`
- Expected values:
  - `W_elem = 1.45393670059607155e-01`
  - `eta(gauss 1) = [8.61239151820701165e-05, -1.83754200797972111e-04]`
  - `eta(gauss 2) = [1.16956533467332466e-03, -1.64969318401331591e-03]`
  - `flag_num_diff = false` at both Gauss points
- Full-surface values:
  - Per-Gauss `C_elem`, `curv0_elem`, `curvppal`, `vppal`, prepared `pe`, converged `eta`,
    `W`, and `ddWdeta`
  - Element-level `W_elem` and `f_elem`

## Source

These values were captured from the authoritative same-trace Fortran replay described in:

- `.humanize/rlcr/2026-04-13_23-46-20/round-0-summary.md`
- `document/translation_notes.md`

The replay reconstructed the first constrained-step evaluation state by:

1. Replaying the archived compression case with the committed `imperfection_trace_fortran.dat`
   using a patched canonical Fortran `crunch_it` that dumps `step1_after_increment.dat` and
   `step1_after_imperfection.dat` on load step 1.
2. Restoring constrained DOFs from `step1_after_increment.dat` onto `step1_after_imperfection.dat`
   so the reconstructed state matched the `long(..., x0_BC, ...)` entry state in `minimize.f90`.
3. Replacing the coordinate block in `nano_final_config.dat` with that reconstructed state and
   archiving the resulting helper inputs under `reconstructed_case/`.
4. Running the self-contained Fortran helper programs based on
   `test/cases/tools/dump_element_energy_oracle.f90` and
   `test/cases/tools/dump_first_step_full_oracle.f90` on `reconstructed_case/`.

Important: `element83_full_oracle.dat` is generated with the converged same-trace Fortran `eta`
for each Gauss point before evaluating the stored prepared-bond `pe` surface. The helper computes
the final `W_elem` and `f_elem` internally from committed source as well; it does not depend on any
disposable `/tmp` binaries.

## Current Regression Contract

- `test/integration/test_first_constrained_step_oracle.cpp` reconstructs the first constrained-step
  entry state from committed replay inputs and compares the translated kernel against the committed
  Fortran oracle.
- `test/unit/test_first_constrained_step_oracle.cpp` reads `element83_state.dat`,
  `element83_expected.dat`, and `element83_full_oracle.dat` directly and keeps a standalone
  exact-state gate on the archived first-step kernel surface.

Current measured state:

- `FirstConstrainedStepOracle.Element83ReplayMatchesCommittedFortranOracle` is green.
- `FirstConstrainedStepOracle.Element83UnitFixtureMatchesCommittedFortranOracle` is green.
- The remaining analytical red gates are the broader archived constitutive/kernel regressions:
  `ElementState.MatchesArchivedCompressionSimulatorOracleFixtures`,
  `ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures`,
  and `ElementEnergy.FElemMatchesFortranOracle`.

## Reproduction

From the C++ repository root, rebuild the full first-step oracle from committed inputs with:

```bash
mkdir -p /tmp/first_step_mods /tmp/first_step_obj
gfortran -std=legacy -O0 -J /tmp/first_step_mods -c \
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
  test/cases/tools/dump_first_step_full_oracle.f90
mv ./*.o /tmp/first_step_obj/
gfortran -std=legacy -O0 -J /tmp/first_step_mods -o /tmp/dump_first_step_full_oracle \
  /tmp/first_step_obj/headers.o /tmp/first_step_obj/Taylor.o /tmp/first_step_obj/BSpline.o \
  /tmp/first_step_obj/gauss.o /tmp/first_step_obj/geometry.o /tmp/first_step_obj/principal.o \
  /tmp/first_step_obj/exponential.o /tmp/first_step_obj/brenner.o /tmp/first_step_obj/brenner2.o \
  /tmp/first_step_obj/morse.o /tmp/first_step_obj/mm3.o /tmp/first_step_obj/Hyper_pot_inner_alg.o \
  /tmp/first_step_obj/newton_inner.o /tmp/first_step_obj/dump_first_step_full_oracle.o
/tmp/dump_first_step_full_oracle \
  test/cases/first_constrained_step_oracle/reconstructed_case \
  test/cases/first_constrained_step_oracle/element83_full_oracle.dat
```
