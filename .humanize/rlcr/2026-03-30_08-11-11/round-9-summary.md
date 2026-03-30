# Round 9 Summary

## Work Completed
- Archived a committed `nvdw=1` preprocessor oracle case under `test/cases/graphene_self_contact/prepro_run/`, including `nano_vdw.dat`, `nano_tub_loc.dat`, `nano_crease.dat`, and a committed `ghost_coords.dat` artifact plus `test/cases/graphene_self_contact/build_provenance.md`.
- Added real preprocessor-side parsing for the optional vdW block in `data.dat` (`ngauss_vdw`, cutoff/bond radii, LJ parameters, `meval`, optional `alpha_sharp`, and cyclic `nself_contact`) before the cyclic crease block.
- Expanded the C++ vdW preprocessing state in `include/fce/types.hpp`, added `include/fce/vdw_preprocessor.hpp` plus `src/core/vdw_preprocessor.cpp`, and used that path from `src/core/preprocessor.cpp` to:
  - compute `Vcut`,
  - generate vdW quadrature shape functions / weights,
  - initialize the single-sheet self-contact density branch (`rho = 2/s0`),
  - emit the extended vdW dims metadata,
  - write `nano_vdw.dat`,
  - and write `nano_tub_loc.dat` from computed `ngauss_vdw` for `nvdw=1`.
- Added `nano_vdw.dat` reader/writer support plus extended `nano_dims.dat` reader/writer support in `include/fce/io.hpp` and `src/core/io.cpp`.
- Extended the oracle comparator so `nvdw=1` cases compare the committed vdW payload directly (`nano_dims.dat` vdW fields, `nano_vdw.dat`, and `nano_tub_loc.dat`), not just the legacy disabled-vdW bridge files.
- Added unit/integration coverage for the new self-contact oracle:
  - `ReadDims.GrapheneSelfContact`
  - `ReadVdw.GrapheneSelfContact`
  - `RoundTrip.DimsSelfContact`
  - `RoundTrip.Vdw`
  - `PreprocessorOracle.ArchivedSelfContactPreproInputMatchesOracleOutputs`
- Added BitLesson `BL-20260330-data-dat-vdw-order` to capture the parser-order failure mode that initially suppressed both vdW and crease outputs.

## Files Changed
- `.humanize/bitlesson.md`
- `CMakeLists.txt`
- `include/fce/io.hpp`
- `include/fce/types.hpp`
- `include/fce/vdw_preprocessor.hpp`
- `src/core/io.cpp`
- `src/core/preprocessor.cpp`
- `src/core/vdw_preprocessor.cpp`
- `test/cases/README.md`
- `test/cases/graphene_self_contact/build_provenance.md`
- `test/cases/graphene_self_contact/prepro_run/data.dat`
- `test/cases/graphene_self_contact/prepro_run/ghost_coords.dat`
- `test/cases/graphene_self_contact/prepro_run/nano_BCs.dat`
- `test/cases/graphene_self_contact/prepro_run/nano_Mesh.dat`
- `test/cases/graphene_self_contact/prepro_run/nano_config.dat`
- `test/cases/graphene_self_contact/prepro_run/nano_crease.dat`
- `test/cases/graphene_self_contact/prepro_run/nano_dims.dat`
- `test/cases/graphene_self_contact/prepro_run/nano_general.dat`
- `test/cases/graphene_self_contact/prepro_run/nano_tub_loc.dat`
- `test/cases/graphene_self_contact/prepro_run/nano_vdw.dat`
- `test/cases/graphene_self_contact/prepro_run/nano_zero.dat`
- `test/cases/graphene_self_contact/prepro_run/prepro.log`
- `test/integration/test_oracle_roundtrip.cpp`
- `test/integration/test_prepro_oracle.cpp`
- `test/support/oracle_compare.cpp`
- `test/unit/test_io.cpp`

## Validation
- `cmake --build build --target integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^PreprocessorOracle\.ArchivedSelfContactPreproInputMatchesOracleOutputs$'` -> initial RED before the implementation; failed because `nano_vdw.dat` and `nano_crease.dat` were missing and `nano_tub_loc.dat` mismatched (`400` expected vs `9400`)
- `cmake --build build --target unit_tests integration_tests -j4` -> PASS
- `ctest --test-dir build --output-on-failure -R '^(ReadDims\.GrapheneSelfContact|ReadVdw\.GrapheneSelfContact|RoundTrip\.(DimsSelfContact|Vdw)|PreprocessorOracle\.ArchivedSelfContactPreproInputMatchesOracleOutputs)$'` -> PASS (`5/5`)
- `ctest --test-dir build --output-on-failure -R '^(Read.*|RoundTrip\..*|PreprocessorOracle\..*)$'` -> PASS (`25/25`)
- `ctest --test-dir build --output-on-failure` -> PASS (`40/40`)

## Remaining Items
- `task2g` is still partial. This round closes the single-sheet self-contact `nvdw=1` preprocessor branch and its archived oracle evidence, but the broader `vdw_previous` scope is still incomplete:
  - no archive-backed multi-sheet / twist `nvdw=1` preprocessor case is wired into the C++ test suite yet,
  - the `nCodeLoad=1000` atomic-density branch is implemented structurally but still unverified against a committed oracle,
  - and the larger runtime vdW / self-contact milestone remains unimplemented.
- Milestones 3 through 8 are still pending, including the constitutive kernels, solver mainline, runtime vdW, cyclic runtime controller, checkpoint/restart, MPI equivalence, VTU output, and final documentation.
- `AGENT.md` and `document/translation_notes.md` are still missing, so AC-13 remains partial.

## Goal Tracker Update Request

### Requested Changes:
- Add a Plan Evolution note that Round 9 advanced `task2g` by implementing the real `data.dat` vdW block parsing, adding a committed self-contact `nvdw=1` oracle case, emitting `nano_vdw.dat`, and switching `nano_tub_loc.dat` to computed `ngauss_vdw` spans on the `nvdw=1` path.
- Update `task2g` notes/evidence to cite:
  - `test/cases/graphene_self_contact/prepro_run/`
  - `test/cases/graphene_self_contact/build_provenance.md`
  - `include/fce/vdw_preprocessor.hpp`
  - `src/core/vdw_preprocessor.cpp`
  - `ReadDims.GrapheneSelfContact`
  - `ReadVdw.GrapheneSelfContact`
  - `RoundTrip.DimsSelfContact`
  - `RoundTrip.Vdw`
  - `PreprocessorOracle.ArchivedSelfContactPreproInputMatchesOracleOutputs`
  - full-suite `40/40` pass
- Keep `task2g` status as pending, but revise the blocking-side-issue language to reflect that there is now committed `nvdw=1` oracle coverage; the remaining blocker is the unverified broader multi-sheet / twist / runtime vdW scope, not the absence of any `nvdw=1` preprocessor evidence.

### Justification:
Round 9 materially advances the previously empty `nvdw=1` preprocessing path. The C++ preprocessor now parses the real optional vdW block from `data.dat`, writes the committed vdW payload, and matches a real Fortran self-contact oracle case end-to-end. That is not enough to close AC-8, but it does remove the “no committed `nvdw=1` oracle case” gap and narrows the remaining blocker to the larger unfinished vdW scope.

## BitLesson Delta
- Action: add
- Lesson ID(s): `BL-20260330-data-dat-vdw-order`
- Notes: The new lesson records that the optional `data.dat` vdW block must be parsed before the cyclic crease block; otherwise the parser silently misreads `ncrease`, suppresses `nano_vdw.dat`, and writes the wrong `nano_tub_loc.dat`.
