# Round 0 Contract

## Mainline Objective
Complete Milestone 0 (Fortran Archaeology and Oracle Capture): build the Fortran oracle from commit `7d3f77f`, run the graphene compression and cyclic crumple reference cases, archive all outputs, and write `document/fortran_conventions.md`.

## Target ACs
- **AC-1**: Fortran oracle baseline built, run, and captured with all reference outputs stored and data conventions documented.

## Blocking Side Issues In Scope
None identified at round start.

## Queued Side Issues Out Of Scope
- All Milestones 1–8 are deferred to subsequent rounds.

## Round Success Criteria
1. Fortran `PrePro` and `crunch_it` build cleanly from commit `7d3f77f` source.
2. Graphene 20 nm×20 nm compression case runs to completion (nloadstep=100); all `nano_*.dat` files archived in `test/cases/graphene_compression_prepro/` and `test/cases/graphene_compression_simulator/`.
3. Cyclic crumple case (nCodeLoad=30) runs to completion; outputs archived in `test/cases/graphene_cyclic_crumple/`.
4. `document/fortran_conventions.md` written, covering: 1-based indexing, unit system (nm, eV), field ordering in each `nano_*.dat`, sign conventions, MPI rank-0 output assumption, active source file list (including `lbfgs.f`).
5. AC-1 positive tests pass; AC-1 negative test (backup variants produce wrong output) verified.
