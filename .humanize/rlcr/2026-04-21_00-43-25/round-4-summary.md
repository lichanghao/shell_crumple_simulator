# Round 4 Summary

## Work Completed

- Fixed the cyclic replay Fortran oracle extractor so it now respects the archived runtime contract in `nano_general.dat`:
  - when `nW_hat=0`, it keeps `eta` fixed and does not force `newton_inner`
  - when `nW_hat=1`, it still runs the guarded inner-relaxation path and fails hard on nonzero `fail_mode`
- Regenerated [replay_step1_accepted_2_element3200_full_oracle.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/replay_step1_accepted_2_element3200_full_oracle.dat) from the corrected extractor, so the committed oracle now represents the actual archived accepted-state-2 cyclic runtime surface instead of an unconditional inner-relaxation helper path.
- Updated [build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/build_provenance.md) so it no longer contradicts the committed artifact and now documents the `nW_hat`-sensitive contract explicitly.
- Added a direct C++ parser/regression in [test_element_energy.cpp](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/unit/test_element_energy.cpp) that loads the committed cyclic element-3200 oracle artifact and verifies the current C++ `compute_element_energy(...)` result matches it exactly on the accepted-state-2 replay surface.

## Files Changed

- `test/cases/tools/dump_cyclic_replay_element_oracle.f90`
- `test/cases/graphene_cyclic_crumple/build_provenance.md`
- `test/cases/graphene_cyclic_crumple/replay_step1_accepted_2_element3200_full_oracle.dat`
- `test/unit/test_element_energy.cpp`
- `.humanize/rlcr/2026-04-21_00-43-25/round-4-summary.md`

## Validation

- Rebuilt and reran the corrected Fortran extractor against the canonical Fortran simulator sources; it regenerated the accepted-state-2 element-3200 oracle artifact successfully with the archived `nW_hat=0` contract.
- Rebuilt `unit_tests`:
  - `cmake --build build --target unit_tests -j4`
- Ran the new focused cyclic oracle regression:
  - `./build/unit_tests --gtest_filter='ElementEnergy.CyclicReplayAcceptedStateTwoElement3200MatchesFortranOracle'`
  - Result: PASS
- Re-ran the stable related element-energy oracle tests together with the new cyclic oracle regression:
  - `./build/unit_tests --gtest_filter='ElementEnergy.FElemMatchesFortranOracle:ElementEnergy.FlagNumDiffStressesMatchFortranOracle:ElementEnergy.CyclicReplayAcceptedStateTwoElement3200MatchesFortranOracle'`
  - Result: 3/3 PASS

## Remaining Items

- `GT-AC1` remains open. The accepted-state-2 element-3200 oracle is now committed and exercised from C++, but the main executable-path cyclic replay acceptance test is still red/timing out and the runtime constrained-step fix has not landed yet.
- `GT-AC2`, `GT-AC3`, `GT-AC4`, and runtime `GT-AC5` remain open; no runtime vdW/self-contact, cyclic restart acceptance, or MPI parity implementation landed in this round.
- The next round should use the now-committed and C++-consumed accepted-state-2 element oracle plus the accepted-state-2/3 replay fixtures to fix the live cyclic constrained-step replay lane instead of doing more oracle-surface cleanup.

## BitLesson Delta

Action: none
Lesson ID(s): NONE
Notes: Re-read `.humanize/bitlesson.md` before the work. `bitlesson-selector` was not available in the current shell environment, so there was no selector output or new BitLesson entry for this round.

## Goal Tracker Update Request

### Requested Changes:
- Update the existing cyclic accepted-state-2 open issue to note that the element-3200 oracle artifact is now both reproducible and exercised by a passing C++ regression, while the main executable-path cyclic replay acceptance target remains red.

### Justification:
- This is concrete acceptance-surface progress for `GT-AC1`: the element-level replay contract is no longer just committed evidence on disk; it is now consumed and verified from the C++ test suite. That sharpens the remaining work to the live runtime replay/output path.
