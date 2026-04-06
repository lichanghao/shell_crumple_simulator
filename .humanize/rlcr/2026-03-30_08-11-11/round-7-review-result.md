# Round 7 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 4/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. Round 7 closes a documentation/provenance gap, but it does not advance the original implementation mainline. The simulator is still a stub at [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1), and [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) plus [document/translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) are still missing. Claude’s own summary leaves Milestones 3 through 8 pending, so AC-5 through AC-12 remain unmet and AC-13 remains partial. The current `34/34` passing tests only cover the preprocessor/unit scaffold and do not change that status.

2. `task2g` is still unimplemented, so AC-2 and AC-8 remain open. [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L190) still hardcodes preserved `nano_tub_loc.dat` span values through `archived_tub_span_per_element()`, [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L656) still writes `nano_tub_loc.dat` from those constants instead of translated vdW preprocessing state, and [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp#L105) still carries only a placeholder `VdwData`. The repository also still has no committed `nvdw=1` archived preprocessor oracle case or neighbor-list/shape-function fixtures under `test/cases/`.

3. AC-3 is still incomplete. [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp#L20) covers partition-of-unity, finite-difference self-consistency, and out-of-domain rejection, but the required five interior and five boundary Fortran oracle fixtures are still absent. `test/cases/bspline_oracle/` does not exist, so the plan’s direct `1e-14` oracle checks for `BSpline` and `DBSpline` are still missing.

## Goal Tracker Update Request Assessment

Approved. Round 7 does fix the narrow provenance reproducibility gap previously called out in Round 6. [test/cases/graphene_compression_prepro/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_prepro/build_provenance.md#L46) now points to the committed helper at [test/cases/tools/dump_ghost_coords.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_ghost_coords.f90#L1), and the documented compile/run flow reproduces the archived `ghost_coords.dat` files without changing oracle outcomes. I updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md).

## Goal Alignment Check

- AC-1 remains met.
- AC-2 has incremental progress but is still blocked by `task2g`; the real `nvdw=1` preprocessing path is not translated.
- AC-3 has incremental progress but is still blocked by the missing Fortran interior/boundary B-spline oracle fixtures.
- AC-4 remains met, and Round 7 improves its audit trail by committing the helper and self-contained reproduction instructions.
- AC-5 through AC-12 are still unaddressed in the implementation. The simulator path remains a stub, so these criteria are not merely deferred; they are unfinished original-plan work.
- AC-13 remains partial because the build scaffold exists, but `AGENT.md` and `document/translation_notes.md` are still absent.
- No task IDs from [document/plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L259) appear to be forgotten from the tracker; they are represented in Active, Completed, or the analyze-only final test task.
- There are no formal tracker deferrals, but Claude’s “Remaining Items” are still mandatory scope from the original plan and must be implemented rather than carried forward indefinitely.

## Required Implementation Plan

1. Finish `task2g` as a real translation, not an oracle-preserving bridge. Add vdW preprocessor data structures and translated logic for neighbor-list generation, shape-function payloads, and `vdw_previous`-equivalent state. Remove `archived_tub_span_per_element()` from [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L190) and make [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L656) emit `nano_tub_loc.dat` from computed state.

2. Archive and test a real `nvdw=1` preprocessor oracle case under `test/cases/`. Extend the comparator and integration tests so `nano_tub_loc.dat`, vdW neighbor lists, and shape-function outputs are compared directly against the Fortran archive.

3. Close AC-3 with committed oracle fixtures. Add `test/cases/bspline_oracle/` containing five interior and five boundary 12-node patch fixtures from the frozen Fortran code, then extend the B-spline tests to compare `BSpline`, `DBSpline`, and derivative helpers at absolute `1e-14`.

4. Implement Milestone 3 exactly from the Fortran kernels: exponential map, geometry/bond-vector decomposition, Brenner REBO, inner Newton, element energy/force, and principal curvature. Add oracle-backed unit tests for AC-5 and AC-6 before integrating them into the solver.

5. Replace the simulator stub with the real Milestone 4 pipeline: global assembly, `lbfgs.f`-equivalent minimization, load control, `pasapas`, and reaction-force output. Then add end-to-end serial oracle tests against [test/cases/graphene_compression_simulator/np1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1).

6. Implement VTU output and validation for AC-12, then implement runtime vdW/self-contact, cyclic controller, crease memory, crease analysis, and checkpoint/restart for AC-8 through AC-10 using the archived cyclic case as the oracle baseline.

7. Finish Milestone 8 with multi-rank consistency and cross-rank restart tests for AC-10 and AC-11, then add [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) and [document/translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) to satisfy AC-13.

8. Run the final full integration suite only after the above work lands, and do not claim milestone completion again until the repository contains translated implementations and oracle-backed tests for every remaining AC.

## Verification

- `gfortran -c -O0 -fallow-argument-mismatch ../finite_crystal_elasticity/grapheneCompressionOriginPrePro/headers.f90 -J /tmp -o /tmp/headers.o && gfortran -c -O0 -fallow-argument-mismatch ../finite_crystal_elasticity/grapheneCompressionOriginPrePro/connect_mesh.f90 -I /tmp -J /tmp -o /tmp/connect_mesh.o && gfortran -O0 -fallow-argument-mismatch test/cases/tools/dump_ghost_coords.f90 /tmp/headers.o /tmp/connect_mesh.o -I /tmp -J /tmp -o /tmp/dump_ghost_coords && /tmp/dump_ghost_coords test/cases/graphene_compression_prepro && /tmp/dump_ghost_coords test/cases/graphene_cyclic_crumple/prepro_run && ctest --test-dir build --output-on-failure -R '^PreprocessorOracle'` passed (`7/7`).
- `ctest --test-dir build --output-on-failure` passed (`34/34`).

Round 7 remains incomplete.
