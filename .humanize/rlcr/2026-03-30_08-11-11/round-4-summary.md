# Round 4 Summary

## Work Completed
- Added explicit preprocessor chirality validation before `theta` is computed. Invalid `nchir` values, negative chirality indices, zero-zero indices, and zero denominators in `2*xn1 + xn2` now fail with a clear `std::runtime_error` instead of silently emitting `NAN` bond vectors.
- Added the AC-2 invalid-input regression `PreprocessorOracle.InvalidChiralityInputIsRejected`, which mutates the archived compression `data.dat` to `nchir=0`, `xn1=xn2=0` and now verifies that `run_preprocessor(...)` throws.
- Added the AC-2 corrupted-output regression `PreprocessorOracle.CorruptedGeneratedMeshIsRejectedByOracleComparator`, which swaps generated mesh connectivity in `nano_Mesh.dat` and verifies that the oracle comparator rejects the corrupted output.
- Added out-of-domain guards to `BSpline`, `DBSpline`, and `DDBSpline`, plus a unit test that verifies all three evaluators reject coordinates outside the valid triangular parameter domain.
- Added the AC-4 wrong-anchor negative regression `GhostNodes.WrongAnchorChoiceDoesNotMatchExpectedGhostPosition`, proving that the incorrect parallelogram anchor choice does not satisfy the expected ghost-node position.
- Committed the code change as `91e6fa8` (`Add preprocessor negative coverage guards`).

## Files Changed
- `src/core/bspline.cpp`
- `src/core/preprocessor.cpp`
- `test/integration/test_prepro_oracle.cpp`
- `test/unit/test_bspline.cpp`
- `test/unit/test_ghost_nodes.cpp`

## Validation
- Red test run before the production fixes:
  - `cmake --build build --target unit_tests integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^(BSpline|GhostNodes|PreprocessorOracle)'`
  - Failed on `BSpline.OutOfDomainCoordinatesAreRejected` and `PreprocessorOracle.InvalidChiralityInputIsRejected`, confirming the missing guards.
- Targeted green verification after the fixes:
  - `cmake --build build --target unit_tests integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^(BSpline|GhostNodes|PreprocessorOracle)'`
  - Passed `9/9` targeted tests, including the new negative regressions.
- Full regression verification:
  - `ctest --test-dir build --output-on-failure`
  - Passed `31/31` tests.

## Remaining Items
- `task2g` remains pending. The preprocessor still uses preserved disabled-vdW span constants for `nano_tub_loc.dat`; the actual `vdw_previous`-equivalent preprocessing path and any `nvdw=1` oracle coverage are still missing.
- AC-3 remains partial. Round 4 added the out-of-domain negative guard, but the required interior and boundary oracle fixtures for the B-spline basis are still absent.
- Milestones 3-8 remain pending: constitutive kernels, simulator pipeline, VTU output, real vdW/self-contact runtime behavior, cyclic runtime logic, checkpoint/restart, MPI equivalence, and final documentation.

## Goal Tracker Update Request

### Requested Changes:
- Remove the blocking issue that says invalid chirality input is still accepted and can emit `NAN` bond vectors in `nano_general.dat`. Round 4 added both the preprocessor guard and the negative regression test.
- Remove the blocking issue that says AC-2 corrupted-output negative coverage is still missing. Round 4 added `PreprocessorOracle.CorruptedGeneratedMeshIsRejectedByOracleComparator`.
- Update the B-spline blocker to reflect that the out-of-domain guard and negative test now exist, while the required oracle fixtures are still pending.
- Update the AC-2 evidence row to include `PreprocessorOracle.InvalidChiralityInputIsRejected` and `PreprocessorOracle.CorruptedGeneratedMeshIsRejectedByOracleComparator`.
- Update the AC-4 evidence row to include `GhostNodes.WrongAnchorChoiceDoesNotMatchExpectedGhostPosition`, since the previously missing wrong-anchor negative regression now exists.

### Justification:
Round 4 was specifically targeted at the remaining Milestone 2 negative-coverage gaps surfaced by the Round 3 review. Those gaps should stop appearing as open blockers now that the corresponding guards and regressions are in place. The tracker should still keep AC-3 partial because oracle fixtures are still missing, and it should keep `task2g` pending because the real vdW preprocessing path is still unimplemented.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: Existing BitLessons were sufficient for this round. The new work extended negative coverage and simple guard logic, but it did not uncover a new cross-round project lesson beyond the disabled-vdW oracle trap already captured in Round 3.
