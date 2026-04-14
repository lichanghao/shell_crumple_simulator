# Round 12 Summary

## Work Completed
- Added a second step-one runtime consistency check:
  `E2ECompression.GeneratedStepOneVtuMatchesGeneratedEnergyAndReactionRows`
  in `test/integration/test_e2e_compression.cpp`.
- This complements `CompressionCaseFiles.ArchivedStepOneVtuMatchesArchivedEnergyAndReactionRows` by
  proving that the generated step-one VTU snapshot, generated `energy.dat` row, and generated
  `force.dat` row are internally consistent under the current translated assembly/reaction logic.

## Files Changed
- `test/integration/test_e2e_compression.cpp`

## Validation
- `cmake --build build --target integration_tests -j4`
  - Passed.
- `./build/integration_tests --gtest_filter='CompressionCaseFiles.ArchivedStepOneVtuMatchesArchivedEnergyAndReactionRows:E2ECompression.GeneratedStepOneVtuMatchesGeneratedEnergyAndReactionRows'`
  - Passed both tests.
- The main executable-path AC-7 gate remains red:
  - `./build/integration_tests --gtest_filter='E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace'`
  - still fails with:
    - energy relative error `1.3407384932259225`
    - `GNORM` relative error `0.068245136318090996`
    - force-column relative errors `1.3407271675185646`, `0.91213791706456004`, `0.55602311548162575`

## Remaining Items
- The archived step-one VTU / row set is internally consistent.
- The generated step-one VTU / row set is also internally consistent.
- Therefore the remaining AC-7 step-one failure is in the live constrained trajectory itself:
  the solver is converging to the wrong state, not merely writing inconsistent outputs.
- After that, the remaining original-plan runtime work is still open:
  - reaction-force parity / 50-step run
  - runtime vdW/self-contact
  - cyclic loading / crease memory
  - checkpoint/restart
  - MPI acceptance parity
  - executable-path real-`nvdw=1` VTU/PVD coverage
- `bitlesson-selector` remained unavailable in this environment (`zsh:1: command not found: bitlesson-selector`).

## Goal Tracker Update Request

### Requested Changes:
- Update `task4d` notes or the corresponding open issue to record that both the archived and
  generated step-one VTU snapshots are internally consistent with their own `energy.dat` /
  `force.dat` rows under the current translated assembly/reaction logic.

### Justification:
- This round removes another diagnostic branch cleanly: the remaining step-one failure is no longer
  attributable to inconsistent row generation or reaction post-processing. The bug surface is now
  the live constrained state trajectory itself.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable lesson was added; this round added a generated-step-one runtime consistency check.
