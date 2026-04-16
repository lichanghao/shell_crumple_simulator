# Round 6 Summary

## Work Completed

- Added an explicit `stop_on_first_trial` mode to `LbfgsSolver::minimize(...)`.
- Switched `minimize_free` to use that mode, while leaving constrained `minimize` on the normal line-search path.
- This mirrors the source Fortran `minimize_free` behavior observed on the cyclic case: the reverse-communication caller exits on the first trial-point return instead of continuing the line search loop.
- Updated the direct `LbfgsSolver` test callers to the new signature.

## Key Result

- The committed cyclic post-free oracle is now green:
  - `E2ECyclicRuntime.CrunchItPostMinimizeFreeStateMatchesCommittedCyclicOracle` passes.
- The cyclic replay-row gate is still red, but much closer than before:
  - energy relative error is down from about `0.08626` to about `0.000341`
  - `GNORM` relative error is now about `0.04080`
  - force relative errors remain about `8.2862` and `0.1554`
- That means the shared post-`minimize_free` mismatch is no longer the primary cyclic blocker; the remaining gap has moved downstream into the first constrained cyclic step and/or reaction output.

## Files Modified

- `include/fce/lbfgs.hpp`
- `src/core/lbfgs.cpp`
- `src/core/solver.cpp`
- `test/integration/test_e2e_compression.cpp`
- `test/unit/test_lbfgs.cpp`

## Validation

- `cmake --build build --target integration_tests crunch_it unit_tests -j4`
- `./build/integration_tests --gtest_filter='E2ECyclicRuntime.CrunchItPostMinimizeFreeStateMatchesCommittedCyclicOracle:E2ECyclicRuntime.CrunchItReplaysCommittedCyclicStepOneTraceDeterministically'`
  - `CrunchItPostMinimizeFreeStateMatchesCommittedCyclicOracle`: passes
  - `CrunchItReplaysCommittedCyclicStepOneTraceDeterministically`: still fails on the reduced row mismatch above
- `./build/unit_tests`
  - the new `LbfgsSolver` signature compiles and the dedicated L-BFGS tests still pass
  - the full binary still reports two failing element-energy tests outside this solver slice:
    - `ElementEnergy.FlagNumDiffStressesMatchFortranOracle`
    - `ElementEnergy.BrennerMaterialMatchesFortranOracle`

## Remaining Items

- The cyclic replay-row gate remains red after the post-free fix, so the next blocker is now inside the first constrained cyclic step rather than `minimize_free`.
- `task7b` / `task7c` remain untouched: no translated `update_creases`, no translated `analyse_creases`, no `crease_map.dat`.
- `task7d` remains partial: no MPI restore broadcast and no restart-parity proof yet.
- Runtime vdW, real-`nvdw=1` VTU coverage, and multi-rank acceptance coverage are still open.

## Goal Tracker Update Request

### Requested Changes:
- Add a Plan Evolution note that Round 6 fixed the shared cyclic post-`minimize_free` mismatch by mirroring the Fortran free-minimize first-trial exit behavior.
- Update `task7a` notes to mark the post-free oracle as green and to state that the remaining cyclic blocker is now the first constrained replay row, especially reaction output.
- Update the post-free open issue to note that it is no longer the active cyclic blocker after this round, while keeping the constrained-step cyclic replay-row issue open.

### Justification:
- The repository state materially changed this round: the committed cyclic post-free oracle now passes, which narrows the remaining solver work to the first constrained cyclic step.
- The tracker should reflect that the loop has genuinely moved past one of the two explicit cyclic red gates.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` remains unavailable in this environment, so no selector-driven lesson update was possible this round.
