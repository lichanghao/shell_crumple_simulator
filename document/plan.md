# C++ Translation of Graphene Finite Crystal Elasticity Simulator

## Goal Description

Translate the Fortran 90 graphene simulation codebase — comprising `grapheneCompressionOriginPrePro/` (preprocessor, 11 source files) and `grapheneCompressionOriginVersion/` (FEM simulator, 31 source files) — into a functionally equivalent C++17 implementation. The C++ code must reproduce the same physics, algorithms, and numerical results as the canonical Fortran reference (commit `7d3f77f` of `finite_crystal_elasticity/`) within accepted engineering tolerances. The translation covers the full feature set: core graphene compression, van der Waals interactions, cyclic crumpling loading modes (nCodeLoad=30/31), self-contact, irreversible crease memory, checkpoint/restart, and MPI parallelization. Outputs required: `nano_*.dat` inter-program data files and VTU/ParaView visualization.

## Acceptance Criteria

Following TDD philosophy, each criterion includes positive and negative tests for deterministic verification.

- **AC-1**: Fortran oracle baseline is built, run, and captured with all reference outputs stored.
  - Positive Tests (expected to PASS):
    - Fortran `PrePro` builds from commit `7d3f77f` source and runs the graphene 20 nm×20 nm case producing all `nano_*.dat` files.
    - Fortran `crunch_it` builds from commit `7d3f77f` source and runs the graphene compression case to completion (nloadstep=100) without crashing.
    - Reference `nano_*.dat` files and energy/reaction output from the Fortran run are archived in `test/cases/graphene_compression_prepro/` and `test/cases/graphene_compression_simulator/`.
    - Data conventions document records: 1-based vs 0-based indexing choice, unit system (nm, eV), field ordering in each `nano_*.dat`, sign conventions for BCs, and MPI rank partitioning approach.
  - Negative Tests (expected to FAIL):
    - Attempting to build with backup variants (`*.f90_mod2`, `*.f90_good`, `*.f90A`) instead of canonical `.f90` files fails to produce the reference oracle output.

- **AC-2**: C++ preprocessor correctly generates `nano_*.dat` files matching the Fortran oracle for the 20 nm×20 nm graphene case.
  - Positive Tests (expected to PASS):
    - Discrete/topology fields (node count, element count, connectivity indices, DOF counts): exact integer match vs oracle.
    - Floating-point coordinates and Jacobians `J0`: absolute deviation ≤ 1×10⁻¹⁰.
    - Initial deformation gradients `F0` (2×2 tensors per Gauss point): absolute deviation ≤ 1×10⁻¹⁰.
    - BC vectors, rotation matrices, load parameters: absolute deviation ≤ 1×10⁻¹⁰.
    - vdW neighbor lists (when `nvdw=1`) and shape functions: absolute deviation ≤ 1×10⁻¹⁰.
  - Negative Tests (expected to FAIL):
    - A deliberately corrupted mesh (swapped connectivity) causes validation to report mismatch rather than silently pass.
    - Running the C++ preprocessor with invalid `data.dat` (out-of-range chirality indices) exits with a clear error, not a silent wrong output.

- **AC-3**: B-spline basis functions and derivatives are correct for all valid 12-node patch configurations.
  - Positive Tests (expected to PASS):
    - For 5 interior patch configurations, `N_i(ξ,η)` matches Fortran `BSpline` output within absolute 1×10⁻¹⁴.
    - For 5 boundary patch configurations (including ghost-node cases), `N_i` and `∂N_i/∂ξ`, `∂N_i/∂η` match Fortran within absolute 1×10⁻¹⁴.
    - Partition-of-unity check: `|Σ N_i − 1| < 1×10⁻¹⁴` for all valid `(ξ,η)` in the element.
  - Negative Tests (expected to FAIL):
    - Wrong knot span selection (off-by-one in element index) is caught by the partition-of-unity check failing.
    - Basis evaluation outside the valid parameter domain returns an error/assertion rather than silently extrapolating.

- **AC-4**: Ghost node positions match the Fortran parallelogram extrapolation convention within absolute 1×10⁻¹².
  - Positive Tests (expected to PASS):
    - For all boundary elements in the 20 nm×20 nm mesh, C++ ghost node coordinates match Fortran within 1×10⁻¹².
    - Ghost node connectivity table matches Fortran integer-for-integer.
  - Negative Tests (expected to FAIL):
    - Deliberately using the wrong anchor node in the parallelogram formula (swapping two neighbors) produces ghost positions that fail the 1×10⁻¹² tolerance check.

