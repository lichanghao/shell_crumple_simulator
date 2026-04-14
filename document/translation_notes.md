# Translation Notes

This document records the current C++ translation status against the canonical Fortran reference at commit `7d3f77f`. It is a status note, not a completion claim.

## Reference baseline

- Oracle repository: `../finite_crystal_elasticity/`
- Frozen commit: `7d3f77f`
- Primary archived cases live under `test/cases/`
- Detailed file-format and convention notes live in `document/fortran_conventions.md`

## High-level status

| Area | Status | Notes |
|------|--------|-------|
| Oracle capture and fixture archiving | translated and verified | Compression, cyclic, kernel, and preprocessor oracle fixtures are committed under `test/cases/`. |
| CMake/project scaffold | translated and verified | `CMakeLists.txt` builds `fce_core`, `PrePro`, `crunch_it`, `unit_tests`, and `integration_tests`. |
| Core data types and `nano_*.dat` I/O | translated and verified | Public types live in `include/fce/types.hpp`; Fortran-format readers/writers live in `src/core/io.cpp`. |
| Preprocessor mesh/ghost/B-spline/reference config path | translated and verified | Compression and cyclic preprocessor oracles are exercised by integration tests. |
| Preprocessor-side `nvdw=1` preparation | translated and verified | Single-sheet self-contact and bilayer-twist local-density cases are archived and tested. |
| Constitutive kernels and archived element-state coverage | translated and partially verified | Brenner, principal, and exponential fixtures are present, but the archived `ElementState` / `ElementEnergy` kernel gates are still red. |
| Runtime assembly path | translated and partially verified | Archived compression VTU snapshots reproduce assembly energy, but the live executable path still diverges later in the solve. |
| Runtime VTU/PVD output | translated and partially verified | XML parseability and archived compression field checks are covered; executable-path real `nvdw=1` oracle coverage is still missing. |
| Runtime vdW/self-contact assembly | not yet translated | Preprocessor-side support exists, but simulator-side `vdw_modules.f90` behavior is still pending. |
| Cyclic controller, crease memory, checkpoint/restart | not yet translated end to end | AC-9 and AC-10 remain open. |
| MPI parity verification | partial | MPI wrapper and element partitioning exist, but `np=1/2/4` acceptance coverage is still pending. |

## Executable and library mapping

| Fortran responsibility | C++ location | Status |
|------------------------|--------------|--------|
| Preprocessor main | `src/prepro/main.cpp`, `src/core/preprocessor.cpp` | translated |
| Simulator main | `src/simulator/main.cpp`, `src/core/solver.cpp`, `src/core/simulator.cpp` | partial |
| Mesh generation | `src/core/mesh_generator.cpp` | translated |
| Ghost-node handling | `src/core/ghost_nodes.cpp` | translated |
| B-spline basis | `src/core/bspline.cpp` | translated |
| Reference deformation gradient / Jacobian | `src/core/reference_config.cpp` | translated |
| Boundary-condition setup | `src/core/load_pre.cpp`, `src/core/load_controller.cpp` | partial |
| Brenner and inner constitutive logic | `src/core/constitutive.cpp`, `src/core/element_state.cpp`, `src/core/element_energy.cpp` | partial, archived kernel parity still open |
| Principal/exponential helpers | `src/core/principal.cpp`, `src/core/exponential.cpp`, `src/core/taylor.cpp` | translated |
| L-BFGS runtime minimization | `src/core/lbfgs.cpp` | partial, still under archived runtime parity work |
| Runtime output | `src/core/runtime_output.cpp` | partial |
| vdW preprocessing | `src/core/vdw_preprocessor.cpp` | translated for preprocessor scope |
| MPI wrapper | `src/core/mpi_env.cpp` | translated |

## Oracle-backed evidence currently in tree

- Preprocessor compression oracle: `test/cases/graphene_compression_prepro/`
- Simulator compression oracle: `test/cases/graphene_compression_simulator/np1/`
- Cyclic oracle: `test/cases/graphene_cyclic_crumple/`
- `nvdw=1` preprocessor oracles: `test/cases/graphene_self_contact/prepro_run/` and `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/`
- B-spline oracle fixtures: `test/cases/bspline_oracle/`
- Constitutive and archived simulator-state fixtures: `test/cases/constitutive_oracle/`
- Element-energy oracle fixtures: `test/cases/element_energy_oracle/`

## Known open gaps

