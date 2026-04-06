# Round 21 Summary

## Work Completed
- Added archived simulator-derived constitutive fixtures under `test/cases/constitutive_oracle/archived_compression_np1/` for element IDs `83` through `87` at both Gauss points, generated from the frozen serial compression simulator output in `test/cases/graphene_compression_simulator/np1/`.
- Added `test/cases/tools/dump_archived_constitutive_oracle.f90`, a Fortran-side helper that reads archived `nano_config.dat`, `nano_Mesh.dat`, `nano_zero.dat`, and `nano_general.dat`, evaluates `metric`, `curv`, `principal`, `def_bonds_`, `newton_inner`, and `Hyper_pot_inner`, and writes a combined geometry/bond/Newton corpus.
- Extended `test/unit/test_element_state.cpp` with `ElementState.MatchesArchivedCompressionSimulatorOracleFixtures`, which rebuilds the same archived element-Gauss patches from the committed simulator files in C++, then compares canonical `ElementState`, prepared-bond outputs, and the state-based Newton solve against the Fortran-emitted archived fixtures.
- Replaced the old AC-6 provenance-gap note in `test/cases/constitutive_oracle/build_provenance.md` with an exact archived-fixture reproduction recipe and selector provenance, and updated `test/cases/README.md` so the new oracle corpus is part of the documented case inventory.
- Touched `test/cases/tools/dump_constitutive_oracle.f90` to clarify that it remains the synthetic corpus generator while the new archived helper owns the simulator-derived slice.

## Files Changed
- `test/unit/test_element_state.cpp`
- `test/cases/tools/dump_archived_constitutive_oracle.f90`
- `test/cases/tools/dump_constitutive_oracle.f90`
- `test/cases/constitutive_oracle/archived_compression_np1/case_01.dat`
- `test/cases/constitutive_oracle/archived_compression_np1/case_02.dat`
- `test/cases/constitutive_oracle/archived_compression_np1/case_03.dat`
- `test/cases/constitutive_oracle/archived_compression_np1/case_04.dat`
- `test/cases/constitutive_oracle/archived_compression_np1/case_05.dat`
- `test/cases/constitutive_oracle/archived_compression_np1/case_06.dat`
- `test/cases/constitutive_oracle/archived_compression_np1/case_07.dat`
- `test/cases/constitutive_oracle/archived_compression_np1/case_08.dat`
- `test/cases/constitutive_oracle/archived_compression_np1/case_09.dat`
- `test/cases/constitutive_oracle/archived_compression_np1/case_10.dat`
- `test/cases/constitutive_oracle/build_provenance.md`
- `test/cases/README.md`

## Validation
- BitLesson:
  - Read `.humanize/bitlesson.md`
  - `bitlesson-selector "Round 21 archived constitutive oracle fixtures from simulator final state"`
  - Result: command not found in this environment, so I followed the local BitLesson guidance directly
- Red phase:
  `cmake --build build --target unit_tests && ctest --test-dir build --output-on-failure -R 'ElementState.MatchesArchivedCompressionSimulatorOracleFixtures'`
  Failed as expected because `test/cases/constitutive_oracle/archived_compression_np1/` did not exist yet.
- Archived helper generation:
  `gfortran -std=legacy -O0 -J /tmp/archived_constitutive_mods -c ../finite_crystal_elasticity/grapheneCompressionOriginVersion/headers.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/Taylor.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/BSpline.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/gauss.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/geometry.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/principal.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/exponential.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner2.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/morse.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/mm3.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/Hyper_pot_inner_alg.f90 ../finite_crystal_elasticity/grapheneCompressionOriginVersion/newton_inner.f90 test/cases/tools/dump_archived_constitutive_oracle.f90`
  Pass
- Archived fixture emission:
  `/tmp/dump_archived_constitutive_oracle test/cases/graphene_compression_simulator/np1 test/cases/constitutive_oracle/archived_compression_np1`
  Pass
- Green phase:
  `cmake --build build --target unit_tests && ctest --test-dir build --output-on-failure -R 'ElementState.MatchesArchivedCompressionSimulatorOracleFixtures'`
  Pass
- Focused verification:
  `ctest --test-dir build --output-on-failure -R 'ElementState|NewtonInner|Brenner'`
  Pass `16/16`
- Full regression:
  `ctest --test-dir build --output-on-failure`
  Pass `70/70`
- Sanity:
  `git diff --check`
  Pass

## Remaining Items
- `task3e` is still open. This round adds archived constitutive evidence, but it does not translate `ener_elem.f90` or connect the constitutive kernels to a production element-energy path.
- `task3b` still lacks a production caller even though it now has archived Fortran-derived geometry/bond evidence.
- Milestone 4+ work remains untouched.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable project lesson emerged beyond the already-recorded guidance to prefer archive-backed evidence and exact file-format tracing.

## Goal Tracker Update Request

### Requested Changes:
- Update `task3b` to record that archived Fortran-derived geometry/bond fixtures now exist under `test/cases/constitutive_oracle/archived_compression_np1/` and that `ElementState.MatchesArchivedCompressionSimulatorOracleFixtures` compares the canonical C++ geometry and prepared-bond path against them.
- Update `task3d` to record that a 10-case archived simulator-derived constitutive corpus now exists from `test/cases/graphene_compression_simulator/np1/`, replacing the prior provenance-gap note in `test/cases/constitutive_oracle/build_provenance.md` with an exact reproduction recipe and archived selector list.
- Remove the Milestone 3 blocker text that specifically claimed the geometry/bond oracle fixtures and archived-state Newton provenance were missing, while keeping Milestone 3 pending on the still-missing `ener_elem` / production integration work.

### Justification:
This round moves the constitutive work out of the synthetic-fixture-only phase. The canonical C++ element-state pipeline is now checked against Fortran-emitted archived simulator data, and the provenance file contains a concrete reproduction path instead of an explicit gap. That is real progress on the plan’s oracle-evidence requirement, even though Milestone 3 still cannot be considered complete until `task3e` and the simulator-side energy path exist.
