# Round 13 Summary

## Work Completed
- Added a direct stop-step1 VTU oracle comparison:
  `E2ECompression.CrunchItStepOneVtuSnapshotMatchesArchivedOracle`
  in `test/integration/test_e2e_compression.cpp`.
- This complements the two existing row-consistency diagnostics by checking the actual generated
  step-one runtime snapshot against the archived `mesh_config_0001.vtu` oracle.

## Files Changed
- `test/integration/test_e2e_compression.cpp`

## Validation
- `cmake --build build --target integration_tests -j4`
  - Passed.
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItStepOneVtuSnapshotMatchesArchivedOracle'`
  - Failed.
  - The generated step-one VTU geometry diverges strongly from the archived oracle:
    - `points "mesh_config_0001.vtu"` max relative error `3433.7172760570934`
- Previously added consistency checks remain green:
  - `CompressionCaseFiles.ArchivedStepOneVtuMatchesArchivedEnergyAndReactionRows`
  - `E2ECompression.GeneratedStepOneVtuMatchesGeneratedEnergyAndReactionRows`

## Remaining Items
- The generated step-one VTU is internally consistent with the generated `energy.dat` / `force.dat`
  row, and the archived step-one VTU is internally consistent with the archived row.
- The new direct VTU comparison proves the remaining AC-7 bug is the live constrained state
  trajectory itself: the solver is producing the wrong step-one geometry, not merely writing
  inconsistent rows from a correct state.
- The primary red gates remain:
  - `E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace`
  - `E2ECompression.CrunchItStepOneVtuSnapshotMatchesArchivedOracle`
  - `E2ECompression.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts`
- AC-8 through AC-12 runtime work is still pending.
- `bitlesson-selector` remained unavailable in this environment (`zsh:1: command not found: bitlesson-selector`).

## Goal Tracker Update Request

### Requested Changes:
- Update `task4d` notes / the corresponding open issue to record that the generated step-one VTU
  snapshot itself now has a direct failing oracle comparison against the archived `mesh_config_0001.vtu`.

### Justification:
- This round removes the last ambiguity about where the step-one failure lives. The remaining bug is
  in the live constrained state trajectory itself, not in row generation, reaction postprocessing,
  or internal consistency of the generated outputs.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable lesson was added; this round added a direct step-one VTU oracle comparison.
