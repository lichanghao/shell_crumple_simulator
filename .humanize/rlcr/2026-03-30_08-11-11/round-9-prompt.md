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
# Round 8 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 5/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. Round 8 closes the AC-3 evidence gap, but it still does not advance the original-plan mainline beyond the preprocessor slice. The simulator entry point is still a stub at [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1), there are still no C++ translation units for the Milestone 3 through 8 kernels and runtime, and the required documentation files `AGENT.md` and `document/translation_notes.md` are still absent from the repository root. That leaves AC-5 through AC-12 unmet and AC-13 partial. Claude’s Round 8 work therefore remains incomplete original-plan work, not an end-to-end implementation.

2. `task2g` remains unimplemented, so AC-2 and AC-8 are still open. The preprocessor still hardcodes archived vdW span constants through `archived_tub_span_per_element()` at [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L190), still writes `nano_tub_loc.dat` from those preserved constants instead of translated preprocessing state at [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L656), and still exposes only a placeholder `VdwData` at [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp#L105). There is still no committed `nvdw=1` oracle case or neighbor-list/shape-function fixture coverage under `test/cases/`.

## Round 8 Delta Assessment

The narrow B-spline change itself checks out. [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp#L191) now compares all 10 committed fixtures at `1e-14`, and the committed reproduction helper at [test/cases/tools/dump_bspline_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_bspline_oracle.f90#L1) regenerates the fixture data exactly.

Verification I ran:

- `./build/unit_tests '--gtest_filter=BSpline.*'` passed (`4/4`).
- `ctest --test-dir build --output-on-failure` passed (`35/35`).
- `gfortran -c -O0 -fallow-argument-mismatch ../finite_crystal_elasticity/grapheneCompressionOriginPrePro/BSpline.f90 -o /tmp/fortran_bspline.o && gfortran -O0 -fallow-argument-mismatch test/cases/tools/dump_bspline_oracle.f90 /tmp/fortran_bspline.o -o /tmp/dump_bspline_oracle && /tmp/dump_bspline_oracle <tmpdir> && diff -ru --exclude README.md <tmpdir> test/cases/bspline_oracle` passed.

## Goal Tracker Update Request Assessment

Approved. Round 8 does close the remaining AC-3 oracle-evidence gap. I updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) to:

- record the Round 8 AC-3 closure in the Plan Evolution Log,
- update `task2c` evidence to cite the committed fixtures, helper, oracle test, and `35/35` suite pass,
- remove the stale AC-3 blocking-side-issue entry.

## Goal Alignment Check

- AC-1 remains met.
- AC-2 has not materially advanced this round and is still blocked by the missing real `nvdw=1` preprocessing path in `task2g`.
- AC-3 is now met by the committed interior/boundary Fortran fixtures plus the direct `1e-14` oracle test.
- AC-4 remains met.
- AC-5 through AC-12 are still unimplemented in the C++ codebase. They are tracked, but they were effectively ignored this round.
- AC-13 remains partial because the scaffold exists, but `AGENT.md` and `document/translation_notes.md` still do not exist.
- No task IDs from [document/plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L261) are forgotten from the tracker.
- There are still no formal tracker deferrals, but Claude’s “Remaining Items” are mandatory original-plan scope and must be implemented rather than carried forward again.

## Required Implementation Plan

1. Finish `task2g` as a real translation of [vdw_previous.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginPrePro/vdw_previous.f90). Expand [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp) with the actual preprocessing vdW state, add dedicated C++ translation units for vdW preprocessing, remove `archived_tub_span_per_element()` from [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L190), and make [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L656) emit `nano_tub_loc.dat` from computed neighbor-list and shape-function state.

2. Archive and test a real `nvdw=1` preprocessor oracle case under `test/cases/`. Extend [test/support/oracle_compare.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/support/oracle_compare.cpp) and [test/integration/test_prepro_oracle.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_prepro_oracle.cpp) so `nano_tub_loc.dat`, neighbor-list outputs, and shape-function payloads are compared directly against Fortran artifacts instead of preserved constants.

3. Implement Milestone 3 as a new constitutive layer in C++. Port [exponential.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/exponential.f90), [geometry.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/geometry.f90), [brenner.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/brenner.f90), [Hyper_pot_inner_alg.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/Hyper_pot_inner_alg.f90), [newton_inner.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/newton_inner.f90), [ener_elem.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/ener_elem.f90), and [principal.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/principal.f90) into focused `include/fce/*` and `src/core/*` files. Add oracle-backed unit fixtures for Brenner and inner-Newton states before integration so AC-5 and AC-6 can be verified directly.

4. Replace the simulator stub with the real runtime path. Translate [pre_ener.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/pre_ener.f90), [energy.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/energy.f90), [load.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/load.f90), [pasapas.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/pasapas.f90), [get_reac.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/get_reac.f90), and [lbfgs.f](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/lbfgs.f). Update [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) to read the archived `nano_*.dat` inputs, run the translated solver, and emit the simulator outputs. Then add an end-to-end serial oracle test against `test/cases/graphene_compression_simulator/np1/` for AC-7.

5. Implement VTU output and validation. Port [paraview_vtu_output.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/paraview_vtu_output.f90), add C++ VTU writer tests that validate XML structure and field contents against archived simulator state, and wire VTU emission into the translated simulator path so AC-12 is actually exercised.

6. Implement runtime vdW and self-contact using [vdw_modules.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/vdw_modules.f90). Add the Lennard-Jones kernel, spatial binning, topological exclusion, and integration into global assembly. Verify against archived Fortran vdW outputs and add negative coverage for cutoff and exclusion behavior so AC-8 is closed on both preprocessing and runtime sides.

7. Implement cyclic loading, crease memory, crease analysis, and checkpoint/restart. Translate [crease.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/crease.f90) and [crease_analysis.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/crease_analysis.f90), add phase tracking and L-BFGS-history reset in the runtime load controller, and implement checkpoint read/write for `x0`, `eta`, and `K0_ref`. Then add cyclic oracle tests against `test/cases/graphene_cyclic_crumple/` for AC-9 and AC-10.

8. Finish Milestone 8 and documentation. Add multi-rank consistency tests for `np=1`, `np=2`, and `np=4`, cross-rank restart mismatch detection, and the final AC-spanning integration suite. Create `AGENT.md` and `document/translation_notes.md` with project structure, build/run instructions, milestone notes, bugs encountered, and verification evidence. Only after all of that is in place should Claude claim original-plan completion.

Round 8 remains incomplete.
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
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-9-summary.md

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
