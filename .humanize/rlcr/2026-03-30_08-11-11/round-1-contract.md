# Round 1 Contract

## Mainline Objective
Deliver a runnable C++ preprocessor that reads the archived graphene `data.dat`, writes the required `nano_*.dat` outputs, and is verified against the preprocessor oracle.

## Target ACs
AC-2: C++ preprocessor correctly generates `nano_*.dat` files matching the Fortran oracle.
AC-3: B-spline basis functions and derivatives are correct for the translated 12-node patch configurations.

## Blocking Issues In Scope
- Ghost-node generation and connectivity must be correct enough to support oracle-equivalent mesh output (AC-4 dependency for AC-2).
- `task1e` field-by-field comparison helper must exist so the preprocessor oracle test can fail and pass for the right reasons.
- The build/test graph must include the new preprocessor modules instead of leaving `PrePro` as a stub.

## Queued And Explicitly Out Of Scope
- Simulator/constitutive milestones (AC-5 through AC-12).
- Full vdW force kernel and cyclic/checkpoint paths beyond whatever preprocessor file stubs are required for the archived compression case.
- `AGENT.md` and `document/translation_notes.md`; still required by the overall plan, but not blocking this preprocessor round.

## Success Criteria
1. `src/prepro/main.cpp` invokes a real preprocessor pipeline on an input working directory and writes the archived compression-case `nano_*.dat` file set.
2. Oracle comparison coverage exists for the preprocessor path, with `task1e` implemented as a reusable helper and an integration test that fails on mismatches.
3. Unit coverage exists for translated B-spline and ghost/reference-config behavior where those kernels are required to trust the preprocessor output.
4. The new preprocessor sources are compiled by CMake, and targeted tests for the new functionality pass.
