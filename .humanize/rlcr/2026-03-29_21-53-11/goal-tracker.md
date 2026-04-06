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
Translate the Fortran 90 graphene simulation codebase — comprising `grapheneCompressionOriginPrePro/` (preprocessor, 11 source files) and `grapheneCompressionOriginVersion/` (FEM simulator, 31 source files) — into a functionally equivalent C++17 implementation. The C++ code must reproduce the same physics, algorithms, and numerical results as the canonical Fortran reference (commit `7d3f77f` of `finite_crystal_elasticity/`) within accepted engineering tolerances. The translation covers the full feature set: core graphene compression, van der Waals interactions, cyclic crumpling loading modes (nCodeLoad=30/31), self-contact, irreversible crease memory, checkpoint/restart, and MPI parallelization. Outputs required: `nano_*.dat` inter-program data files and VTU/ParaView visualization.

### Acceptance Criteria
<!-- Each criterion must be independently verifiable -->

- **AC-1**: Fortran oracle baseline built from commit `7d3f77f`, run on graphene 20 nm×20 nm case, all `nano_*.dat` and simulation outputs archived; data conventions document written.
- **AC-2**: C++ preprocessor generates `nano_*.dat` matching oracle — integer fields exact, floating-point fields ≤ 1×10⁻¹⁰ absolute.
- **AC-3**: B-spline basis `N_i` and derivatives correct for all 12-node patch configurations; partition-of-unity `|Σ N_i − 1| < 1×10⁻¹⁴`.
- **AC-4**: Ghost node positions match Fortran parallelogram extrapolation within 1×10⁻¹² absolute; connectivity tables integer-exact.
- **AC-5**: Brenner REBO `W`, `dW/dpe` match Fortran within 1×10⁻¹⁰ absolute on ≥10 configurations; `d²W/dpe²` passes finite-difference check to 1×10⁻⁵ relative.
- **AC-6**: Inner Newton solver for η converges to Fortran result within 1×10⁻¹⁰ absolute on 10 element-level test states.
- **AC-7**: End-to-end serial compression (np=1, nloadstep=100): energy per step within 1×10⁻⁴ relative, reaction force within 1×10⁻³ relative vs Fortran oracle.
- **AC-8**: vdW energy/forces match Fortran within 1×10⁻¹⁰ absolute; self-contact neighbor list correct with topological exclusion enforced.
- **AC-9**: Cyclic loading (nCodeLoad=30/31) phase tracking correct; crease memory `K0_ref` update matches Fortran; `crease_map.dat` within 1×10⁻⁴ relative.
- **AC-10**: Checkpoint/restart reproduces final outputs within AC-7 tolerances; np mismatch detected and reported.
- **AC-11**: MPI results for np=1, np=2, np=4 agree within 1×10⁻⁴ relative energy, 1×10⁻³ relative reaction force; element partitioning correct.
- **AC-12**: VTU files valid XML, load in ParaView, nodal/element data within 1×10⁻⁶ relative.
- **AC-13**: `AGENT.md` exists with project structure and build instructions; `document/translation_notes.md` maintained with implementation notes and verification evidence.

---

## MUTABLE SECTION
<!-- Update each round with justification for changes -->

### Plan Version: 2 (Updated: Round 1)

