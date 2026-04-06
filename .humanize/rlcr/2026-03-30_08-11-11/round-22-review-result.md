# Round 22 Review

Mainline Progress Verdict: STALLED

Goal Alignment Summary:
`ACs: 9/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Mainline Gaps

1. Round 22 did not implement any of the original-plan work, so the mainline remains stalled at the same Milestone 3/Milestone 4 boundary as Round 21. [round-22-summary.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-22-summary.md#L3) still contains placeholder bullets for work, files, validation, and remaining items, and [round-22-summary.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/round-22-summary.md#L18) explicitly says no implementation was performed. The tracker still has [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L82) through [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L107) pending, the simulator is still the stub in [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1), and the core build in [CMakeLists.txt](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L39) through [CMakeLists.txt](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L79) still has no `element_energy`, `energy`, `lbfgs`, `pasapas`, `reaction`, runtime vdW, checkpoint, crease, or VTU modules. This round did not advance any acceptance criterion beyond prior status.

2. The archived constitutive corpus is still built from the wrong simulator state, so `task3b` and `task3d` remain blocked exactly as in Round 21. [dump_archived_constitutive_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_archived_constitutive_oracle.f90#L139) through [dump_archived_constitutive_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_archived_constitutive_oracle.f90#L157) still hard-code `nano_config.dat` and read it via `read_config(...)`. The provenance text in [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L92) through [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L99) still claims final-state archived evidence, but the emitted fixture [case_01.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/archived_compression_np1/case_01.dat#L2) through [case_01.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/archived_compression_np1/case_01.dat#L20) is the undeformed identity/zero-curvature/zero-eta state. That matches the flat input coordinates in [nano_config.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1/nano_config.dat#L4), not the deformed output in [nano_final_config.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1/nano_final_config.dat#L4). The Round 21 blocker remains unresolved.

## Blocking Side Issues

1. The archived regression still leaves one bond direction unvalidated. In [test_element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_state.cpp#L531) through [test_element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_state.cpp#L547), the archived-state test only iterates `i < 2` when comparing `prepared_bonds.Ei`, so `Ei[2]` is never checked. The direct unit test still passes under `./build/unit_tests --gtest_filter=ElementState.MatchesArchivedCompressionSimulatorOracleFixtures`, which means the current regression can still miss a defect localized to the third bond orientation.

## Queued Side Issues

1. The documentation milestone remains untouched: `AGENT.md` and `document/translation_notes.md` are still absent, and [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L106) through [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L118) correctly keep `task8c` open. This is valid follow-up work, but it must not displace the stalled Milestone 3/Milestone 4 mainline.

## Goal Alignment Check

- AC-1 to AC-4 remain met from earlier rounds.
- AC-5 remains partial: the standalone Brenner kernel exists, but the geometry/bond provenance and `ener_elem` integration required by the plan are still missing.
- AC-6 remains partial: the archived-state requirement is still unmet because the corpus is built from `nano_config.dat` rather than archived simulator load-step or final states.
- AC-7 remains not met: [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) is still a stub, and the Milestone 4 modules are still absent from [CMakeLists.txt](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L39) through [CMakeLists.txt](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/CMakeLists.txt#L79).
- AC-8 remains partial: only preprocessor-side vdW preprocessing is implemented; runtime vdW/self-contact remains blocked in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L109) through [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md#L113).
- AC-9 and AC-10 remain not met.
- AC-11 remains partial: MPI wrapper utilities exist, but there is still no solver path to compare `np=1`, `np=2`, and `np=4`.
- AC-12 remains not met.
- AC-13 remains partial: the documentation deliverables are still missing.
- Forgotten items: none. Every original-plan task is still present in the tracker.
- Deferred items: none are formally recorded in the tracker. The problem is execution stall, not tracker omission.
- Plan evolution: none. No new implementation change justifies a tracker evolution entry.

## Goal Tracker Update Request Assessment

No tracker change was requested, and no tracker correction is needed. The mutable section still matches the codebase: `task3a` through `task8d` remain pending, and the existing Milestone 3 / runtime-vdW blockers remain valid. I did not modify `goal-tracker.md`.

## Required Implementation Plan

1. Fix the archived constitutive provenance first. Update [dump_archived_constitutive_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_archived_constitutive_oracle.f90) to read `nano_final_config.dat` for the final-state geometry slice, regenerate `test/cases/constitutive_oracle/archived_compression_np1/`, rewrite [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md), and add an explicit nontrivial-state assertion so the corpus cannot silently collapse back to the undeformed input.

2. Capture the AC-6 Newton states from the actual Fortran load-step loop in `pasapas.f90`, commit ten distinct `(load step, element, gauss)` archived simulator states under `test/cases/graphene_compression_simulator/np1/`, and rebuild the Newton provenance corpus from those archived states instead of helper-synthetic inputs.

3. Close Milestone 3 in plan order. Add `include/fce/element_energy.hpp` and `src/core/element_energy.cpp` as a direct translation of `ener_elem.f90`, reusing `geometry`, `principal`, `element_state`, `exponential`, and `constitutive` rather than duplicating formulas. Extend [test_element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_state.cpp) to validate all three `Ei` rows and add per-Gauss-point energy/force oracle coverage for the new element kernel.

4. Immediately follow with simulator-side assembly: add `include/fce/energy.hpp` and `src/core/energy.cpp` for rank-local element loops, persistent `eta` updates, force accumulation, and MPI reduction. Verify that serial assembly equals the sum of the element fixtures and that each element is owned exactly once under the current partitioning utilities.

5. Replace the simulator stub with the Milestone 4 solver path in plan order: translate `lbfgs.f`, then the simulator-side `nCodeLoad=3` controller, then `pasapas`, then reaction-force extraction, then the serial oracle comparison. Do not stop after helper-level kernel work; `crunch_it` must run the archived `nano_*.dat` case and reproduce AC-7.

6. After the serial solver path is real, finish the remaining original-plan milestones in order: VTU output, runtime vdW/self-contact, cyclic loading, crease memory, checkpoint/restart, multi-rank parity, and the AC-13 documentation deliverables.

## Verification

- `git status --short`
- `git log --oneline --decorate -n 12`
- `ctest --test-dir build -N`
- `./build/unit_tests --gtest_filter=ElementState.MatchesArchivedCompressionSimulatorOracleFixtures`
