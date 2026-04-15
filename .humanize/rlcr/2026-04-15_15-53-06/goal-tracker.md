# Goal Tracker

<!--
This file tracks the ultimate goal, acceptance criteria, and plan evolution.
It prevents goal drift by maintaining a persistent anchor across all rounds.

RULES:
- IMMUTABLE SECTION: Do not modify after initialization
- MUTABLE SECTION: Update each round, but document all changes
- Every task must be in one of: Active, Completed, or Deferred
- Deferred items require explicit justification
-->

## IMMUTABLE SECTION
<!-- Do not modify after initialization -->

### Ultimate Goal
Translate the Fortran 90 graphene simulation codebase — comprising `grapheneCompressionOriginPrePro/` (preprocessor, 11 source files) and `grapheneCompressionOriginVersion/` (FEM simulator, 31 source files) — into a functionally equivalent C++17 implementation. The C++ code must reproduce the same physics, algorithms, and numerical results as the canonical Fortran reference (commit `7d3f77f` of `finite_crystal_elasticity/`) within accepted engineering tolerances. The translation covers the full feature set: core graphene compression, van der Waals interactions, cyclic crumpling loading modes (`nCodeLoad=30/31`), self-contact, irreversible crease memory, checkpoint/restart, and MPI parallelization. Outputs required: `nano_*.dat` inter-program data files and VTU/ParaView visualization.

### Acceptance Criteria
<!-- Each criterion must be independently verifiable -->

- **AC-1**: Fortran oracle baseline is built, run, and captured with all reference outputs stored.
- **AC-2**: C++ preprocessor correctly generates `nano_*.dat` files matching the Fortran oracle (integer fields exact, float <= `1e-10` absolute).
- **AC-3**: B-spline basis functions and derivatives are correct for all 12-node patch configurations; partition-of-unity `|sum N_i - 1| < 1e-14`.
- **AC-4**: Ghost node positions match Fortran parallelogram extrapolation within `1e-12`; connectivity is integer-exact.
- **AC-5**: Brenner REBO `W`, `dW/dpe`, and `d2W/dpe2` are correct within `1e-10` absolute.
- **AC-6**: Inner Newton solver for `eta` converges matching Fortran within `1e-10` absolute.
- **AC-7**: End-to-end serial compression (`np=1`, `nCodeLoad=3`, `nloadstep=50`) matches the Fortran oracle within `1e-4` relative energy and `1e-3` relative reaction-force tolerance.
- **AC-8**: Runtime vdW energy/forces match Fortran within `1e-10` absolute and neighbor-list semantics are correct.
- **AC-9**: Cyclic loading (`nCodeLoad=31`) and crease memory `K0_ref` updates are correct; `crease_map.dat` matches within `1e-4` relative.
- **AC-10**: Checkpoint/restart reproduces final outputs within AC-7 tolerances.
- **AC-11**: MPI results for `np=1`, `np=2`, and `np=4` agree within `1e-4` relative energy and `1e-3` relative reaction-force tolerance.
- **AC-12**: VTU files are valid XML and nodal/element runtime data matches within `1e-6` relative.
- **AC-13**: `AGENT.md` and `document/translation_notes.md` are maintained and match the repository state.

---

## MUTABLE SECTION
<!-- Update each round with justification for changes -->

### Plan Version: 1 (Updated: Round 0, 2026-04-15)

