# Round 32 Contract

## Mainline Objective
Restore honest AC-7 runtime parity by fixing the live solver-state evolution and executable-path oracle contract for the archived `graphene_compression_simulator/np1` case.

## Target ACs
- AC-7: End-to-end serial compression runtime matches the archived Fortran oracle on the real executable path.

## Blocking Issues
- `task4d`: the runtime solver still reuses `input.initial_config.eta` instead of carrying forward the converged per-element `eta` state.
- `task4f`: the executable contract is still wrong for `crunch_it <case_dir> 50`, required runtime artifacts are incomplete, and the current AC-7 test uses a self-oracle instead of the archived Fortran oracle.

## Queued / Out Of Scope
- `task4e` torque parity beyond the force-summing slice, unless a minimal change is strictly required to keep the runtime path coherent.
- Milestone 5 onward (`task5a`-`task8d`), including VTU output, runtime vdW, cyclic loading, checkpointing, MPI verification, and documentation.
- The queued cyclic repeated-curvature provenance refinement noted in the Round 31 review.

## Success Criteria
- A failing regression test first demonstrates at least one reopened AC-7 contract gap on the real runtime path.
- The solver owns and updates mutable live `eta` state across assembly/minimization/load steps instead of reading the immutable initial state.
- `crunch_it <case_dir> 50` executes the pasapas solver path, regenerates fresh runtime outputs, and writes the required AC-7 artifacts needed for oracle comparison.
- The AC-7 integration coverage compares fresh runtime outputs against the archived Fortran `np1` oracle in a writable temporary directory without relying on `np1_cpp_oracle`.
- Goal tracker mutable sections and round summary reflect the actual reopened/closed state with no overclaims.
