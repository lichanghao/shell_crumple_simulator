# Round 8 Summary

## Work Completed
- Committed `da8ad51` (`Add B-spline oracle fixtures`).
- Added a committed Fortran oracle fixture set under `test/cases/bspline_oracle/`:
  - `interior_01.dat` through `interior_05.dat`
  - `boundary_01.dat` through `boundary_05.dat`
- Added the in-repo reproduction helper `test/cases/tools/dump_bspline_oracle.f90`, which regenerates the fixture set from the frozen Fortran `BSpline.f90` source.
- Added `BSpline.MatchesCommittedFortranOracleFixtures`, which loads all 10 fixtures and checks `BSpline`, `DBSpline`, and `DDBSpline` at absolute tolerance `1e-14`.
- Added minimal provenance/docs for the new fixture set in `test/cases/bspline_oracle/README.md` and `test/cases/README.md`.

## Files Changed
- `test/cases/README.md`
- `test/cases/bspline_oracle/README.md`
- `test/cases/bspline_oracle/interior_01.dat`
- `test/cases/bspline_oracle/interior_02.dat`
- `test/cases/bspline_oracle/interior_03.dat`
- `test/cases/bspline_oracle/interior_04.dat`
- `test/cases/bspline_oracle/interior_05.dat`
- `test/cases/bspline_oracle/boundary_01.dat`
- `test/cases/bspline_oracle/boundary_02.dat`
- `test/cases/bspline_oracle/boundary_03.dat`
- `test/cases/bspline_oracle/boundary_04.dat`
- `test/cases/bspline_oracle/boundary_05.dat`
- `test/cases/tools/dump_bspline_oracle.f90`
- `test/unit/test_bspline.cpp`

## Validation
- `cmake --build build --target unit_tests -j4 && ctest --test-dir build --output-on-failure -R '^BSpline\\.MatchesCommittedFortranOracleFixtures$'` -> initial RED before fixture generation because `test/cases/bspline_oracle/interior_01.dat` did not exist
- `mkdir -p test/cases/bspline_oracle && gfortran -c -O0 -fallow-argument-mismatch ../finite_crystal_elasticity/grapheneCompressionOriginPrePro/BSpline.f90 -o /tmp/fortran_bspline.o && gfortran -O0 -fallow-argument-mismatch test/cases/tools/dump_bspline_oracle.f90 /tmp/fortran_bspline.o -o /tmp/dump_bspline_oracle && /tmp/dump_bspline_oracle test/cases/bspline_oracle` -> PASS (all 10 fixtures regenerated from the frozen Fortran source)
- `cmake --build build --target unit_tests -j4 && ctest --test-dir build --output-on-failure -R '^BSpline\\.'` -> PASS (`4/4`)
- `ctest --test-dir build --output-on-failure` -> PASS (`35/35`)

## Remaining Items
- `task2g` remains pending: the real `nvdw=1` preprocessing path, neighbor-list generation, shape functions, and `vdw_previous`-equivalent state are still not translated.
- Milestones 3 through 8 remain pending, including the simulator mainline, vdW runtime, cyclic runtime controller, checkpoint/restart, MPI equivalence, VTU output, and final documentation.
- `AGENT.md` and `document/translation_notes.md` are still missing, so AC-13 remains partial.

## Goal Tracker Update Request

### Requested Changes:
- Mark AC-3 as `MET`.
- Update the `task2c` completed/verified evidence row to cite:
  - `test/cases/bspline_oracle/`
  - `test/cases/tools/dump_bspline_oracle.f90`
  - `BSpline.MatchesCommittedFortranOracleFixtures`
  - full-suite `35/35` pass
- Add a Plan Evolution note that Round 8 closed the B-spline oracle-fixture gap with a committed Fortran fixture set plus an in-repo reproduction helper.
- Remove any blocker/open-issue language that says the interior/boundary Fortran B-spline fixtures are still missing.

### Justification:
Round 8 delivers the exact evidence gap that kept AC-3 open: five committed interior fixtures, five committed boundary fixtures, and a direct `1e-14` comparison of `BSpline`, `DBSpline`, and `DDBSpline` against the frozen Fortran outputs. The fixture generator is also committed, so the oracle data is reproducible rather than opaque.

## BitLesson Delta
- Action: none
- Lesson ID(s): NONE
- Notes: The round added a direct oracle fixture set and its reproduction helper, but it did not expose a new reusable failure pattern beyond the existing archive-provenance lessons.