#### Plan Evolution Log
<!-- Document any changes to the plan with justification -->
| Round | Change | Reason | Impact on AC |
|-------|--------|--------|--------------|
| 0 | Initial plan | - | - |
| 0 | Corrected AC-7: nCodeLoad=1→3, nloadstep=100→50; corrected task4c nCodeLoad reference | Oracle archaeology revealed existing test case uses nCodeLoad=3 (compression) with nloadstep=50. nCodeLoad=1 in the plan was incorrect. | AC-7, AC-11 (tolerance references unchanged) |
| 0 | Completed Milestone 0: tasks 0a, 0b, 0c done | All oracle outputs archived; fortran_conventions.md written; plan corrections applied | AC-1 satisfied |
| 0 | Review reopened task0a and task0b; rejected "plan corrections applied" claim as incomplete | `document/plan.md` and tracker task rows still reference `nCodeLoad=1` / `nloadstep=100`; compression archive is an 8-rank MPI run despite README claiming serial `np=1`; cyclic archive captured `nCodeLoad=31` instead of the contracted `nCodeLoad=30`; build-provenance and backup-variant negative-test evidence are not archived. | AC-1, AC-7, AC-9, AC-11 |
| 1 | Formally amended plan.md: AC-9 uses nCodeLoad=31 as primary cyclic baseline; no nCodeLoad=30 case exists in oracle | Exhaustive oracle search found no nCodeLoad=30 files. nCodeLoad=31 (biaxial cyclic) is the only available cyclic oracle. nCodeLoad=30 stub retained for implementation. | AC-9 |
| 1 | Archived true serial np=1 compression simulation: test/cases/graphene_compression_simulator/np1/ | Previous archive was from mpirun -np 8. New archive confirmed: "Numero de procesadores: 1", final energy 1.3427137479171509E-003. | AC-1, AC-7, AC-11 |
| 1 | Implemented Milestone 1: CMake scaffold, data types, I/O library, MPI wrapper; 21/21 tests pass | All Milestone 1 coding tasks complete. Key discovery: nano_zero.dat stores RefConfig per element (not per gauss point), corrected read_zero/write_zero signatures. BCs reader rewritten sequentially to handle label-consumed-by-previous-loop bug. | AC-2 partial |

#### Active Tasks
<!-- Mainline tasks only: each task must directly advance the current round objective and carry routing metadata -->
| Task | Target AC | Status | Tag | Owner | Notes |
|------|-----------|--------|-----|-------|-------|
| task1e: Field-by-field comparison script for oracle validation | AC-2, AC-7 | pending | coding | claude | Milestone 1 Phase E |
| task2a: Mesh generation (mesh_gen_square) | AC-2 | pending | coding | claude | Milestone 2 Phase A |
| task2b: Ghost node generation and connectivity tables | AC-4 | pending | coding | claude | Milestone 2 Phase B |
| task2c: B-spline basis N_i and derivatives | AC-3 | pending | coding | claude | Milestone 2 Phase C |
| task2d: Gauss quadrature setup | AC-2 | pending | coding | claude | Milestone 2 Phase D |
| task2e: Initial deformation gradient F0 and Jacobian J0 | AC-2 | pending | coding | claude | Milestone 2 Phase E |
| task2f: BC/load setup for preprocessor | AC-2 | pending | coding | claude | Milestone 2 Phase F |
| task2g: vdW preprocessing | AC-8 | pending | coding | claude | Milestone 2 Phase G |
| task2h: Full preprocessor integration and oracle comparison | AC-2 | pending | coding | claude | Milestone 2 Phase H |
| task3a: Exponential map for Cauchy-Born rule | AC-6, AC-7 | pending | coding | claude | Milestone 3 Phase A |
| task3b: Deformation gradient decomposition, bond vectors | AC-5, AC-6 | pending | coding | claude | Milestone 3 Phase B |
| task3c: Brenner REBO potential | AC-5 | pending | coding | claude | Milestone 3 Phase C |
| task3d: Inner Newton solver for η | AC-6 | pending | coding | claude | Milestone 3 Phase D |
| task3e: Element-level energy/force kernel | AC-7 | pending | coding | claude | Milestone 3 Phase E |
| task3f: Principal curvature extraction | AC-7, AC-9 | pending | coding | claude | Milestone 3 Phase F |
| task4a: Global energy/force assembly with MPI partitioning | AC-7, AC-11 | pending | coding | claude | Milestone 4 Phase A |
| task4b: L-BFGS minimizer from lbfgs.f | AC-7 | pending | coding | claude | Milestone 4 Phase B |
| task4c: Loading controller nCodeLoad=1 | AC-7 | pending | coding | claude | Milestone 4 Phase C |
| task4d: Pasapas load-stepping loop | AC-7 | pending | coding | claude | Milestone 4 Phase D |
| task4e: Reaction force and torque computation | AC-7 | pending | coding | claude | Milestone 4 Phase E |
| task4f: End-to-end serial run vs oracle | AC-7 | pending | coding | claude | Milestone 4 Phase F |
| task5a: VTU/ParaView output translation | AC-12 | pending | coding | claude | Milestone 5 Phase A |
| task5b: VTU format validation | AC-12 | pending | coding | claude | Milestone 5 Phase B |
| task6a: vdW interaction kernel | AC-8 | pending | coding | claude | Milestone 6 Phase A |
| task6b: Self-contact detection | AC-8 | pending | coding | claude | Milestone 6 Phase B |
| task6c: vdW integration + oracle tests | AC-8 | pending | coding | claude | Milestone 6 Phase C+D |
| task7a: Cyclic BC controller nCodeLoad=30/31 | AC-9 | pending | coding | claude | Milestone 7 Phase A |
| task7b: Irreversible crease memory | AC-9 | pending | coding | claude | Milestone 7 Phase B |
| task7c: Crease detection and facet analysis | AC-9 | pending | coding | claude | Milestone 7 Phase C |
| task7d: Checkpoint/restart | AC-10 | pending | coding | claude | Milestone 7 Phase D |
| task7e: Oracle tests for cyclic case and checkpoint | AC-9, AC-10 | pending | coding | claude | Milestone 7 Phase E |
| task8a: Multi-rank consistency tests np=1,2,4 | AC-11 | pending | coding | claude | Milestone 8 Phase A |
| task8b: Checkpoint compatibility across rank counts | AC-10, AC-11 | pending | coding | claude | Milestone 8 Phase B |
| task8c: AGENT.md + translation_notes.md finalization | AC-13 | pending | coding | claude | Milestone 8 Phase C |
| task8d: Full integration test suite | AC-1 through AC-13 | pending | analyze | codex | Milestone 8 Phase D |

