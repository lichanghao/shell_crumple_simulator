# Round 3 Summary

## Work Completed

- Hardened the new cyclic replay Fortran oracle extractor in [test/cases/tools/dump_cyclic_replay_element_oracle.f90](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/tools/dump_cyclic_replay_element_oracle.f90):
  - it now rejects coordinate-dump row-index mismatches instead of silently accepting reordered input
  - it now rejects nonzero `newton_inner` `fail_mode` instead of emitting an untrustworthy oracle artifact
  - it reconstructs ghost coordinates through the canonical Fortran `ghost_nodes` path before evaluating the target patch
- Added a committed provenance note at [test/cases/graphene_cyclic_crumple/build_provenance.md](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/build_provenance.md) documenting the exact scratch-build command used to compile and run the extractor against the canonical Fortran simulator sources.
- Generated and committed a source-backed accepted-state-2 element oracle artifact at [test/cases/graphene_cyclic_crumple/replay_step1_accepted_2_element3200_full_oracle.dat](/Users/changhaoli/github_projects/finite_crystal_elasticity_Cpp/test/cases/graphene_cyclic_crumple/replay_step1_accepted_2_element3200_full_oracle.dat).
- Used that hardened extractor/oracle path to confirm that the cyclic accepted-state-2 element-3200 constitutive/force surface still disagrees materially with the current C++ replay state. This does not close `GT-AC1`, but it upgrades the earlier heuristic probe into a committed source-backed debugging contract.

## Files Changed

- `test/cases/tools/dump_cyclic_replay_element_oracle.f90`
- `test/cases/graphene_cyclic_crumple/build_provenance.md`
- `test/cases/graphene_cyclic_crumple/replay_step1_accepted_2_element3200_full_oracle.dat`
- `.humanize/rlcr/2026-04-21_00-43-25/round-3-summary.md`

## Validation

- Rebuilt the hardened extractor against the canonical Fortran simulator sources in a scratch directory:
  - `gfortran -c headers.f90 BSpline.f90 Taylor.f90 gauss.f90 geometry.f90 ghost_nodes.f90 principal.f90 exponential.f90 morse.f90 mm3.f90 brenner.f90 brenner2.f90 Hyper_pot_inner_alg.f90 newton_inner.f90 dump_cyclic_replay_element_oracle.f90`
  - `gfortran -o dump_cyclic_replay_element_oracle ...`
- Ran the extractor successfully on the accepted-state-2 cyclic replay inputs and emitted `/tmp/replay_step1_accepted_2_element3200_full_oracle.dat`, then copied the result into the committed oracle path.
- Re-ran the stable existing element-energy oracle tests and kept them green:
  - `./build/unit_tests --gtest_filter='ElementEnergy.FElemMatchesFortranOracle:ElementEnergy.FlagNumDiffStressesMatchFortranOracle'`

## Remaining Items

- `GT-AC1` remains open. The main cyclic executable-path acceptance test is still red, and the new committed element-3200 oracle now shows that the accepted-state-2 cyclic constitutive/force surface still diverges materially from the current C++ replay state.
- `GT-AC2`, `GT-AC3`, `GT-AC4`, and runtime `GT-AC5` are still open; no runtime vdW/self-contact, restart acceptance, or MPI parity work landed in this round.
- The next round needs to use the committed accepted-state-2 element oracle plus the existing accepted-state-2/3 replay fixtures to fix the live cyclic constrained-step replay lane instead of adding more uncommitted diagnostics.

## BitLesson Delta

Action: none
Lesson ID(s): NONE
Notes: Re-read `.humanize/bitlesson.md` before the work. `bitlesson-selector` was not available in the current shell environment, so there was no selector output or new BitLesson entry for this round.

## Goal Tracker Update Request

### Requested Changes:
- Update the open issue added in Round 2 to note that the cyclic accepted-state-2 element-3200 extractor is now hardened and reproducible, and that the committed oracle artifact still disagrees materially with the current C++ accepted-state-2 replay surface.

### Justification:
- This is a concrete tightening of the existing cyclic debugging blocker: the extractor path is no longer “untrusted helper only”; it is now a documented, reproducible Fortran-side oracle surface that the next runtime fix can target directly.
