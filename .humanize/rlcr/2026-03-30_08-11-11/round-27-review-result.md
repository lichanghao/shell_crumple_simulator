# Round 27 Review

Mainline Progress Verdict: ADVANCED

Goal Alignment Summary:
`ACs: 6/13 addressed | Forgotten items: 0 | Unjustified deferrals: 1`

## Mainline Gaps

1. `task4a` was not started even though the Round 27 contract required it immediately after the Milestone 3 oracle work. The contract explicitly requires a real global assembly implementation plus a load-step-1 energy oracle check in [round-27-contract.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-27-contract.md#L20), but the summary explicitly defers `task4a` in [round-27-summary.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-27-summary.md#L71), the simulator entry point is still a stub in [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1), and the tracker still lists all of `task4a` through `task4f` as pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L96). This means AC-7 did not advance this round and the round cannot be treated as complete.

2. `task3f` is still only partially advanced, not closed. The new principal-fixture corpus is useful, but it covers only analytical/distinct-curvature cases: the provenance explicitly says all 10 archived cases are `flag_num_diff=false` in [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/principal_exponential_oracle/build_provenance.md#L18), and the oracle test repeats that assumption in [test_principal.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_principal.cpp#L229). The repeated-curvature numerical-fallback branch still lacks Fortran-backed evidence, which matters for AC-9. The tracker had drift here: the Round 27 evolution log claimed closure while the active-task row still kept `task3f` pending. I corrected that tracker state in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L76) and [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L95).

3. The summary misstates the acceptance criteria and uses that mislabeling to overclaim closure. In the original plan, AC-5 is Brenner kernel parity, AC-6 is inner-Newton parity, and AC-7 is end-to-end compression simulation in [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L47) and [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L65). The summary instead labels principal as AC-5 and Brenner-through-element-energy as AC-7 in [round-27-summary.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-27-summary.md#L65). That is not a harmless wording issue: it masks the fact that the entire solver core and later milestones remain untouched in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L96).

## Blocking Side Issues

1. No new blocker was introduced by the Round 27 code itself. The existing runtime vdW/self-contact gap remains the only tracked blocker for AC-8 in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L117); code search still shows only preprocessor-side vdW support, with no simulator/runtime assembly path.

## Queued Side Issues

1. The tracker/reporting drift around `task3f` and the AC labels is fixed in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L42), but the round summary still claims `task3f` and AC-7 are closed in [round-27-summary.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-27-summary.md#L57). This is no longer a blocker once the tracker is corrected, but the next round must not reuse those claims.

## Goal Alignment Check

- AC-1: met.
- AC-2: met.
- AC-3: met.
- AC-4: met.
- AC-5: met. Round 27’s Brenner-through-element-energy oracle is valid supporting evidence, but it does not substitute for AC-7.
- AC-6: met.
- AC-7: not met. `task4a` through `task4f` remain untouched.
- AC-8: partial. Preprocessor-side vdW exists, runtime vdW/self-contact does not.
- AC-9: partial. Principal analytical branch now has direct Fortran fixtures, but repeated-curvature/fallback plus crease-memory integration remain open.
- AC-10: not met.
- AC-11: partial. MPI wrapper/partitioning exist, but no solver path exists to compare across ranks.
- AC-12: not met.
- AC-13: partial. `AGENT.md` and `document/translation_notes.md` are still missing.

No original-plan tasks are forgotten from the tracker after the correction above. The one unjustified deferral is `task4a`, because the round contract required it in this round and no new blocker was identified.

## Required Implementation Plan

1. Finish `task3f` properly before claiming Milestone 3 complete. Add a second Fortran-derived principal oracle that exercises the repeated-curvature `flag_num_diff=true` branch, using a committed archived-state geometry that actually triggers the fallback path, and extend [test_principal.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_principal.cpp#L229) to compare that fallback-path output directly against Fortran.

2. Implement Milestone 4 Phase A in a new simulator-core translation unit under `include/fce/` and `src/core/`. Load archived simulator inputs with the existing I/O helpers, partition elements with `MpiEnv`, call `compute_element_energy` for each owned element, and all-reduce the assembled energy and force totals. This work belongs directly under the Milestone 4 Phase A requirement in [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L224).

3. Add the missing `task4a` oracle test immediately. Assemble the compressed graphene mesh for load step 1 from `test/cases/graphene_compression_simulator/np1` and compare the total energy against the first row of `energy.dat` within AC-7 tolerance. Add a corrupted-mesh negative test that fails before returning an energy.

4. Translate `lbfgs.f` into a dedicated solver module, then implement the simulator-side load controller (`nCodeLoad=3` first), pasapas loop, and reaction-force computation in the Milestone 4 order from [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L224). Do not leave `crunch_it` as a stub while claiming AC-7 progress.

5. Replace the stub [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) with a real `crunch_it` driver that reads `nano_*.dat`, runs the 50-step compression case, and writes `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat`.

6. Close AC-7 with end-to-end integration coverage: per-step energy vs oracle, step-50 reaction force vs oracle, step-25 displacements vs oracle, plus fail-fast behavior on corrupted mesh input.

7. Continue directly through Milestones 5 through 8 in plan order after AC-7: VTU output and validation, runtime vdW/self-contact, cyclic loading plus crease memory plus checkpoint/restart, MPI consistency checks, then `AGENT.md`, `document/translation_notes.md`, and the full integration suite. The required sequence is already fixed in [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L232).

## Verification

- `./build/unit_tests`
- `./build/integration_tests`
