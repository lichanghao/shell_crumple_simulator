## Work Completed

- Removed the hidden `/tmp` dependency from `test/cases/tools/dump_first_step_full_oracle.f90`.
  The helper now computes the first-step `W_elem` and `f_elem` internally from committed source
  instead of shelling out to `/tmp/dump_element_energy_oracle_round4`.
- Updated `test/cases/first_constrained_step_oracle/build_provenance.md` so it explicitly describes
  the self-contained helper path and no longer implies a disposable `/tmp` binary is required for
  reproducing `element83_full_oracle.dat`.
- Updated `document/translation_notes.md` so the AC-7 status note matches the verified Round 5
  state: replay gate green, exact-state unit gate green, broader archived constitutive/kernel gates
  still red.

## Files Changed

- `document/translation_notes.md`
- `test/cases/first_constrained_step_oracle/build_provenance.md`
- `test/cases/tools/dump_first_step_full_oracle.f90`

## Validation

- Rebuilt the standalone first-step full-oracle helper from committed source and reran it locally.
  The regenerated output matched the committed `element83_full_oracle.dat`.
- `cmake --build build --target unit_tests integration_tests -j4`
  - Passed.
- `./build/unit_tests --gtest_filter='FirstConstrainedStepOracle.Element83UnitFixtureMatchesCommittedFortranOracle'`
  - Passed.
- `./build/unit_tests --gtest_filter='ElementState.MatchesArchivedCompressionSimulatorOracleFixtures:ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures:ElementEnergy.FElemMatchesFortranOracle:FirstConstrainedStepOracle.Element83UnitFixtureMatchesCommittedFortranOracle'`
  - Failed only on the broader archived constitutive/kernel gates:
    - `ElementState.MatchesArchivedCompressionSimulatorOracleFixtures`
    - `ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures`
    - `ElementEnergy.FElemMatchesFortranOracle`

## Remaining Items

- The first-step helper/provenance/documentation cleanup is done.
- The remaining analytical blocker is now the broader archived constitutive surface, not the
  first-step exact-state oracle path.
- After those archived kernel gates are green, the next blocker remains
  `E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace`.
- AC-8 through AC-12 runtime work is still pending.
- `bitlesson-selector` remained unavailable in this environment (`zsh:1: command not found: bitlesson-selector`).

## Goal Tracker Update Request

### Requested Changes:
- Mark `task8c` progress: `document/translation_notes.md` now matches the current first-step gate
  state and no longer reports the exact-state unit gate as red.
- Remove or close the open issue about the `/tmp/dump_element_energy_oracle_round4` dependency in
  the first-step full-oracle helper, since the helper now computes `W_elem` / `f_elem` internally
  from committed source.
- Record Round 6 progress as “self-contained first-step full-oracle helper and aligned AC-13 note,”
  while leaving the broader archived constitutive/kernel gates open.

### Justification:
- This round closes the remaining AC-13/provenance hardening work around the first-step oracle path.
  The first-step fixtures, unit gate, replay gate, provenance note, and translation notes are now
  consistent with each other and reproducible from committed source.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable lesson was added; this round was self-containment and documentation alignment.
