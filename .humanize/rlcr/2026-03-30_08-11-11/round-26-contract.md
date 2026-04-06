# Round 26 Contract

## Blocking Issues (must fix to un-stall)

### Issue 1: AC-2 regression — preprocessor double-shift (task2h)

**Root cause**: `preprocessor.cpp` lines 732-739 contain a manual `neigh_vert += 1` loop that was a pre-Round-24 workaround for a bug in `write_mesh`. Round 24 fixed `write_mesh` (io.cpp:580) to perform the 0→1 conversion unconditionally for all valid entries (`ni >= 0`). The manual loop now causes a double-shift (+2 instead of +1) for all interior elements (`neigh_elem != 0 && neigh_vert >= 0`).

**Fix**: Remove lines 733-739 (the manual loop). Replace `mesh_out` (a copy with manual shift) with direct `meshT` in the `write_mesh` call. `meshT.connect[*].neigh_vert` is stored 0-based; `write_mesh` adds +1 for all valid entries.

**Acceptance**: All 5 of the following pass:
- `PreprocessorOracle.ArchivedCompressionCaseMatchesOracle`
- `PreprocessorOracle.ArchivedCyclicPreproInputMatchesOracleOutputs`
- `PreprocessorOracle.ArchivedSelfContactPreproInputMatchesOracleOutputs`
- `PreprocessorOracle.ArchivedBilayerTwistVdw1000PreproInputMatchesOracleOutputs`
- `RoundTrip.Mesh`

### Issue 2: task3e still open — no Fortran oracle for flag_num_diff=true path

**Root cause**: The committed oracle fixture (case_01.dat) uses element 83 which has non-trivial curvature and takes the analytical path (flag_num_diff=false). The S_m fix in element_energy.cpp lines 96-104 is only code-inspected + smoke-tested; no Fortran-derived fixture forces the flag_num_diff=true branch.

**Fix**:
1. Extend `dump_element_energy_oracle.f90` to emit per-Gauss `flag_num_diff`, `S_n[3]`, `S_m[3]` values in addition to W_elem and f_elem. Use a second fixture directory (`flat_geom_np1/`) for a synthetic-but-Fortran-computed case with flat z=0 geometry that forces `flag_num_diff=true`.
2. Add a new C++ test `FlagNumDiffStressesMatchFortranOracle` that reads the flat fixture and asserts direct S_n, S_m, W_elem, f_elem parity.

**Acceptance**: New test passes; committed fixture in `flat_geom_np1/case_01.dat` reproduces Fortran S_n and S_m for the degenerate (flat) case.

## Non-blocking queued items (do not defer further)

- task3a exponential map: add Fortran-derived oracle fixture
- task3c Brenner: add Fortran-backed element-energy path test
- task3f principal curvature: add Fortran-derived oracle fixture
- task4a-4f: global assembly, solver, pasapas, reaction force, end-to-end run

## Out of scope this round

Milestones 5-8 remain deferred pending Milestone 4 completion.
