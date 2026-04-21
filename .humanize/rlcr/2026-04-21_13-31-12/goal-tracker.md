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
Finish the C++17 translation of the graphene finite crystal elasticity preprocessor and simulator so the remaining executable-path physics features match the frozen Fortran oracle at commit `7d3f77f`, with deterministic regression evidence for cyclic loading, runtime vdW/self-contact, checkpoint/restart, MPI parity, and VTU outputs.

### Acceptance Criteria
<!-- Each criterion must be independently verifiable -->
<!-- Claude must extract or define these in Round 0 -->
- `GT-AC1`: The cyclic `nCodeLoad=31` executable path matches the committed same-trace Fortran replay through the first constrained step, including accepted-state progression, `GNORM`, reaction output, and emitted `energy.dat`/`force.dat` rows within existing test tolerances.
- `GT-AC2`: Runtime vdW/self-contact translation is connected to the simulator so executable-path `nvdw=1` oracle cases produce Fortran-matching energy, force, neighbor-list, and emitted field data within committed test tolerances.
- `GT-AC3`: Checkpoint/restart for `x0`, `eta`, and `K0_ref` round-trips correctly across interrupted cyclic runs and detects incompatible restart conditions instead of silently corrupting state.
- `GT-AC4`: MPI runtime parity is verified for `np=1`, `np=2`, and `np=4`, with consistent energy/reaction output and correct element partitioning across supported oracle cases.
- `GT-AC5`: VTU/PVD runtime output remains parser-valid and matches executable-path oracle data, including real `nvdw=1` field coverage for `atomic_density` and `W_density`.
- `GT-AC6`: Project documentation and final verification evidence stay synchronized with the translated code, including `document/translation_notes.md`, `document/fortran_conventions.md`, `AGENT.md`, and end-to-end acceptance coverage.

---

## MUTABLE SECTION
<!-- Update each round with justification for changes -->

### Plan Version: 1 (Updated: Round 0)

#### Plan Evolution Log
<!-- Document any changes to the plan with justification -->
| Round | Change | Reason | Impact on AC |
|-------|--------|--------|--------------|
| 0 | Initial tracker seeded from the verified state at the end of the prior RLCR session | The previous loop stopped via circuit breaker, so the restarted session needs the same anchored runtime blockers, completed checkpoint rejection surface, and accepted-state-2 element oracle progress rather than a blank tracker | Keeps the restarted loop aligned with `GT-AC1` through `GT-AC6` without reopening already verified sub-surfaces |
| 4 | Updated the cyclic accepted-state-2 element-oracle issue to reflect that the committed element-3200 artifact is exercised by a passing C++ regression | The prior RLCR session converted the element-level oracle into an active C++ regression, so the mutable tracker history must preserve that real acceptance-surface progress | Clarifies that `GT-AC1` has a verified element-level contract while the executable-path cyclic replay lane remains the main blocker |