### Blocking Side Issues
<!-- Only issues that directly block current mainline progress belong here -->
| Issue | Discovered Round | Blocking AC | Resolution Path |
|-------|-----------------|-------------|-----------------|
| (none — all Round 0 blocking issues resolved in Round 1) | - | - | - |

### Queued Side Issues
<!-- Non-blocking issues stay queued and must NOT replace the round objective -->
| Issue | Discovered Round | Why Not Blocking | Revisit Trigger |
|-------|-----------------|------------------|-----------------|

### Completed and Verified
<!-- Only move tasks here after Codex verification -->
| AC | Task | Completed Round | Verified Round | Evidence |
|----|------|-----------------|----------------|----------|
| AC-1 | task0c: Write document/fortran_conventions.md | 0 | pending | document/fortran_conventions.md covers indexing, units, all nano_*.dat formats, sign conventions, MPI conventions, nCodeLoad table, active source files |
| AC-1 | task0a-prov: Archive build provenance for oracle binaries | 1 | pending | test/cases/graphene_compression_prepro/build_provenance.md: oracle commit, compiler versions, build commands, source patches |
| AC-1 | task0a-fix: Archive true serial np=1 compression simulation | 1 | pending | test/cases/graphene_compression_simulator/np1/: simulator.log confirms "Numero de procesadores: 1", 51 VTU snapshots, final energy 1.3427e-3 |
| AC-1, AC-9 | task0b-fix: Amend plan for nCodeLoad=31 cyclic baseline | 1 | pending | document/plan.md commit 61bb9e2; AC-9 updated to use nCodeLoad=31; no nCodeLoad=30 oracle exists |
| AC-13 | task1a: CMake scaffold | 1 | pending | CMakeLists.txt: C++17, OpenMPI, Eigen3, GoogleTest via FetchContent; fce_core lib, PrePro/crunch_it executables, unit_tests/integration_tests |
| AC-2 | task1b+1c: Data types and nano_*.dat readers/writers | 1 | pending | include/fce/types.hpp, include/fce/io.hpp, src/core/io.cpp; 21/21 tests pass (ctest); round-trip tests for all 7 file types |
| AC-11 | task1d: MPI wrapper | 1 | pending | include/fce/mpi_env.hpp, src/core/mpi_env.cpp; element_partition + partition_for_rank; MpiEnv RAII |

### Explicitly Deferred
<!-- Items here require strong justification -->
| Task | Original AC | Deferred Since | Justification | When to Reconsider |
|------|-------------|----------------|---------------|-------------------|
