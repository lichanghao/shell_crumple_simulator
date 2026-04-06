Your work is not finished. Read and execute the below with ultrathink.

## Original Implementation Plan

**IMPORTANT**: Before proceeding, review the original plan you are implementing:
@document/plan.md

This plan contains the full scope of work and requirements. Ensure your work aligns with this plan.

---

For all tasks that need to be completed, please use the Task system (TaskCreate, TaskUpdate, TaskList) to track each item in order of importance.
You are strictly prohibited from only addressing the most important issues - you MUST create Tasks for ALL discovered issues and attempt to resolve each one.

Before executing each task in this round:
1. Read @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/bitlesson.md
2. Run `bitlesson-selector` for each task/sub-task
3. Follow selected lesson IDs (or `NONE`) during implementation

---
Below is Codex's review result:
<!-- CODEX's REVIEW RESULT START -->
# Round 6 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 4/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. `task2g` is still unimplemented, so AC-2 and AC-8 remain open. [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L190) still hardcodes the preserved `nano_tub_loc.dat` span bridge through `archived_tub_span_per_element()`, [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L656) still writes those constants instead of translated `vdw_previous` output, and [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp#L105) still carries only a placeholder `VdwData`. Round 6 did not change that. Any attempt to push this into a future phase is still incomplete original-plan work, not an acceptable deferral.