- **AC-5**: Brenner REBO potential evaluates `W`, `dW/dpe`, and `d²W/dpe²` correctly.
  - Positive Tests (expected to PASS):
    - For at least 10 distinct bond configurations (covering equilibrium, compressed, stretched states): `W` and `dW/dpe` match Fortran `brenner.f90` within absolute 1×10⁻¹⁰ (or relative 1×10⁻⁸ when `|W| > 1×10⁻⁶`).
    - `d²W/dpe²` passes a finite-difference consistency check: FD-estimated second derivative agrees with analytical to relative 1×10⁻⁵.
    - Potential evaluates correctly for all three bond vectors `E_1`, `E_2`, `E_3` in a reference graphene unit cell.
  - Negative Tests (expected to FAIL):
    - Bond length beyond the Brenner cutoff radius returns `W=0`, `dW=0`, not a nonzero value.
    - A bond vector with zero norm triggers an error/assertion rather than producing NaN silently.

- **AC-6**: Inner Newton solver for shift vector η converges to the correct equilibrium matching Fortran.
  - Positive Tests (expected to PASS):
    - For 10 element-level test states (drawn from Fortran load-step outputs), converged η matches Fortran `newton_inner` result within absolute 1×10⁻¹⁰.
    - Newton residual at convergence satisfies the inner tolerance criterion (≤ `crit(2)` = 1×10⁻⁸ by default).
    - Convergence is achieved within the same iteration count as Fortran (±1 iteration) for all test states.
  - Negative Tests (expected to FAIL):
    - An element state far from equilibrium (pathologically deformed) triggers the Newton failure penalty mode, not a silent wrong η.
    - Exceeding the maximum inner iteration count triggers the documented fallback (penalty applied), not a hang or crash.

- **AC-7**: End-to-end graphene compression simulation matches Fortran oracle within engineering tolerance.
  - Positive Tests (expected to PASS):
    - For graphene 20 nm×20 nm, nCodeLoad=1, nloadstep=100 (serial, np=1): total energy at each load step agrees with Fortran within relative 1×10⁻⁴ (absolute tolerance floor 1×10⁻¹² eV).
    - Reaction force at final load step (step 100) agrees with Fortran within relative 1×10⁻³.
    - Nodal displacements at step 50 agree with Fortran within relative 1×10⁻³ (absolute floor 1×10⁻¹² nm).
    - Simulation exits cleanly with no NaN/Inf in energy or forces throughout all 100 load steps.
  - Negative Tests (expected to FAIL):
    - A simulation with deliberately corrupted `nano_Mesh.dat` (wrong connectivity) diverges or produces NaN, detected before completing.
    - Running with `np=1` vs `np=4` on the same case produces results consistent within the stated tolerances (MPI consistency check).

- **AC-8**: Van der Waals interactions (Lennard-Jones type) are correctly implemented and verified.
  - Positive Tests (expected to PASS):
    - vdW energy and forces for a known neighbor configuration match Fortran `vdw_modules.f90` within absolute 1×10⁻¹⁰.
    - Self-contact detection identifies same-sheet pairs beyond topological exclusion distance `min_topo_dist` correctly.
    - Bin-based neighbor search produces the same neighbor list as the Fortran implementation for the reference graphene case with `nvdw=1`.
  - Negative Tests (expected to FAIL):
    - Pairs within the topological exclusion distance are NOT included in the self-contact neighbor list.
    - A pair beyond the cutoff radius `r_cut` contributes zero energy, not a small nonzero value.

- **AC-9**: Cyclic loading (nCodeLoad=30/31) and crease memory function correctly.
  - Positive Tests (expected to PASS):
    - For nCodeLoad=30 (uniaxial cyclic): loading/release phase signs flip correctly; L-BFGS history resets at phase transitions.
    - `K0_ref` (reference curvature) is updated correctly after each release phase, matching Fortran `crease.f90` update logic.
    - Crease detection output `crease_map.dat` (columns: ielem, kappa_mean, kappa_max, is_creased, n_neigh, min_dihedral_deg) is generated with values matching Fortran within relative 1×10⁻⁴.
  - Negative Tests (expected to FAIL):
    - A fresh run without prior crease history initializes `K0_ref=0` correctly, not with stale values.
    - Setting `ncrease=0` skips crease analysis; `crease_map.dat` is not generated.

