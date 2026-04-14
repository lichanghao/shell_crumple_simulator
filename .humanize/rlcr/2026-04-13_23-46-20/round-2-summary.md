## Work Completed

- Archived the reconstructed exact first constrained-step element-83 patch under
  `test/cases/first_constrained_step_oracle/element83_state.dat` so the kernel entry state is now
  available from committed repo artifacts without rerunning `crunch_it`.
- Added a standalone unit-level red gate at `test/unit/test_first_constrained_step_oracle.cpp`.
  It reads the committed exact-state fixture directly and compares `compute_element_energy` against
  the committed same-trace Fortran oracle in `element83_expected.dat`, while also checking
  `compute_element_state(...).flag_num_diff` on both Gauss points.
- Updated `test/cases/first_constrained_step_oracle/build_provenance.md` to document the exact-state
  fixture and the new unit/integration regression split.
- Fixed `document/translation_notes.md` so the high-level status table no longer claims the
  constitutive kernel is fully verified while the archived `ElementState` / `ElementEnergy` gates
  are still red.

## Files Changed

- `CMakeLists.txt`
- `document/translation_notes.md`
- `test/cases/first_constrained_step_oracle/build_provenance.md`
- `test/cases/first_constrained_step_oracle/element83_state.dat`
- `test/unit/test_first_constrained_step_oracle.cpp`

## Validation

- `cmake --build build --target unit_tests integration_tests -j4`
  - Passed.
- `./build/unit_tests --gtest_filter='FirstConstrainedStepOracle.Element83UnitFixtureMatchesCommittedFortranOracle:ElementState.MatchesArchivedCompressionSimulatorOracleFixtures:ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures:ElementEnergy.FElemMatchesFortranOracle'`
  - Failed as expected.
  - New unit-level exact-state gate is now red directly from committed fixtures:
    - `W_elem`: actual `1.4539364867024104e-01`, expected `1.5168019536748686e-01`
    - `eta(gauss 1)`: actual `[8.6123916408285331e-05, -1.8375415984645584e-04]`,
      expected `[1.7059377382830062e-04, -1.3871993792757202e-04]`
    - `eta(gauss 2)`: actual `[1.1695652522516237e-03, -1.6496930596894301e-03]`,
      expected `[1.1767640905118179e-03, -1.5551411794653993e-03]`
  - Existing archived kernel gates remain red:
    - `ElementState.MatchesArchivedCompressionSimulatorOracleFixtures`
    - `ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures`
    - `ElementEnergy.FElemMatchesFortranOracle`
- `./build/integration_tests --gtest_filter='FirstConstrainedStepOracle.Element83ReplayMatchesCommittedFortranOracle:E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace'`
  - Failed with the same measured surface as before:
    - first constrained-step integration gate: same `eta` / `W_elem` mismatch as the new unit gate
    - step-1 executable-path gate: relative energy error `1.3407384932259225`,
      relative `GNORM` error `0.068245136318090996`,
      force-column relative errors `1.3407271675185646`, `0.91213791706456004`, `0.55602311548162575`

## Remaining Items

- The exact-state unit gate exists now, but it still only pins `flag_num_diff` plus final
  `eta` / `W_elem`. The broader requested full-surface parity (`C_elem`, `curv0_elem`, `curvppal`,
  `vppal`, prepared `pe`, `ddWdeta`, and where needed `f_elem`) is still not archived for this
  reconstructed state.
- The archived constitutive/kernel surface remains red before the executable-path step-1 replay
  branches, so the active implementation target is still inside `compute_element_state`,
  `compute_principal_curvature`, `Hyper_pot_inner`, and `solve_inner_newton`.
- AC-7 executable-path parity and the remaining plan items for AC-8 through AC-12 are still open.
- `bitlesson-selector` was still unavailable in this environment (`zsh:1: command not found: bitlesson-selector`).

## Goal Tracker Update Request

### Requested Changes:
- Record that Round 2 added `test/cases/first_constrained_step_oracle/element83_state.dat` plus
  `FirstConstrainedStepOracle.Element83UnitFixtureMatchesCommittedFortranOracle`, so the
  first constrained-step blocker is now reproducible both as an integration replay and as a
  standalone unit-level exact-state gate.
- Keep the “exact-state archival/unit-regression gap” open, but narrow it to the missing
  full-surface parity fields rather than the absence of any standalone exact-state fixture.
- Mark `task8c` progress on `document/translation_notes.md`: the high-level constitutive status
  table now matches the measured red kernel state.

### Justification:
- This round closes the review complaint that the first constrained-step evidence depended only on
  executable-path reconstruction. The repo now contains both the exact-state fixture and a unit test
  that consumes it directly, while the remaining gap is specifically the missing archived full-surface
  Fortran outputs for that state.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable lesson was solid enough to add; this round mostly converted an existing
  first-step replay diagnosis into a standalone fixture and unit gate.
