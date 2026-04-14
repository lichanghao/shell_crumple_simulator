# Round 9 Summary

## Work Completed
- Fixed the build-tree path handling in `test/integration/test_e2e_compression.cpp` so the
  integration binary uses the target-provided absolute `ORACLE_DIR` / `CRUNCH_IT_BIN` values instead
  of the stale relative fallback macros.
- Sanitized `CMAKE_CXX_FLAGS` in `CMakeLists.txt` to strip legacy relative-path `ORACLE_DIR` and
  `CRUNCH_IT_BIN` macro injections that were overriding the target-specific absolute definitions for
  the test binaries.

## Files Changed
- `CMakeLists.txt`
- `test/integration/test_e2e_compression.cpp`

## Validation
- `cmake -S . -B build && cmake --build build --target integration_tests -j4`
  - Passed.
- `ctest --test-dir build -R '^E2ECompression\\.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts$' --output-on-failure`
  - No longer fails immediately in `E2ECompression::SetUp()` on the missing relative
    `test/cases/...` path.
  - Now reaches runtime execution and times out at `180.06 sec`.
- Previously refreshed archived kernel gates remain green:
  - `ElementState.MatchesArchivedCompressionSimulatorOracleFixtures`
  - `ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures`
  - `ElementEnergy.FElemMatchesFortranOracle`
  - `FirstConstrainedStepOracle.Element83UnitFixtureMatchesCommittedFortranOracle`
- The executable-path constrained-step gate remains red:
  - `E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace`
  - energy relative error `1.3407384932259225`
  - `GNORM` relative error `0.068245136318090996`
  - force-column relative errors `1.3407271675185646`, `0.91213791706456004`, `0.55602311548162575`

## Remaining Items
- The `ctest` setup-path bug is fixed, but the full 50-step runtime-artifact test still times out.
- The remaining AC-7 blocker is still the executable-path constrained-step regression:
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
- Update `task8d` notes to reflect that the build-tree integration-suite path assumption is fixed:
  the representative `ctest` invocation now reaches solver execution instead of failing immediately
  in test setup, but still times out at 180 seconds.
- Record Round 9 progress as “ctest path reliability restored, timeout remains,” while keeping the
  AC-7 executable-path regression and the 50-step timeout as open blockers.

### Justification:
- This round closes a real acceptance-harness defect in the integration suite. The remaining
  `task8d` problem is now runtime cost / physics parity, not a broken test harness path.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable lesson was added; this round fixed the build-tree integration-test path semantics.