- **AC-10**: Checkpoint/restart produces consistent results.
  - Positive Tests (expected to PASS):
    - A run interrupted at cycle boundary and restarted from `nano_checkpoint.dat` produces identical final outputs (within AC-7 tolerances) compared to an uninterrupted run.
    - Checkpoint file contains `x0`, `eta`, and `K0_ref` arrays that restore state correctly on all MPI ranks.
  - Negative Tests (expected to FAIL):
    - Deleting `nano_checkpoint.dat` before a run forces a fresh start (no stale-state restart).
    - A checkpoint written with np=4 cannot silently corrupt a restart with np=1 (mismatch is detected and reported).

- **AC-11**: MPI parallelization is correct and consistent with serial execution.
  - Positive Tests (expected to PASS):
    - For the 20 nm×20 nm graphene case, results with np=1, np=2, and np=4 all agree within relative 1×10⁻⁴ for energy and relative 1×10⁻³ for reaction forces.
    - Element partitioning (`pre_ener` equivalent) correctly distributes elements across ranks with no element counted twice or missed.
    - Global energy MPI_ALLREDUCE produces the same result as manual summation within machine epsilon.
  - Negative Tests (expected to FAIL):
    - Incorrect ghost-element partitioning (an element on two ranks) is detected by an energy-conservation check.
    - MPI_ALLREDUCE on vdW energy when `nvdw=0` is skipped (no segfault from uninitialized pointers).

- **AC-12**: VTU/ParaView output is generated correctly.
  - Positive Tests (expected to PASS):
    - Output `.vtu` files are valid XML and load correctly in ParaView.
    - Nodal positions, displacements, and element data match the simulation state within relative 1×10⁻⁶.
  - Negative Tests (expected to FAIL):
    - Requesting VTU output with an uninitialized mesh state triggers an assertion, not a corrupt file.

- **AC-13**: Documentation is maintained throughout development.
  - Positive Tests (expected to PASS):
    - `AGENT.md` exists in C++ repo root with project structure, build instructions, and key design decisions.
    - `document/translation_notes.md` contains per-milestone implementation notes, bugs encountered, and verification evidence.
    - Build instructions produce working executables on macOS with gfortran/g++/OpenMPI environment.
  - Negative Tests (expected to FAIL):
    - Building without the documented dependencies (Eigen3, GoogleTest) fails with a clear error, not a silent wrong build.

## Path Boundaries

### Upper Bound (Maximum Acceptable Scope)
The implementation includes the full graphene simulation pipeline in C++17 with MPI: preprocessor (`PrePro` equivalent) and simulator (`crunch_it` equivalent) as separate executables sharing a common core library. All loading modes (nCodeLoad=1,2,3,30,31), van der Waals interactions with self-contact, irreversible crease memory, checkpoint/restart, and VTU/ParaView output are implemented. Unit tests cover every major kernel. End-to-end oracle tests compare C++ vs Fortran for at least two cases (standard compression and one cyclic case). Documentation is complete and up to date.

### Lower Bound (Minimum Acceptable Scope)
The implementation includes a working C++ preprocessor and simulator for the standard graphene compression case (nCodeLoad=1) in serial, with correct B-spline FEM, Brenner REBO potential, inner Newton relaxation, and outer L-BFGS minimization. All `nano_*.dat` I/O is correct, VTU output is generated, and end-to-end energy/reaction results match Fortran within AC-7 tolerances. All acceptance criteria AC-1 through AC-7 and AC-12, AC-13 are satisfied; AC-8 through AC-11 may be deferred to a subsequent milestone.

