# Round 6 Summary

## Work Completed
- Committed `901bea8` (`Archive ghost coordinate oracle evidence`) and `06fb40b` (`Tighten ghost oracle tolerance`).
- Added committed archived ghost-coordinate artifacts for the compression and cyclic preprocessor oracle cases: `test/cases/graphene_compression_prepro/ghost_coords.dat` and `test/cases/graphene_cyclic_crumple/prepro_run/ghost_coords.dat`.
- Generated those artifacts from the canonical Fortran `ghost_nodes` routine in `grapheneCompressionOriginPrePro/connect_mesh.f90` and documented the provenance in `test/cases/graphene_compression_prepro/build_provenance.md`.
- Reworked the preprocessor oracle comparator so the oracle side reads archived `ghost_coords.dat` directly instead of regenerating ghost positions in C++. The actual side still computes ghost nodes from current outputs by default, but accepts an explicit `ghost_coords.dat` override for negative-regression coverage.
- Added direct positive and negative integration coverage around the ghost-coordinate archive path:
  - assert the archived cases include `ghost_coords.dat`
  - reject a deliberately corrupted generated ghost-coordinate artifact
  - keep the existing wrong-anchor and corrupted-mesh regressions
- Tightened the positive preprocessor oracle comparisons to `1e-12`, matching the AC-4 ghost-position tolerance.
- Fixed a pre-existing reader bug exposed by the new direct oracle path: `read_mesh()` now derives `Mesh::numnods` from parsed connectivity, preventing `ghost_nodes()` from overwriting real-node coordinates when working from disk-loaded meshes.

## Files Changed
- `src/core/io.cpp`
- `test/cases/README.md`
- `test/cases/graphene_compression_prepro/build_provenance.md`
- `test/cases/graphene_compression_prepro/ghost_coords.dat`
- `test/cases/graphene_cyclic_crumple/prepro_run/ghost_coords.dat`
- `test/integration/test_prepro_oracle.cpp`
- `test/support/oracle_compare.cpp`
- `test/unit/test_io.cpp`

## Validation
- `cmake --build build --target integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^PreprocessorOracle'` -> initial RED (`2/7` failures) before implementation, proving the missing archive-backed ghost path
- `cmake --build build --target unit_tests integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^(ReadMesh\\.GrapheneCompression|PreprocessorOracle)'` -> PASS (`8/8`) after the direct archive path and `read_mesh()` fix
- `cmake --build build --target integration_tests -j4 && ctest --test-dir build --output-on-failure -R '^PreprocessorOracle' && ctest --test-dir build --output-on-failure` -> PASS (`7/7` targeted, `34/34` full suite) after tightening the positive oracle checks to `1e-12`

## Remaining Items
- `task2g` remains pending: the real `nvdw=1` preprocessing path, neighbor-list generation, shape functions, and `vdw_previous`-equivalent state are still not translated.
- AC-3 remains partial: the required 5 interior and 5 boundary Fortran B-spline oracle fixtures are still missing.
- Milestones 3 through 8 remain pending, including the simulator mainline, vdW runtime, cyclic runtime controller, checkpoint/restart, MPI equivalence, VTU output, and final documentation.

## Goal Tracker Update Request

### Requested Changes:
- Mark AC-4 as `MET`.
- Remove the blocking side issue stating that ghost-node acceptance coverage is still indirect because the comparator does not check archived ghost coordinates.
- Update the AC-4 evidence row to cite:
  - `test/cases/graphene_compression_prepro/ghost_coords.dat`
  - `test/cases/graphene_cyclic_crumple/prepro_run/ghost_coords.dat`
  - `test/support/oracle_compare.cpp` direct archive-backed ghost-coordinate comparison
  - `PreprocessorOracle.ArchivedCompressionCaseMatchesOracle` and `PreprocessorOracle.ArchivedCyclicPreproInputMatchesOracleOutputs` passing at `1e-12`
  - `PreprocessorOracle.CorruptedGeneratedGhostCoordinatesAreRejectedByOracleComparator`
  - full-suite `34/34` pass
- Add a Plan Evolution note that Round 6 closed the AC-4 evidence gap and, in the process, exposed and fixed a disk-reader bug where `read_mesh()` left `Mesh::numnods` unset.

### Justification:
AC-4 asked for archived ghost-node positions to match Fortran within `1e-12` with exact connectivity. Round 6 now compares the generated ghost positions directly against committed Fortran-derived `ghost_coords.dat` artifacts rather than synthesizing the oracle side in C++. The positive compression and cyclic oracle tests pass at `1e-12`, and the new negative regression proves the archive-backed comparison actually rejects a perturbed generated ghost coordinate. The prior blocker about indirect evidence is therefore resolved.

## BitLesson Delta
- Action: add
- Lesson ID(s): BL-20260330-read-mesh-numnods
- Notes: Round 6 exposed that a disk mesh reader can leave derived dimension fields unset and still look correct under shared-parser comparisons; archive-backed oracle artifacts are what surfaced the real bug.