### AC-7 runtime parity

The main executable-path regression is still red. The archived compression harness is deterministic now, but the live C++ solver path still diverges from the archived Fortran trajectory in energy, reaction force, and `nano_final_config.dat`.

Recent repository-grounded debugging narrowed that mismatch further:

- A same-trace Fortran probe showed that the archived `mesh_config_0000.vtu` is not the post-`minimize_free` state. In the canonical runtime, `Optim.f90` writes `mesh_config_0000.vtu` before `pasapas()` runs, so matching that visible step-0 artifact does not prove the hidden state entering load step 1 is correct.
- The committed executable-path regression now reads the archived `energy.dat` and `force.dat` directly instead of duplicating stale hard-coded step-1 literals. Round 4 reconciled the committed first-step exact-state fixtures to one authoritative same-trace Fortran replay source, so the remaining AC-7 problem is no longer oracle integrity for that element-level gate; it is the still-red analytical kernel and the still-red executable-path step-one energy/reaction output.
- The hidden post-`minimize_free` handoff is no longer the primary blocker. The executable path now matches the committed canonical `post_minimize_free_coords.dat` fixture within about `1.06e-7` max absolute error, the dumped post-free `eta` field matches exactly under the traced replay, and the traced step-1 states after the boundary increment and imperfection injection also agree within about `1.06e-7` max absolute error.
- The remaining divergence shows up on the very first constrained-step energy evaluation, before the outer L-BFGS trajectory meaningfully branches. On the reconciled authoritative same-trace Fortran replay for reconstructed element 83, the replay-only gate is green again, but the standalone exact-state unit gate is still red on prepared-bond `pe` and downstream `eta`, `W`, `W_elem`, and `f_elem`.
- That means the live AC-7 blocker is now inside the analytical element-level inner-relaxation path (`compute_element_state`, `Hyper_pot_inner`, `solve_inner_newton`) on the first constrained-step state, not in replay wiring, the frozen imperfection trace contract, or the outer L-BFGS control flow alone.
- Round 1 of the 2026-04-13 RLCR continuation also re-confirmed that the broader archived kernel-oracle surface is still red in this workspace: `ElementState.MatchesArchivedCompressionSimulatorOracleFixtures`, `ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures`, and `ElementEnergy.FElemMatchesFortranOracle` all fail before the executable-path step-1 replay branches. A small Fortran-alignment cleanup landed in `compute_element_energy` so the analytical stress path now always recomputes `Hyper_Pot` on the final relaxed bond state, matching `ener_elem.f90`, but that did not resolve the deeper geometry/inner-relaxation mismatch.

### AC-8 runtime vdW

Preprocessor-side `nvdw=1` translation is present, but simulator-side vdW/self-contact assembly is still absent. That blocks both runtime physics parity and the remaining real-`nvdw=1` VTU evidence.

### AC-12 executable-path real `nvdw=1` VTU oracle

The repository now validates generated `.vtu` and `.pvd` files with a parser-backed XML check (`test/support/validate_vtk_xml.py`) and compares archived compression runtime snapshots, but it still lacks an executable-path real `nvdw=1` runtime oracle series with nonzero `atomic_density` and `W_density`.

### AC-9, AC-10, and AC-11

Cyclic `nCodeLoad=30/31`, irreversible crease memory, checkpoint/restart, and multi-rank parity remain milestone-level open work.

## Porting rules that have repeatedly mattered

- Read the exact canonical Fortran loop before deciding record counts or array ownership.
- Treat all on-disk indices as 1-based unless the file format explicitly says otherwise.
- Prefer archived Fortran artifacts over regenerated C++ artifacts when proving parity.
- Distinguish preprocessor-side `nvdw=1` progress from runtime vdW/self-contact progress; they are not interchangeable.
- Treat the committed `imperfection_trace_fortran.dat` fixture as a frozen record of the Fortran `pasapas.f90` imperfection slot, not as proof that executable-path AC-7 parity is already solved.

## Recommended next steps

1. Resume the archived compression executable-path mismatch investigation in `src/core/solver.cpp`, `src/core/load_controller.cpp`, `src/core/simulator.cpp`, and related runtime tests.
2. Translate simulator-side vdW/self-contact logic from `vdw_modules.f90` and connect it to a real executable-path `nvdw=1` oracle case.
3. Finish cyclic controller, crease memory, checkpoint/restart, and MPI acceptance coverage after the runtime mainline is stable.