### Allowed Choices
- **Can use**: C++17, CMake 3.20+, Eigen3 for dense linear algebra, GoogleTest/Catch2 for unit tests, OpenMPI for MPI, gfortran/g++ (GCC) compiler suite, header-only libraries.
- **Can use**: Any C++ standard library containers, algorithms, and parallel primitives consistent with C++17.
- **Can use**: Custom L-BFGS implementation translated from `lbfgs.f` (Nocedal), or `liblbfgs` library if it matches the same algorithm (two-loop recursion) and stopping criteria as the Fortran.
- **Cannot use**: External FEM frameworks (deal.II, FEniCS, etc.) that would replace the custom B-spline kernel.
- **Cannot use**: LAPACK/BLAS replacements for operations explicitly coded in the Fortran (e.g., the 2×2 tensor operations); translate those directly.
- **Cannot use**: Heuristic workarounds that bypass any mathematical step documented in the reference papers.
- **Required design**: Object-oriented architecture is required. Encapsulate mesh, material model, solver, and I/O concerns in dedicated classes. Do not write monolithic procedural code that mirrors the Fortran module structure directly.
- **Fixed by design**: The exponential Cauchy-Born algorithm, inner Newton algorithm, and L-BFGS algorithm must match the Fortran implementations exactly (same steps, same stopping criteria, same line-search behavior).

## Feasibility Hints and Suggestions

> **Note**: This section is for reference and understanding only. These are conceptual suggestions, not prescriptive requirements.

### Conceptual Approach

**Project layout**:
```
finite_crystal_elasticity_Cpp/
├── CMakeLists.txt
├── AGENT.md
├── document/
├── include/fce/          ← shared headers
├── src/
│   ├── core/             ← shared kernels (B-spline, Brenner, geometry, vdW, solver)
│   ├── prepro/           ← preprocessor executable
│   └── simulator/        ← simulator executable
├── test/
│   ├── unit/             ← GoogleTest unit tests per kernel
│   ├── integration/      ← golden-oracle comparison tests
│   └── cases/            ← archived Fortran reference inputs/outputs
└── third_party/          ← Eigen3, GoogleTest (submodule or FetchContent)
```

**Key mapping from Fortran to C++**:
- Fortran `MODULE` with derived types → C++ `struct` / `class` in `include/fce/types.hpp`
- Fortran global module variables → constructor-initialized C++ objects passed by reference
- Fortran `COMMON` blocks (lbfgs.f) → encapsulated in C++ LBFGS solver class
- MPI calls → thin wrapper layer allowing easy mock-out for serial unit tests
- 1-based Fortran array indices → 0-based C++ arrays throughout; document this convention explicitly in AC-1 artifact

**Verification workflow**:
1. Run Fortran oracle for a test case → save intermediate dumps (element-level η, energy per step)
2. Run C++ on same case → compare intermediate + final outputs
3. Use a Python or shell comparison script for field-by-field tolerance checks

### Relevant References
- `../finite_crystal_elasticity/document/codebase_analysis.md` — detailed Fortran code structure and data flow
- `../finite_crystal_elasticity/document/build_and_run_notes.md` — build procedures and known issues
- `../finite_crystal_elasticity/reference/cauchy_born_derivation.md` — theoretical derivations
- `../finite_crystal_elasticity/grapheneCompressionOriginPrePro/headers.f90` — canonical data type definitions
- `../finite_crystal_elasticity/grapheneCompressionOriginVersion/Optim.f90` — simulator main entry point
- `../finite_crystal_elasticity/grapheneCompressionOriginVersion/pasapas.f90` — load-stepping and cycle controller
- `../finite_crystal_elasticity/grapheneCompressionOriginVersion/ener_elem.f90` — per-element energy kernel
- `../finite_crystal_elasticity/grapheneCompressionOriginVersion/lbfgs.f` — Nocedal L-BFGS (Fortran, must be translated, not replaced)

## Dependencies and Sequence

### Milestones

1. **Milestone 0 — Fortran Archaeology and Oracle Capture**
   - Phase A: Build Fortran PrePro and crunch_it from commit `7d3f77f` on the project's macOS environment (gfortran/mpif90).
   - Phase B: Run graphene 20 nm×20 nm compression case; archive all `nano_*.dat` input/output and energy log in `test/cases/`.
   - Phase C: Run a cyclic crumpling case (nCodeLoad=30) and archive its outputs.
   - Phase D: Write `document/fortran_conventions.md`: 1-based indexing, units, file format field ordering, sign conventions, MPI rank-0 output assumption, active source files list (including `lbfgs.f`).