#### Active Tasks
<!-- Map each task to its target Acceptance Criterion and routing tag -->
| Task | Target AC | Status | Tag | Owner | Notes |
|------|-----------|--------|-----|-------|-------|
| `task6a` vdW interaction kernel (runtime LJ/binning path) | GT-AC2 | pending | coding | claude | Runtime solver-side vdW kernel is still missing |
| `task6b` Self-contact detection with topological exclusion on runtime path | GT-AC2 | pending | coding | claude | Depends on runtime vdW kernel integration |
| `task6c` Integrate runtime vdW/self-contact into global assembly with oracle tests | GT-AC2, GT-AC5 | pending | coding | claude | Must remove `E_vdw = 0.0` placeholder and add real executable-path evidence |
| `task7a` Cyclic BC/controller parity for `nCodeLoad=30/31` on executable path | GT-AC1 | pending | coding | claude | The accepted-state-2/3 constrained-step replay lane is still the main blocker |
| `task7b` Irreversible crease memory init/update/output with `K0_ref` lifecycle validation | GT-AC1, GT-AC3 | pending | coding | claude | Partial short-lane coverage exists, real archived cyclic lane still open |
| `task7c` Crease detection / facet analysis parity on cyclic runtime path | GT-AC1, GT-AC6 | pending | coding | claude | Archived final-state `crease_map.dat` parity exists; executable-path replay coverage still missing |
| `task7d` Real cyclic checkpoint/restart for `x0`, `eta`, and `K0_ref` | GT-AC3 | pending | coding | claude | Shared loader and short-lane restart coverage exist, archived replay-path restart still open |
| `task7e` Cyclic oracle and restart acceptance on the archived replay lane | GT-AC1, GT-AC3 | pending | coding | claude | Must close the runtime cyclic acceptance proof, not only helper/unit surfaces |
| `task8a` Multi-rank consistency tests for `np=1`, `np=2`, and `np=4` | GT-AC4 | pending | coding | claude | Runtime parity is still unproven |
| `task8b` Checkpoint compatibility across rank counts on executable path | GT-AC3, GT-AC4 | pending | coding | claude | Rank-mismatch rejection exists; full parity/restart compatibility still open |
| `task8c` Final AGENT / translation-notes synchronization after runtime milestones are green | GT-AC6 | pending | coding | claude | Must follow actual runtime closure, not precede it |
| `task8d` Full integration suite and final oracle comparison sweep | GT-AC6 | pending | analyze | codex | Final acceptance sweep only after runtime ACs are green |

