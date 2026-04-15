# Round 14 Summary

## Work Completed
- Added a new step-one geometry diagnostic:
  `E2ECompression.CrunchItStepOnePreservesArchivedBcNodeGeometry`
  in `test/integration/test_e2e_compression.cpp`.
- This test proves that the generated step-one snapshot preserves the archived constrained-node
  geometry exactly while the free-node geometry still drifts.

## Files Changed
- `test/integration/test_e2e_compression.cpp`

## Validation
- `cmake --build build --target integration_tests -j4`
  - Passed.
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItStepOnePreservesArchivedBcNodeGeometry'`
  - Passed.
- Existing runtime diagnostics now establish all of the following simultaneously:
  - `CompressionCaseFiles.ArchivedStepOneVtuMatchesArchivedEnergyAndReactionRows` is green
  - `E2ECompression.GeneratedStepOneVtuMatchesGeneratedEnergyAndReactionRows` is green
  - `E2ECompression.CrunchItStepOneVtuSnapshotMatchesArchivedOracle` is red
  - `E2ECompression.CrunchItStepOnePreservesArchivedBcNodeGeometry` is green
  - `E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace` is red

## Remaining Items
- The remaining AC-7 step-one mismatch is now isolated more tightly:
  - archived step-one VTU is internally consistent with archived row outputs
  - generated step-one VTU is internally consistent with generated row outputs
  - constrained-node geometry matches archive exactly
  - free-node geometry still diverges from archive
- That means the remaining bug lives in the free constrained state trajectory itself, not in:
  - load increment / BC scattering
  - row writing
  - reaction postprocessing consistency
  - archived step-one oracle consistency
- The primary red gates remain:
  - `E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace`
  - `E2ECompression.CrunchItStepOneVtuSnapshotMatchesArchivedOracle`
  - `E2ECompression.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts`
- AC-8 through AC-12 runtime work is still pending.
- `bitlesson-selector` remained unavailable in this environment (`zsh:1: command not found: bitlesson-selector`).

## Goal Tracker Update Request

### Requested Changes:
- Update `task4d` notes or the corresponding AC-7 open issue to record that step-one BC-node
  geometry already matches the archived VTU exactly; the remaining VTU mismatch is confined to
  free-node geometry.

### Justification:
- This round removes one more branch from the runtime search space. The remaining step-one bug is
  not in BC/load application; it is in the free-node constrained solve trajectory.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable lesson was added; this round added a focused BC-node geometry oracle for step 1.
