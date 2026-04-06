# Round 10 Summary

## Work Completed
- Archived a committed multi-sheet `nvdw=1` preprocessor oracle under `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/`, including `nano_vdw.dat`, `nano_tub_loc.dat`, and a committed `ghost_coords.dat` artifact plus `test/cases/graphene_bilayer_twist_vdw_1000/build_provenance.md`.
- Added archive-backed integration coverage for the twist local-density path:
  - `PreprocessorOracle.ArchivedBilayerTwistVdw1000PreproInputMatchesOracleOutputs`
  - `PreprocessorOracle.BilayerTwistVdw1000ForcesNborderOverrideToTwo`
  - extended `ArchivedCasesIncludeGhostCoordinateArtifacts` to require the committed bilayer-twist `ghost_coords.dat`
- Ported the missing twist-path preprocessor behavior from the Fortran oracle into `src/core/preprocessor.cpp`:
  - second-sheet rotation for `nCodeLoad=222/333/1000` on both `x0` and `xg`
  - exact `read_data.f90` ordering for the `nborder` override (`xlength` scaling first, forced `nborder=2` afterwards)
  - exact Fortran auxiliary-mesh dimensions for the bilayer ghost mesh (`xlength/ncol*(ncol+1)`, `ylength/ncol*(nrow+1)`)
  - exact total-mesh `neigh_elem` offset behavior during multi-sheet merge, including the zero-entry offset quirk baked into the archived `nano_Mesh.dat`
- Fixed two archive-compatibility gaps that only showed up on the bilayer twist oracle:
  - non-cyclic `nano_BCs.dat` archives that stop after `BCs%value` are now accepted by `read_bcs()` with the expected non-cyclic defaults
  - the code-1000 vdW density path now matches the archived Fortran buffer behavior by using the pre-ghost-node `x0` buffer, the widened-single `PI` constant from `Prepro.f90`, and the sheet-2 `xg(numno+1:...)` tail compatibility slice before writing `nano_vdw.dat`
- Added BitLesson `BL-20260330-bilayer-twist-density-buffer` to capture the non-obvious code-1000 archive-compatibility rules.

## Files Changed
- `.humanize/bitlesson.md`
- `.humanize/rlcr/2026-03-30_08-11-11/round-10-summary.md`
- `src/core/io.cpp`
- `src/core/preprocessor.cpp`
- `src/core/vdw_preprocessor.cpp`
- `test/cases/README.md`
- `test/cases/graphene_bilayer_twist_vdw_1000/build_provenance.md`
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/data.dat`
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/ghost_coords.dat`
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/nano_BCs.dat`
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/nano_Mesh.dat`
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/nano_config.dat`
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/nano_dims.dat`
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/nano_general.dat`
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/nano_tub_loc.dat`
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/nano_vdw.dat`
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/nano_zero.dat`
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/prepro.log`
- `test/integration/test_prepro_oracle.cpp`

## Validation
- `cmake --build build --target integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^(PreprocessorOracle\.(ArchivedBilayerTwistVdw1000PreproInputMatchesOracleOutputs|BilayerTwistVdw1000ForcesNborderOverrideToTwo))$'`
  - initial RED: failed on `nano_dims.dat`, then `nano_zero.dat`, then `nano_Mesh.dat`, then `nano_vdw.dat`
  - final GREEN: PASS (`2/2`)
- `ctest --test-dir build --output-on-failure`
  - PASS (`42/42`)
- Additional root-cause verification during debugging:
  - tiny standalone gfortran reproduction confirmed that `REAL(8), PARAMETER :: PI = 3.1415926` in `Prepro.f90` widens a default-real literal, so the effective twist constant is `static_cast<double>(3.1415926f)`
  - instrumented throwaway Fortran builds in `/tmp` confirmed the archived code-1000 density path uses the exact auxiliary-mesh span formulas and the sheet-2 `x0` tail compatibility slice described above

## Remaining Items
- `task2g` remains partial even after this round. The archived bilayer local-density preprocessor branch is now covered, but the broader runtime vdW / self-contact scope is still unfinished.
- The larger simulator milestones remain open: constitutive kernels, solver mainline, runtime vdW, cyclic runtime controller, checkpoint/restart, MPI equivalence, VTU output, and final documentation.
- `AGENT.md` and `document/translation_notes.md` are still missing, so the documentation acceptance criteria remain partial.

## Goal Tracker Update Request

### Requested Changes:
- Add a Plan Evolution note that Round 10 closed the queued AC-3 / `task2g` preprocessor oracle gap for the multi-sheet twist local-density branch by committing `graphene_bilayer_twist_vdw_1000` and wiring it into the archive-backed oracle suite.
- Update the queued twist-preprocessor side issue to note that the two specific preprocessor parity gaps identified by the Round 9 review are now closed:
  - forced `nborder >= 2` for `nCodeLoad=222/1000`
  - second-sheet rotation before `Def_Grad` and ghost-mesh generation
- Add Round 10 evidence references:
  - `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/`
  - `test/cases/graphene_bilayer_twist_vdw_1000/build_provenance.md`
  - `PreprocessorOracle.ArchivedBilayerTwistVdw1000PreproInputMatchesOracleOutputs`
  - `PreprocessorOracle.BilayerTwistVdw1000ForcesNborderOverrideToTwo`
  - full-suite `42/42` pass
- Keep `task2g` overall status as pending, but narrow the remaining blocker language to the unfinished runtime vdW / self-contact / simulator scope rather than the now-closed preprocessor oracle gaps.

### Justification:
Round 10 closes the specific twist preprocessor parity work the stop-gate review asked for. The C++ preprocessor now reproduces a committed multi-sheet `nvdw=1` code-1000 archive case end-to-end, including the twist geometry, forced border rows, mesh metadata quirks, and the archived local-density payload. That materially advances `task2g`, even though the broader runtime vdW milestone is still open.

## BitLesson Delta
- Action: add
- Lesson ID(s): BL-20260330-bilayer-twist-density-buffer
- Notes: The new lesson records the archive-compatibility details that make the bilayer code-1000 preprocessor path pass: the widened-single twist `PI`, the exact auxiliary-mesh span formulas, the no-extra-`ghost_nodes()` density path, and the sheet-2 `xg(numno+1:...)` tail compatibility slice.
