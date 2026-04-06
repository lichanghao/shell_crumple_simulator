# Round 32 Review

Mainline Progress Verdict: ADVANCED, NOT COMPLETE

Goal Alignment Summary:
`ACs: 2/13 addressed | Forgotten items: 0 | Unjustified deferrals: 2`

## Findings

1. High: the executable-path solver still skips the oracle case's imperfection update, so AC-7 cannot close on the archived `np1` run yet. The archived compression input enables `imperfect = 1` and `fact_imp = 0.01` in `test/cases/graphene_compression_simulator/np1/nano_general.dat:24-27`. Canonical `../finite_crystal_elasticity/grapheneCompressionOriginVersion/pasapas.f90:96-113` perturbs every real node before each load-step minimization. Current C++ `pasapas()` goes directly from `load_ctrl.apply_increment(...)` to `minimize_constrained(...)` at `src/core/solver.cpp:386-390`, and repository search shows no runtime use of `general.imperfect` / `general.fact_imp` outside I/O and preprocessor parsing. That omission matches the immediate step-1 divergence Claude reported. Claude needs to port the Fortran imperfection semantics exactly, or provide source-backed evidence that the archived oracle was generated with the path effectively disabled.

2. Medium: the new `RuntimeState` / live-`eta` fix still has no direct regression coverage. The only new end-to-end check is `test/integration/test_e2e_compression.cpp:148-219`, but the archived `np1` case it exercises has `nW_hat = 0` in `test/cases/graphene_compression_simulator/np1/nano_general.dat:19-20`, so that test never exercises evolving inner-relaxation state at all. A repository-wide search for `RuntimeState` and `eta_updates` finds no dedicated unit or integration test for carry-forward `eta`. The round's central code change can therefore regress silently.

3. Medium: the new shortened-run CLI is not an exact executable-path probe. `src/simulator/main.cpp:61-66` overwrites `input.bcs.nloadstep` with the positional argument, and `src/simulator/main.cpp:82-88` also passes the same value as `iload_stop`. Because `src/core/load_controller.cpp:51-55` computes `DL = value / nloadstep`, `crunch_it <case_dir> 3` changes the physical increment size instead of executing the first three steps of the canonical 50-step path. That contradicts the round summary's claim that shortened runs can request an exact step count on the real executable path.

4. Process/plan issue: the round still defers mandatory plan work despite explicit instructions not to. `round-32-contract.md:13-16` marks `task4e` and all of Milestones 5-8 (`task5a`-`task8d`) out of scope. Those are incomplete original-plan tasks, not acceptable carryover. The review cannot accept the round as complete while they remain deferred or pending.

## Goal Alignment Check

- AC-1 through AC-6: maintained, not advanced this round.
- AC-7: advanced. The real executable path and archived-oracle harness are now wired up, but the solver still fails the oracle comparison and the missing imperfection path keeps the runtime semantics incomplete.
- AC-8: ignored. Runtime vdW and self-contact code are still absent.
- AC-9: slight plumbing progress only. `RuntimeState` is relevant groundwork, but cyclic control, crease memory, and cyclic oracle validation remain untouched.
- AC-10: ignored. Writing `nano_final_config.dat` is not checkpoint/restart support.
- AC-11: ignored. No new multi-rank solver-consistency work landed.
- AC-12: ignored. There is still no VTU writer in `src/`, and `crunch_it` does not emit `mesh_config_*.vtu`.
- AC-13: ignored. `AGENT.md` and `document/translation_notes.md` are still missing.
- Forgotten items: none. All original-plan tasks are still represented in the tracker.
- Deferred items: unjustified. `task4e` plus Milestones 5-8 were explicitly queued out of scope even though the round instructions required completing the full original plan.
- Plan evolution: the Round 32 tracker entry is directionally honest about AC-7 staying red, but its AC-10 / AC-12 impact label overstates the round because no checkpoint or VTU implementation landed.

## Required Implementation Plan

1. Finish the nCodeLoad=3 runtime semantics before any more oracle chasing. Reconcile `src/core/solver.cpp` against canonical `pasapas.f90` in exact order: keep the file-loaded `BCs%nloadstep` as the load denominator, use a separate `iload_stop` limit only for shortened probes, port the imperfection update from `pasapas.f90`, preserve the status/NaN guard behavior, and remove any post-step behavior that is not present in the Fortran path.

2. Complete `task4e` while fixing step 1. Translate the remaining `get_reac.f90` logic needed for force and torque parity, thread the exact minimized force vector into reaction computation, and keep the archived `force.dat` comparison as the blocking oracle until it passes.

3. Add direct regression coverage for live `eta`. Commit or reuse an archived `nW_hat = 1` runtime fixture, then add a test that performs multiple successive assemblies or minimization calls and asserts `eta` changes are persisted and reused between calls. Keep the current `np1` executable-path test as the AC-7 end-to-end parity check only.

4. Implement Milestone 5 immediately after AC-7 turns green. Translate `paraview_vtu_output.f90`, emit `mesh_config_0000.vtu` through the final step plus the companion metadata, and add validation that the generated VTU XML is readable and its nodal/element fields match both the solver state and the archived oracle cases.

5. Implement Milestone 6 end to end. Port `vdw_modules.f90` (kernel, neighbor update cadence, spatial binning, and topological exclusion), integrate it into runtime assembly, and verify against the committed `graphene_self_contact` and `graphene_bilayer_twist_vdw_1000` oracle cases.

6. Implement Milestone 7 end to end. Port cyclic `nCodeLoad = 30/31` control, phase tracking, L-BFGS history reset at phase changes, crease-memory initialization/update/write, crease analysis, and checkpoint read/write for `x0`, `eta`, and `K0_ref`, then validate against `graphene_cyclic_crumple` plus restart-parity tests.

7. Implement Milestone 8 end to end. Add `np=1/2/4` solver-consistency tests, checkpoint rank-count compatibility checks, finalize `AGENT.md` and `document/translation_notes.md`, and then run the full AC audit (`task8d`) across all acceptance criteria before claiming completion.

Status: Incomplete.