2. **Milestone 1 — Project Infrastructure**
   - Phase A: CMake project scaffold, C++17 + OpenMPI + Eigen3 + GoogleTest configured.
   - Phase B: Core C++ data types (`Mesh`, `BCData`, `MatData`, `VdwData`, `CreaseData`) mirroring Fortran derived types from `headers.f90`.
   - Phase C: `nano_*.dat` reader and writer stubs with round-trip unit tests (read Fortran output, write back, verify byte-level or field-level match).
   - Phase D: MPI init/finalize wrapper and rank-partitioning utilities.
   - Phase E: Test harness scripts for field-by-field `nano_*.dat` comparison against archived Fortran oracle.

3. **Milestone 2 — Preprocessor Translation**
   - Phase A: Mesh generation (`mesh_gen_square`) — rectangular grid, element connectivity, boundary edge tagging.
   - Phase B: Ghost node generation and connectivity tables (`connect_mesh`).
   - Phase C: B-spline basis functions and derivatives (`BSpline`) with partition-of-unity tests.
   - Phase D: Gauss quadrature setup.
   - Phase E: Initial deformation gradient and Jacobian computation (`Def_Grad`).
   - Phase F: BC and loading setup (`load.f90` prepro side, including cyclic parameters for nCodeLoad=30/31).
   - Phase G: vdW preprocessing (neighbor list, shape functions, `vdw_previous`).
   - Phase H: Full preprocessor integration, run vs oracle, verify AC-2.

4. **Milestone 3 — Constitutive Model**
   - Phase A: Exponential map for Cauchy-Born rule (`exponential.f90`).
   - Phase B: Deformation gradient decomposition and bond vector computation (`geometry.f90`).
   - Phase C: Brenner REBO potential (`brenner.f90`) — `W`, `dW/dpe`, `d²W/dpe²`; verify AC-5.
   - Phase D: Inner Newton solver for shift vector η (`newton_inner.f90`, `Hyper_pot_inner_alg.f90`); verify AC-6.
   - Phase E: Element-level energy/force computation (`ener_elem.f90`) with constitutive test fixtures.
   - Phase F: Principal curvature extraction (`principal.f90`).

5. **Milestone 4 — FEM Solver Core**
   - Phase A: Global energy and force assembly with MPI element partitioning (`energy.f90`, `pre_ener.f90`).
   - Phase B: L-BFGS minimizer translated from `lbfgs.f` + Wolfe line search; verify against Fortran convergence trajectory.
   - Phase C: Loading controller for all nCodeLoad modes in v1 scope (nCodeLoad=1) (`load.f90` simulator side).
   - Phase D: Pasapas load-stepping loop (`pasapas.f90`).
   - Phase E: Reaction force and torque computation (`get_reac.f90`).
   - Phase F: End-to-end serial run vs oracle; verify AC-7.

6. **Milestone 5 — VTU Output**
   - Phase A: Translate `paraview_vtu_output.f90` to C++.
   - Phase B: Verify VTU files load correctly in ParaView; verify AC-12.

7. **Milestone 6 — Van der Waals and Self-Contact**
   - Phase A: vdW interaction kernel (`vdw_modules.f90`) — Lennard-Jones, neighbor list, spatial binning.
   - Phase B: Self-contact detection with topological exclusion (`min_topo_dist`).
   - Phase C: Integration into global energy assembly.
   - Phase D: vdW oracle tests against archived Fortran; verify AC-8.

8. **Milestone 7 — Cyclic Loading, Crease Memory, and Checkpoint**
   - Phase A: Cyclic BC controller (nCodeLoad=30/31) — phase tracking, sign flip, L-BFGS history reset.
   - Phase B: Irreversible crease memory (`crease.f90`) — `init_creases`, `update_creases`, `write_crease_stats`.
   - Phase C: Crease detection and facet analysis (`crease_analysis.f90`).
   - Phase D: Checkpoint/restart — `write_checkpoint` / `read_checkpoint` for `x0`, `eta`, `K0_ref`.
   - Phase E: Oracle tests for cyclic case; verify AC-9, AC-10.

9. **Milestone 8 — MPI Parallelization Verification and Documentation**
   - Phase A: Verify multi-rank consistency (np=1, np=2, np=4); verify AC-11.
   - Phase B: Checkpoint compatibility across rank counts.
   - Phase C: AGENT.md and `document/translation_notes.md` finalized; verify AC-13.
   - Phase D: Full end-to-end integration test suite covering all AC.

## Task Breakdown

