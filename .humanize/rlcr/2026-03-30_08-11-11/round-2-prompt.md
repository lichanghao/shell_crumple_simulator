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
# Round 1 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 6/13 addressed | Forgotten items: 0 | Unjustified deferrals: 3`

## Findings

1. `src/simulator/main.cpp` is still a stub, so the original plan is nowhere near complete and Claude's "remaining milestones pending" section is an admission of incomplete work, not an acceptable finish state.
Evidence:
- [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) still prints `FCE Simulator (C++17) — stub`.
- There are no constitutive-model, solver, VTU, vdW kernel, crease-memory, checkpoint, or restart source files under `src/core/` or `include/fce/`.
- `AGENT.md` and `document/translation_notes.md` are still absent from the repository.
Impact:
- AC-5 through AC-12 are still unimplemented.
- AC-13 remains incomplete.
Required action:
- Resume the original plan from Milestone 3 onward and do not treat those milestones as optional future work.

2. Claude's claim that the preprocessor side now handles cyclic parameters is not true in any usable sense. The actual `PrePro` executable crashes on the archived cyclic `nCodeLoad=31` input, and the required cyclic preprocessor artifact is still not implemented.
Evidence:
- Running `cmake --build build --target PrePro crunch_it -j4` succeeds, so this is not a stale-source issue.
- Running `./build/PrePro <tmpdir>` on a temp copy of `test/cases/graphene_cyclic_crumple/prepro_run/data.dat` exits with `rc=139` and produces no `nano_*.dat` outputs.
- [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L566) only writes `nano_dims.dat`, `nano_general.dat`, `nano_zero.dat`, `nano_config.dat`, `nano_BCs.dat`, `nano_Mesh.dat`, and `nano_tub_loc.dat`; there is no `nano_crease.dat` write path.
- [src/core/io.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/io.cpp#L620) leaves `write_crease()` as a stub.
Impact:
- `task2f` is not actually finished for cyclic cases.
- The round's own contract said the build/test graph must stop leaving `PrePro` as a stub; the binary now links the new code, but the cyclic execution path is still broken.
- This also undermines the tracker note that the missing docs are the only queued issue.
Required action:
- Make `PrePro` run successfully on the archived cyclic preprocessor input.
- Implement `nano_crease.dat` writing for `ncrease=1`.
- Add a cyclic preprocessor oracle test that exercises `nCodeLoad=31` instead of validating compression only.

3. `task2g` was marked complete even though vdW preprocessing was not implemented. The current code only hard-codes the archived compression-case partition span in `nano_tub_loc.dat`; there is no neighbor-list, shape-function, or `vdw_previous` equivalent.
Evidence:
- [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L636) writes `nano_tub_loc.dat` from a fixed `ngauss_vdw = 47` span and nothing else vdW-specific.
- [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp#L105) explicitly says the full vdW state is deferred to Milestone 6.
- Repository search finds no vdW preprocessing implementation beyond the placeholder data type and `nano_tub_loc.dat` I/O.
Impact:
- `task2g` should not be marked complete.
- AC-2's vdW-positive case for `nvdw=1` is not implemented.
- AC-8 has not started, despite the tracker implying partial completion through `task2g`.
Required action:
- Move `task2g` back out of "completed" status.
- Implement the actual preprocessor-side vdW data generation before claiming task parity.

4. AC-2 negative validation is still missing. The preprocessor silently accepts invalid chirality input and can emit NaNs instead of failing clearly.
Evidence:
- [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L203) computes `theta` directly from `xn1/xn2` with no range or zero-denominator validation.
- A temp `data.dat` with `nchir=0` and `xn1=xn2=0` exits with `rc=0` and writes `mat1%E = NAN` rows in `nano_general.dat`.
- There is no test covering the required negative case from AC-2.
Impact:
- AC-2 is only partially satisfied. The positive compression oracle passes, but the required invalid-input failure mode is absent.
Required action:
- Validate chirality indices before computing `theta`.
- Reject invalid `data.dat` with a clear exception.
- Add the negative integration test required by AC-2.

5. AC-3 is only self-consistency-tested, not oracle-verified as required, and the implementation does not reject out-of-domain evaluation.
Evidence:
- [test/unit/test_bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_bspline.cpp#L20) only checks partition of unity and finite-difference agreement at a few representative points.
- There are no tests for the 5 interior and 5 boundary Fortran fixtures required by AC-3.
- [src/core/bspline.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/bspline.cpp#L8) evaluates closed-form polynomials for any `(v,w)` with no domain guard, so the AC-3 negative case "outside valid parameter domain returns an error/assertion" is still unmet.
Impact:
- The tracker's claim that AC-3 is complete is overstated.
Required action:
- Add oracle fixtures for the required interior and boundary patch configurations.
- Add domain validation for invalid `(v,w)`.
- Add the negative out-of-domain test from AC-3.

6. The implementation diverges from the plan's required architecture. The plan requires dedicated object-oriented components; the current translation remains a mostly procedural port centered on one large orchestration function.
Evidence:
- [document/plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L139) requires object-oriented encapsulation and explicitly forbids monolithic procedural structure.
- [src/core/preprocessor.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/preprocessor.cpp#L192) contains a single long `run_preprocessor()` function that directly coordinates parsing, mesh generation, BC setup, reference state generation, and output writing.
- The new modules are exposed as free functions rather than the dedicated classes called for in the plan.
Impact:
- This is a plan-design mismatch, even where behavior is partially correct.
Required action:
- Refactor toward the planned component boundaries while implementing the remaining milestones, instead of continuing to add more logic to the current procedural path.

## Validation Notes

- Claude's reported `ctest --test-dir build --output-on-failure` result is reproducible: 26/26 current tests pass.
- That passing set is not sufficient to establish full round completion because it only covers:
  - the compression preprocessor oracle case,
  - narrow self-consistency tests for B-splines,
  - a toy ghost-node extrapolation check,
  - prior I/O and MPI helper tests.
- Claude's reported build command did not build the `PrePro` or `crunch_it` executables. Only after rebuilding those targets directly could the real executable behavior be checked.

## Goal Alignment Check

- AC-1: addressed and complete.
- AC-2: partially addressed. Compression-case oracle parity is present, but the invalid-input negative test is missing and cyclic preprocessor handling is broken.
- AC-3: partially addressed. Self-consistency tests exist, but the Fortran fixture comparisons and out-of-domain rejection are missing.
- AC-4: partially addressed. Compression-case mesh parity exists, but the negative anchor-node failure test is still absent.
- AC-5: not addressed.
- AC-6: not addressed.
- AC-7: not addressed.
- AC-8: not addressed. `task2g` completion is a tracker overclaim.
- AC-9: not addressed. Cyclic preprocessor execution currently crashes.
- AC-10: not addressed.
- AC-11: partially addressed through the MPI wrapper only.
- AC-12: not addressed.
- AC-13: partially addressed through the scaffold, but the required documents are still missing.

Tracker assessment:
- The Round 1 plan-evolution entry claiming the round "closed the preprocessor milestone cleanly" is not defensible because cyclic preprocessor support is broken and `task2g` is not implemented.
- The round contract's out-of-scope buckets are unjustified deferrals relative to the original plan and this review rubric.
- The "Active Tasks" table is being used as a mixed state table. Completed Milestone 2 tasks should not remain in the active section while downstream milestones are still pending.

## Directive Implementation Plan

1. Finish Milestone 2 completely, not just the compression happy path.
- Fix the cyclic `nCodeLoad=31` preprocessor crash first.
- Implement `nano_crease.dat` read/write support and hook it into `run_preprocessor()` when `ncrease=1`.
- Replace the current `task2g` placeholder with actual vdW preprocessing data generation: neighbor lists, shape functions, and any archived `vdw_previous`-equivalent outputs needed by the simulator.
- Add negative preprocessor tests for invalid chirality indices and corrupted mesh comparison.
- Add the missing AC-3 negative/out-of-domain test and the required Fortran-based BSpline fixture comparisons.

2. Implement the constitutive-model milestone exactly as the original plan specifies.
- Add the exponential Cauchy-Born map, bond-vector geometry, Brenner REBO potential, inner Newton solver, element energy kernel, and principal-curvature extraction.
- Back each kernel with oracle or finite-difference tests matching AC-5 and AC-6 tolerances.

3. Replace the simulator stub with the real Milestone 4 solver pipeline.
- Add MPI-aware assembly, the translated `lbfgs.f` algorithm, the load controller, the `pasapas` stepping loop, and reaction-force computation.
- Build an end-to-end serial oracle test for the graphene compression case and verify AC-7 before moving on.

4. Implement VTU output and validate it.
- Translate the ParaView writer.
- Add XML-validity and state-consistency tests required by AC-12.

5. Implement the real vdW/self-contact kernel.
- Add the Lennard-Jones interaction code, topological-exclusion filtering, spatial binning, and assembly integration.
- Compare vdW energy, forces, and neighbor lists against the archived oracle to satisfy AC-8.

6. Implement cyclic loading, crease memory, and checkpoint/restart.
- Add the cyclic BC controller for `nCodeLoad=30/31`, L-BFGS history resets, crease-memory state updates, crease analysis output, and checkpoint serialization/restoration.
- Add cyclic oracle tests using the archived `graphene_cyclic_crumple` case to satisfy AC-9 and AC-10.

7. Finish MPI verification and documentation.
- Add `np=1/2/4` comparison tests and checkpoint rank-compatibility checks for AC-11.
- Create `AGENT.md` and `document/translation_notes.md`, then keep them updated with each completed milestone so AC-13 is no longer deferred.

8. Refactor toward the required architecture while landing the remaining milestones.
- Split the current procedural pipeline into the dedicated components the plan already mandates, so the simulator work does not continue to accumulate inside monolithic free functions.

The project is still incomplete and should continue from this plan rather than opening another round summary that treats the remaining milestones as optional.
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
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-2-summary.md

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
