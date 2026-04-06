# Round 1 Summary

## Work Completed

### Blocking Issue Fixes (Milestone 0 defects)

**task0a-fix: Serial np=1 compression simulation archived**
- Re-ran the oracle simulator with `mpirun -np 1` in `/tmp/fce_np1_run/`
- Confirmed: `simulator.log` line 1 = "Numero de procesadores: 1"
- Final energy: 1.3427137479171509E-003 (load step 50, IFLAG=0)
- Archived 66 files to `test/cases/graphene_compression_simulator/np1/`
- Outputs: `energy.dat`, `force.dat`, `output.dat`, `simulator.log`, 51 VTU snapshots (steps 0–50), `nano_final_config.dat`, input `nano_*.dat` copies

**task0a-prov: Build provenance documented**
- Created `test/cases/graphene_compression_prepro/build_provenance.md`
- Documents: oracle commit `7d3f77fab2378d675d14ebeac3d8e65d94221a4f`, gfortran 15.2.0, OpenMPI 5.0.9, macOS arm64
- Includes all 9 source patches needed for compilation, preprocessor and simulator build commands, run commands for np=1 and np=8

**task0b-fix: Plan amended for nCodeLoad=31**
- Exhaustive oracle search confirmed: no nCodeLoad=30 files exist anywhere
- Amended `document/plan.md`: AC-9 now uses nCodeLoad=31 (biaxial cyclic) as primary cyclic oracle baseline
- Amended task4c: "Loading controller for nCodeLoad=3; stubs for nCodeLoad=30/31"
- Committed as `61bb9e2` (prior to this round)

### Milestone 1 Implementation

**task1a: CMake scaffold**
- `CMakeLists.txt`: C++17, `find_package(MPI REQUIRED)`, `find_package(Eigen3 REQUIRED NO_MODULE)` (no version constraint — installed 5.0.1 is incompatible with version requirement syntax), GoogleTest via FetchContent v1.14.0
- Targets: `fce_core` library (io.cpp + mpi_env.cpp), executables `PrePro` + `crunch_it`, test executables `unit_tests` + `integration_tests`
- Key fix: `ORACLE_DIR="${CMAKE_SOURCE_DIR}/test/cases"` compile definition for BOTH test targets (absolute path to avoid CWD issues)

**task1b+1c: Data types and nano_*.dat readers/writers (21/21 tests pass)**
- `include/fce/types.hpp`: all structs matching Fortran headers.f90 — `TriElement`, `Mesh`, `BCData`, `MatData`, `RefConfig`, `CreaseData`, `ConfigData`; using `Vec2/Vec3/Mat22`
- `include/fce/io.hpp`: full reader/writer declarations for nano_dims, nano_general, nano_zero, nano_config, nano_BCs, nano_Mesh, nano_tub_loc, nano_crease (stub)
- `src/core/io.cpp`: all reader/writer implementations with `fmt_d()` for Fortran D-exponent output, `FileReader` helper, `parse_fortran_double()`

Key discoveries and bugs fixed:
1. **is_label() separator bug**: Dash-separator lines (` ---------`) start with `-`, so original `is_label()` treated them as data → `stoi: no conversion`. Fix: detect `c == '-' && next == '-'` as label.
2. **BCs reader label-consumption bug**: After variable-length data loop `break`s on a label, the next search loop called `getline()` again and skipped the already-consumed label → SEGFAULT at EOF. Fix: Rewrote `read_bcs()` as a single sequential scan using `goto` labels to avoid re-searching.
3. **nano_zero.dat storage structure**: File stores ONE `RefConfig` per element (not per element×gauss). Fortran write loop is `do ielem = 1, meshT%numele` — no igauss loop. Changed `read_zero(path, numele, ngauss)` → `read_zero(path, numele)`.

**task1d: MPI wrapper**
- `include/fce/mpi_env.hpp`: `MpiEnv` RAII class, `element_partition(n, size, rank)`, `partition_for_rank(parts, rank)` declarations
- `src/core/mpi_env.cpp`: `MPI_Init_thread(MPI_THREAD_SINGLE)`, rank/size queries; partition formula: `base=n/size, extra=n%size, istart=rank*base+min(rank,extra)`

**Test coverage (21/21 pass)**
- `test/unit/test_io.cpp`: `ParseFortranDouble` (D-exponent and standard E), `ReadDims`, `ReadGeneral`, `ReadZero`, `ReadConfig`, `ReadBCs`, `ReadMesh`, `ReadTubLoc`, `ElementPartition` (serial, 2-rank, odd)
- `test/unit/test_mpi_env.cpp`: `SerialRankSize`, `PartitionCoverage` (1–8 ranks), `PartitionForRank`
- `test/integration/test_oracle_roundtrip.cpp`: Round-trip for Dims, General, Zero, Config, BCs, Mesh

## Files Changed

**Created:**
- `CMakeLists.txt`
- `include/fce/types.hpp`
- `include/fce/io.hpp`
- `include/fce/mpi_env.hpp`
- `src/core/io.cpp`
- `src/core/mpi_env.cpp`
- `src/prepro/main.cpp` (stub)
- `src/simulator/main.cpp` (stub)
- `test/unit/test_io.cpp`
- `test/unit/test_mpi_env.cpp`
- `test/integration/test_oracle_roundtrip.cpp`
- `test/cases/graphene_compression_prepro/build_provenance.md`
- `test/cases/graphene_compression_simulator/np1/` (66 files)

**Modified:**
- `test/cases/README.md` (np1/ subdirectory description)
- `document/plan.md` (nCodeLoad corrections — committed as 61bb9e2)
- `.humanize/rlcr/2026-03-29_21-53-11/goal-tracker.md` (blocking issues resolved, 7 tasks moved to Completed)

**Committed:** `268383b` — Milestone 1: C++17 scaffold, data types, nano_*.dat I/O with 21 passing tests (79 files, +985287)

## Validation

```
cd build && ctest --output-on-failure
100% tests passed, 0 tests failed out of 21
Total Test time (real) = 0.61 sec
```

All tests: ParseFortranDouble (2), ReadDims, ReadGeneral, ReadZero, ReadConfig, ReadBCs, ReadMesh, ReadTubLoc, ElementPartition (3), MpiEnv (3), RoundTrip (6)

## Remaining Items (task1e — field-by-field comparison script)

`task1e` (oracle comparison script) was scoped for Milestone 1 but does not block any AC independently — the round-trip tests already validate read/write fidelity. This can be deferred to Milestone 2 Phase H when a full preprocessor comparison is needed. No other Milestone 1 items remain.

## BitLesson Delta
- Action: add
- Lesson ID(s): BL-20260330-zero-per-element, BL-20260330-bcs-label-consumption
- Notes: Added two lessons from Round 1 failures: (1) nano_zero.dat stores RefConfig per element not per gauss point — always verify array indexing from the Fortran write loop, not from what seems logical; (2) BCS reader label-consumption bug — when a variable-length data loop breaks on a label, do not call getline() again in the next search loop or that label will be silently skipped; use sequential scan with goto or explicit label-reuse logic.
