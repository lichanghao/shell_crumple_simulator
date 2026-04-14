# Round 10 Summary

## Work Completed
- Added a new integration-level archive-consistency check:
  `CompressionCaseFiles.ArchivedStepOneVtuMatchesArchivedEnergyAndReactionRows`
  in `test/integration/test_e2e_compression.cpp`.
- The new test reassembles the archived step-one VTU snapshot with the current translated kernels and
  verifies that the archived `mesh_config_0001.vtu`, `energy.dat`, and `force.dat` row describe the
  same physical state.

## Files Changed
- `test/integration/test_e2e_compression.cpp`

## Validation
- `cmake --build build --target integration_tests -j4`
  - Passed.
- `./build/integration_tests --gtest_filter='CompressionCaseFiles.ArchivedStepOneVtuMatchesArchivedEnergyAndReactionRows'`
  - Passed.
- The existing runtime blocker remains:
  - `./build/integration_tests --gtest_filter='E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace'`
  - still fails with the known step-one energy / `GNORM` / force-column mismatches.
- The build-tree full-run test still launches and then times out at 180 seconds:
  - `ctest --test-dir build -R 'E2ECompression.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts' --output-on-failure`

## Remaining Items
- The archived step-one runtime artifacts are internally consistent, so the remaining AC-7 step-one
  failure is a real executable-path bug, not an inconsistent oracle row.
- The remaining AC-7 blocker is still:
  - `E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace`
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
- Add to `task4d` notes or Open Issues: the archived step-one VTU snapshot is internally consistent
  with the archived `energy.dat` / `force.dat` row under the current translated assembly/reaction
  logic, so the remaining step-one mismatch is in the live executable path, not in the archived
  runtime oracle set.

### Justification:
- This round closes one more diagnostic branch cleanly. It rules out a stale or contradictory
  archived step-one runtime oracle and narrows the remaining work to the constrained minimization /
  runtime execution path itself.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable lesson was added; this round added a focused archive-consistency check for the step-one runtime oracle.
