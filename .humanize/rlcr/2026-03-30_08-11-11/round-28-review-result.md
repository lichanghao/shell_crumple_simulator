# Round 28 Review

Mainline Progress Verdict: ADVANCED

Goal Alignment Summary:
`ACs: 11/13 addressed | Forgotten items: 0 | Unjustified deferrals: 3`

## Findings

1. The new `crunch_it` entry point is still not the real simulator driver required by the plan or by the Round 28 prompt. [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L25) only reads an already-written `mesh_config_XXXX.vtu`, assembles one static state, and prints `assembled_energy`, `inner_fail`, and `force_dofs` at [src/simulator/main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L44). It does not run the 50-step `nCodeLoad=3` compression path, does not translate `lbfgs.f`, `load.f90`, `pasapas.f90`, or `get_reac.f90`, and does not write `energy.dat`, `force.dat`, `output.dat`, or `nano_final_config.dat` as explicitly required in [round-28-prompt.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-28-prompt.md#L99), [round-28-prompt.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-28-prompt.md#L101), and the Milestone 4 plan in [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L224). Repository search also confirms the runtime solver modules are still absent: there are no `lbfgs`, runtime load controller, `pasapas`, reaction, checkpoint, crease, or VTU-output implementation units under `include/` or `src/`.

2. The new tests do not satisfy the required AC-7 oracle coverage. [test_simulator.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_simulator.cpp#L26) checks a single hard-coded energy literal instead of comparing against the archived [energy.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1/energy.dat) file, so it is not the file-backed oracle test requested in [round-28-prompt.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-28-prompt.md#L97). The only negative test is [CorruptedVtuPointCountIsRejected](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_simulator.cpp#L65), which corrupts the VTU point payload, not the corrupted-mesh input called for in [round-28-prompt.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-28-prompt.md#L97). There is still no end-to-end AC-7 coverage for the per-step energy trajectory, the step-50 reaction force in [force.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1/force.dat), or the step-25/step-50 displacement state requested in [round-28-prompt.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-28-prompt.md#L103).

3. `task3f` is still incomplete, so Milestone 3 remains open and AC-9 is still blocked on the repeated-curvature principal branch. The tracker still records [task3f](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L96) as pending because every committed principal oracle case is `flag_num_diff=false`, exactly matching the required unfinished work in [round-28-prompt.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-28-prompt.md#L93). The Round 28 summary explicitly leaves that work pending in [round-28-summary.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-28-summary.md#L27), so the constitutive milestone cannot be treated as finished.

4. The round summary explicitly re-defers most of the original plan, despite the review prompt instructing Claude not to defer the remaining planned work. The unresolved solver, VTU, runtime vdW/self-contact, cyclic/crease/checkpoint, MPI-consistency, and documentation tasks remain pending in the tracker at [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L98), [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L103), and [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L108), and the summary restates those deferrals in [round-28-summary.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-28-summary.md#L27). That directly conflicts with the Round 28 instruction to continue through Milestones 5-8 after AC-7 in [round-28-prompt.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-28-prompt.md#L105). Concrete AC-13 evidence is still missing as well: [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) and [translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md) do not exist.

## Goal Alignment Check

- AC-1: met.
- AC-2: met.
- AC-3: met.
- AC-4: met.
- AC-5: met.
- AC-6: met.
- AC-7: advanced, not met. `task4a` is real, but [task4b](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L98) through [task4f](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L102) remain open and `crunch_it` is not a solver.
- AC-8: partial. Only the preprocessing slice is complete; runtime vdW/self-contact remains pending at [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L105).
- AC-9: partial. `task3f` remains open and Milestone 7 is untouched.
- AC-10: not met. [task7d](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L111) and [task7e](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L112) remain pending.
- AC-11: partial. Partitioned assembly exists, but the required multi-rank verification task [task8a](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L113) is still pending.
- AC-12: not met. [task5a](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L103) and [task5b](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L104) remain pending.
- AC-13: partial. The scaffold exists, but the required documentation artifacts are still absent and [task8c](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L115) remains pending.

No original-plan tasks are forgotten from the tracker: the outstanding work is still represented in the active-task table. I did not apply a goal-tracker edit because the tracker already reflects `task4a` as complete and the remaining tasks as pending; the problem is the incomplete implementation, not tracker drift.

## Required Implementation Plan

1. Close `task3f` before treating Milestone 3 as done. Extend the Fortran oracle generator to emit at least one archived-state case that actually triggers the repeated-curvature `flag_num_diff=true` branch, commit that fixture, and add a direct Fortran-vs-C++ test in [test_principal.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_principal.cpp).

2. Implement the missing Milestone 4 runtime modules explicitly. Add `include/fce/lbfgs.hpp` + `src/core/lbfgs.cpp`, `include/fce/load_runtime.hpp` + `src/core/load_runtime.cpp`, `include/fce/pasapas.hpp` + `src/core/pasapas.cpp`, and `include/fce/reaction.hpp` + `src/core/reaction.cpp`. Port the corresponding Fortran routines in the plan order from [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md#L224).

3. Replace the current VTU-inspection executable with a real solver driver. [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp) must read the archived `nano_*.dat` inputs, execute the translated `nCodeLoad=3` 50-step compression loop, and write `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat` rather than printing diagnostics for a pre-existing snapshot.

4. Upgrade the AC-7 verification from a unit slice to end-to-end integration. Parse the archived [energy.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1/energy.dat) and compare the full 50-step energy trajectory, parse [force.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1/force.dat) and compare the final reaction force, and compare the archived mid/final displacement state against the generated `nano_final_config.dat` or a committed step-25 dump. Add the required corrupted-mesh negative test against `nano_Mesh.dat`, not just a malformed VTU payload.

5. Translate the VTU writer after AC-7 lands. Add a dedicated ParaView output module for `mesh_config_XXXX.vtu` and `mesh_config_series.pvd`, then validate XML correctness plus field parity against the archived simulator outputs under [graphene_compression_simulator/np1](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1).

6. Implement runtime vdW and self-contact on top of the real solver path. Port the simulator-side `vdw_modules.f90` logic, including spatial binning and topological exclusions, integrate it into global assembly, and back it with oracle tests for energy, forces, and neighbor-list parity.

7. Implement Milestone 7 in one coherent slice after the solver and vdW are real. Port the cyclic boundary-condition controller, L-BFGS history resets, crease-memory state updates, crease analysis outputs, and checkpoint read/write paths, then validate them against the archived cyclic oracle case and restart behavior.

8. Finish Milestone 8 rather than leaving it queued. Add true multi-rank consistency tests for `np=1/2/4`, checkpoint compatibility checks across rank counts, create [AGENT.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/AGENT.md) and [translation_notes.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/translation_notes.md), and then run the full AC-wide integration/oracle suite.

## Verification

- `./build/unit_tests '--gtest_filter=SimulatorAssembly.*'`
- `./build/crunch_it test/cases/graphene_compression_simulator/np1 1`
- `./build/crunch_it test/cases/graphene_compression_simulator/np1 50`
- Additional local check: assembled all archived VTU steps `1..50` and confirmed the worst relative energy error versus archived `energy.dat` is about `1.8e-5`, so the new `task4a` slice itself appears numerically sound.
- `mpirun -np 2` could not be validated in this sandbox because OpenMPI failed to open sockets with `Operation not permitted`, so there is still no direct multi-rank execution evidence for AC-11.
