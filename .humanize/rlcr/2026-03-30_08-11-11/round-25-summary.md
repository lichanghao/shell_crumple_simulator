# Round 25 Summary

## Work Completed

### Blocking Issue 1: flag_num_diff S_m branch mismatch (fixed)

The canonical `ener_elem.f90` lines 76-84 (S_m bending-stress loop) are identical to
the S_n loop (lines 66-74): both perturb `C_elem_`. The C++ `element_energy.cpp`
previously perturbed `curv0_elem` for S_m, which is more physically correct but
differs from the checked-in Fortran.

Fix: changed `element_energy.cpp` lines 96-102 so S_m also perturbs `C_elem`
(matching the canonical Fortran exactly). Comment updated to cite `ener_elem.f90`
lines 76-84. The flag_num_diff=true test still passes (S_m == S_n; all forces finite).

### Blocking Issue 2: Fortran-backed f_elem oracle (added)

Created `test/cases/tools/dump_element_energy_oracle.f90`:
- Reads the archived compression simulator state (dims, general, zero, config, mesh)
- Computes `ener_elem` inline for element 83 (1-based): both Gauss points, nW_hat=true,
  eta=0 initial condition, reference_curvature=0
- For the archived non-trivial geometry, `flag_num_diff=false`; uses analytical path:
  `def_bonds` → `My_Hyper_Pot` (inline Morse wrapper) → `My_Stresses` → force accumulation
- `energy.f90` is intentionally excluded because it contains an MPI include; `Hyper_Pot`
  and `Stresses` are reproduced as contained subroutines (`My_Hyper_Pot`, `My_Stresses`)

Fixture generated and committed:
`test/cases/element_energy_oracle/archived_compression_np1/case_01.dat`
```
83  2                      ← element 83, ngauss=2
1.06283279442793996E-007   ← W_elem
(12 rows of 3 values)      ← f_elem(1..12, 1..3)
```

Added `ElementEnergy.FElemMatchesFortranOracle`: reads the fixture and asserts all 36
`f_elem` components (12 nodes × 3 directions) within 1e-8 absolute. Passes on first run.

## Files Changed

- **modified** `src/core/element_energy.cpp`: S_m branch in flag_num_diff path now
  perturbs `C_elem` (was `curv0_elem`); matches canonical `ener_elem.f90` lines 76-84
- **created** `test/cases/tools/dump_element_energy_oracle.f90`: Fortran oracle driver
  for element-level energy/force computation (element 83, analytical path)
- **created** `test/cases/element_energy_oracle/archived_compression_np1/case_01.dat`:
  Fortran-derived W_elem and f_elem fixture for element 83
- **created** `test/cases/element_energy_oracle/build_provenance.md`: reproduction recipe
- **modified** `test/unit/test_element_energy.cpp`: add `ElementEnergy.FElemMatchesFortranOracle`

## Validation

```
./build/unit_tests
[==========] 59 tests from 22 test suites ran.
[  PASSED  ] 59 tests.

./build/integration_tests --gtest_filter=RoundTrip.Mesh
[  PASSED  ] 1 test.
```

All 59 unit tests pass (58 prior + 1 new). `RoundTrip.Mesh` still passes.
Pre-existing `PreprocessorOracle` failures remain unrelated to this round's work.

## Key Findings

**The Fortran S_m bug**: Both S_n and S_m loops in `ener_elem.f90` perturb `C_elem_`.
This is likely a copy-paste defect (bending stress should perturb curvature, not metric),
but since the `flag_num_diff` path only fires for near-flat degenerate geometry and the
archived simulator uses the analytical path for all non-trivial elements, the bug has no
practical effect. For fidelity to the canonical oracle, C++ now replicates it exactly.

**Fortran MPI dependency**: `energy.f90` includes `mpif.h` inside the global `energy`
subroutine. `Hyper_Pot` and `Stresses` are in the same file but don't need MPI. Rather
than compiling `energy.f90`, the oracle inlines both as standalone contained subroutines.
This pattern should be followed for future oracle programs that need outer-potential
or stress logic.

## Remaining Items

- `task3a`: exponential-map kernel is implemented but not exercised through a production
  simulator caller; still lacks Fortran-derived oracle fixtures
- `task3c` (Brenner): tested via synthetic fixtures only; no end-to-end element-energy path
- `task3f` (principal curvature oracle fixtures): still pending
- `task4a` onward: global assembly, L-BFGS, pasapas, reaction force, end-to-end serial run

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: The S_m loop copy-paste defect (both stress loops identical) is a Fortran bug.
  Faithfully replicating it was necessary for oracle consistency. The `energy.f90` MPI
  dependency is a recurring pattern: any new oracle that needs `Hyper_Pot` or `Stresses`
  must either inline them or find a way to separate them from `energy.f90`.
