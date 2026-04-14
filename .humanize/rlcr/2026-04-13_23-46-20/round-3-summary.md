## Work Completed

- Extended `test/cases/first_constrained_step_oracle/` with
  `element83_full_oracle.dat`, which archives the fuller same-trace Fortran analytical surface for
  the reconstructed first constrained-step element-83 state:
  - per-Gauss `C_elem`, `curv0_elem`, `curvppal`, `vppal`, `flag_num_diff`, prepared `pe`,
    converged `eta`, `W`, `ddWdeta`, iteration count, and fail mode
  - element-level `W_elem` and `f_elem`
- Expanded `test/unit/test_first_constrained_step_oracle.cpp` so the standalone exact-state unit
  gate now reads that fuller oracle and checks:
  - `compute_element_state`
  - `solve_inner_newton`
  - `prepare_element_state`
  - `compute_element_energy`
- Updated `test/cases/first_constrained_step_oracle/build_provenance.md` to document the full-surface
  oracle payload and the stronger unit regression contract.

## Files Changed

- `test/cases/first_constrained_step_oracle/build_provenance.md`
- `test/cases/first_constrained_step_oracle/element83_full_oracle.dat`
- `test/unit/test_first_constrained_step_oracle.cpp`

## Validation

- `cmake --build build --target unit_tests -j4`
  - Passed.
- `./build/unit_tests --gtest_filter='FirstConstrainedStepOracle.Element83UnitFixtureMatchesCommittedFortranOracle'`
  - Failed as expected on the fuller archived first-step surface.
  - The stronger exact-state unit gate now shows that the reconstructed first-step mismatch is not
    only in final `eta` / `W_elem`; it also appears in prepared bond `pe`.
  - Representative failures from the fuller gate:
    - gauss 1 `pe[4]`: actual `2.0884071446376438`, expected `2.0904391082852567`
    - gauss 2 `pe[4]`: actual `2.030868999631207`, expected `2.053526924525376`
    - gauss 1 `eta`: actual `[8.6123916408285331e-05, -1.8375415984645584e-04]`,
      expected `[1.7059377382830062e-04, -1.3871993792757202e-04]`
    - element `W_elem`: actual `1.4539364867024104e-01`, expected `1.5168019536748686e-01`
- `./build/unit_tests --gtest_filter='FirstConstrainedStepOracle.Element83UnitFixtureMatchesCommittedFortranOracle:ElementState.MatchesArchivedCompressionSimulatorOracleFixtures:ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures:ElementEnergy.FElemMatchesFortranOracle'`
  - Failed.
  - Existing archived kernel gates remain red with the same broader mismatch surface as before.

## Remaining Items

- The first-step exact-state oracle is now fuller, but AC-6 / AC-7 kernel parity is still broken.
  The active implementation target remains inside:
  - `compute_element_state`
  - `compute_principal_curvature`
  - `Hyper_pot_inner`
  - `solve_inner_newton`
  - `compute_element_energy`
- AC-7 executable-path parity remains blocked by the same kernel mismatch.
- AC-8 through AC-12 runtime work is still pending.
- `bitlesson-selector` was still unavailable in this environment (`zsh:1: command not found: bitlesson-selector`).

## Goal Tracker Update Request

### Requested Changes:
- Record that Round 3 extended the standalone exact-state oracle from a minimal
  `flag_num_diff` + `eta` + `W_elem` gate to a fuller analytical surface gate with archived
  `C_elem`, `curv0_elem`, `curvppal`, `vppal`, prepared `pe`, `ddWdeta`, `W`, `W_elem`, and `f_elem`.
- Update the first-step open issue to note that the exact-state unit gate now proves the mismatch
  reaches prepared-bond `pe` as well as final `eta` / `W_elem`.

### Justification:
- This round closes the remaining review complaint that the standalone exact-state oracle was too
  narrow. The repo now contains a fuller archived analytical surface for the reconstructed first-step
  state, and the unit test consumes it directly.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable lesson was added; this round strengthened the archived first-step oracle and
  exposed a broader prepared-bond mismatch on that state.
