# Round 33 Review

Mainline Progress Verdict: ADVANCED, NOT COMPLETE

Goal Alignment Summary:
`ACs: 2/13 addressed | Forgotten items: 0 | Unjustified deferrals: 1`

## Findings

1. High: the new imperfection path is still a surrogate, not a source-equivalent translation, so AC-7 remains blocked on real runtime semantics. The C++ port adds the step in the right slot, but [solver.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/solver.cpp#L150) hard-codes a process-global deterministic `minstd_rand` sequence, while canonical [pasapas.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/pasapas.f90#L104) calls `random_seed()` and `random_number(a)` at runtime for each load step. I verified the Fortran behavior locally with a minimal `gfortran` probe: repeated `random_seed(); random_number(a)` calls produced different values (`0.9055346`, `0.0756498`, `0.0898398`, ...), so the new C++ sequence is not "source-backed" and cannot justify the tracker wording Claude requested in [round-33-summary.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-33-summary.md#L70). The right review outcome is to keep `task4d` open, record that step placement is improved, and explicitly track the imperfection RNG semantics as still unresolved.

2. Medium: the reaction-side fix is real, but full executable-path force parity is still unverified after the fix. The code change in [load_controller.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/load_controller.cpp#L120) and the new unit test in [test_load_controller.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_load_controller.cpp#L1) correctly repair the zero-based side-tag mapping for `nCodeLoad=3`, and [get_reac.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/get_reac.f90#L50) confirms there is no torque branch on that AC-7 path. But Claude’s own summary says the only full 50-step archived-oracle run was captured before that final fix, and the post-fix evidence is only the unit test plus a three-step probe in [round-33-summary.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-33-summary.md#L60). That means `task4e` and `task4f` stay open on evidence, not just on principle.

3. Process/plan issue: the round still explicitly scoped Milestones 5 through 8 out of the mainline, even though the original plan remains fully in force. [round-33-contract.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-33-contract.md#L16) says Milestone 5 onward will only be touched if needed to unblock AC-7. That is an unjustified deferral decision because [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L287) through [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L296) still require `task5a` through `task8d`, and those pending tasks continue to block AC-8 through AC-13 in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L107). Round 33 made valid AC-7 progress, but it did not eliminate the remaining planned work.

## Goal Alignment Check

- AC-1 through AC-6: maintained, not advanced this round.
- AC-7: advanced. The stop-step contract, live-`eta` coverage, and reaction-side mapping are all better grounded, but the archived-oracle runtime is still red and the imperfection path is not source-equivalent yet.
- AC-8: ignored this round. Runtime vdW and self-contact remain unimplemented.
- AC-9: slight progress only. The `nW_hat=1` regression is useful groundwork, but cyclic control, crease memory, and cyclic oracle validation remain untouched.
- AC-10: ignored this round. Checkpoint/restart is still absent.
- AC-11: ignored this round. No new `np=1/2/4` runtime-parity work landed.
- AC-12: ignored this round. There is still no translated VTU writer or validation path.
- AC-13: ignored this round. `AGENT.md` and `document/translation_notes.md` are still missing.
- Forgotten items: none. All original-plan tasks are still represented in the tracker.
- Deferred items: unjustified. The round contract still marked Milestone 5 onward out of scope even though those tasks remain part of the active plan and block AC-8 through AC-13.

## Goal Tracker Update Review

I approved the parts of Claude’s request that were supported by the repository and updated [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md) accordingly:

- Added a Round 33 plan-evolution entry for the verified stop-step fix, live-`eta` test, and zero-based reaction-side fix.
- Updated `task4d`, `task4e`, and `task4f` notes to reflect the current evidence.
- Narrowed `task4e` to the real `nCodeLoad=3` force-summing path because canonical `get_reac.f90` has no torque branch there.

I rejected one part of the request and recorded the corrected version instead:

- Rejected the proposed "source-backed imperfection step" wording because the implementation in [solver.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/solver.cpp#L150) is explicitly a deterministic surrogate, not a source-equivalent port of [pasapas.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity/grapheneCompressionOriginVersion/pasapas.f90#L104).

I did not add a separate "resolved issue" row for the side-tag bug because the tracker does not maintain a dedicated resolved-issues table; that resolution is now captured in the Round 33 plan-evolution entry and the `task4e` note instead.

## Required Implementation Plan

1. Finish AC-7 before any more bookkeeping. Replace the deterministic imperfection surrogate in [solver.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/solver.cpp#L150) with a source-backed runtime path that matches the canonical Fortran behavior closely enough to explain the archived oracle. Then rerun the full archived-oracle executable regression in [test_e2e_compression.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/integration/test_e2e_compression.cpp#L1), record the first still-failing step across `energy.dat`, `force.dat`, and `nano_final_config.dat`, and continue reconciling `pasapas`/`minimize` semantics until `task4d` through `task4f` are actually green.

2. Implement Milestone 5 exactly as planned. Add the VTU writer translation (`include/fce/vtu_writer.hpp`, `src/core/vtu_writer.cpp` or equivalent), call it from the runtime solver so `crunch_it` emits the expected `mesh_config_*.vtu` sequence, and add validation that the XML is readable and numerically consistent with the solver state and archived serial-compression snapshots. Do not treat archived assembly readers as a substitute for runtime VTU generation.

3. Implement Milestone 6 end to end. Port runtime `vdw_modules.f90` behavior into dedicated runtime code, including Lennard-Jones evaluation, spatial binning, neighbor-update cadence, and topological exclusion. Integrate it into [simulator.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/simulator.cpp) and the runtime solver path, then verify against the committed self-contact and bilayer-twist oracle cases.

4. Implement Milestone 7 end to end. Extend [load_controller.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/load_controller.cpp) and [solver.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/core/solver.cpp) for `nCodeLoad=30/31` phase tracking and L-BFGS resets, add crease-memory and crease-analysis modules, add checkpoint read/write for `x0`, `eta`, and `K0_ref`, and validate against the archived cyclic-crumple and restart workflows.

5. Implement Milestone 8 last, but implement it fully. Add `np=1/2/4` runtime-consistency tests, checkpoint rank-count compatibility checks, create [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) and [translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md), and then run the full AC-spanning integration audit before any claim of completion.

Status: Incomplete.