### Completed and Verified
<!-- Only move tasks here after Codex verification -->
| AC | Task | Completed Round | Verified Round | Evidence |
|----|------|-----------------|----------------|----------|
| GT-AC6 | `task0a` Build Fortran oracle, run graphene compression case, archive `nano_*.dat` | prior repo state | prior repo state | Archived oracle directories exist under `test/cases/graphene_compression_prepro/` and `test/cases/graphene_compression_simulator/np1/`; case map documented in `AGENT.md`; fixtures are consumed by `PreprocessorOracle.*` and `E2ECompression.*` tests listed by `ctest --test-dir build -N` |
| GT-AC6 | `task0b` Run Fortran cyclic crumple case and archive outputs | prior repo state | prior repo state | Archived cyclic oracle exists under `test/cases/graphene_cyclic_crumple/`; cyclic replay and restart tests are listed by `ctest --test-dir build -N` (`E2ECyclicRuntime.*`) |
| GT-AC6 | `task0c` Write `document/fortran_conventions.md` | prior repo state | prior repo state | `document/fortran_conventions.md` exists and is referenced by `AGENT.md` and `document/translation_notes.md` |
| GT-AC6 | `task1a` CMake scaffold: C++17, MPI, Eigen3, GoogleTest | prior repo state | prior repo state | `CMakeLists.txt`; build targets `PrePro`, `crunch_it`, `unit_tests`, `integration_tests`, `checkpoint_integration_tests` are discoverable via `ctest --test-dir build -N` |
| GT-AC6 | `task1b` Core C++ data types mirroring Fortran headers | prior repo state | prior repo state | `include/fce/types.hpp`, `include/fce/io.hpp`, `include/fce/simulator.hpp`; coverage via `Read*`, `SimulatorInput.*`, and solver/unit tests listed by `ctest --test-dir build -N` |
| GT-AC6 | `task1c` `nano_*.dat` readers/writers with round-trip unit tests | prior repo state | prior repo state | `RoundTrip.*`, `Read*`, and checkpoint I/O tests listed by `ctest --test-dir build -N` |
| GT-AC4 | `task1d` MPI init/finalize wrapper and rank utilities | prior repo state | prior repo state | `MpiEnv.*` tests listed by `ctest --test-dir build -N` |
| GT-AC6 | `task1e` Field-by-field oracle comparison support | prior repo state | prior repo state | `test/support/oracle_compare.cpp`; used by `PreprocessorOracle.*` and round-trip/oracle integration tests |
| GT-AC6 | `task2a` Mesh generation (`mesh_gen_square`) | prior repo state | prior repo state | `PreprocessorOracle.ArchivedCompressionCaseMatchesOracle`, `RoundTrip.Mesh`, and oracle-compare support listed by `ctest --test-dir build -N` |
| GT-AC6 | `task2b` Ghost node generation and connectivity tables | prior repo state | prior repo state | `GhostNodes.*`, `PreprocessorOracle.ArchivedCasesIncludeGhostCoordinateArtifacts`, `RoundTrip.Mesh` |
| GT-AC6 | `task2c` B-spline basis and derivative tests | prior repo state | prior repo state | `BSpline.*` tests listed by `ctest --test-dir build -N` |
| GT-AC6 | `task2d` Gauss quadrature setup | prior repo state | prior repo state | Gauss-backed kernel tests across `ElementState.*`, `ElementEnergy.*`, and preprocessor oracle tests |
| GT-AC6 | `task2e` Initial `F0` / `J0` computation | prior repo state | prior repo state | `ReadZero.GrapheneCompression`, `RoundTrip.Zero`, `PreprocessorOracle.ArchivedCompressionCaseMatchesOracle` |
| GT-AC6 | `task2f` Preprocessor BC/load setup including cyclic params | prior repo state | prior repo state | `ReadBCs.GrapheneCompression`, `RoundTrip.BCs`, cyclic prepro oracle tests |
| GT-AC6 | `task2g` vdW preprocessing (neighbor lists, shape functions) | prior repo state | prior repo state | `ReadVdw.GrapheneSelfContact`, `RoundTrip.Vdw`, `PreprocessorOracle.ArchivedSelfContactPreproInputMatchesOracleOutputs`, `LoadedVdwCaseWritesNonzeroDensityArrays` |
| GT-AC6 | `task2h` Full preprocessor integration and oracle comparison | prior repo state | prior repo state | `PreprocessorOracle.*` integration tests listed by `ctest --test-dir build -N` |
| GT-AC6 | `task3a` Exponential map / Cauchy-Born rule | prior repo state | prior repo state | `Exponential.*` tests listed by `ctest --test-dir build -N` |
| GT-AC6 | `task3b` Bond/deformation decomposition helpers | prior repo state | prior repo state | `Geometry.*`, `OuterPotential.*`, and constitutive/oracle tests |
| GT-AC6 | `task3c` Brenner / outer potential oracle tests | prior repo state | prior repo state | `Brenner.*`, `OuterPotential.BrennerMatchesEvaluateBrenner`, `ElementEnergy.BrennerMaterialMatchesFortranOracle` |
| GT-AC6 | `task3d` Inner Newton solver for `eta` | prior repo state | prior repo state | `NewtonInner.*` tests listed by `ctest --test-dir build -N` |
| GT-AC6 | `task3e` Element-level energy/force kernel | prior repo state | prior repo state | `ElementEnergy.*` tests listed by `ctest --test-dir build -N` |
| GT-AC6 | `task3f` Principal curvature extraction | prior repo state | prior repo state | `Principal.*` and `ElementState.*` tests |
| GT-AC6 | `task4a` Global energy/force assembly with MPI partitioning | prior repo state | prior repo state | `SimulatorAssembly.*`, `MpiEnv.Partition*`, and integration assembly tests |
| GT-AC6 | `task4b` L-BFGS translation and solver tests | prior repo state | prior repo state | `Lbfgs*` tests listed by `ctest --test-dir build -N` |
| GT-AC6 | `task4c` Loading controller for runtime paths | prior repo state | prior repo state | `LoadController.*` tests listed by `ctest --test-dir build -N` |
| GT-AC6 | `task4d` `pasapas` load-stepping loop | prior repo state | prior repo state | `E2ECompression.*`, `E2ECyclicRuntime.*`, and simulator executable-path tests |
| GT-AC6 | `task4e` Reaction force/torque computation | prior repo state | prior repo state | `LoadController.ComputeReaction*`, generated VTU vs generated row checks, bilayer moment tests |
| GT-AC6 | `task4f` End-to-end serial compression run vs oracle | prior repo state | prior repo state | `E2ECompression.*` integration tests listed by `ctest --test-dir build -N` |
| GT-AC5 | `task5a` VTU/ParaView output translation | prior repo state | prior repo state | `RuntimeOutput.*`, `E2ECompression.RuntimeOutputReplaysArchivedCompressionSnapshotsIndependentlyOfSolver`, parser-backed XML validation helpers |
| GT-AC5 | `task5b` VTU format validation / load test | prior repo state | prior repo state | `RuntimeOutput.*`, `RuntimeOutputVdwCase.LoadedVdwCaseWritesNonzeroDensityArrays`, XML validator path used by integration tests |
| GT-AC3 | Shared checkpoint resume logic now deterministically rejects malformed checkpoints and rank-count mismatches without depending on the MPI launcher path | prior loop | prior loop | `./build/checkpoint_integration_tests --gtest_filter='CheckpointRejectionRuntime.CrunchItRejectsCheckpointWrittenWithDifferentRankCount:CheckpointRejectionRuntime.CrunchItRejectsMalformedCheckpointAcrossRanks' --gtest_brief=1`; `./checkpoint_integration_tests --gtest_filter='CheckpointRejectionRuntime.CrunchItRejectsCheckpointWrittenWithDifferentRankCount:CheckpointRejectionRuntime.CrunchItRejectsMalformedCheckpointAcrossRanks' --gtest_brief=1`; `ctest --test-dir build --output-on-failure -R '^CheckpointRejectionRuntime\\.(CrunchItRejectsCheckpointWrittenWithDifferentRankCount|CrunchItRejectsMalformedCheckpointAcrossRanks)$'` |

