# Round 0 Contract

## Mainline Objective
Acknowledge completed Milestones 0 and 1, initialize the goal tracker for the new loop, and begin Milestone 2 (mesh generation, ghost nodes, B-spline basis, Gauss quadrature, deformation gradient F0/J0, BC/load setup for preprocessor).

## Target ACs
- AC-2 (C++ preprocessor generates nano_*.dat matching oracle)
- AC-3 (B-spline basis correct)
- AC-4 (Ghost node positions correct)

## Blocking Side Issues In Scope
None.

## Context from Prior Work
Milestones 0 and 1 are complete and committed (commit 268383b):
- Oracle baseline archived (test/cases/), build provenance documented
- CMakeLists.txt scaffold with 21/21 passing tests
- All data types (include/fce/types.hpp)
- All nano_*.dat readers/writers (src/core/io.cpp) with round-trip tests
- MPI wrapper (src/core/mpi_env.cpp)

## Success Criteria for Round 0
1. Goal tracker initialized and reflects all prior completed work
2. Round contract written
3. Milestone 2 tasks (task2a through task2h) implemented with tests
4. All existing 21 tests continue to pass

## Notes
This is a fresh loop start after the previous loop was cancelled due to the stop hook detecting plan.md modifications. All prior committed work is preserved. The new loop baseline is the corrected plan.md (nCodeLoad=3, nloadstep=50, nCodeLoad=31 cyclic baseline).
