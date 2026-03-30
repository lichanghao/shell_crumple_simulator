# B-Spline Oracle Fixtures

These fixtures contain canonical Fortran outputs for `BSpline`, `DBSpline`, and `DDBSpline`
evaluated at 10 representative `(v, w)` points:

- Interior points: `interior_01.dat` through `interior_05.dat`
- Boundary points: `boundary_01.dat` through `boundary_05.dat`

Each fixture stores:

1. one line with `v w`
2. 12 lines of shape-function values `N_i`
3. 12 lines of first derivatives `dN_i/dv dN_i/dw`
4. 12 lines of second derivatives `d²N_i/dv² d²N_i/dw² d²N_i/dvdw`

## Reproduction

From the C++ repo root:

```bash
mkdir -p test/cases/bspline_oracle
gfortran -c -O0 -fallow-argument-mismatch ../finite_crystal_elasticity/grapheneCompressionOriginPrePro/BSpline.f90 -o /tmp/fortran_bspline.o
gfortran -O0 -fallow-argument-mismatch test/cases/tools/dump_bspline_oracle.f90 /tmp/fortran_bspline.o -o /tmp/dump_bspline_oracle
/tmp/dump_bspline_oracle test/cases/bspline_oracle
```

Source of truth:

- Fortran source: `../finite_crystal_elasticity/grapheneCompressionOriginPrePro/BSpline.f90`
- Helper committed in this repo: `test/cases/tools/dump_bspline_oracle.f90`
