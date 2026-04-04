# Principal and Exponential Oracle Provenance

## Oracle Repository

- **Path**: `../finite_crystal_elasticity/`
- **Frozen commit**: `7d3f77fab2378d675d14ebeac3d8e65d94221a4f`
- **Commit message**: "Document Modules 1–3 in build_and_run_notes.md"

## Fixture Scope

`case_01.dat` … `case_10.dat` — Fortran oracle for `principal()` and `def_bonds()`.

- **Elements**: 83–87 (1-based), first interior elements in the compressed graphene mesh
- **Gauss points**: 1 and 2 for each element (10 cases total)
- **Source**: `nano_final_config.dat` deformed-state node positions (compression load step 50)
- **Bond geometry**: eta=0 (inner displacement zero); A_norm and Ei are independent of Newton

### Path

Analytical path (`flag_num_diff=false`): all 10 archived cases have distinct principal curvatures.
The archived compression geometry guarantees `|curv0_elem[0]| > 1e-4` for all selected elements.

### Fixture Format (36 rows per file)

```
Row  1: ielem igauss   (1-based header)
Row  2: C_elem(3)
Row  3: curv0_elem(3)
Row  4: flag_num_diff  (0 = false, 1 = true)
Row  5: curvppal(2)
Row  6: vppal(1,1:2)
Row  7: vppal(2,1:2)
Row  8: dcurvppaldC(1)%val(3)
Row  9: dcurvppaldC(2)%val(3)
Row 10: dcurvppaldk(1)%val(3)
Row 11: dcurvppaldk(2)%val(3)
Row 12: dvppaldC(1,1)%val(3)
Row 13: dvppaldC(1,2)%val(3)
Row 14: dvppaldC(2,1)%val(3)
Row 15: dvppaldC(2,2)%val(3)
Row 16: dvppaldk(1,1)%val(3)
Row 17: dvppaldk(1,2)%val(3)
Row 18: dvppaldk(2,1)%val(3)
Row 19: dvppaldk(2,2)%val(3)
Row 20: A_norm(3)        (eta=0 bond norms)
Row 21: Ei(1,1:2)        (eta=0 unit bond vectors, bond 1)
Row 22: Ei(2,1:2)        (bond 2)
Row 23: Ei(3,1:2)        (bond 3)
Row 24: pe(6)            (all 6 bond scalars on one line)
Row 25: dpedC(1)%val(3)
Row 26: dpedC(2)%val(3)
Row 27: dpedC(3)%val(3)
Row 28: dpedC(4)%val(3)
Row 29: dpedC(5)%val(3)
Row 30: dpedC(6)%val(3)
Row 31: dpedk(1)%val(3)
Row 32: dpedk(2)%val(3)
Row 33: dpedk(3)%val(3)
Row 34: dpedk(4)%val(3)
Row 35: dpedk(5)%val(3)
Row 36: dpedk(6)%val(3)
```

## Tolerance Note

The C++ `compute_principal_curvature` and `compute_deformed_bonds_with_derivatives` are
faithful translations of the Fortran `principal()` and `def_bonds()` subroutines using the
same algorithmic structure. Comparisons pass at 1e-12 absolute tolerance.

## Reproduction

```bash
mkdir -p /tmp/pexp_mods /tmp/pexp_obj
gfortran -std=legacy -O0 -J /tmp/pexp_mods -c \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/headers.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/Taylor.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/BSpline.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/gauss.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/geometry.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/principal.f90 \
  ../finite_crystal_elasticity/grapheneCompressionOriginVersion/exponential.f90 \
  finite_crystal_elasticity_Cpp/test/cases/tools/dump_principal_exponential_oracle.f90
mv *.o /tmp/pexp_obj/
gfortran -std=legacy -O0 -J /tmp/pexp_mods \
  -o /tmp/dump_principal_exponential_oracle /tmp/pexp_obj/*.o
/tmp/dump_principal_exponential_oracle \
  finite_crystal_elasticity_Cpp/test/cases/graphene_compression_simulator/np1 \
  finite_crystal_elasticity_Cpp/test/cases/principal_exponential_oracle
```
