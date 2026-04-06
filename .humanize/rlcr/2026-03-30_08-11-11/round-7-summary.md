# Round 7 Summary

## Work Completed
- Committed `9db7bf7` (`Commit ghost coordinate provenance helper`).
- Added the missing in-repo Fortran helper at `test/cases/tools/dump_ghost_coords.f90`.
- Updated `test/cases/graphene_compression_prepro/build_provenance.md` so the ghost-coordinate reproduction path points to the committed helper and uses a self-contained compile/run sequence from this repository.
- Verified the committed helper rebuilds and regenerates both `test/cases/graphene_compression_prepro/ghost_coords.dat` and `test/cases/graphene_cyclic_crumple/prepro_run/ghost_coords.dat` from the frozen sibling Fortran source without changing the oracle comparison outcomes.

## Files Changed
- `test/cases/graphene_compression_prepro/build_provenance.md`
- `test/cases/tools/dump_ghost_coords.f90`

## Validation
- `gfortran -c -O0 -fallow-argument-mismatch ../finite_crystal_elasticity/grapheneCompressionOriginPrePro/headers.f90 -J /tmp -o /tmp/headers.o && gfortran -c -O0 -fallow-argument-mismatch ../finite_crystal_elasticity/grapheneCompressionOriginPrePro/connect_mesh.f90 -I /tmp -J /tmp -o /tmp/connect_mesh.o && gfortran -O0 -fallow-argument-mismatch test/cases/tools/dump_ghost_coords.f90 /tmp/headers.o /tmp/connect_mesh.o -I /tmp -J /tmp -o /tmp/dump_ghost_coords && /tmp/dump_ghost_coords test/cases/graphene_compression_prepro && /tmp/dump_ghost_coords test/cases/graphene_cyclic_crumple/prepro_run && ctest --test-dir build --output-on-failure -R '^PreprocessorOracle'` -> PASS (`7/7`)
- `ctest --test-dir build --output-on-failure` -> PASS (`34/34`)

## Remaining Items
- `task2g` remains pending: the real `nvdw=1` preprocessing path, neighbor-list generation, shape functions, and `vdw_previous`-equivalent state are still not translated.
- AC-3 remains partial: the required 5 interior and 5 boundary Fortran B-spline oracle fixtures are still missing.
- Milestones 3 through 8 remain pending, including the simulator mainline, vdW runtime, cyclic runtime controller, checkpoint/restart, MPI equivalence, VTU output, and final documentation.

## Goal Tracker Update Request

### Requested Changes:
- Add a Plan Evolution note or evidence-note update that Round 7 closed the ghost-coordinate provenance gap by committing `test/cases/tools/dump_ghost_coords.f90` and updating the reproduction instructions in `test/cases/graphene_compression_prepro/build_provenance.md`.
- If the tracker is carrying any open note about the ghost-coordinate provenance helper being missing, remove it.

### Justification:
Round 6 already closed AC-4 technically, but Round 7 resolved the remaining reproducibility gap in the provenance note by committing the helper source that the documentation references and verifying the documented regeneration path end-to-end. This improves auditability without changing AC status.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: The round only packaged an in-repo provenance helper and did not uncover a new reusable failure mode.