| Task ID | Description | Target AC | Tag | Depends On |
|---------|-------------|-----------|-----|------------|
| task0a | Build Fortran oracle from commit 7d3f77f; run graphene compression case; archive nano_*.dat | AC-1 | coding | — |
| task0b | Run Fortran cyclic crumple case; archive outputs | AC-1 | coding | task0a |
| task0c | Write fortran_conventions.md: indexing, units, file formats, sign conventions, active file list | AC-1 | analyze | task0a |
| task1a | CMake scaffold: C++17, OpenMPI, Eigen3, GoogleTest | AC-13 | coding | task0c |
| task1b | Core C++ data types mirroring Fortran headers.f90 derived types | AC-2, AC-7 | coding | task1a |
| task1c | nano_*.dat readers/writers with round-trip unit tests | AC-2 | coding | task1b |
| task1d | MPI init/finalize wrapper and rank utilities | AC-11 | coding | task1a |
| task1e | Field-by-field comparison script for oracle validation | AC-2, AC-7 | coding | task1c |
| task2a | Mesh generation (mesh_gen_square): grid, connectivity, BC tags | AC-2 | coding | task1b |
| task2b | Ghost node generation and connectivity tables | AC-4 | coding | task2a |
| task2c | B-spline basis N_i and derivatives with partition-of-unity tests | AC-3 | coding | task1a |
| task2d | Gauss quadrature setup | AC-2 | coding | task1a |
| task2e | Initial deformation gradient F0 and Jacobian J0 | AC-2 | coding | task2a, task2d |
| task2f | BC/load setup for preprocessor (including cyclic parameters) | AC-2 | coding | task2a |
| task2g | vdW preprocessing (neighbor list, shape functions) | AC-8 | coding | task2a, task2d |
| task2h | Full preprocessor integration and oracle comparison | AC-2 | coding | task2a, task2b, task2c, task2d, task2e, task2f, task2g |
| task3a | Exponential map for Cauchy-Born rule | AC-6, AC-7 | coding | task1b |
| task3b | Deformation gradient decomposition, bond vector computation | AC-5, AC-6 | coding | task3a |
| task3c | Brenner REBO potential W, dW/dpe, d²W/dpe² with oracle tests | AC-5 | coding | task3b |
| task3d | Inner Newton solver for η with element-level oracle tests | AC-6 | coding | task3c |
| task3e | Element-level energy/force kernel (ener_elem) | AC-7 | coding | task3d |
| task3f | Principal curvature extraction | AC-7, AC-9 | coding | task3e |
| task4a | Global energy/force assembly with MPI element partitioning | AC-7, AC-11 | coding | task3e, task1d |
| task4b | L-BFGS minimizer translated from lbfgs.f with convergence tests | AC-7 | coding | task4a |
| task4c | Loading controller for nCodeLoad=1 | AC-7 | coding | task1b |
| task4d | Pasapas load-stepping loop | AC-7 | coding | task4b, task4c |
| task4e | Reaction force and torque computation | AC-7 | coding | task4d |
| task4f | End-to-end serial run vs oracle (AC-7 full verification) | AC-7 | coding | task4d, task4e |
| task5a | VTU/ParaView output translation from paraview_vtu_output.f90 | AC-12 | coding | task4f |
| task5b | VTU format validation and ParaView load test | AC-12 | coding | task5a |
| task6a | vdW interaction kernel (LJ potential, spatial binning) | AC-8 | coding | task4f |
| task6b | Self-contact detection with topological exclusion | AC-8 | coding | task6a |
| task6c | vdW integration into global energy assembly + oracle tests | AC-8 | coding | task6b |
| task7a | Cyclic BC controller (nCodeLoad=30/31), phase tracking, LBFGS reset | AC-9 | coding | task4f |
| task7b | Irreversible crease memory: init, update, write_crease_stats | AC-9 | coding | task3f, task7a |
| task7c | Crease detection and facet analysis (crease_analysis) | AC-9 | coding | task7b |
| task7d | Checkpoint/restart: write_checkpoint, read_checkpoint for x0, eta, K0_ref | AC-10 | coding | task7a, task7b |
| task7e | Oracle tests for cyclic case and checkpoint restart | AC-9, AC-10 | coding | task7c, task7d |
| task8a | Multi-rank consistency tests np=1, np=2, np=4 | AC-11 | coding | task7e |
| task8b | Checkpoint compatibility across rank counts | AC-10, AC-11 | coding | task8a |
| task8c | AGENT.md creation and document/translation_notes.md finalization | AC-13 | coding | task8a |
| task8d | Full integration test suite covering all AC; final oracle comparison | AC-1 through AC-13 | analyze | task8a, task8b, task8c |

