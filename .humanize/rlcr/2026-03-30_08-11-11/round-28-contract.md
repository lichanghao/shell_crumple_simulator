# Round 28 Contract

## Mainline Objective

Implement Milestone 4 Phase A so the simulator has a real global assembly path instead of a stub.

This round will land a production `task4a` slice that:
- loads the archived compression simulator inputs,
- assembles total energy and nodal forces over the owned element range,
- all-reduces those totals through `MpiEnv`, and
- proves the path against the archived `energy.dat` load-step-1 oracle.

## Target Acceptance Criteria

- **AC-7**: start the end-to-end serial compression path with a real load-step-1 energy oracle.
- **AC-11**: partitioned element assembly reduces correctly through the MPI wrapper.

## Blocking Issues

- None currently block this round's objective. The outstanding `task3f` repeated-curvature
  fallback oracle gap is real, but it does not prevent implementing and validating the first
  global assembly slice.

## Queued / Explicitly Out Of Scope

- `task3f` repeated-curvature `flag_num_diff=true` Fortran-backed principal fixture work.
- Milestone 4 Phases B-F (`task4b` through `task4f`): L-BFGS, load controller, pasapas,
  reaction force, and full end-to-end driver behavior.
- Runtime vdW/self-contact (`task6a`-`task6c`) and documentation milestone work (`task8c`).

## Success Criteria

1. Add a failing test first for simulator assembly on the archived compression case.
2. Implement a dedicated simulator-core module under `include/fce/` and `src/core/`.
3. The new assembly path computes the total load-step-1 energy from
   `test/cases/graphene_compression_simulator/np1` within the AC-7 tolerance.
4. A corrupted-mesh negative test fails before returning an assembled result.
5. Existing unit and integration suites remain green after the new assembly tests are added.