#### Plan Evolution Log
| Round | Change | Reason | Impact on AC |
|-------|--------|--------|--------------|
| 0 | Initialized this new RLCR loop from the verified repository state instead of copying the previous tracker verbatim | The prior loop ended with additional verified work, plus a tracker hygiene complaint about completed tasks still living in Active Tasks | AC-1 through AC-13 |
| 0 | Cleaned the task inventory so only genuinely open work remains in Active Tasks | The previous tracker mixed completed and open items, which obscured the real remaining backlog | AC-2 through AC-13 |
| 0 | Recorded the deterministic replay/archive split for step one as current AC-7 context | Commit `27f2d28` established that the frozen replay trace is a separate executable-path contract from the archived `np1/` step-one outputs | AC-7, AC-13 |
| 0 | Advanced checkpoint/restart groundwork with archive-backed checkpoint I/O and runtime-state plumbing | AC-10 was entirely absent from the executable-path codebase; landing the real `nano_checkpoint.dat` contract is the smallest production slice that can be verified now without pretending cyclic stepping is already translated | AC-10, AC-13 |
| 1 | Converted cyclic/checkpoint work from pure I/O scaffolding into a runnable executable-path slice by allowing `nCodeLoad=30/31` to enter `pasapas` and wiring checkpoint ingestion into `crunch_it` | The runtime can now execute a fresh first cyclic step and resume from a single-rank checkpoint instead of failing before load stepping begins, which is real progress even though cyclic oracle parity is still red | AC-9, AC-10, AC-13 |

#### Active Tasks
| Task | Target AC | Status | Tag | Owner | Notes |
|------|-----------|--------|-----|-------|-------|
| task4d: `pasapas` load-stepping loop | AC-7 | in_progress | coding | claude | The deterministic replay lane is now green against committed replay-specific step-one fixtures, but archived step-one parity is still unresolved on the executable path. The remaining blocker is to explain and close the archive-contract mismatch in constrained minimization / emitted step-one runtime state. |
| task4e: Reaction-force computation for `nCodeLoad=3` | AC-7 | pending | coding | claude | Replay-lane rows are now stable, but archive-backed executable-path parity for reaction outputs is still not closed. |
| task4f: End-to-end serial run vs oracle | AC-7 | pending | coding | claude | The 50-step archived-oracle executable-path test still does not complete within the current budget. |
| task5a: VTU/ParaView output translation | AC-12 | pending | coding | claude | XML/loadability proof exists, but executable-path runtime VTU parity is still incomplete and there is still no real runtime `nvdw=1` oracle series. |
| task5b: VTU format validation | AC-12 | pending | coding | claude | Depends on executable-path VTU/PVD oracle coverage, especially for runtime `nvdw=1`. |
| task6a: vdW interaction kernel | AC-8 | pending | coding | claude | Runtime vdW/self-contact remains untranslated even though preprocessor-side `nvdw=1` parity is complete. |
| task6b: Self-contact detection | AC-8 | pending | coding | claude | Depends on simulator-side vdW stack. |
| task6c: vdW integration and oracle tests | AC-8 | pending | coding | claude | Depends on `task6a` and `task6b`. |
| task7a: Cyclic BC controller `nCodeLoad=30/31` | AC-9 | in_progress | coding | claude | The executable path no longer throws for cyclic runtime modes and now emits cyclic-format rows, but `nCodeLoad=31` still misinterprets the 0-based corner tags in `mnodBC`, so first-step oracle parity remains broken. |
| task7b: Irreversible crease memory | AC-9 | pending | coding | claude | Not started on the executable path. |
| task7c: Crease detection and facet analysis | AC-9 | pending | coding | claude | Not started on the executable path. |
| task7d: Checkpoint/restart | AC-10 | in_progress | coding | claude | `crunch_it` now ingests `nano_checkpoint.dat` for cyclic runs, restores `coords`/`eta`/`K0_ref`, resumes from `checkpoint.iload + 1`, and writes checkpoints at end-of-cycle boundaries. MPI broadcast, rank-mismatch validation, and uninterrupted-vs-restarted parity are still missing. |
| task7e: Oracle tests for cyclic case and checkpoint | AC-9, AC-10 | pending | coding | claude | Depends on cyclic/checkpoint runtime implementation. |
| task8a: Multi-rank consistency tests `np=1,2,4` | AC-11 | pending | coding | claude | Assembly groundwork exists, but acceptance-level MPI parity is still open. |
| task8b: Checkpoint compatibility across rank counts | AC-10, AC-11 | pending | coding | claude | Depends on checkpoint implementation. |
| task8c: `AGENT.md` and `document/translation_notes.md` finalization | AC-13 | in_progress | coding | claude | Documentation currently reflects the replay/archive split and current runtime gaps, but must remain aligned as runtime backlog closes. |
| task8d: Full integration test suite | AC-1 through AC-13 | pending | analyze | codex | The full integration surface still contains red archived-runtime gates and timing issues; rerun only after the current AC-7 executable-path blocker materially changes. |

