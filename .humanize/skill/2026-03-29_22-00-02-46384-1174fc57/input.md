# Ask Codex Input

## Question

You are helping document the Fortran conventions for the finite_crystal_elasticity C++ translation project.

Based on the following file format samples and source code observations, write a complete  document. This document will be used by C++ developers who are translating Fortran code to ensure they correctly handle all conventions.

## Source Repository Info
- Repository: /Users/changhaoli/github_projects/finite_crystal_elasticity/
- Oracle commit: 7d3f77f
- Active source files in grapheneCompressionOriginPrePro/ and grapheneCompressionOriginVersion/
- Legacy Fortran file: lbfgs.f (must be translated, not replaced)

## nano_dims.dat format sample:


## nano_general.dat format sample:


## nano_zero.dat format:
- Per-element, per-Gauss-point records
- For each Gauss point: J0 (scalar Jacobian), then F0 as 2x2 matrix (row 1, then row 2)
- 2 Gauss points per element, so 5 values per element (J0, F0[0][0], F0[0][1], J0, F0[1][0], F0[1][1])

## nano_config.dat format:
- Header: 'Config Data' / 'Nodal positions'
- Then numnods lines of 3 floats (x, y, z in nm)
- Then inner displacement eta for each node (initially zeros)

## nano_BCs.dat format:
- Header: 'BCs data'
- BCs%nloadstep: integer (50 for standard compression)
- BCs%nCodeLoad: integer (3 for compression, 30 for uniaxial cyclic, 31 for biaxial cyclic)
- BCs%mdofBC: list of constrained DOF indices (1-based)
- Followed by: rotation matrix, center coordinates, loading value, free DOFs, etc.

## nano_Mesh.dat format:
- Header: 'Mesh data' / 'Connect'
- Per-element records with 'New element' marker
- Each element: 3 vertex node indices (1-based), then 12-node B-spline patch indices
- Ghost node flags and indices included

## Key Observations from Source Code:
1. All Fortran arrays use 1-based indexing
2. nano_BCs.dat uses nCodeLoad=3 for standard graphene compression
3. The standard test case: 40x40 mesh, 20nm x 20nm, nCodeLoad=3, nloadstep=50, ngauss=2
4. Floating-point values use Fortran 'D' exponent notation (D+02 means E+02)
5. MPI: rank 0 handles I/O; elements are distributed round-robin across ranks
6. lbfgs.f uses COMMON blocks for state (must become a C++ class)
7. Active files: headers.f90, BSpline.f90, connect_mesh.f90, Def_Grad.f90, gauss.f90, gmsh_out.f90, load.f90, mesh_gen.f90, Prepro.f90, read_data.f90, vdw_previous.f90 (prepro) and all .f90 + lbfgs.f (simulator)

## Task:
Write the complete content for . Include ALL of the following sections:
1. Overview (purpose, oracle commit)
2. Array Indexing Convention (Fortran 1-based → C++ 0-based, with specific examples from nano_*.dat)
3. Unit System (lengths in nm, energies in eV, forces in eV/nm, curvatures in 1/nm)
4. nano_*.dat File Formats (one subsection per file with exact field ordering)
5. Floating-Point Format (Fortran D-exponent, double precision 64-bit)
6. Sign Conventions for Boundary Conditions
7. MPI Conventions (rank-0 I/O, element distribution)
8. Active Source Files List (both prepro and simulator, including lbfgs.f)
9. nCodeLoad Reference Table

Write complete, precise content suitable for use as a reference document.

## Configuration

- Model: gpt-5.4
- Effort: high
- Timeout: 3600s
- Timestamp: 2026-03-29_22-00-02
- Tool: codex
