# Round 21 Review

Mainline Progress Verdict: PARTIAL

Goal Alignment Summary:
`ACs: 9/13 addressed | Forgotten items: 0 | Unjustified deferrals: 0`

## Findings

1. The new `archived_compression_np1` corpus is built from the wrong simulator file, so it does not provide the final-state or load-step archived evidence Claude claims. In [dump_archived_constitutive_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_archived_constitutive_oracle.f90#L139), `config_path` is hard-coded to `nano_config.dat`, and the helper then loads that file via `read_config(...)` at [dump_archived_constitutive_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_archived_constitutive_oracle.f90#L157). The frozen Fortran simulator clearly treats `nano_config.dat` as the input state and writes the final state to `nano_final_config.dat`: see [Optim.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/../finite_crystal_elasticity/grapheneCompressionOriginVersion/Optim.f90#L57), [Optim.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/../finite_crystal_elasticity/grapheneCompressionOriginVersion/Optim.f90#L179), [Optim.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/../finite_crystal_elasticity/grapheneCompressionOriginVersion/Optim.f90#L426), and [read.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/../finite_crystal_elasticity/grapheneCompressionOriginVersion/read.f90#L317). The new provenance text is therefore false where it says the fixtures come from “final-state simulator outputs” while also naming `nano_config.dat` as the source in [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md#L84). The committed fixtures themselves show the consequence: [case_01.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/archived_compression_np1/case_01.dat#L2) is the undeformed identity/zero-curvature/zero-eta state, and inspection of all ten fixtures shows the same pattern. That matches the flat input coordinates in [nano_config.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1/nano_config.dat#L5), not the deformed output in [nano_final_config.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1/nano_final_config.dat#L5). This means Round 21 does not close `task3b` or `task3d`, and the requested tracker change removing the archived-provenance blocker must be rejected.

2. The round still defers plan-required work, so the implementation remains materially incomplete against the original plan. The active tracker still has `task3e` through `task8d` pending in [goal-tracker.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/.humanize/rlcr/2026-03-30_08-11-11/goal-tracker.md), and the simulator entry point is still the stub in [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1). Claude’s summary explicitly carries those tasks forward instead of completing them. That is an honest status report, but it is still incomplete work under [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md). Claude needs to execute the implementation plan below rather than updating the tracker to imply the constitutive milestone is now unblocked.

3. The new archived regression leaves one entire bond direction unvalidated. In [test_element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_state.cpp#L531), the archived test only iterates `i < 2` when comparing `prepared_bonds.Ei`, so `Ei[2]` is never checked even though the fixture stores all three rows and the prepared-bond path is supposed to validate all three bond directions. A defect localized to the third bond orientation would currently pass this test.

## Goal Alignment Check

- AC-1 to AC-4 remain met from earlier rounds.
- AC-5 remains partial. The standalone Brenner translation still exists, but `task3b` and `task3e` remain open, and the Round 21 oracle update does not add valid archived geometry/bond evidence.
- AC-6 remains partial. Round 21 attempts to improve provenance, but the new corpus is sourced from `nano_config.dat` and degenerates to trivial undeformed states, so the archived-state requirement is still not satisfied.
- AC-7 remains not met. `ener_elem`, assembly, L-BFGS, load control, `pasapas`, reaction-force extraction, and the serial oracle run are still absent.
- AC-8 remains partial. Only the preprocessor-side vdW work is complete.
- AC-9 and AC-10 remain not met.
- AC-11 remains partial. MPI wrapper utilities exist, but there is still no solver path to validate multi-rank parity.
- AC-12 remains not met.
- AC-13 remains partial because the documentation milestone is still incomplete.

Forgotten items: none. Every original-plan task still appears in the tracker.

Deferred items: none are formally recorded in the tracker, but Claude’s summary still pushes `task3e` and everything downstream out of this round. Those are blockers, not acceptable completions.

Plan evolution: the requested Round 21 tracker evolution is not valid because it rests on a false archived-provenance claim.

## Goal Tracker Update Request

Rejected.

- Do not update `task3b` or `task3d` to claim archived simulator-derived constitutive evidence exists. The committed helper and fixtures currently read `nano_config.dat`, not `nano_final_config.dat`, and the resulting corpus stays at the undeformed initial state.
- Do not remove the Milestone 3 blocker text about missing geometry/bond oracle fixtures and archived-state Newton provenance. That blocker still exists after this round.
- No goal-tracker changes were applied.

## Required Implementation Plan

1. Fix the archived constitutive provenance before touching the tracker again. Update [dump_archived_constitutive_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_archived_constitutive_oracle.f90) and [test_element_state.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_state.cpp) so the archived geometry/bond fixtures are rebuilt from `nano_final_config.dat` for the final-state slice, not `nano_config.dat`. Regenerate `test/cases/constitutive_oracle/archived_compression_np1/`, rewrite [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/constitutive_oracle/build_provenance.md), and add explicit assertions that the archived states are nontrivial so this regression cannot silently collapse back to the undeformed input.

2. Capture real archived load-step Newton states instead of pretending final-state input is enough. The frozen simulator has the live `x0` and `eta` arrays inside the load-step loop in [pasapas.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/../finite_crystal_elasticity/grapheneCompressionOriginVersion/pasapas.f90#L115) and synchronizes `eta` before per-step output at [pasapas.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/../finite_crystal_elasticity/grapheneCompressionOriginVersion/pasapas.f90#L165). Use that path to archive ten distinct `(load step, element, gauss)` constitutive states from the compression oracle, commit them under `test/cases/graphene_compression_simulator/np1/`, and regenerate the AC-6 fixtures from those archived states. The final corpus must not be ten copies of the undeformed initialization.

3. Close the remaining constitutive milestone in plan order. Add `include/fce/element_energy.hpp` and `src/core/element_energy.cpp`, and translate [ener_elem.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/../finite_crystal_elasticity/grapheneCompressionOriginVersion/ener_elem.f90#L1) directly. Reuse the existing `geometry`, `principal`, `element_state`, `exponential`, and `constitutive` kernels; do not duplicate formulas. Add focused tests that compare per-Gauss-point energy, `eta`, stress terms, and nodal force contributions against the archived Fortran fixtures.

4. Build the simulator-side assembly path immediately after `ener_elem`. Add `include/fce/energy.hpp` and `src/core/energy.cpp` for rank-local element loops, persistent `eta` state updates, force accumulation, and MPI reduction. Verify that serial assembly equals the sum of the per-element oracle fixtures and that each element is owned exactly once under the current MPI partitioning utilities.

5. Replace the simulator stub with the Milestone 4 solver path in the order required by the plan: translate `lbfgs.f`, then the simulator-side load controller for `nCodeLoad=3`, then `pasapas`, then reaction-force extraction, then the serial oracle comparison. Do not stop after helper-level kernel work. [main.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/src/simulator/main.cpp#L1) should not remain a stub after the next implementation slice.

6. Finish the remaining milestones instead of carrying forward more deferrals. After the serial solver path is real, implement VTU output, runtime vdW/self-contact, cyclic loading, crease memory, checkpoint/restart, multi-rank parity, and the AC-13 documentation deliverables in the original plan order. The pass condition is still the full acceptance criteria in [plan.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/document/plan.md), not another narrow constitutive refactor.

## Verification

- `ctest --test-dir build --output-on-failure -R 'ElementState\\.MatchesArchivedCompressionSimulatorOracleFixtures'`
- Result: pass `1/1`

Round 21 remains incomplete.
