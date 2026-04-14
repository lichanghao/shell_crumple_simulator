# Round 4 Summary

## Work Completed
- Added env-gated step-trace dumps in `src/core/solver.cpp` via `FCE_TRACE_COORD_DUMPS` so Round 4 can compare C++ step-1 runtime states directly against a same-trace canonical Fortran replay.
- Verified with fresh dump comparisons that the C++ executable matches the canonical Fortran replay through:
  - post-`minimize_free` coordinates
  - post-`minimize_free` `eta`
  - step-1 coordinates after boundary increment
  - step-1 coordinates after imperfection injection
- Re-ran a fresh canonical Fortran same-trace step-1 replay in `/tmp` and confirmed the archived step-1 oracle row is real:
  - final step-1 energy `5.3713040471184820E-05`
  - final step-1 GNORM/CRITC `9.84014213E-06`
  - Fortran `NUMITER = 12937`
- Captured the current C++ monitored step-1 replay and confirmed the remaining blocker is the constrained optimization path itself:
  - C++ terminates normally at `NUMITER = 7124`
  - C++ final step-1 energy remains `1.6989678325785170E-04`
  - C++ final GNORM/CRITC is `8.932149010E-06`
- Tested the stale reverse-communication stop gate against the actual Fortran caller contract and updated `src/core/lbfgs.cpp` so the `IFLAG=1` outer-loop gate checks stale `CRIT_CONV` (`GNORM` in the Fortran caller), not stale raw `||g||`. This did not resolve the step-1 parity failure.

## Files Changed
- `src/core/lbfgs.cpp`
- `src/core/solver.cpp`

## Validation
- `./build/integration_tests --gtest_filter=E2ECompression.CrunchItPostMinimizeFreeStateMatchesCanonicalFortranDump`
  - passed
- `./build/integration_tests --gtest_filter=E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace`
  - still fails with step-1 energy/force mismatches
- Fresh same-trace canonical Fortran replay in `/tmp/fce-round4-fortran-run`
  - post-free `eta` dump matched the new C++ dump exactly
  - final step-1 row reproduced the archived `5.3713040471184820E-05` oracle after `12937` iterations
- Fresh monitored C++ replay in `/tmp/fce-round4-cpp-run`
  - terminates at `7124` iterations with step-1 energy `1.6989678325785170E-04`

## Remaining Items
- AC-7 remains blocked in the constrained runtime path after imperfection injection.
- The remaining mismatch is no longer in post-free hidden state, increment logic, imperfection replay, or the archived step-1 oracle itself.
- The next debugging target should be the constrained minimization internals after the first identical pre-minimization state: either force/gradient evolution on nonzero-`eta` constrained states or an L-BFGS/MCSRCH path divergence that only appears after the shared early iterations.

## BitLesson Delta
- Action: update
- Lesson ID(s): BL-20260405-lbfgs-premature-exit
- Notes: Round 4 verified that the Fortran caller-side stale `GNORM` gate is the stale `CRIT_CONV` scalar, not stale raw `||g||`, and that correcting the C++ gate preserves the post-free parity fix but does not solve the downstream constrained-step divergence.

## Goal Tracker Update Request

### Requested Changes:
- Update the AC-7 open issue detail to record that post-`minimize_free` hidden-state parity now includes `eta`, not just coordinates.
- Add the new constrained-step evidence that canonical Fortran same-trace step 1 reaches the archived `5.3713040471184820E-05` row only after `12937` iterations, while current C++ exits normally at `7124` iterations with `1.6989678325785170E-04`.

### Justification:
These findings materially narrow the remaining AC-7 blocker. The bug is no longer in trace replay, step-0 hidden-state carry-over, or the archived step-1 oracle source; it is now specifically the constrained optimizer/runtime force evolution after the shared pre-minimization state.
- Lesson ID(s): NONE
- Notes: [what changed and why]