### Completed and Verified
| AC | Task | Completed Round | Verified Round | Evidence |
|----|------|-----------------|----------------|----------|
| AC-1 | task0a: Oracle baseline archived (compression simulator) | prior | prior | `test/cases/graphene_compression_simulator/np1/`; archived run log and outputs are committed. |
| AC-1 | task0b: Cyclic crumple oracle archived | prior | prior | `test/cases/graphene_cyclic_crumple/prepro_run/` and `test/cases/graphene_cyclic_crumple/simulator_run/`. |
| AC-1 | task0c: `document/fortran_conventions.md` | prior | prior | Repository documentation records indexing, units, and file-ordering conventions. |
| AC-13 | task1a: CMake scaffold | prior | prior | `CMakeLists.txt` builds the project with C++17, MPI, Eigen3, and GoogleTest. |
| AC-2 | task1b+1c and task1e: core nano-dat I/O plus comparison helpers | prior | prior | `include/fce/types.hpp`, `include/fce/io.hpp`, `src/core/io.cpp`, and oracle comparison support are committed and covered. |
| AC-11 | task1d: MPI wrapper | prior | prior | `include/fce/mpi_env.hpp` and `src/core/mpi_env.cpp` are committed with partition coverage. |
| AC-2 | task2a: Mesh generation (`mesh_gen_square`) | prior | prior | Implemented in `src/core/mesh_generator.cpp`; archive-backed preprocessor coverage exists. |
| AC-4 | task2b: Ghost node generation and connectivity tables | 1 | 6 | Archived `ghost_coords.dat` artifacts plus direct `1e-12` oracle comparison are committed and passing. |
| AC-3 | task2c: B-spline basis and derivatives | 1 | 8 | `test/cases/bspline_oracle/`; `BSpline.MatchesCommittedFortranOracleFixtures`; partition-of-unity and derivative checks pass. |
| AC-2 | task2d: Gauss quadrature setup | prior | prior | Exercised through the archive-backed preprocessor pipeline. |
| AC-2 | task2e: Initial deformation gradient `F0` and Jacobian `J0` | prior | prior | Covered through archived preprocessor parity tests. |
| AC-2 | task2f: BC/load setup for preprocessor | prior | prior | Compression and cyclic preprocessor oracle comparisons pass. |
| AC-8 | task2g: Preprocessor-side vdW preprocessing | 10 | 14 | Single-sheet self-contact and bilayer-twist preprocessor oracle cases are committed and passing. |
| AC-2 | task2h: Full preprocessor integration and oracle comparison | 26 | 26 | Archived preprocessor parity and round-trip coverage are green. |
| AC-6, AC-7 | task3a: Exponential map for the Cauchy-Born rule | prior | prior | Fortran-derived fixtures are committed and passing. |
| AC-5, AC-6 | task3b: Deformation gradient decomposition and bond vectors | 23 | 23 | `Exponential.MatchesArchivedCompressionFortranOracle` plus manual `ElementState` composition coverage are green. |
| AC-5 | task3c: Brenner REBO potential | prior | prior | Dedicated Brenner oracle coverage exists in the unit suite. |
| AC-6 | task3d: Inner Newton solver for `eta` | 8 | 8 | `NewtonInner.MatchesCommittedFortranOracleFixtures`, archived `ElementState` parity, and the first constrained-step exact-state gate are green. |
| AC-7 | task3e: Element-level energy/force kernel | 8 | 8 | `ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures`, `ElementEnergy.FElemMatchesFortranOracle`, and the first constrained-step exact-state gate are green. |
| AC-7, AC-9 | task3f: Principal curvature extraction | prior | prior | Direct Fortran-derived fallback coverage exists. |
| AC-7, AC-11 | task4a: Global energy/force assembly with MPI partitioning | 28 | 29 | `SimulatorAssembly.ArchivedEnergyTrajectoryMatchesOracleFile` and related coverage are committed and passing. |
| AC-7 | task4b: L-BFGS minimizer from `lbfgs.f` | prior | prior | The translated Nocedal/MCSRCH path is implemented and unit-tested. |
| AC-7 | task4c: Loading controller for `nCodeLoad=3`; stubs for `nCodeLoad=30/31` | prior | prior | Compression-path BC incrementing and reaction-force aggregation are implemented. |

