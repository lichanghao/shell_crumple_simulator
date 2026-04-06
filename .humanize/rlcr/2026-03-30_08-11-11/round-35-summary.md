# Round 35 Summary

## Work Completed
- Re-read the plan, goal tracker, Round 35 review, and `.humanize/bitlesson.md` before touching the AC-7 mainline.
- Tried to run `bitlesson-select.sh` for the round task, but the model-backed selector invocation hung in this shell; I proceeded with the already-loaded relevant lessons and recorded the new lesson manually at the end of the round.
- Added a deterministic imperfection replay contract for the runtime solver:
  - `load_simulator_input()` now reads an optional case-local `imperfection_trace.dat`
  - `SimulatorInput` now carries the loaded per-step scalar sequence
  - `apply_imperfections()` now consumes the injected per-step scalar when present and only falls back to entropy-backed sampling when no trace file exists
  - the runtime now rejects a short trace instead of silently mixing traced and random steps
- Added integration coverage for the new contract:
  - repeated `crunch_it <case> 1` runs with a full injected trace must produce byte-identical `energy.dat`, `force.dat`, `output.dat`, and `nano_final_config.dat`
  - a too-short trace must fail fast
- Added a checked-in deterministic trace artifact for the archived compression runtime case:
  - `test/cases/graphene_compression_simulator/np1/imperfection_trace.dat`
  - this is an injected replay aid for the C++ archived-oracle harness, not a frozen Fortran output artifact
- Reran the archived-oracle executable comparison under the fixed trace and captured the now-stable remaining AC-7 mismatch.

## Files Changed
- `include/fce/simulator.hpp`
- `src/core/simulator.cpp`
- `src/core/solver.cpp`
- `test/integration/test_e2e_compression.cpp`
- `test/cases/graphene_compression_simulator/np1/imperfection_trace.dat`
- `test/cases/README.md`
- `.humanize/rlcr/2026-03-30_08-11-11/round-35-summary.md`

## Validation
- `cmake --build build --target unit_tests integration_tests crunch_it -j4`
  - PASS
- `./build/unit_tests --gtest_filter='LoadController.*:SimulatorAssembly.StatefulAssemblyUsesRuntimeEtaInsteadOfInitialConfig'`
  - PASS (`3/3`)
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItReusesRecordedImperfectionTraceDeterministically:E2ECompression.CrunchItRejectsShortImperfectionTrace'`
  - PASS (`2/2`)
- Repeated fresh step-1 probes on fresh copies of `test/cases/graphene_compression_simulator/np1`
  - PASS: both runs produced identical step-1 energy `5.74298201e-05`
- Repeated fresh 3-step probes on fresh copies of `test/cases/graphene_compression_simulator/np1`
  - PASS: `cmp` reported identical `energy.dat` files across both runs
  - deterministic early trajectory:
    - step 1 `5.74298201e-05`
    - step 2 `7.47995924e-05`
    - step 3 `1.44786817e-04`
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts'`
  - FAIL after `625216 ms` (~10.4 min), but the mismatch is now stable instead of run-dependent
  - first three energy rows are now deterministic:
    - actual step 1 `5.74298201e-05` vs oracle `5.72105277e-05` (relative error `3.8330724722555058e-03`)
    - actual step 2 `7.47995924e-05` vs oracle `1.03739788e-04` (relative error `2.7896911686441628e-01`)
    - actual step 3 `1.44786817e-04` vs oracle `5.91982000e-05` (relative error `1.4457976256034815`)
  - the late trajectory still diverges heavily; the deterministic run finishes at step 50 with `7.77787e-04`
  - final configuration still fails badly, especially in the tail z-coordinates

## Remaining Items
- AC-7 remains open. The round removed the stochastic evidence blocker, but the runtime trajectory itself is still wrong after the first increment.
- The next mainline debugging target is no longer “make the mismatch reproducible”; it is “reconcile deterministic pasapas/minimization/reaction semantics against the archived Fortran trajectory.”
- AC-12 and Milestones 6-8 remain untouched original-plan scope.

## Goal Tracker Update Request

### Requested Changes:
- Add a Round 35 plan-evolution entry noting that AC-7 replay now supports an injected per-step imperfection trace through `imperfection_trace.dat`, making archived-oracle runtime evidence reproducible.
- Update `task4d` notes to record that the stochastic blocker is removed on the C++ side:
  - repeated step-1 and 3-step executable probes are now byte-stable with the injected trace
  - the deterministic early trajectory is `5.74298201e-05`, `7.47995924e-05`, `1.44786817e-04`
  - step 1 is closer to the oracle than the previous stochastic runs, but the trajectory still diverges badly from step 2 onward
- Update `task4f` notes to record the new deterministic full archived-oracle evidence:
  - full 50-step executable-path replay still fails
  - the first failing energy row is now stably step 1 at relative error `3.8330724722555058e-03`
  - later energy rows, force rows, and final configuration still diverge heavily
- Resolve or downgrade the Round 34 blocking issue about stochastic imperfection evidence, replacing it with a narrower remaining blocker about deterministic runtime trajectory mismatch after the randomness contract is fixed.
- Add a new note that the checked-in `imperfection_trace.dat` under the archived compression simulator case is a C++ replay artifact, not a frozen Fortran oracle output.

### Justification:
- The tracker should distinguish the now-resolved reproducibility blocker from the still-open solver-trajectory blocker.
- The new deterministic trace contract gives future rounds stable runtime evidence and removes the ambiguity that previously invalidated row-by-row claims.
- Recording that the trace file is a replay aid avoids overstating provenance while still documenting the exact contract the current archived-oracle harness depends on.

## BitLesson Delta
- Action: add
- Lesson ID(s): BL-20260406-imperfection-trace-contract
- Notes: The new lesson captures the rule that stochastic archived-oracle replay needs an injectable full-length per-step trace contract, with explicit rejection of short traces.
