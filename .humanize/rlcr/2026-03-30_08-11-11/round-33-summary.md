# Round 33 Summary

## Work Completed
- Re-read the plan, goal tracker, Round 32 summary, Round 32 review, and wrote `round-33-contract.md` to keep the mainline on AC-7 runtime semantics.
- Fixed the shortened executable-path semantics in `src/simulator/main.cpp`:
  - the positional second argument is now treated as a stop-step limit, not as a replacement `BCs%nloadstep`
  - `crunch_it <case_dir> 3` now executes the first three steps of the canonical 50-step path and writes load parameters `0.02`, `0.04`, `0.06`
  - values larger than the file-loaded `BCs%nloadstep` are rejected
- Ported the missing imperfection step into `pasapas()` in the correct place:
  - the perturbation is applied after `load_doit(...)` and before `minimize_constrained(...)`
  - the constrained-DOF snapshot is no longer refreshed after imperfection injection, so BC positions remain the load-controlled values while the free-state guess is perturbed
  - used a deterministic scalar-per-step sequence so the archived-oracle regression remains reproducible
- Added direct regression coverage for live runtime `eta` state:
  - `SimulatorAssembly.StatefulAssemblyUsesRuntimeEtaInsteadOfInitialConfig` uses the committed `graphene_bilayer_twist_vdw_1000/prepro_run` case (`nW_hat=1`)
  - forces the inner solver to accept the provided eta immediately via a huge `crit_local`
  - proves the stateful assembly path reads and preserves `RuntimeState::eta` instead of silently reverting to `input.initial_config.eta`
- Closed the `nCodeLoad=3` reaction-side mapping bug in `LoadController::compute_reaction()`:
  - added `test/unit/test_load_controller.cpp`
  - `LoadController.ComputeReactionMatchesGetReacNCodeLoad3Semantics` exposed that the side tags were interpreted as if they were still 1-based
  - fixed the mapping so reaction side 1 corresponds to stored tag `0` and side 2 to stored tag `1`
- Added explicit unit coverage for the load-step denominator:
  - `LoadController.ApplyIncrementUsesStoredNloadstepDenominator` verifies `apply_increment()` still uses the file-loaded `BCs%nloadstep` denominator
- Inspected canonical `get_reac.f90` and confirmed the AC-7 `nCodeLoad=3` path has no torque term. Torque handling is required for other loading modes, but not for this archived compression oracle.

## Files Changed
- `CMakeLists.txt`
- `src/core/load_controller.cpp`
- `src/core/solver.cpp`
- `src/simulator/main.cpp`
- `test/unit/test_simulator.cpp`
- `test/unit/test_load_controller.cpp`
- `.humanize/bitlesson.md`
- `.humanize/rlcr/2026-03-30_08-11-11/round-33-contract.md`
- `.humanize/rlcr/2026-03-30_08-11-11/round-33-summary.md`

## Validation
- `cmake --build build --target unit_tests crunch_it integration_tests -j4`
  - PASS
- `./build/unit_tests --gtest_filter='LoadController.*:SimulatorAssembly.StatefulAssemblyUsesRuntimeEtaInsteadOfInitialConfig'`
  - PASS (`3/3`)
- `./build/unit_tests --gtest_filter='SimulatorAssembly.*:Lbfgs*'`
  - PASS (`9/9`) before the reaction-side unit-test addition
- Manual executable-path stop-step probe:
  - `./build/crunch_it <temp_copy>/np1 3`
  - PASS
  - load parameters written to `energy.dat` are `0.02`, `0.04`, `0.06`, confirming the denominator fix
  - early energies now move from the Round 32 values toward the archived oracle:
    - Round 32 step 1: `5.79788888e-05`
    - Round 33 step 1: `5.03028429e-05`
    - Oracle step 1: `5.72105277e-05`
    - Round 32 step 2: `5.91932754e-05`
    - Round 33 step 2: `8.50012013e-05`
    - Oracle step 2: `1.03739788e-04`
- `./build/integration_tests --gtest_filter='E2ECompression.CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts'`
  - FAIL after `754292 ms` (~12.6 min), but the correct archived-oracle executable-path regression still runs end-to-end
  - key current failures:
    - energy remains outside tolerance across the trajectory, though early steps are materially closer than in Round 32
    - reaction-force rows still diverge from the archived oracle
    - final configuration still diverges substantially in many coordinates
  - note: this full 50-step run was captured immediately before the final `LoadController::compute_reaction()` side-tag fix; the post-fix evidence is the passing unit test plus the post-fix 3-step executable probe

## Remaining Items
- AC-7 remains open: runtime solver semantics are still not Fortran-equivalent even after restoring the stop-step contract, imperfection placement, live-`eta` coverage, and the reaction-side fix.
- The deterministic scalar imperfection sequence is a reproducible surrogate for the missing runtime path, but it is not yet proven to be the exact archived-oracle source. The checked-in graphene `pasapas.f90` and the project notes still disagree on the imperfection implementation.
- VTU output (`task5a`-`task5b`), runtime vdW/self-contact (`task6a`-`task6c`), cyclic/crease/checkpoint work (`task7a`-`task7e`), MPI consistency (`task8a`-`task8b`), and documentation (`task8c`-`task8d`) are still pending original-plan work.

## Goal Tracker Update Request

### Requested Changes:
- Add a Round 33 plan-evolution entry noting that:
  - shortened executable probes now preserve the file-loaded `BCs%nloadstep` denominator and use only `iload_stop` to terminate early
  - the runtime path now includes a source-backed imperfection step in the canonical position
  - the new direct `nW_hat=1` test proves `RuntimeState::eta` is consumed by stateful assembly
  - `task4e` for AC-7 should be narrowed to the `nCodeLoad=3` force-summing path because `get_reac.f90` has no torque branch for that loading mode
- Update `task4d` notes to record that step ordering is closer to Fortran now, but AC-7 is still red on the archived executable-path oracle.
- Update `task4e` notes to record the reaction-side mapping bug fix plus the new unit coverage for `get_reac.f90` `nCodeLoad=3` semantics.
- Update `task4f` notes to record the Round 33 archived-oracle evidence:
  - full executable-path regression still fails
  - runtime improved from the Round 32 trajectory
  - final configuration and reaction outputs remain materially off
- Add the side-tag mapping bug as a resolved lesson-backed issue if the tracker keeps issue-level notes for AC-7 runtime mismatches.

### Justification:
- These changes keep the mutable tracker honest without overclaiming AC-7 closure.
- They distinguish resolved contract bugs (stop-step semantics, live eta usage, reaction side mapping) from the still-open solver trajectory mismatch.
- They also prevent `task4e` from staying artificially open for missing torque logic that does not exist on the archived `nCodeLoad=3` path.

## BitLesson Delta
- Action: add
- Lesson ID(s): BL-20260406-bc-side-tags-zero-based
- Notes: `BCData::mnodBC` side tags are already 0-based after I/O conversion. The `nCodeLoad=3` reaction code must treat stored tag `0` as Fortran side 1 and stored tag `1` as Fortran side 2.