### Explicitly Deferred
| Task | Original AC | Deferred Since | Justification | When to Reconsider |
|------|-------------|----------------|---------------|-------------------|
| (none) | - | - | - | - |

### Open Issues
| Issue | Discovered Round | Blocking AC | Resolution Path |
|-------|-----------------|-------------|-----------------|
| The deterministic replay fixture `imperfection_trace_fortran.dat` and the archived `np1/` step-one outputs are not the same oracle contract. Replay-specific monitor, eval-sequence, `energy.dat`, and `force.dat` fixtures are now green on the executable path, but that does not close the archived-runtime mismatch. | 0 | AC-7, AC-13 | Preserve the replay lane as a stable debugging baseline, but keep archive-backed parity gates separate and unresolved until the archived step-one runtime state is explained. |
| Archived step-one `energy.dat`, `force.dat`, and `mesh_config_0001.vtu` are internally consistent, but the frozen `np1/simulator.log` step-one equilibrium line disagrees with those archived numeric artifacts. | 0 | AC-7, AC-13 | Treat archived `energy.dat` / `force.dat` / VTU data as the authoritative step-one numeric oracle; treat `simulator.log` as context only unless a source-backed explanation appears. |
| The live executable path still fails the archived step-one parity gates even after the replay split: free-node geometry and archive-backed first-load rows remain mismatched. | 32 | AC-7, AC-9, AC-10 | Compare constrained minimization, emitted step-one state, and reaction output against canonical `pasapas.f90`, `minimize.f90`, `load.f90`, and `get_reac.f90`, starting from the now-stable replay baseline. |
| The translated `nCodeLoad=31` load controller currently applies the wrong corner motions because `mnodBC(:,2)` is stored 0-based in C++, but the cyclic branch still treats the tags like the original 1-based Fortran values. A fresh first-step run is therefore far from the archived cyclic oracle. | 1 | AC-9, AC-10 | Fix the `LoadController::apply_increment` tag mapping for `nCodeLoad=31`, then add an archive-backed first-step cyclic parity test that checks energy/force rows against `test/cases/graphene_cyclic_crumple/simulator_run/`. |
| Archived `mesh_config_0000.vtu` is a pre-`pasapas()` artifact, so step-0 VTU parity cannot prove the hidden post-free state matches Fortran. | 1 | AC-7 | Continue using direct hidden-state comparisons or pre-step-1 dumps rather than archived step-0 geometry parity as a physical oracle. |
| Runtime vdW/self-contact remains unimplemented even though preprocessor-side `nvdw=1` parity is complete. | 3 | AC-8 | Translate the simulator-side `vdw_modules.f90` path and add runtime oracle coverage. |
| The repository still lacks an executable-path real-`nvdw=1` VTU/PVD oracle series. | 40 | AC-12, AC-8 | Add a real runtime `nvdw=1` executable-path case and compare VTU/PVD payloads end to end. |
| The archived-oracle executable-path compression test still does not complete within the current `ctest` budget. | 42 | AC-7, AC-13 | First close the archived step-one runtime mismatch so the runtime follows the archived trajectory, then finish the 50-step run within the existing budget. |
