# Round 2 Summary

## Work Completed
- Investigated the archived cyclic `nCodeLoad=31` preprocessor failure using the real `PrePro` executable path instead of the compression-only oracle harness.
- Reproduced the crash under AddressSanitizer and traced it to the late first-sheet BC metadata recomputation in `run_preprocessor()`, which re-entered `load_pre()` with a zero-sized scratch `BCData`.
- Fixed the crash by capturing the populated first-sheet BC metadata during the normal preprocessing loop and reusing that state when finalizing accumulated BC outputs, rather than calling `load_pre()` a second time on an invalid scratch buffer.
- Added a cyclic regression test that copies the archived `graphene_cyclic_crumple` `data.dat`, runs `fce::run_preprocessor(...)`, and verifies the expected `nano_*.dat` outputs are emitted without crashing.
- Committed the code change as `dd81f2d` (`Fix cyclic preprocessor crash`).

## Files Changed
- `src/core/preprocessor.cpp`
- `test/integration/test_prepro_oracle.cpp`

## Validation
- `ctest --test-dir build --output-on-failure -R '^PreprocessorOracle'`
  - Passed `PreprocessorOracle.ArchivedCompressionCaseMatchesOracle`
  - Passed `PreprocessorOracle.ArchivedCyclicPreproInputDoesNotCrash`
- `ctest --test-dir build --output-on-failure`
  - Passed `27/27` tests
- AddressSanitizer reproduction before the fix:
  - Built a standalone ASan `PrePro` binary and reproduced the cyclic crash in `src/core/load_pre.cpp`
  - Verified the crash no longer reproduces after the `run_preprocessor()` fix

## Remaining Items
- `nano_crease.dat` generation for `ncrease=1` is still not implemented, so cyclic preprocessor parity is still incomplete.
- `task2g` is still only a compression-case placeholder for `nano_tub_loc.dat`; the actual vdW preprocessing data generation required by AC-8 remains unimplemented.
- The AC-2 negative validation for invalid chirality input is still missing.
- The AC-3 oracle fixture coverage and out-of-domain B-spline validation are still missing.
- Milestones 3-8 remain pending: constitutive kernels, simulator pipeline, VTU output, vdW/self-contact physics, cyclic crease-memory behavior, checkpoint/restart, MPI equivalence, and final documentation.

## Goal Tracker Update Request

### Requested Changes:
- Update `task2f` evidence to reflect the new Round 2 cyclic regression coverage: the archived `nCodeLoad=31` preprocessor input now runs without crashing and is covered by `PreprocessorOracle.ArchivedCyclicPreproInputDoesNotCrash`.
- Move `task2g: vdW preprocessing` out of `completed` status. The current implementation only preserves the archived compression-case `nano_tub_loc.dat` span convention and does not implement the vdW preprocessing outputs or neighbor-list generation required by AC-8.
- Add an open issue that cyclic preprocessor parity is still incomplete because `nano_crease.dat` writing for `ncrease=1` is still missing.

### Justification:
These tracker corrections align the mutable state with the actual Round 2 evidence. The cyclic crash itself is fixed, but the review's broader Milestone 2 concerns remain partly open, and the tracker should distinguish the completed crash fix from the still-missing cyclic crease output and full vdW preprocessing work.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: Existing BitLessons were sufficient for this round. The cyclic crash was a task-specific control-flow bug in `run_preprocessor()` rather than a reusable cross-round lesson that needs a new entry.
