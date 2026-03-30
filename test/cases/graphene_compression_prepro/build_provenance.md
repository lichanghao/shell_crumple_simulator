# Oracle Build Provenance

## Oracle Repository

- **Path**: `../finite_crystal_elasticity/` (relative to this C++ repo root)
- **Frozen commit**: `7d3f77fab2378d675d14ebeac3d8e65d94221a4f`
- **Commit message**: "Document Modules 1–3 in build_and_run_notes.md"
- **Commit date**: 2026-03-29 17:49:06 -0400

Verify with:
```bash
cd /path/to/finite_crystal_elasticity
git log --oneline -1   # should show: 7d3f77f Document Modules 1–3 in build_and_run_notes.md
git rev-parse HEAD     # should show: 7d3f77fab2378d675d14ebeac3d8e65d94221a4f
```

## Compiler and Runtime

```
GNU Fortran (Homebrew GCC 15.2.0_1) 15.2.0
MPI: Open MPI 5.0.9 (mpif90 wrapper)
Platform: macOS Darwin 24.6.0 (arm64)
```

## Preprocessor Build

Source directory: `finite_crystal_elasticity/grapheneCompressionOriginPrePro/`

```bash
cd grapheneCompressionOriginPrePro/
gfortran -c -O3 -fallow-argument-mismatch headers.f90
gfortran -c -O3 -fallow-argument-mismatch *.f90
gfortran -O3 -o PrePro *.o
```

Known source patches applied at commit `7d3f77f`:
- `gmsh_out.f90`: `CHARACTER(11)` → `CHARACTER(*)` for filename argument
- `Prepro.f90` lines 508–511: `BCs(imesh)%mnodBC(:,1)` → `BCs(imesh)%mnodBC(1:BCs(imesh)%nnodBC,1)` (and similar for col 2, mdofBC, mdofOP) to fix shape mismatch on BC assembly

Run:
```bash
echo "1" | ./PrePro
```
(The `echo "1"` selects the mesh type interactively; `1` = flat sheet.)

## Simulator Build

Source directory: `finite_crystal_elasticity/grapheneCompressionOriginVersion/`

```bash
cd grapheneCompressionOriginVersion/
mpif90 -c -O3 -fallow-argument-mismatch headers.f90
mpif90 -c -O3 -fallow-argument-mismatch *.f90
mpif90 -c -O3 -fallow-argument-mismatch *.f
mpif90 -O3 -o crunch_it *.o
```

Known source patches applied at commit `7d3f77f`:
- `fem_ensight_wrap.f90`: `.eq.` → `.eqv.` for logical comparisons (lines 44, 49, 54)
- `load.f90` line 157: `if(iload.gt.-10)` → `if(iload.lt.-10)` (swap compression direction to x)
- `energy.f90`: `vdw1%W=0.d0` → `if (vdw1%nvdw.eq.1) vdw1%W=0.d0` (vdW pointer guard)
- `minimize.f90`: guarded `MPI_ALLREDUCE(vdw1%W...)` with `if (vdw1%nvdw.eq.1)` check
- `ensight.f90`: `ENSMAX=100` → `ENSMAX=500` in `ensight_summary` to match `ensight_out`
- `Optim.f90` line 391: `call MPI_FINALIZE()` → `call MPI_FINALIZE(ierr)` (prevent post-run SIGSEGV)
- `ener_elem.f90`: inner-relaxation failure treated as hard error (skip contribution via `cycle`)
- `energy.f90`: halt on any inner-relaxation failure to prevent NaN propagation to L-BFGS
- `mesh_gen.f90` (T2 triangles): `[in2, in4, in3]` → `[in4, in3, in2]` vertex reordering
- `newton_inner.f90`: initialized `det`, increased `maxn` 25→100, added step damping (cap at `0.1*A0`/iter)

## Archived Binary

The compiled binary `crunch_it` (arm64 Mach-O, gfortran 15.2.0 runtime) is stored at:
```
finite_crystal_elasticity/test/cases/graphene_compression_version/build_artifacts/crunch_it
```
(This binary was used for both the 8-rank MPI run and the np=1 serial run archived here.)

## Backup Variant Exclusion (Negative Test Evidence)

The following files are present in `grapheneCompressionOriginVersion/` as backup/experimental variants and MUST NOT be compiled in place of their canonical counterparts:

| Backup file | Canonical | Known difference |
|-------------|-----------|-----------------|
| `brenner.f90_good` | `brenner.f90` | Earlier version before step-damping fix |
| `brenner.f90_mod2` | `brenner.f90` | Experimental parameter modification |
| `brenner.f90_mod3` | `brenner.f90` | Further experimental modification |
| `brenner2.f90_*` | `brenner2.f90` | Same series |
| `pasapas.f90A`, `pasapas.f90N` | `pasapas.f90` | Pre-fix snapshots |
| `read.f90A` | `read.f90` | Pre-fix snapshot |
| `Optim.f90A` | `Optim.f90` | Pre-fix snapshot (missing `ierr` in `MPI_FINALIZE`) |

These backup variants are not built as part of the canonical oracle. If any were substituted (e.g., using `brenner.f90_good` as `brenner.f90`), the simulation would diverge or produce different energy trajectories because:
- `brenner.f90_good`/`_mod2`/`_mod3` use different parameterizations or lack numerical fixes
- `Optim.f90A` causes a SIGSEGV at post-run finalization due to the missing `ierr` argument

The `gfortran -c *.f90` compile pattern excludes files with non-`.f90` extensions (`*.f90_good`, `*.f90_mod2`, etc.), so they are automatically excluded from the build.

## Run Commands

### Serial np=1 run (AC-7 validation baseline):
```bash
cd <working-dir-with-nano_*.dat>
mpirun -np 1 <path-to-crunch_it>
```

### 8-rank MPI run (reference archive):
```bash
cd <working-dir-with-nano_*.dat>
mpirun -n 8 <path-to-crunch_it>
```
