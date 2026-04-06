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
# Round 12 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 10/13 addressed | Forgotten items: 0 | Unjustified deferrals: 1`

## Findings

1. Round 12 still does not execute the original plan beyond the constitutive slice, so the mainline remains blocked at the same place as Round 11. The shared core library still contains no C++ translations of `exponential.f90`, `geometry.f90`, `ener_elem.f90`, `principal.f90`, solver assembly, L-BFGS, runtime vdW, cyclic, checkpoint, or VTU modules in [CMakeLists.txt](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L39), and the simulator target still points at a stub entry point in [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1). The tracker still lists `task3a`, `task3b`, `task3e`, `task3f`, and every Milestone 4-8 task as pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L73). Claude’s “Remaining Items” section is therefore another unjustified deferral of required work, not completion.

2. `task3d` still does not satisfy the plan-designed AC-6 oracle provenance even after the fixture-count expansion. The new corpus is larger, and the explicit `fail_mode=1/2/3` checks are useful, but the committed provenance still says these are fixed-output fixtures regenerated from helper-defined inputs in [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L11), and the helper itself hard-codes the Newton states in [dump_constitutive_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_constitutive_oracle.f90#L177). The original plan requires the 10 AC-6 states to be drawn from archived Fortran load-step outputs, not from synthetic helper inputs, so this task made real progress but is still not plan-complete.

3. The `nCode_Pot` semantics are still unresolved, and Round 12’s default-value change is not an end-to-end fix. The translated constitutive layer hard-requires `nCode_Pot == 2` in [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L243) and [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp#L412), and the default/comment were changed to match that local implementation in [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp#L81). But the committed compression oracle input still stores `mat1%nCode_Pot = 1` in [nano_general.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_prepro/nano_general.dat#L7), our own convention doc still says `1=Brenner REBO` in [document/fortran_conventions.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/fortran_conventions.md#L81), while the Fortran simulator reader and inner-potential dispatcher treat the 7-parameter Brenner path as `nCode_Pot == 2` in [read.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/read.f90#L56) and [Hyper_pot_inner_alg.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/Hyper_pot_inner_alg.f90#L165). Changing the C++ default only hides this mismatch for hand-built unit-test materials; it does not make file-backed simulator inputs usable, so this remains a real integration blocker for AC-7.

4. The repository documentation has already drifted behind the committed Round 12 fixture set. [test/cases/README.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/README.md#L57) still advertises only `newton_inner/case_01.dat` through `case_04.dat`, even though the tree now carries `case_01.dat` through `case_10.dat`. This is low severity, but it is a concrete mismatch between the claimed state of the oracle corpus and the repository documentation.

## Goal Alignment Check

- AC-1 through AC-4 remain in the same verified state as prior rounds.
- AC-5 made meaningful progress this round: the cutoff-to-zero branch and both negative tests now exist.
- AC-6 also made meaningful progress this round: the corpus is now 10 fixtures and `fail_mode=1/2/3` are all exercised, but the plan-required archived load-step provenance is still missing.
- AC-7 is still blocked because the required Milestone 3 and Milestone 4 translations are absent and the simulator executable is still a stub.
- AC-8 has no new runtime progress this round; only the earlier preprocessor-side work exists.
- AC-9 through AC-12 saw no executable implementation progress this round.
- AC-13 remains partial: `AGENT.md` and `document/translation_notes.md` are still absent.
- Forgotten items: none. The tracker still covers the original task list.
- Deferred items: not justified. Claude again treats the untouched original-plan remainder as future work even though the plan does not authorize stopping after the constitutive slice.
- Plan evolution: no new plan change is justified. The tracker update should record partial progress only, not reinterpret the remaining original scope as optional.

## Goal Tracker Update Requests

Partially approved and applied.

- Approved: the tracker now records the verified `51/51` suite result for Round 12 and the new Brenner/Newton coverage details in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L42).
- Approved: `task3c` and `task3d` notes now reflect the cutoff-path work, the 10-fixture Newton corpus, and the explicit `fail_mode=1/2/3` checks in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L75).
- Approved: `task3a`, `task3b`, `task3e`, `task3f`, and all simulator/runtime milestones remain open in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L73).
- Added: a new blocking issue for unresolved `nCode_Pot` semantics between the constitutive slice, committed `nano_general.dat` inputs, and project documentation in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L100).
- Rejected: any interpretation that Round 12 closes Milestone 3 or justifies deferring the untouched solver/runtime milestones.

