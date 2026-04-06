# Round 38 Summary

## Work Completed
- Moved the VTU/PVD writer out of `solver.cpp` into a dedicated runtime output module: `include/fce/runtime_output.hpp` and `src/core/runtime_output.cpp`.
- Expanded the runtime VTU snapshot contract for the current `nvdw=0` compression path so the generated XML now emits the canonical field order and names required by `paraview_vtu_output.f90`:
  - `FieldData/TimeValue`
  - `Points`
  - `PointData/atomic_density` (zero-valued for `nvdw=0`)
  - `Cells` (`connectivity`, `offsets`, `types`)
  - `CellData/inner_displacement`
  - `CellData/W_density` (zero-valued for `nvdw=0`)
- Added explicit runtime-output validation in the new module so invalid mesh/runtime state fails with a thrown error instead of silently writing corrupt XML.
- Kept `pasapas()` on the same runtime path, but changed it to call the new module for snapshot/PVD emission.
- Expanded the focused executable-path VTU integration test so it now validates:
  - step-0 points and `inner_displacement` against the archived compression oracle
  - connectivity / offsets / types against the archived compression oracle
  - zero-valued `atomic_density` and `W_density` arrays for steps 0 and 1
  - step-1 points against the generated `nano_final_config.dat`
  - step-1 `inner_displacement` against the averaged generated runtime `eta`
  - PVD metadata for steps 0 and 1
- Added focused unit coverage for the new module:
  - positive snapshot/PVD emission on a minimal mesh/state
  - negative failure on mismatched runtime state

## Files Changed
- `CMakeLists.txt`
- `include/fce/runtime_output.hpp`
- `src/core/runtime_output.cpp`
- `src/core/solver.cpp`
- `test/integration/test_e2e_compression.cpp`
- `test/unit/test_runtime_output.cpp`

## Validation
- `cmake --build build --target unit_tests integration_tests crunch_it -j4` — passed
- `./build/unit_tests --gtest_filter='RuntimeOutput.*'` — passed
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItWritesRuntimeVtuSeriesAndValidatesFullDataArrays'` — passed
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItReusesRecordedImperfectionTraceDeterministically:E2ECompression.CrunchItRejectsShortImperfectionTrace'` — passed
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts'` — still fails on the same AC-7 solver-path mismatch after a full 50-step run; the richer VTU contract did not change the deterministic failing trajectory.
- Latest full-run deterministic evidence after the runtime-output refactor is unchanged in the early load steps:
  - step 1 energy `5.74298201e-05` vs oracle `5.72105277e-05`
  - step 2 energy `7.47995924e-05` vs oracle `1.03739788e-04`
  - step 3 energy `1.44786817e-04` vs oracle `5.91982000e-05`

## Remaining Items
- `task5a` is still not fully closed. The canonical field names/order now exist for the `nvdw=0` path, but runtime `atomic_density` / `W_density` for `nvdw>0` still require simulator-side vdW state that is not yet loaded.
- `task5b` is stronger but still not complete. The focused writer tests now cover the full DataArray set for the compression path plus the invalid-state negative path, but AC-12 still depends on later-step archived VTU parity and the missing `nvdw>0` runtime density path.
- AC-7 remains red. The archived-oracle executable regression still diverges in energy, force, and final configuration from step 1 onward.
- Milestones 6-8 remain unimplemented on the runtime path.

## Goal Tracker Update Request

### Requested Changes:
- Add a Round 38 plan-evolution entry recording that Milestone 5 now has a dedicated `runtime_output` module, canonical zero-density VTU fields for the `nvdw=0` runtime path, and broader executable-path/negative validation.
- Keep `task5a` pending, but update its notes to reflect real progress:
  - the VTU/PVD writer is no longer embedded in `solver.cpp`
  - the runtime now emits the canonical field order and names for the compression (`nvdw=0`) path
  - `atomic_density` / `W_density` for `nvdw>0` remain blocked on loading simulator-side vdW state
- Keep `task5b` pending, but update its notes to reflect the new passing evidence:
  - `RuntimeOutput.*` unit tests now cover positive emission and explicit invalid-state rejection
  - `E2ECompression.CrunchItWritesRuntimeVtuSeriesAndValidatesFullDataArrays` now validates connectivity/offsets/types, zero-valued density arrays, later-step points vs generated final config, and later-step `inner_displacement` vs generated runtime `eta`
  - the full archived-oracle executable regression still fails only on the same AC-7 solver mismatch profile
- Add or update a blocking side issue noting that full VTU density output for `nvdw>0` depends on Milestone 6 runtime vdW/self-contact state loading.

### Justification:
This round materially advanced Milestone 5 without pretending AC-12 is finished. The runtime output path is now a real module with explicit validation instead of a local helper, the executable-path tests validate the full compression-path DataArray set rather than only step-0 geometry, and invalid runtime state now fails loudly instead of writing malformed XML. The remaining blocker is no longer “missing VTU fields for the compression path”; it is the absence of runtime vdW state for `nvdw>0` plus the still-red AC-7 solver trajectory.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: no new reusable lesson was added this round.
