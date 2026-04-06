# Round 1 Contract

## Mainline Objective

Repair all Milestone 0 oracle defects identified in the Round 0 review, formally reconcile all control documents (plan.md, goal-tracker, README), then implement Milestone 1 (C++ project scaffold + nano_*.dat I/O layer).

## Target ACs

- **AC-1**: Fortran oracle baseline fully repaired — serial np=1 compression baseline archived, cyclic plan formally amended to nCodeLoad=31, build provenance documented, plan.md corrections committed.
- **AC-13** (partial): CMake scaffold, AGENT.md skeleton, data types, and nano_*.dat readers/writers in place.

## Blocking Side Issues In Scope

1. **Compression oracle is mislabeled np=8 run**: Re-run compression simulator with `mpirun -np 1` to get a true serial baseline. Keep the np=8 artifacts but relabel them.
2. **Cyclic oracle mismatches plan (nCodeLoad=30 vs 31)**: No nCodeLoad=30 test case exists in the Fortran oracle repository. Formally amend plan.md to use nCodeLoad=31 as the cyclic baseline. Document the rationale.
3. **Plan/tracker drift (nCodeLoad=1 vs 3, nloadstep=100 vs 50)**: Fix plan.md and commit. The earlier fix was reverted because it was uncommitted; fix it properly this time.
4. **Missing build provenance**: Archive compiler version, build command transcript, and commit proof for the oracle binaries. The backup-variant negative test cannot be fully reproduced without rebuilding, so document it structurally with evidence from the conventions doc.

## Queued Issues Out Of Scope

- Milestones 2–8 (mesh generation, constitutive kernels, FEM solver, VTU, vdW, cyclic physics, MPI verification) — begin only after Milestone 1 scaffold is complete.
- Minor: round-0-summary line count discrepancy (291 vs 292 lines in fortran_conventions.md).
- Minor: prepro.log mislabeled as "build log" in README.

## Round Success Criteria

1. `test/cases/graphene_compression_simulator/np1/` exists with serial run outputs; `simulator.log` shows `Numero de procesadores: 1`.
2. `document/plan.md` committed with AC-7 = nCodeLoad=3/nloadstep=50, AC-9 = nCodeLoad=31 as primary cyclic baseline, task4c updated.
3. `test/cases/README.md` correctly documents serial vs MPI runs and nCodeLoad=31 as the cyclic baseline.
4. `test/cases/graphene_compression_prepro/build_provenance.md` documents compiler version, commit hash proof, and build steps.
5. `CMakeLists.txt` builds cleanly with C++17, OpenMPI, Eigen3, GoogleTest targets configured.
6. `include/fce/` has data type headers; `src/` has nano_*.dat reader/writer stubs with D-exponent parsing.
7. `AGENT.md` exists with project structure and build instructions.
8. Unit tests for nano_*.dat readers pass against archived oracle files.
