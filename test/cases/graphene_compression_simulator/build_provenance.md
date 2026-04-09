# Compression Imperfection Trace Provenance

This note documents how `imperfection_trace_fortran.dat` was captured for the archived
compression simulator case.

## Source baseline

- Fortran repository: `../finite_crystal_elasticity/`
- Frozen commit: `7d3f77f`
- Runtime source of truth: `grapheneCompressionOriginVersion/pasapas.f90`
- Archived simulator input: `test/cases/graphene_compression_simulator/np1/nano_*.dat`

## Capture method

1. Copy `grapheneCompressionOriginVersion/` into a temporary writable directory.
2. Instrument the `pasapas.f90` imperfection slot immediately after:
   - `call random_seed()`
   - `call random_number(a)`
3. Append `iload` and `a` to a temporary `imperfection_trace.dat` file on rank 0.
4. Compile the temporary simulator copy with:
   - `mpifort -c -O3 -fallow-argument-mismatch headers.f90 *.f90 *.f`
   - `mpifort -O3 -o crunch_it_trace *.o`
5. Copy the archived `np1/nano_*.dat` files plus `nano_tub_loc.dat` into a temporary run
   directory and run:
   - `mpirun -np 1 crunch_it_trace`
6. Strip the leading `iload` column from the temporary `imperfection_trace.dat` output and
   commit the resulting scalar-only file as `imperfection_trace_fortran.dat`.

## Important caveat

The canonical Fortran runtime calls `random_seed()` with no explicit seed before every load-step
draw. That means rerunning the instrumented capture on the same code path can produce a different
trace. The committed `imperfection_trace_fortran.dat` should therefore be treated as a frozen,
source-backed fixture captured from the Fortran runtime slot, not as a deterministically
regenerable byproduct of an uninstrumented replay.
