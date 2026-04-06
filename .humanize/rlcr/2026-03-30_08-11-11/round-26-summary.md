# Round 26 Summary

## Work Completed

### Blocking Issue 1: AC-2 regression — preprocessor double-shift (fixed)

`preprocessor.cpp` lines 730-742 had a manual `neigh_vert += 1` loop that was a pre-Round-24 workaround for a bug in `write_mesh`. Round 24 fixed `write_mesh` (io.cpp:580) to perform the 0→1 conversion unconditionally for all valid entries (`ni >= 0`). The manual loop then caused a double-shift, breaking 4 PreprocessorOracle tests.

Fix: removed lines 733-739 (the manual loop and the `mesh_out` copy). The `write_mesh` call now uses `meshT` directly (comment added to document the 0-based storage invariant of `meshT.connect[*].neigh_vert`).

All 5 tests pass: `PreprocessorOracle.ArchivedCompressionCaseMatchesOracle`, `ArchivedCyclicPreproInputMatchesOracleOutputs`, `ArchivedSelfContactPreproInputMatchesOracleOutputs`, `ArchivedBilayerTwistVdw1000PreproInputMatchesOracleOutputs`, and `RoundTrip.Mesh`.

### Blocking Issue 2: task3e — flag_num_diff S_n/S_m oracle (added)

Extended `dump_element_energy_oracle.f90` to compute a second case: element 83 with z=0
(flat geometry). Changed the oracle interface: the second argument is now `<oracle-dir>`
(parent of `archived_compression_np1/` and `flat_geom_np1/`). The oracle writes:

1. `archived_compression_np1/case_01.dat` — unchanged (same format, same values)
2. `flat_geom_np1/case_01.dat` — extended format with per-Gauss flag_num_diff, S_n[3], S_m[3]

**Key discovery**: The Fortran `principal_` subroutine takes `flag_num_diff` as an INPUT
(not output). Canonical `ener_elem.f90` passes the same `flag_num_diff` variable (set by
the earlier `principal(...)` call) to `principal_` in the numerical-diff loop. If an
uninitialized variable is passed, `principal_` takes the wrong eigenvector branch and
produces NaN vppal → NaN S_n/S_m. Fixed in oracle by passing `flag_num_diff` (not
`flag_dummy`) to `principal_`.

Added `ElementEnergy.FlagNumDiffStressesMatchFortranOracle`:
- Reads `flat_geom_np1/case_01.dat` (20 rows for ngauss=2)
- Verifies W_elem (tol 1e-6), f_elem (rel tol 1e-7), and per-Gauss S_n/S_m (tol 1e-6)
- Asserts S_n[i] == S_m[i] exactly (both use identical C_elem perturbation formula)
- Tolerance rationale: h=1e-8 one-sided FD cancellation + gfortran/g++ rounding order gives
  differences up to ~3e-7; the exact equality assertion is the definitive test of the Round-25
  S_m fix

## Files Changed

- **modified** `src/core/preprocessor.cpp`: removed manual `neigh_vert += 1` loop; direct call
  to `io::write_mesh(sep + "nano_Mesh.dat", meshT, d.ngauss)` with comment
- **modified** `test/cases/tools/dump_element_energy_oracle.f90`: extended to compute flat (z=0)
  case for element 83; oracle-dir semantics changed from archived subdir to parent dir; fixed
  `flag_dummy` → `flag_num_diff` in `principal_` calls; added `write_stress_fixture` subroutine
- **created** `test/cases/element_energy_oracle/flat_geom_np1/case_01.dat`: Fortran-derived
  W_elem, f_elem, and per-Gauss S_n/S_m for element 83 with flat geometry (20 rows)
- **modified** `test/cases/element_energy_oracle/build_provenance.md`: documents new flat fixture
  format, principal_ input semantics, and updated reproduction recipe
- **modified** `test/unit/test_element_energy.cpp`: added `FlagNumDiffStressesMatchFortranOracle`

## Validation

```
./build/unit_tests
[==========] 60 tests from 22 test suites ran.
[  PASSED  ] 60 tests.

./build/integration_tests
[==========] 18 tests from 2 test suites ran.
[  PASSED  ] 18 tests.
```

All 60 unit tests pass (59 prior + 1 new). All 18 integration tests pass (4 previously
failing PreprocessorOracle tests now pass; 14 previously passing tests still pass).

## Remaining Items

- `task3a`: exponential-map kernel — Fortran-derived oracle fixtures still missing
- `task3c` (Brenner): tested via synthetic fixtures only; no end-to-end element-energy path
- `task3f` (principal curvature): Fortran-derived oracle fixtures still missing
- `task4a` onward: global assembly, L-BFGS, pasapas, reaction force, end-to-end serial run

## BitLesson Delta

- Action: add
- Lesson ID(s): BL-20260404-principal-flag-input
- Notes: The Fortran `principal_` subroutine takes `flag_num_diff` as a BIDIRECTIONAL
  argument (read before write in the degenerate branch). Canonical `ener_elem.f90` always
  passes the same `flag_num_diff` variable that was set by the previous `principal(...)` call.
  In any oracle or translation that calls `principal_` in a numerical-diff loop, pass the
  already-computed `flag_num_diff` — not a fresh uninitialized variable. If an uninitialized
  variable is passed, `principal_` silently takes the wrong eigenvector branch and produces
  NaN vppal for flat/degenerate geometry.
