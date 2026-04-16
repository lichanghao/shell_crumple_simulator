# Round 7 Summary

## Work Completed

- Added a `stop_on_first_trial` mode to `LbfgsSolver::minimize(...)`.
- Routed `minimize_free(...)` through that mode while leaving constrained `minimize(...)` on the normal line-search path.
- Updated direct `LbfgsSolver` call sites in tests to the new signature.

## Key Result

- The cyclic post-free oracle is now green:
  - `E2ECyclicRuntime.CrunchItPostMinimizeFreeStateMatchesCommittedCyclicOracle` passes.
- The cyclic replay-row gate is still red, but it improved substantially:
  - energy relative error dropped from about `0.08626` to about `0.000341`
  - `GNORM` relative error is now about `0.04080`
  - reaction-column relative errors are still large at about `8.2862` and `0.1554`
- This means the shared post-`minimize_free` mismatch is no longer the primary cyclic blocker. The remaining gap is now in the first constrained cyclic step and/or reaction output.

## Files Modified

- `include/fce/lbfgs.hpp`
- `src/core/lbfgs.cpp`
- `src/core/solver.cpp`
- `test/integration/test_e2e_compression.cpp`
- `test/unit/test_lbfgs.cpp`

## Validation

- `cmake --build build --target integration_tests crunch_it unit_tests -j4`
- `./build/integration_tests --gtest_filter='E2ECyclicRuntime.CrunchItPostMinimizeFreeStateMatchesCommittedCyclicOracle:E2ECyclicRuntime.CrunchItReplaysCommittedCyclicStepOneTraceDeterministically'`
  - `CrunchItPostMinimizeFreeStateMatchesCommittedCyclicOracle`: passed
  - `CrunchItReplaysCommittedCyclicStepOneTraceDeterministically`: failed with the reduced replay-row mismatch above
- `./build/unit_tests`
  - the new `LbfgsSolver` signature compiles and the dedicated solver tests still run
  - the full unit binary still reports two pre-existing element-energy failures outside this solver slice:
    - `ElementEnergy.FlagNumDiffStressesMatchFortranOracle`
    - `ElementEnergy.BrennerMaterialMatchesFortranOracle`

## Remaining Items

- The next concrete cyclic blocker is now downstream of `minimize_free`: the first constrained replay row, especially reaction output.
- `task7b` / `task7c` remain open: no translated `update_creases`, no translated `analyse_creases`, no `crease_map.dat`.
- `task7d` remains partial: no MPI restore broadcast and no restart-parity proof yet.
- Runtime vdW, real-`nvdw=1` VTU coverage, and multi-rank acceptance coverage are still open.

## Goal Tracker Update Request

### Requested Changes:
- Add a Plan Evolution note that Round 7 closed the committed cyclic post-`minimize_free` oracle by mirroring the source free-minimize first-trial exit behavior.
- Update `task7a` and the cyclic open-issue notes to reflect that the post-free gate is now green and the remaining cyclic blocker is the first constrained replay row, especially reaction output.

### Justification:
- The repository state materially changed this round: one of the two explicit cyclic red gates is now green.
- The tracker should show that the solver work has genuinely moved from the shared post-free handoff to the first constrained cyclic step.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` remains unavailable in this environment, so no selector-driven lesson update was possible this round.
