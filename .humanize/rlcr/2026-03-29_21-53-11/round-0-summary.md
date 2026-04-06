# Round 0 Summary — Fortran Archaeology and Oracle Capture

## What Was Implemented

### task0a — Fortran oracle baseline, graphene compression case

The Fortran oracle at commit `7d3f77f` of `../finite_crystal_elasticity/` was confirmed built (gfortran 15.2.0 + OpenMPI). The preprocessor (`PrePro`) and simulator (`crunch_it`) reference outputs were archived for the standard graphene compression case:

- Config: 40×40 mesh, 20 nm×20 nm, nCodeLoad=3 (uniaxial compression), nloadstep=50, ngauss=2, no vdW
- `test/cases/graphene_compression_prepro/`: data.dat + 7 nano_*.dat files + meshini.msh + prepro.log
- `test/cases/graphene_compression_simulator/`: energy.dat (53 lines = header + 2 init + 50 steps), force.dat, nano_final_config.dat, output.dat, simulator.log

**Plan correction**: AC-7 and task4c originally referenced nCodeLoad=1 with nloadstep=100. Oracle archaeology revealed nCodeLoad=3 with nloadstep=50. Both were corrected in plan.md and goal-tracker.

### task0b — Fortran cyclic crumple case

The cyclic crumple oracle outputs were confirmed and archived:

- Config: 40×40 mesh, 20 nm×20 nm, nCodeLoad=31 (biaxial corner-loaded cyclic), 5 cycles, nloadstep_comp=20, nloadstep_rel=20, ncrease=1
- `test/cases/graphene_cyclic_crumple/prepro_run/`: data.dat + 8 nano_*.dat files (including nano_crease.dat) + prepro.log
- `test/cases/graphene_cyclic_crumple/simulator_run/`: energy.dat, force.dat, crease_map.dat, nano_final_config.dat, output.dat, nano_checkpoint.dat, simulator.log

### task0c — document/fortran_conventions.md

Written directly (Codex route failed due to zsh shell-escaping of Fortran D-exponent notation in ask-codex.sh). Covers:

1. 1-based Fortran → 0-based C++ for all indices
2. Unit system: nm / eV / eV·nm / 1/nm; Jacobians in nm²
3. D-exponent float parsing: replace `D`/`d` with `E`/`e` before strtod
4. All nano_*.dat formats with exact field ordering (nano_dims, nano_general, nano_zero, nano_config, nano_BCs, nano_Mesh, nano_tub_loc, nano_crease)
5. BC sign conventions (nCodeLoad=3: right edge compressed in −x)
6. MPI conventions (rank-0 I/O only, MPI_ALLREDUCE for global assembly)
7. nCodeLoad reference table (0,1,2,3,10,11,13,30,31,222,1000)
8. Active source file lists for prepro (11 files) and simulator (31 files); backup variants excluded
9. Key C++ implementation notes: D-exponent parsing, COMMON blocks → class members, ghost node encoding

## Files Changed

| File | Action |
|------|--------|
| `test/cases/graphene_compression_prepro/` | All oracle files archived (data.dat, 7 nano_*.dat, meshini.msh, prepro.log) |
| `test/cases/graphene_compression_simulator/` | All oracle files archived (energy.dat, force.dat, nano_final_config.dat, output.dat, simulator.log) |
| `test/cases/graphene_cyclic_crumple/prepro_run/` | All oracle files archived (data.dat, 8 nano_*.dat, prepro.log) |
| `test/cases/graphene_cyclic_crumple/simulator_run/` | All oracle files archived (energy.dat, force.dat, crease_map.dat, nano_final_config.dat, output.dat, nano_checkpoint.dat, simulator.log) |
| `test/cases/README.md` | Written: documents oracle commit, compiler, case inventory, nCodeLoad reference |
| `document/fortran_conventions.md` | Written: 292-line conventions reference for C++ translators |
| `document/plan.md` | Updated: AC-7 nCodeLoad=1→3, nloadstep=100→50; task4c description corrected |
| `.humanize/rlcr/2026-03-29_21-53-11/goal-tracker.md` | Updated: tasks 0a/0b/0c moved to Completed; plan evolution log updated |
| `.humanize/bitlesson.md` | Updated: 2 lessons added |

## Validation

| Check | Result |
|-------|--------|
| graphene_compression_prepro/ has all 7 nano_*.dat | PASS |
| graphene_compression_simulator/energy.dat has 53 lines (50 load steps) | PASS |
| graphene_cyclic_crumple/simulator_run/ has nano_checkpoint.dat and crease_map.dat | PASS |
| document/fortran_conventions.md covers all 9 required topics per round-0-contract | PASS |
| plan.md AC-7 now references nCodeLoad=3, nloadstep=50 | PASS |
| goal-tracker tasks 0a/0b/0c marked completed | PASS |

## Remaining Items

None for Round 0. Round 1 targets Milestone 1: CMake scaffold (task1a), C++ data types (task1b), nano_*.dat readers/writers (task1c), MPI wrapper (task1d), oracle comparison script (task1e).

## BitLesson Delta

Action: add
Lesson ID(s): BL-20260329-codex-shell-escape, BL-20260329-nCodeLoad-mismatch
Notes: Added two lessons from Round 0 failures: (1) ask-codex.sh fails in zsh with Fortran D-exponent and percent characters — use Write tool directly for complex analyze-tagged docs; (2) always verify loading parameters from actual oracle data.dat/nano_BCs.dat before writing ACs — plan had wrong nCodeLoad=1 and nloadstep=100 before archaeology confirmed nCodeLoad=3 and nloadstep=50.
