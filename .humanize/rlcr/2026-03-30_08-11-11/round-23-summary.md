# Round 23 Summary

## Work Completed

### Blocking Issue 1: neigh_vert indexing bug in io.cpp (fixed)

`read_mesh` was treating column 0 of each mesh neighbor pair as a ghost flag (`0 = real node,
nonzero = ghost`), but column 0 is actually the neighboring element index (1-based, always nonzero
for interior elements). For elements 83–87 (all interior), this prevented the 1-based→0-based
conversion of `neigh_vert`, shifting all 12 neighbor node coordinates by +1. The fix in
`src/core/io.cpp`: always apply `ni - 1` for any `ni > 0`, regardless of the neighboring element
column.

### Blocking Issue 2: dW_dpe artifact in archived compression test (fixed)

The Fortran `Inner_Morse` subroutine (nCode_Pot=1) never writes its `dW/dpe` array; gfortran -O0
zero-initializes the local, so the fixture stores zeros. The C++ correctly computes dW/dpe. The
`dW_dpe` comparison was removed from `test_element_state.cpp` with an explanatory comment; dW/dpe
correctness is covered by the synthetic Brenner oracle in `test_constitutive.cpp`.

### Blocking Issue 3: Non-trivial archived fixtures (already resolved in prior context)

`dump_archived_constitutive_oracle.f90` reads `nano_final_config.dat` (deformed state); fixtures
have `|curv0_elem[0]| > 1e-4` (element 83 yields `curv0_elem[0] ≈ -0.043`). The existing
non-triviality assertion in `test_element_state.cpp` verifies this property.

### task3e: Element-level energy/force kernel (new)

#### `evaluate_outer_potential` (constitutive.hpp/cpp)

Added `OuterPotentialOutput { double W; Vec6 dW; }` and `evaluate_outer_potential(mat, pe)`
implementing `Hyper_Pot` from `energy.f90`:
- Morse (nCode_Pot=1): uses existing `vstretch_bis` + `vangle_bis` private helpers in
  `constitutive.cpp`, same formula as inner but without eta chain rule
- Brenner (nCode_Pot=2): wraps `evaluate_brenner`, returning W and dW

#### `include/fce/element_energy.hpp` + `src/core/element_energy.cpp`

Direct translation of `ener_elem.f90`. Per Gauss-point loop:
1. Extract `dn` / `ddn` from `gauss.shapef[igauss]` → call `compute_element_state`
2. If `nW_hat`: `solve_inner_newton` → update eta on convergence; W and dW_dpe always valid from
   the final evaluation inside `solve_inner_newton_impl`
3. Compute Ei / A_norm from current eta (updated or not)
4. `flag_num_diff` path: `compute_deformed_bonds` + `evaluate_outer_potential` for base W; finite
   differences (h=1e-8) perturbing `C_elem` and `curv0_elem` for S_n and S_m
5. Analytical path: `compute_deformed_bonds_with_derivatives` + `evaluate_outer_potential` (only
   when `!nW_hat`); `Stresses` formula: `S_n[j] += dW[i]*dpedC[i][j]`, same for S_m
6. Force accumulation: `f_elem[node][k] += (S_n[ij]*dC[node][k][ij] + S_m[ij]*dcurv[node][k][ij])*weight`
7. Energy: `W_elem += W * weight`

#### `test/unit/test_element_energy.cpp` (4 new tests)

- `OuterPotential.MorseWMatchesInnerWAtConvergedEta`: recomputes Ei/A_norm at the fixture's
  converged eta, then calls `compute_deformed_bonds` and `evaluate_outer_potential`, verifying
  `outer.W == fixture.W` for all 10 archived fixtures.
- `OuterPotential.BrennerMatchesEvaluateBrenner`: verifies `evaluate_outer_potential` and
  `evaluate_brenner` agree on W and dW for a non-trivial Brenner pe vector.
- `ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures`: for each element pair
  (cases 01–02, 03–04, …, 09–10), calls `compute_element_energy` with `nW_hat=true` and verifies
  `result.eta[ig] ≈ fixture.eta[ig]` and `result.W_elem ≈ sum(fixture.W[ig] * weight[ig])`.
- `ElementEnergy.NoInnerRelaxationProducesZeroEtaAndNonzeroW`: verifies that with `nW_hat=false`
  eta is unchanged from zero and W_elem is finite non-zero.

## Files Changed

- **modified** `src/core/io.cpp`: fix `read_mesh` neigh_vert 1-based→0-based conversion (always
  apply for `ni > 0`, not conditioned on the neighboring element column)
- **modified** `include/fce/constitutive.hpp`: add `OuterPotentialOutput` struct and
  `evaluate_outer_potential` declaration
- **modified** `src/core/constitutive.cpp`: implement `evaluate_outer_potential` (Morse + Brenner)
- **created** `include/fce/element_energy.hpp`: `ElementEnergyResult` + `compute_element_energy`
- **created** `src/core/element_energy.cpp`: full `ener_elem.f90` translation
- **created** `test/unit/test_element_energy.cpp`: 4 unit tests (outer potential + element energy)
- **modified** `test/unit/test_element_state.cpp`: removed `dW_dpe` comparison from archived
  compression test with explanatory comment about Inner_Morse Fortran artifact
- **modified** `CMakeLists.txt`: added `src/core/element_energy.cpp` to `fce_core` and
  `test/unit/test_element_energy.cpp` to `unit_tests`
- **regenerated** `test/cases/constitutive_oracle/archived_compression_np1/case_*.dat`: fixed
  to use `nano_final_config.dat` source (non-trivial deformed state)
- **modified** `test/cases/tools/dump_archived_constitutive_oracle.f90`: reads `nano_final_config.dat`
- **modified** `test/cases/constitutive_oracle/build_provenance.md`: updated to document the
  `nano_final_config.dat` source, non-triviality guarantee, and fixture structure

## Validation

```
./build/unit_tests
[==========] 56 tests from 22 test suites ran.
[  PASSED  ] 56 tests.
```

All 56 unit tests pass (52 prior + 4 new). The pre-existing `RoundTrip.Mesh` integration test
failure is unrelated to this round's work (verified by git-stash regression: it fails on the
unmodified baseline too).

## Remaining Items

- `task3c` (Brenner): still only tested via synthetic fixtures; archived simulator states exercise
  Morse not Brenner, so the full Brenner-through-simulator path is not yet covered
- `task4a` (global assembly): `compute_element_energy` is implemented but not yet wired into a
  simulator-level element loop with MPI partitioning
- All Milestone 4–8 tasks remain pending

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: The neigh_vert indexing bug (column 0 = neighboring element, not ghost flag) was a
  format misread; the fix is straightforward but only discoverable by tracing the Fortran mesh
  reader. The Inner_Morse dW/dpe non-computation is a Fortran quirk (only the outer Morse writes
  dW); not a C++ bug.