## Claude-Codex Deliberation

### Agreements
- Separating preprocessor and simulator as executables over a shared core library is the right architecture.
- Doing serial verification first before expanding to full MPI is the correct risk reduction order.
- Treating the Fortran oracle as the primary correctness standard, not analytic solutions or finite differences alone.
- Milestone 0 (Fortran archaeology and golden output capture) must precede all C++ design decisions.
- The L-BFGS implementation must match the Fortran `lbfgs.f` algorithm exactly (same two-loop recursion, same stopping criterion, same Wolfe line search).
- Acceptance criteria must be field-specific with type-appropriate tolerances (exact for discrete/topology, floating-point absolute for deterministic preprocessing, relative with absolute floor for end-to-end simulation).
- Cyclic crumpling, self-contact, crease memory, and checkpoint features are confirmed in scope (user decision DEC-2).
- MPI is in scope from the start (user decision DEC-5, resolved as "MPI from the start").
- Output format scope: `nano_*.dat` + VTU/ParaView only; Ensight, NetCDF, gmsh are out of scope.
- Object-oriented, human-readable code design is required; functions must be kept short and focused.

### Resolved Disagreements
- **Scope of Fortran oracle**: First Codex flagged that "current working tree" is not a stable oracle. **Resolution**: Fortran oracle is frozen at commit `7d3f77f` of `finite_crystal_elasticity/`. All acceptance tests reference outputs from that exact commit. Active files include all `.f90` and `.f` files (including `lbfgs.f`), excluding backup variants.
- **Verification strategy per milestone**: Second Codex flagged that "unit tests only" is too narrow. **Resolution**: Each milestone includes the appropriate tier of tests — unit tests for kernels, element-level oracle tests for constitutive model, golden-output comparison for preprocessor, and end-to-end oracle runs for the simulator.
- **Acceptance tolerance for near-zero quantities**: Second Codex flagged that pure relative tolerances break near zero. **Resolution**: All floating-point ACs use mixed tolerances: absolute floor (1×10⁻¹² for geometry, 1×10⁻¹² eV for energy) combined with relative tolerance, so neither dominates near zero.
- **Candidate plan Milestone 0 absence**: First Codex required an explicit archaeology milestone. **Resolution**: Milestone 0 added as the first and mandatory prerequisite for all subsequent work.

### Convergence Status
- Final Status: `converged`
- All user decisions (DEC-1 through DEC-7) resolved. Code style and OOP requirement added from plan annotation. No open items remain.

## Pending User Decisions

- **DEC-1**: Translation scope
  - Claude Position: Graphene only (grapheneCompressionOriginPrePro/ + grapheneCompressionOriginVersion/)
  - Codex Position: N/A — open question
  - Tradeoff Summary: Graphene-only narrows initial risk; all-four would require separate nanotube architecture.
  - Decision Status: **RESOLVED — Graphene only**

- **DEC-2**: Extensions in scope
  - Claude Position: Core only first (lower risk), defer extensions
  - Codex Position: Extensions deferred until scope confirmed by user
  - Tradeoff Summary: All extensions increase implementation complexity; deferral allows faster first delivery.
  - Decision Status: **RESOLVED — All extensions in scope** (vdW, nCodeLoad=30/31, self-contact, crease memory, checkpoint)

- **DEC-3**: Parity definition
  - Claude Position: Numeric parity (~1e-4 relative) is the right engineering standard for FEM/MPI code
  - Codex Position: N/A — open question; flagged that byte-identical is unrealistic across compilers
  - Tradeoff Summary: Byte-identical is impossible for MPI floating-point; numeric parity is correct standard.
  - Decision Status: **RESOLVED — Numeric parity** (~1e-4 relative for simulation, 1e-10 absolute for deterministic preprocessing)

- **DEC-4**: C++ toolchain
  - Claude Position: CMake + C++17 + Eigen3 + GoogleTest
  - Codex Position: N/A — open question
  - Tradeoff Summary: C++17 is mature and well-supported; C++20 offers more features but less compiler stability.
  - Decision Status: **RESOLVED — CMake + C++17 + Eigen3 + GoogleTest**

