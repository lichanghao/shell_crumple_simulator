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
| Constitutive kernels and archived element-state coverage | translated and verified | Brenner, inner Newton, principal, exponential, and archived element-state fixtures are present. |
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
| Brenner and inner constitutive logic | `src/core/constitutive.cpp`, `src/core/element_state.cpp`, `src/core/element_energy.cpp` | translated for the current oracle-backed scope |
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

- Under the frozen Fortran-backed imperfection trace, the step-1 outer-coordinate error is dominated by the physical `z` direction rather than the in-plane `x`/`y` coordinates.
- The generated step-1 VTU `inner_displacement` field already matches the archived oracle, so the remaining defect is in the outer-coordinate trajectory rather than in the averaged `eta` output path.
- Reflecting only the first imperfection-trace value around `0.5` is a false lead: the step-1 energy moves farther away from the archived oracle, not closer.
- A same-trace Fortran probe showed that the archived `mesh_config_0000.vtu` is not the post-`minimize_free` state. In the canonical runtime, `Optim.f90` writes `mesh_config_0000.vtu` before `pasapas()` runs, so matching that visible step-0 artifact does not prove the hidden state entering load step 1 is correct.
- Replaying the canonical Fortran step-1 entry on the original `nano_*.dat` inputs showed that the hidden pre-step-1 state is already non-flat before the constrained minimizer starts. Undoing the step-1 BC increment and uniform imperfection from that Fortran dump still leaves a free-state mismatch versus the current C++ post-free state of about `4.175e-02` in `x`, `5.234e-02` in `y`, and `1.047e-01` in `z`.
- That means the stable AC-7 blocker is no longer just “step-1 constrained solve differs.” The divergence is already present in the state handed from `minimize_free` into the first constrained load step, so future work should compare the post-free hidden state itself rather than using `mesh_config_0000.vtu` as a proxy.

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
