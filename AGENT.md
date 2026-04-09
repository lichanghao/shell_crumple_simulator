# AGENT.md

This repository is a C++17 translation of the canonical Fortran graphene finite-crystal-elasticity codebase at oracle commit `7d3f77f`.

## Repository layout

- `include/fce/`: public headers for core types and kernels.
- `src/core/`: shared translation units used by both executables.
- `src/prepro/main.cpp`: `PrePro` entry point. Reads `data.dat` in a case directory and writes `nano_*.dat`.
- `src/simulator/main.cpp`: `crunch_it` entry point. Runs the archived-runtime-compatible simulator path.
- `test/unit/`: kernel and module tests.
- `test/integration/`: archived-oracle and executable-path regression tests.
- `test/cases/`: frozen Fortran fixtures and helper-generated oracle artifacts.
- `document/fortran_conventions.md`: source-of-truth I/O, indexing, and unit conventions.
- `document/translation_notes.md`: current translation status and known gaps.

## Build

Configure and build from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

Dependencies expected by `CMakeLists.txt`:

- CMake >= 3.18
- A C++17 compiler
- MPI
- Eigen3
- network access at configure time for GoogleTest `FetchContent`, unless already cached

## Executables

Run the preprocessor on a case directory containing `data.dat`:

```bash
./build/PrePro test/cases/graphene_compression_prepro
```

Run the simulator on a directory containing `nano_*.dat`:

```bash
./build/crunch_it test/cases/graphene_compression_simulator/np1
```

Useful simulator modes:

```bash
./build/crunch_it <case_dir> 50
./build/crunch_it <case_dir> --single-step 1
```

`stop_step` must be positive and no greater than `BCs%nloadstep`.

## Tests

Run the full discovered suite:

```bash
ctest --test-dir build --output-on-failure
```

Frequently used focused commands:

```bash
./build/unit_tests --gtest_filter='RuntimeOutput.*'
./build/integration_tests --gtest_filter='PreprocessorOracle.*'
./build/integration_tests --gtest_filter='E2ECompression.*'
```

## Non-negotiable conventions

- Internal indices are 0-based. On-disk `nano_*.dat` indices are 1-based Fortran values.
- `nano_*.dat` floating-point fields use Fortran `D` exponents.
- Units are nm, eV, eV/nm, and 1/nm as documented in `document/fortran_conventions.md`.
- Only canonical Fortran sources count as oracle references. Do not treat `*.f90_mod2`, `*.f90_good`, or `*.f90A` as source of truth.
- Prefer archived-oracle comparisons over C++ self-oracles whenever a fixture already exists under `test/cases/`.

## Archived case map

- `test/cases/graphene_compression_prepro/`: baseline preprocessor oracle.
- `test/cases/graphene_compression_simulator/np1/`: baseline serial simulator oracle, including archived VTU/PVD outputs.
- `test/cases/graphene_cyclic_crumple/`: cyclic preprocessor and simulator oracle.
- `test/cases/graphene_self_contact/prepro_run/`: `nvdw=1` single-sheet preprocessor oracle.
- `test/cases/graphene_bilayer_twist_vdw_1000/prepro_run/`: multi-sheet `nvdw=1` preprocessor oracle.
- `test/cases/bspline_oracle/`, `test/cases/constitutive_oracle/`, `test/cases/element_energy_oracle/`, `test/cases/principal_oracle/`, `test/cases/principal_exponential_oracle/`: kernel-level fixtures.

## Current translation status

- Preprocessor-side mesh, ghost-node, B-spline, reference-config, and `nvdw=1` preprocessing coverage are translated and oracle-backed.
- Core constitutive pieces, principal/exponential helpers, archived element-state fixtures, and the assembly path are translated.
- Runtime VTU/PVD writing now includes parser-backed XML validation in tests.
- The main unresolved runtime gap is still AC-7: the executable-path compression run diverges from the archived Fortran trajectory.
- Runtime vdW/self-contact assembly is still not translated end to end.
- Cyclic controller, crease memory, checkpoint/restart, and MPI parity verification are still incomplete.

## Working rules for future edits

- Check `document/translation_notes.md` and the RLCR goal tracker before claiming a milestone is complete.
- Do not manually edit RLCR state files such as `.humanize/rlcr/*/state.md`.
- When touching file I/O, trace the exact Fortran read/write loop before changing record structure or counts.
- When touching runtime parity, validate against archived fixtures under `test/cases/graphene_compression_simulator/np1/` rather than against newly generated C++ artifacts alone.
