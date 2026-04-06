# Round 25 Contract

## Mainline Objective

**Close task3e canonically**: fix the `flag_num_diff` S_m branch to match the checked-in
Fortran, then add and pass a Fortran-backed oracle test for `f_elem`.

## Target ACs

- **AC-7** (partial closure): `compute_element_energy` now has Fortran-oracle force coverage;
  `flag_num_diff` S_m branch is faithful to canonical `ener_elem.f90`.

## Blocking Issues

1. **flag_num_diff S_m mismatch** — In `ener_elem.f90` lines 76-84 the S_m bending-stress
   loop is identical to the S_n loop: it also perturbs `C_elem_` (not `curv0_elem_`), making
   `S_m == S_n` in the degenerate case. The C++ perturbs `curv0_elem` instead. Fix:
   change `element_energy.cpp` lines 96-102 so S_m also perturbs `C_elem` (matching Fortran).

2. **No direct Fortran-backed oracle for f_elem** — The existing tests verify `W_elem` and `eta`
   against archived fixtures, and test `f_elem` only via C++-internal finite differences.
   Fix: write `test/cases/tools/dump_element_energy_oracle.f90`, run it, commit the fixture
   `test/cases/element_energy_oracle/archived_compression_np1/case_01.dat`, and add a C++
   test that reads the fixture and asserts `f_elem` within 1e-8 absolute tolerance.

## Queued Issues (out of scope this round)

- task3f: Fortran-derived principal-curvature oracle fixtures
- task4a onward: global assembly, L-BFGS, pasapas, reaction force, etc.

## Plan Evolution

The canonical Fortran `ener_elem.f90` S_m loop (lines 76-84) is identical to the S_n loop:
both perturb `C_elem_`. This is a probable copy-paste defect in the Fortran code (bending
stress should perturb the curvature tensor, not the metric), but since we need to match the
canonical oracle we must replicate it exactly. The analytical path (flag_num_diff=false) is
unaffected — it correctly uses `dpedk` for S_m. The flag_num_diff path only fires for
near-flat geometry, which is a rare degenerate case in practice.

## Concrete Success Criteria

1. `element_energy.cpp` S_m branch (flag_num_diff path) perturbs `C_elem` (not `curv0_elem`).
2. Fixture `test/cases/element_energy_oracle/archived_compression_np1/case_01.dat` is
   committed (Fortran-derived f_elem for element 83, analytical path).
3. `ElementEnergy.FElemMatchesFortranOracle` passes: all 12×3 = 36 `f_elem` components
   agree with the Fortran fixture within 1e-8 absolute tolerance.
4. `ElementEnergy.ForcesAreConsistentWithEnergyByFiniteDifference` still passes (C++ FD
   self-consistency for the analytical path).
5. `ElementEnergy.FlagNumDiffPathProducesFiniteEnergyAndForces` still passes (S_m==S_n in
   flag_num_diff path, all forces finite).
6. All 58 existing unit tests still pass (no regressions).
