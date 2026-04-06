# Round 30 Contract

## Mainline Objective

Implement the Milestone 4 runtime solver (task4b through task4f): translate `lbfgs.f` into a C++ L-BFGS solver class, add the `nCodeLoad=3` load controller, implement the `pasapas` load-stepping loop, compute reaction forces, and produce a real 50-step simulator executable that generates `energy.dat`, `force.dat`, and `nano_final_config.dat` from `nano_*.dat` inputs — achieving end-to-end AC-7 verification.

Finish `task3f` (repeated-curvature `flag_num_diff=true` principal Fortran oracle fixture) as a prerequisite to unblock AC-9 later.

## Target ACs

- **AC-7** (primary): End-to-end serial compression (np=1, nCodeLoad=3, nloadstep=50) energy within 1×10⁻⁴ relative and reaction force within 1×10⁻³ relative vs Fortran oracle.
- **AC-7 / AC-9** (secondary): Close `task3f` by committing a Fortran-backed repeated-curvature `flag_num_diff=true` principal fixture.

## Blocking Issues

1. **No runtime solver** — `src/simulator/main.cpp` currently reads archived VTU files and assembles one static state; it must be replaced by a real load-stepping loop. Without this, AC-7 cannot be met.
2. **No L-BFGS** — `task4b` (translated `lbfgs.f`) is the core optimizer; all downstream solver tasks depend on it.
3. **No load controller** — `task4c` must implement the `nCodeLoad=3` compression BC increment logic before `pasapas` can step loads.
4. **No `pasapas` loop** — `task4d` drives the outer load-step iteration; without it, there is no generated `energy.dat` or `force.dat`.
5. **No reaction-force computation** — `task4e` must be present before AC-7 force tolerances can be checked.
6. **`task3f` missing repeated-curvature oracle** — every committed principal fixture is `flag_num_diff=false`; the numerical-fallback branch has no Fortran-backed evidence, which blocks AC-9.

## Queued / Out of Scope This Round

- Milestone 5 (VTU writer, task5a/task5b) — AC-12
- Milestone 6 (runtime vdW/self-contact, task6a–task6c) — AC-8 runtime
- Milestone 7 (cyclic loading, crease memory, checkpoint, task7a–task7e) — AC-9, AC-10
- Milestone 8 (MPI consistency, documentation, task8a–task8d) — AC-11 full, AC-13
- `AGENT.md` and `document/translation_notes.md` creation (task8c)

## Concrete Success Criteria

1. `task3f`: A committed Fortran-backed fixture for the repeated-curvature (`flag_num_diff=true`) principal branch exists under `test/cases/`, and a new `Principal.FlagNumDiffMatchesFortranOracle` test passes.
2. `task4b`: A `LbfgsSolver` class exists in `include/fce/lbfgs.hpp` + `src/core/lbfgs.cpp`, translating `lbfgs.f` two-loop recursion + Wolfe line search, with at least one convergence test validating the solver reduces a quadratic objective.
3. `task4c`: A load controller class (or function set) applies `nCodeLoad=3` uniaxial compression BC increments; stubs for `nCodeLoad=30/31` are present.
4. `task4d`: A `pasapas` load-stepping function drives N load steps, calling L-BFGS at each step and writing per-step energy/displacement.
5. `task4e`: Reaction force and torque computation matches Fortran `get_reac.f90` logic.
6. `task4f`: `./build/crunch_it <case_dir> 50` produces `energy.dat` and `force.dat`; an integration test `SimulatorEndToEnd.EnergyTrajectoryMatchesFortranOracle` verifies all 50 step energies within relative 1×10⁻⁴ and the final reaction force within relative 1×10⁻³ against the archived Fortran oracle.
7. All existing tests continue to pass (unit + integration suites fully green).
