# Round 8 Summary

## Work Completed
- Refreshed the stale archived constitutive fixtures under
  `test/cases/constitutive_oracle/archived_compression_np1/` from the canonical Fortran helper output.
- Refreshed the stale archived element-energy oracle
  `test/cases/element_energy_oracle/archived_compression_np1/case_01.dat` from the canonical
  Fortran helper output.
- Updated `document/translation_notes.md` so it reflects the measured current state:
  - archived constitutive/kernel gates green
  - first-step replay gate green
  - first-step exact-state unit gate green
  - executable-path step-one regression still red

## Files Changed
- `document/translation_notes.md`
- `test/cases/constitutive_oracle/archived_compression_np1/case_01.dat` … `case_10.dat`
- `test/cases/element_energy_oracle/archived_compression_np1/case_01.dat`

## Validation
- Regenerated the archived constitutive fixtures with the canonical Fortran helper:
  - `/tmp/dump_archived_constitutive_oracle_round4 test/cases/graphene_compression_simulator/np1 /tmp/arch_const_refresh`
- Regenerated the archived element-energy oracle with the canonical Fortran helper:
  - `/tmp/dump_element_energy_oracle_round4 test/cases/graphene_compression_simulator/np1 /tmp/elem_energy_refresh`
- `./build/unit_tests --gtest_filter='ElementState.MatchesArchivedCompressionSimulatorOracleFixtures:ElementEnergy.MatchesArchivedCompressionSimulatorOracleFixtures:ElementEnergy.FElemMatchesFortranOracle:FirstConstrainedStepOracle.Element83UnitFixtureMatchesCommittedFortranOracle'`
  - Passed all four tests.
- `./build/integration_tests --gtest_filter='FirstConstrainedStepOracle.Element83ReplayMatchesCommittedFortranOracle:E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace'`
  - `FirstConstrainedStepOracle.Element83ReplayMatchesCommittedFortranOracle` passed.
  - `E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace` still failed with:
    - energy relative error `1.3407384932259225`
    - `GNORM` relative error `0.068245136318090996`
    - force-column relative errors `1.3407271675185646`, `0.91213791706456004`, `0.55602311548162575`

## Remaining Items
- The archived analytical/kernel blocker is closed for the committed archived fixture set.
- The remaining AC-7 blocker is now the executable-path step-one regression:
  - `E2ECompression.CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace`
- After that, the remaining original-plan runtime work is still open:
  - reaction-force parity / 50-step run
  - runtime vdW/self-contact
  - cyclic loading / crease memory
  - checkpoint/restart
  - MPI acceptance parity
  - executable-path real-`nvdw=1` VTU/PVD coverage
- `bitlesson-selector` remained unavailable in this environment (`zsh:1: command not found: bitlesson-selector`).

## Goal Tracker Update Request

### Requested Changes:
- Update `task3d` and `task3e` notes to reflect that the archived constitutive/kernel gates are now green against the refreshed canonical Fortran fixtures.
- Update `task4d` notes and the AC-7 open issue so the remaining blocker is specifically the executable-path step-one energy / `GNORM` / reaction-force mismatch, not the archived constitutive/kernel surface.
- Update `task8c` progress to reflect that `document/translation_notes.md` now matches the measured archived-kernel and first-step gate state.

### Justification:
- This round converts the archived constitutive/kernel red gates from ambiguous failure signals into green checks against refreshed canonical Fortran output. That meaningfully narrows the remaining AC-7 work to the executable path and keeps the documentation aligned with the measured repo state.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: No new reusable lesson was added; this round was about refreshing stale archived oracles from the canonical Fortran helpers and narrowing the remaining blocker surface.