### Explicitly Deferred
<!-- Items here require strong justification -->
| Task | Original AC | Deferred Since | Justification | When to Reconsider |
|------|-------------|----------------|---------------|-------------------|

### Open Issues
<!-- Issues discovered during implementation -->
| Issue | Discovered Round | Blocking AC | Resolution Path |
|-------|-----------------|-------------|-----------------|
| Cyclic replay currently diverges during the constrained solve after accepted state 2 even though earlier checkpoints and `eta` still match the same-trace Fortran replay | prior loop | GT-AC1 | Instrument the free-gradient / force path around accepted states 2-3 and compare against committed replay fixtures |
| Untraced cyclic step-one replay still stalls before step-1 output files are written, even though the traced accepted-state probes complete and still localize the mismatch to the top-right free-node force path | prior loop | GT-AC1 | Fix the constrained solve/update path so the untraced replay reaches the first output row, then reconcile it against the committed replay fixtures |
| The hardened Fortran-side cyclic accepted-state-2 element-3200 oracle extractor is reproducible, its committed artifact is exercised by a passing C++ regression, and that narrows the remaining `GT-AC1` gap to the live cyclic constrained-step replay/output path | 2 | GT-AC1 | Use the now-consumed element-3200 oracle together with the accepted-state-2/3 replay fixtures to fix the executable-path force / `GNORM` / reaction mismatch |
| Simulator-side vdW/self-contact translation is still missing, so runtime `nvdw=1` oracle evidence and emitted field parity cannot yet pass | prior loop | GT-AC2, GT-AC5 | Port `vdw_modules.f90`, wire it into runtime assembly, then add executable-path regression coverage |
| Checkpoint/restart and multi-rank runtime acceptance coverage are incomplete despite existing serial/MPI scaffolding | prior loop | GT-AC3, GT-AC4 | Finish the remaining runtime implementation and add rank-count parity / restart tests before final verification |
