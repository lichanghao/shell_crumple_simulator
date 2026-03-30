# Self-Contact Oracle Provenance

## Oracle Repository

- **Path**: `../finite_crystal_elasticity/`
- **Frozen commit**: `7d3f77fab2378d675d14ebeac3d8e65d94221a4f`
- **Commit message**: "Document Modules 1–3 in build_and_run_notes.md"

## Archived Preprocessor Case

The committed oracle files in `prepro_run/` were copied from the sibling Fortran oracle archive:

```bash
../finite_crystal_elasticity/test/cases/graphene_self_contact/prepro_run/
```

That archived case is a single-sheet cyclic self-contact setup:

- `nCodeLoad=30`
- `nvdw=1`
- `nself_contact=1`
- `ncrease=1`
- `ngauss_vdw=2`

## Ghost Coordinate Artifact

`prepro_run/ghost_coords.dat` was generated in this C++ repository from the frozen Fortran
`ghost_nodes` implementation using the committed reproduction helper:

```bash
gfortran -c -O0 -fallow-argument-mismatch ../finite_crystal_elasticity/grapheneCompressionOriginPrePro/headers.f90 -J /tmp -o /tmp/headers.o
gfortran -c -O0 -fallow-argument-mismatch ../finite_crystal_elasticity/grapheneCompressionOriginPrePro/connect_mesh.f90 -I /tmp -J /tmp -o /tmp/connect_mesh.o
gfortran -O0 -fallow-argument-mismatch test/cases/tools/dump_ghost_coords.f90 /tmp/headers.o /tmp/connect_mesh.o -I /tmp -J /tmp -o /tmp/dump_ghost_coords
/tmp/dump_ghost_coords test/cases/graphene_self_contact/prepro_run
```

The helper reads the committed `nano_Mesh.dat` and `nano_config.dat` files and writes one
`x y z` triplet per ghost node in oracle order.