## Required Implementation Plan

Claude must stop treating the remaining original-plan scope as future work and execute the rest of the translation in this order:

1. Reconcile material-code semantics before any further constitutive or solver work.
   Update [document/fortran_conventions.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/fortran_conventions.md), [include/fce/types.hpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/include/fce/types.hpp), [src/core/io.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/io.cpp), and [src/core/constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/constitutive.cpp) so the C++ constitutive layer accepts the same `nano_general.dat` encoding that the archived oracles use, with explicit tests that start from a committed `nano_general.dat` payload rather than from a hand-built material struct.

2. Finish the missing Milestone 3 kernels as dedicated modules, not as more code stuffed into `constitutive.cpp`.
   Add `include/fce/exponential.hpp` + `src/core/exponential.cpp` for the translated `def_bonds`/derivative path from `exponential.f90`.
   Add `include/fce/geometry.hpp` + `src/core/geometry.cpp` for `metric`, `curv`, bond-vector, and derivative computations from `geometry.f90`.
   Add `include/fce/principal.hpp` + `src/core/principal.cpp` for principal-curvature extraction from `principal.f90`.
   Add `include/fce/element_energy.hpp` + `src/core/element_energy.cpp` for `ener_elem.f90`, including inner-failure handling and crease-curvature subtraction.
   Wire all four into [CMakeLists.txt](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L39).

3. Replace the synthetic constitutive oracle states with plan-compliant archived-state fixtures.
   Instrument the Fortran simulator or write a dedicated extractor so 10 representative element states are dumped from archived load-step outputs under `test/cases/constitutive_oracle/`.
   Keep separate explicit failure fixtures for `fail_mode=1/2/3`, but make the main AC-6 parity corpus come from archived simulator states.
   Add focused unit files such as `test/unit/test_exponential.cpp`, `test/unit/test_geometry.cpp`, `test/unit/test_principal.cpp`, and `test/unit/test_element_energy.cpp` instead of continuing to grow [test/unit/test_constitutive.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_constitutive.cpp#L1).

4. Port the solver core and replace the simulator stub with the real compression path.
   Translate `pre_ener.f90`, `energy.f90`, `lbfgs.f`, simulator-side `load.f90`, `pasapas.f90`, and `get_reac.f90` into dedicated C++ modules under `include/fce/` and `src/core/`.
   Rewrite [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) so `crunch_it` reads the archived `nano_*.dat` files, runs the `nCodeLoad=3` load-stepping loop, and writes `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat`.

5. Add the missing AC-7 end-to-end simulator oracle test immediately after the solver port lands.
   Create a new integration test that runs the translated solver against [test/cases/graphene_compression_simulator/np1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1) and checks the energy trajectory, final reaction force, and representative nodal displacements against the archived oracle tolerances from the plan.

6. Port VTU output on top of the real solver path.
   Translate `paraview_vtu_output.f90` into a dedicated output module, emit `mesh_config_XXXX.vtu` and `mesh_config_series.pvd`, and validate them against the archived compression snapshots before claiming AC-12 progress.

7. Port runtime vdW, self-contact, cyclic loading, crease memory, checkpoint/restart, and MPI consistency after the serial solver path is working.
   Translate `vdw_modules.f90`, cyclic load control from `load.f90`, `crease.f90`, `crease_analysis.f90`, and the checkpoint logic currently exercised in the archived cyclic case.
   Then add `np=1/2/4` consistency tests, checkpoint rank-count compatibility tests, and cyclic-oracle comparisons against [test/cases/graphene_cyclic_crumple/simulator_run](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/simulator_run).

8. Finish the documentation and final integration suite last, but before any completion claim.
   Create `AGENT.md`.
   Create `document/translation_notes.md`.
   Update [test/cases/README.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/README.md#L57) and the milestone evidence so the repository documentation matches the committed fixtures and the actual solver/runtime state.

## Verification

- Rebuilt: `cmake --build build --target unit_tests integration_tests -j4`
- Re-ran focused constitutive coverage: `ctest --test-dir build --output-on-failure -R 'Brenner|NewtonInner'`
- Re-ran full suite: `ctest --test-dir build --output-on-failure`
- Result: pass `51/51`

Round 12 remains incomplete.
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
2. Write your work summary into @/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-13-summary.md

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
