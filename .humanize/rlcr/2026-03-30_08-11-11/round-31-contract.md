# Round 31 Contract

## Mainline Objective

Implement the Milestone 4 runtime solver: task3f (repeated-curvature principal oracle), task4b (L-BFGS translator), task4c (nCodeLoad=3 load controller), task4d (pasapas load-stepping loop), task4e (reaction force), and task4f (end-to-end AC-7 verification) — producing a real 50-step solver executable that generates `energy.dat` and `force.dat` from `nano_*.dat` inputs and verifies them against the Fortran oracle.

## Target ACs

- **AC-7** (primary): End-to-end serial compression (np=1, nCodeLoad=3, nloadstep=50): energy within 1×10⁻⁴ relative, reaction force within 1×10⁻³ relative vs Fortran oracle.
- **AC-9** (prerequisite unlock): task3f repeated-curvature fixture unblocks the crease-memory path later.

## Blocking Issues

1. `task4b` missing — no L-BFGS solver; all solver tasks depend on it.
2. `task4c` missing — no nCodeLoad=3 load controller; pasapas cannot apply load increments.
3. `task4d` missing — no pasapas loop; no generated energy.dat or force.dat.
4. `task4e` missing — no reaction force; AC-7 force tolerance cannot be checked.
5. `task3f` missing repeated-curvature oracle — blocks AC-9 later, and task3f is still pending in tracker.

## Queued / Out of Scope This Round

- Milestone 5 (VTU writer, task5a/task5b) — AC-12
- Milestone 6 (runtime vdW/self-contact, task6a–task6c) — AC-8 runtime
- Milestone 7 (cyclic loading, crease memory, checkpoint) — AC-9 full, AC-10
- Milestone 8 (MPI verification, AGENT.md, translation_notes.md) — AC-11 full, AC-13

## Concrete Success Criteria

1. `task3f`: A Fortran-backed repeated-curvature (`flag_num_diff=true`) fixture committed under `test/cases/principal_oracle/`; `Principal.FlagNumDiffMatchesFortranOracle` test passes.
2. `task4b`: `include/fce/lbfgs.hpp` + `src/core/lbfgs.cpp` translate `lbfgs.f` two-loop recursion + Wolfe line search; unit test reduces a quadratic objective to machine precision.
3. `task4c`: Load controller applies exact per-step compression increment for nCodeLoad=3; stubs for 30/31 throw.
4. `task4d`: `pasapas` drives 50 load steps from `nano_*.dat`, writing `energy.dat` and `output.dat`.
5. `task4e`: Reaction force matches Fortran `get_reac.f90` convention; `force.dat` written with correct column ordering.
6. `task4f`: `./build/crunch_it <case_dir>` runs 50 steps from `nano_*.dat`; `SimulatorEndToEnd.EnergyTrajectoryMatchesFortranOracle` verifies all 50 energies within 1×10⁻⁴ relative and final reaction force within 1×10⁻³ relative.
7. All existing tests continue to pass (unit + integration suites fully green).