- **DEC-5**: MPI scope
  - Claude Position: Serial first (lower risk), MPI as second phase
  - Codex Position: MPI is a real risk if deferred; parallel structure should be planned early
  - Tradeoff Summary: Serial-first reduces debugging scope but may require architectural refactoring for MPI later.
  - Decision Status: **RESOLVED — MPI from the start**

- **DEC-6**: Required output formats
  - Claude Position: nano_*.dat + VTU/ParaView are the core outputs
  - Codex Position: N/A — open question; flagged that output format scope was underspecified
  - Tradeoff Summary: Fewer output formats reduces scope; Ensight/NetCDF/gmsh can be added later if needed.
  - Decision Status: **RESOLVED — nano_*.dat + VTU/ParaView only**

- **DEC-7**: Fortran oracle baseline
  - Claude Position: Freeze at current HEAD commit for stable oracle
  - Codex Position: Moving working tree is not a stable oracle (required change)
  - Tradeoff Summary: Frozen commit guarantees reproducibility; working tree risks silent oracle drift.
  - Decision Status: **RESOLVED — Freeze at commit `7d3f77f` of `finite_crystal_elasticity/`**

## Implementation Notes

### Code Style Requirements
- Implementation code and comments must NOT contain plan-specific terminology such as "AC-", "Milestone", "Step", "Phase", or similar workflow markers.
- These terms are for plan documentation only, not for the resulting codebase.
- Use descriptive, domain-appropriate naming in code instead (e.g., `computeElementEnergy`, `updateShiftVector`, `applyLoadIncrement`).
- **Object-oriented design is required**: encapsulate related data and algorithms into classes with clear single responsibilities (e.g., `MeshGenerator`, `BSplineKernel`, `BrennerPotential`, `LbfgsSolver`, `NanoDatWriter`). Do not replicate Fortran's flat module structure in C++.
- **Keep functions short and focused**: each function should do one thing. Aim for functions no longer than approximately 50 lines; extract named helper methods for complex sub-operations rather than writing long procedural blocks.
- **Prioritize human-readable code**: use clear naming, descriptive intermediate variables, and explicit computation steps. Avoid condensed or micro-optimized monolithic blocks that obscure the algorithm.

### Scientific Computing Requirements
- Strictly follow mathematics/physics theory from the reference papers (`reference/cauchy_born_derivation.md` and the PDF references).
- Do not use heuristics to bypass computational steps. Every algorithm step must trace to either the Fortran source or the reference derivation.
- For any numerical discrepancy found between C++ and Fortran, investigate root cause before accepting a workaround.
- Each implementation step must include a numerical verification method (unit test, oracle comparison, or finite-difference check) before proceeding to the next step.
- Before implementing any component, check `document/translation_notes.md` and `document/fortran_conventions.md` for existing relevant experience.

### Convention Documentation Requirements
- Document all indexing conventions (0-based C++ vs 1-based Fortran) in `document/fortran_conventions.md`.
- Document unit system: lengths in nm, energies in eV, forces in eV/nm.
- Document field ordering in each `nano_*.dat` file to ensure bit-for-bit-compatible reading.
- Update `AGENT.md` whenever the C++ project folder structure changes.

---

## Output File Convention

This plan is the main output file at `document/plan.md`.

When `alternative_plan_language` resolves to a supported language, a translated variant is written. No translation variant is configured for this project (`alternative_plan_language` is empty).

---

--- Original Design Draft Start ---

I need to translate the graphene codebase in `../finite_crystal_elasticity/` (including graphene simulator and prepro code) to a C++ version with exactly the same functionalities.

# Rule to follow

1. You are doing a **Scientific Computing Project**. Any logical, algorithmic, or numerical error will possibly cause fatal errors. Therefore, you need to strictly follow the mathematics/physics theory in the given references.

2. Carefully record the implementation of each step in documentation files (`./document`), especially experience learned from error/bugs. Update `./AGENT.md` and other related file if the structure of the project folder is changed. Generally, accumulate any useful experience into documentations for future re-use.

3. Before doing anything, check documentation to see if there is any existing experience in documentation.

4. Strictly refer the description in references as much as you can. Do not use heuristics ways to bypass computational steps.

5. For each implementation step, find way to numerically verify the correctness of implemented modules.

--- Original Design Draft End ---
