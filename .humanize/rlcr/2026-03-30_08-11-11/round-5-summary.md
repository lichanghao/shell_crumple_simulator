# Round 5 Summary

## Work Completed
- Committed `eef0ab9` (`Harden preprocessor oracle evidence`).
- Made `PreprocessorOracle` tests create unique temporary directories instead of sharing one fixed path, removing the known race risk for parallel or overlapping runs.
- Added direct archived ghost-coordinate verification to the preprocessor oracle comparator by materializing ghost nodes from both actual and archived configs and comparing the appended coordinates edge-by-edge.

## Files Changed
- `test/integration/test_prepro_oracle.cpp`
- `test/support/oracle_compare.cpp`

## Validation
- `cmake --build build --target integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^PreprocessorOracle'` -> PASS (`5/5`) after the temp-dir hardening change
- `cmake --build build --target integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^PreprocessorOracle'` -> PASS (`5/5`) after the ghost-coordinate oracle comparison change
- `ctest --test-dir build --output-on-failure` -> PASS (`32/32`)

## Remaining Items
- `task2g` remains pending: the real `nvdw=1` preprocessing path, neighbor-list generation, shape functions, and `vdw_previous`-equivalent state are still not translated.
- AC-3 remains partial: the required 5 interior and 5 boundary Fortran B-spline oracle fixtures are still missing.
- Milestones 3 through 8 remain pending, including the simulator mainline, vdW runtime, cyclic runtime controller, checkpoint/restart, MPI equivalence, VTU output, and final documentation.

## Goal Tracker Update Request

### Requested Changes:
- Remove the AC-4 blocker stating that positive ghost-position evidence is still indirect.
- Remove the queued side issue stating that `PreprocessorOracle` uses a fixed temporary directory.
- Update the AC-4 evidence row to cite the direct archived ghost-coordinate comparison in `test/support/oracle_compare.cpp` together with the passing `PreprocessorOracle` run and `32/32` full-suite regression.
- Update the Plan Evolution / blocker notes to reflect that Round 5 closed the remaining Milestone 2 ghost-evidence and temp-dir harness gaps, while keeping `task2g` and the AC-3 B-spline-fixture gap open.

### Justification:
Round 5 directly addressed the two concrete review findings that were still preventing AC-4 from moving beyond indirect evidence and leaving the harness unsafe for parallel execution. The new comparator path now checks generated ghost coordinates against the archived oracle itself, and the temp-dir factory no longer reuses a shared path. Those issues should no longer remain listed as open blockers, while the larger unresolved Milestone 2 and simulator tasks should stay open.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable failure pattern was discovered; the round closed two already-identified review items.
