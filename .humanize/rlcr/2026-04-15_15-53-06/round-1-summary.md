# Round 1 Summary

## Work Completed

- Wired crease reference curvature into the real element-energy path instead of carrying `K0_ref` as dead runtime state:
  - `compute_element_energy` now accepts per-Gauss reference curvature,
  - `assemble_energy_forces` passes `RuntimeState::K0_ref` through to the kernel.
- Implemented the first executable cyclic runtime slice for `nCodeLoad=30/31`:
  - `LoadController::apply_increment` now handles cyclic compression/release increments,
  - `pasapas` no longer rejects `nCodeLoad=30/31`,
  - cyclic energy/force rows now use the archived integer-prefix format `(step, cycle, phase, ...)`,
  - cyclic end-of-cycle checkpoint writing is wired through `nano_checkpoint.dat`.
- Added checkpoint restart ingestion groundwork on the executable path:
  - `crunch_it` now reads `nano_checkpoint.dat` for cyclic cases when present,
  - restored `coords`, `eta`, and `K0_ref` seed the runtime state,
  - restart uses `checkpoint.iload + 1` as the resumed load-step start,
  - `pasapas` skips the unconditional step-0 free-minimize path on resumed runs.
- Upgraded the checkpoint tests to use the real archived cyclic checkpoint contract:
  - `read_checkpoint(..., true)` now parses the archived `K0_ref` payload,
  - round-trip coverage includes `K0_ref` instead of the earlier truncated path.
- Added an executable cyclic smoke test showing `crunch_it` can run the first archived cyclic step and emit cyclic-format rows without immediately aborting.

## Files Modified

- `include/fce/element_energy.hpp`
- `src/core/element_energy.cpp`
- `src/core/load_controller.cpp`
- `src/core/simulator.cpp`
- `src/core/solver.cpp`
- `src/simulator/main.cpp`
- `test/unit/test_io.cpp`
- `test/integration/test_oracle_roundtrip.cpp`
- `test/integration/test_e2e_compression.cpp`

## Validation

- `cmake --build build --target unit_tests -j4`
- `cmake --build build --target integration_tests -j4`
- `cmake --build build --target crunch_it -j4`
- `./build/unit_tests --gtest_filter='ReadCheckpoint.GrapheneCyclicCrumple:SimulatorInput.CyclicCaseLoadsCreaseMetadataIntoRuntimeState'`
- `./build/integration_tests --gtest_filter='RoundTrip.Checkpoint:E2ECyclicRuntime.CrunchItRunsFirstCyclicStepAndWritesCyclicRowFormat'`
- Manual smoke run: `./build/crunch_it /tmp/fce-cyclic-debug/prepro_run 1`

## Remaining Items

- Checkpoint restart is still only a partial executable-path implementation:
  - no MPI broadcast of restored checkpoint state yet,
  - no uninterrupted-vs-restarted cyclic parity test yet.
- `K0_ref` is now consumed by the constitutive path, but Fortran `update_creases` and `analyse_creases` are still not translated, so crease memory evolution and `crease_map.dat` are not complete.
- Cyclic runtime now runs instead of aborting, but AC-9 and AC-10 are still in progress rather than complete.

## Goal Tracker Update Request

### Requested Changes:
- Mark `task7a` as `in_progress` with a note that the executable path now supports the first cyclic step for `nCodeLoad=30/31` and emits cyclic-format `energy.dat` / `force.dat` rows, but full cyclic oracle parity is still open.
- Expand `task7d` notes to record that checkpoint ingestion is now wired into `crunch_it` for cyclic runs and that cyclic end-of-cycle checkpoint writing is connected in `pasapas`, while MPI restore and restart-parity verification remain open.
- Add a Plan Evolution note that Round 1 converted cyclic/checkpoint work from pure file-I/O scaffolding into a runnable executable-path slice by removing the `nCodeLoad=30/31` runtime abort and wiring archived checkpoint state into the solver start path.

### Justification:
- These changes reflect the actual repository state after this round: cyclic runtime is no longer a dead stub, checkpoint data is no longer isolated from the executable path, and the remaining work is now narrower and more concrete.

## BitLesson Delta

- Action: none
- Lesson ID(s): NONE
- Notes: `bitlesson-selector` remains unavailable in this environment, so no selector-driven lesson update was possible this round.