2. AC-3 is still incomplete. The only B-spline coverage remains partition-of-unity, finite-difference self-consistency, and out-of-domain rejection in [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp#L20). The required 5 interior and 5 boundary Fortran oracle fixtures are still absent from `test/cases/bspline_oracle/`, so there is still no direct oracle verification of `BSpline` and `DBSpline` at `1e-14`.

3. Milestones 3 through 8 remain unimplemented. [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) is still a stub, and the repository still lacks the constitutive-model, solver, runtime vdW, cyclic, checkpoint, VTU, and final-documentation deliverables required by the original plan. `AGENT.md` and `document/translation_notes.md` are still missing, so AC-5 through AC-12 are unmet and AC-13 remains partial.

4. Round 6's provenance note is not reproducible as written. [test/cases/graphene_compression_prepro/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_prepro/build_provenance.md#L48) says `ghost_coords.dat` came from `dump_ghost_coords.f90`, but that helper source is not committed anywhere in this repo. The archived artifacts and direct comparator are valid, so this does not reopen AC-4, but the provenance claim is incomplete until the helper or an equivalent in-repo reproduction path exists.

## Goal Tracker Update Request Assessment

Approved.

The Round 6 request to close the AC-4 evidence gap is justified. [test/support/oracle_compare.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/support/oracle_compare.cpp#L284) now reads archived oracle `ghost_coords.dat` directly, the positive preprocessor oracle tests run at `1e-12` in [test/integration/test_prepro_oracle.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_prepro_oracle.cpp#L106) and [test/integration/test_prepro_oracle.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_prepro_oracle.cpp#L140), the negative regression in [test/integration/test_prepro_oracle.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_prepro_oracle.cpp#L195) proves the archive-backed comparison rejects perturbed generated ghost coordinates, and [src/core/io.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/io.cpp#L430) fixes the exposed `read_mesh()` `numnods` bug. I updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md).

## Goal Alignment Check

- AC-1 remains met.
- AC-2 has incremental progress but is still blocked by `task2g`.
- AC-3 has incremental progress but is still blocked by missing Fortran fixtures.
- AC-4 is now met on the evidence requested by the plan.
- AC-5 through AC-12 are still unaddressed in the codebase.
- AC-13 remains partial: the scaffold exists, but `AGENT.md` and `document/translation_notes.md` are still missing.
- No plan task IDs appear to be forgotten from Active, Completed, or Deferred.
- There are no formally tracked deferrals, but the summary's "Remaining Items" are still original-plan tasks that must be implemented, not deferred away.

## Required Implementation Plan

1. Finish real preprocessor-side vdW translation before making any more milestone-closure claims. Add `include/fce/vdw_preprocess.hpp` and `src/core/vdw_preprocess.cpp`, expand `VdwData` to carry `ngauss_vdw`, per-sheet spans, neighbor pairs, and shape-function payload, then replace the preserved-span bridge in [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L190) and [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L656) so `nano_tub_loc.dat` and the vdW preprocessor outputs come from translated logic rather than constants.

2. Add a real `nvdw=1` archived preprocessor case under `test/cases/` and extend the preprocessor oracle comparator and integration tests so neighbor-list, shape-function, and `nano_tub_loc.dat` outputs are compared directly against the Fortran archive.

3. Close AC-3 with committed oracle fixtures. Add `test/cases/bspline_oracle/` with 5 interior and 5 boundary 12-node patch fixtures, then extend the tests so `BSpline`, `DBSpline`, and the required derivative helpers are checked against those fixtures at absolute `1e-14`.

4. Implement Milestone 3 exactly from the Fortran kernels. Add `exponential`, `geometry`, `brenner`, `inner_newton`, `element_energy`, and `principal_curvature` modules plus unit and oracle tests for AC-5 and AC-6 before integrating them into the solver.

5. Implement Milestone 4 as a real simulator pipeline. Replace the stub in [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) with translated assembly, `lbfgs.f`-equivalent minimization, load control, pasapas stepping, and reaction-force output, then add end-to-end serial oracle tests against `test/cases/graphene_compression_simulator/np1`.

6. Implement Milestones 5 through 7: VTU output, runtime vdW and self-contact, cyclic controller, crease memory and analysis, and checkpoint/restart, all with oracle-backed tests against the archived cyclic case.

7. Finish Milestone 8 with multi-rank consistency and restart tests, then add the missing `AGENT.md` and `document/translation_notes.md`.

8. If the ghost-coordinate provenance is going to stay documented in-repo, commit the missing `dump_ghost_coords.f90` helper or replace the current note with self-contained reproduction instructions that exist in this repository.

## Verification

- `cmake --build build --target unit_tests integration_tests -j4`
- `ctest --test-dir build --output-on-failure`
<!-- CODEX's REVIEW RESULT  END  -->
---

## Goal Tracker Reference (READ-ONLY after Round 0)

Before starting work, **read** @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md to understand:
- The Ultimate Goal and Acceptance Criteria you're working toward
- Which tasks are Active, Completed, or Deferred
- Any Plan Evolution that has occurred
- Open Issues that need attention

**IMPORTANT**: You CANNOT directly modify goal-tracker.md after Round 0.
If you need to update the Goal Tracker, include a "Goal Tracker Update Request" section in your summary (see below).

---

Note: You MUST NOT try to exit by lying, editing loop state files, or executing `cancel-rlcr-loop`.

After completing the work, please:
0. If the `code-simplifier` plugin is installed, use it to review and optimize your code. Invoke via: `/code-simplifier`, `@agent-code-simplifier`, or `@code-simplifier:code-simplifier (agent)`
1. Commit your changes with a descriptive commit message
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-7-summary.md

## Task Tag Routing Reminder

Follow the plan's per-task routing tags strictly:
- `coding` task -> Claude executes directly
- `analyze` task -> execute via `/humanize:ask-codex`, then integrate the result
- Keep Goal Tracker Active Tasks columns `Tag` and `Owner` aligned with execution

**If Goal Tracker needs updates**, include this section in your summary:
```markdown
## Goal Tracker Update Request

### Requested Changes:
- [E.g., "Mark Task X as completed with evidence: tests pass"]
- [E.g., "Add to Open Issues: discovered Y needs addressing"]
- [E.g., "Plan Evolution: changed approach from A to B because..."]
- [E.g., "Defer Task Z because... (impact on AC: none/minimal)"]

### Justification:
[Explain why these changes are needed and how they serve the Ultimate Goal]
```

Codex will review your request and update the Goal Tracker if justified.
