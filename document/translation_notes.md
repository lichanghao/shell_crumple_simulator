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
| Constitutive kernels and archived element-state coverage | translated and verified | Brenner, principal, exponential, archived constitutive fixtures, archived element-energy fixtures, and the first-step exact-state gates are all green after refreshing the stale flat/Brenner element-energy oracle files from the source Fortran generators. |
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
| Brenner and inner constitutive logic | `src/core/constitutive.cpp`, `src/core/element_state.cpp`, `src/core/element_energy.cpp` | translated for the current archived-oracle-backed scope |
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
- For the archived compression replay lane, the hidden post-`minimize_free` handoff is no longer the primary blocker. The executable path matches the committed canonical `post_minimize_free_coords.dat` fixture within about `1.06e-7` max absolute error, the dumped post-free `eta` field matches exactly under the traced replay, and the traced step-1 states after the boundary increment and imperfection injection also agree within about `1.06e-7` max absolute error.
- The remaining divergence is no longer dominated by the first-step exact-state oracle surface. The reconstructed element-83 replay-only and standalone exact-state gates are green, and the refreshed flat/Brenner `ElementEnergy` oracle fixtures are green again in the current `unit_tests` binary. The remaining AC-7 blocker is still centered on the executable-path step-one energy / `GNORM` / reaction-force output.
- That means the live AC-7 blocker is now on the executable path (`pasapas` / constrained minimization / reaction output), not on the first-step exact-state kernel oracle surface.
- Fresh 2026-04-15 reconciliation work showed that the committed deterministic replay fixture `imperfection_trace_fortran.dat` and the frozen archived `np1/` outputs are not the same oracle contract. A same-trace O3 Fortran replay matches the current C++ constrained-step entry and early monitor sequence, but both differ from the archived `np1/simulator.log` from the very first step-one constrained evaluation. The step-one replay tests therefore use replay-specific oracle fixtures (`replay_step1_monitor.dat`, `replay_step1_eval_sequence.dat`, `replay_step1_energy.dat`, `replay_step1_force.dat`) instead of comparing trace-driven runs directly to the archived `np1/` rows.
- Additional archive reconciliation showed that the frozen `np1/` artifacts are internally mixed as well: the archived step-one `energy.dat`, `force.dat`, and `mesh_config_0001.vtu` agree with each other, but the archived `np1/simulator.log` step-one equilibrium line reports a different terminal energy. Until a source-backed explanation appears, archive-backed numerical parity should treat `energy.dat` / `force.dat` / VTU snapshots as the step-one numeric oracle and treat `simulator.log` as historical context only.
- The cyclic `nCodeLoad=31` runtime now has the same kind of replay-vs-archive split. Committed cyclic step-one replay fixtures (`test/cases/graphene_cyclic_crumple/replay_step1_*.dat`) provide a deterministic source-backed contract. The committed cyclic post-free oracle `E2ECyclicRuntime.CrunchItPostMinimizeFreeStateMatchesCommittedCyclicOracle` is now green, so the remaining cyclic blocker has moved downstream into the first constrained replay row `E2ECyclicRuntime.CrunchItReplaysCommittedCyclicStepOneTraceDeterministically`, especially `GNORM` and reaction output.
- A new generated-state consistency check `E2ECyclicRuntime.GeneratedStepOneVtuMatchesGeneratedEnergyAndReactionRows` is green. It validates the cyclic step-one VTU against `energy.dat` and independently reconstructs the reaction columns from the Fortran `get_reac(..., 3, ...)` rule used by the cyclic `pasapas` path, so the remaining red surface is narrower than a generic emitted-file inconsistency. It still does not by itself prove replay-oracle parity.
- Step-one trace instrumentation is now available on the cyclic runtime path behind `FCE_TRACE_COORD_DUMPS`. The runtime writes `before_first_eval` and `before_output` coordinate/eta/summary/reaction artifacts, and the new cyclic trace regression confirmed an important state transition: immediately before the first constrained assembly, free DOFs still carry the imperfection, but BC DOFs have already been restored from `x0_BC` to their post-increment values.
- Committed same-trace Fortran replay fixtures now exist for the cyclic `before_first_eval` and `before_output` checkpoints. The new fixture-backed tests show that the current C++ path already matches the Fortran replay at `before_first_eval` (coords, eta, and summary within tight tolerance), while `before_output_eta` still matches exactly but `before_output` coordinates and summary have already diverged materially. The current cyclic replay mismatch therefore starts during the constrained solve/update path after the first constrained evaluation, not before it.

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
