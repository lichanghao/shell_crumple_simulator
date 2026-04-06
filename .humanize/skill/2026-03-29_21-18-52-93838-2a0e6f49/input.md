# Ask Codex Input

## Question

You are doing a THIRD-PASS CONVERGENCE REVIEW of an updated candidate plan (v2) for translating Fortran graphene FEM to C++.

## Updated Candidate Plan (v2) — Addressing Previous Required Changes

### Scope
- IN SCOPE v1: graphene preprocessor + simulator, serial execution, core physics (nCodeLoad=1 standard compression), vdW optionally disabled, nano_*.dat I/O, VTU output
- OUT OF SCOPE v1: MPI, cyclic crumpling (nCodeLoad=30/31), self-contact, crease memory, checkpoint/restart, Ensight/NetCDF output, nanotube subprojects
- Canonical Fortran reference: current working tree of grapheneCompressionOriginPrePro/ and grapheneCompressionOriginVersion/ (active .f90 files only, not backup variants)

### Architecture
- C++17, CMake 3.20+, GoogleTest, Eigen3 for dense linear algebra
- L-BFGS: implement the same Nocedal two-loop L-BFGS algorithm as the Fortran lbfgs.f, same stopping criterion, same line search (Wolfe conditions as in the Fortran)
- Shared core library: geometry, constitutive (Brenner+ExpCB), solver (LBFGS+Newton), io
- Executables: prepro, simulator

### Milestones (updated)
- Milestone 0 (Fortran Archaeology): Build Fortran code, identify active code paths, run reference case, capture golden outputs, document data flow and conventions
- Milestone 1 (Project Infrastructure): CMake, directory structure, shared types, basic I/O, test harness
- Milestone 2 (Preprocessor): Mesh generation, B-spline setup, ghost nodes, connectivity, BC setup, nano_*.dat output
- Milestone 3 (Constitutive Model): Brenner REBO, exponential map, deformation gradient, Cauchy-Born, inner Newton for η
- Milestone 4 (FEM Solver): Element energy/force/stiffness, global assembly, L-BFGS, loading controller, pasapas loop
- Milestone 5 (Integration & Documentation): End-to-end vs Fortran oracle, VTU output, AGENT.md + document/ updates

### Acceptance Criteria (updated)
- AC-1 (Fortran Archaeology): Fortran executables built and run; canonical output files stored; active code paths documented; data conventions (indexing base, units, file format) explicitly recorded
- AC-2 (Preprocessor I/O): For graphene 20nm×20nm case, C++ nano_*.dat match Fortran to:
  - Discrete/topology fields (counts, indices): exact integer match
  - Floating-point coordinates and Jacobians: ≤1e-10 absolute (double precision)
  - Deformation gradients F0: ≤1e-10 absolute
  - BC vectors and rotation matrix: ≤1e-10 absolute
- AC-3 (B-spline): For all 12-node patch configurations, N_i and ∂N_i/∂ξ match Fortran BSpline output within 1e-14 absolute (partition-of-unity check: |Σ N_i - 1| < 1e-14)
- AC-4 (Ghost nodes): Ghost node coordinates match Fortran to ≤1e-12 absolute (parallelogram extrapolation)
- AC-5 (Brenner REBO): W, dW/dpe match Fortran on ≥5 distinct bond configurations within 1e-10 absolute; second derivatives verified by finite difference to 1e-6 relative
- AC-6 (Inner Newton): On 10 element-level test states, C++ η matches Fortran to ≤1e-10 absolute; Newton residual < 1e-8 (matching Fortran convergence criteria)
- AC-7 (End-to-end): For graphene compression case (nloadstep=100), energy profile matches Fortran within 1e-4 relative per step; final reaction force within 1e-3 relative; nodal displacements at step 50 within 1e-3 relative
- AC-8 (Documentation): AGENT.md created in C++ repo, document/translation_notes.md maintained, build instructions verified

### Verification in Each Milestone
Each milestone has its own unit tests before proceeding to the next.

### Prior Disagreements Addressed
- Fortran parity is primary oracle (not analytic or FD)
- L-BFGS algorithm is explicitly specified (same as Fortran lbfgs.f)
- AC tolerances are field-specific
- Milestone 0 added for archaeology
- vdW and MPI explicitly deferred

## Your Task
Review this updated plan for reasonability. Output EXACTLY in this format:

AGREE:
- [points accepted as reasonable]

DISAGREE:
- [points considered unreasonable and why]

REQUIRED_CHANGES:
- [must-fix items before this plan is acceptable]

OPTIONAL_IMPROVEMENTS:
- [non-blocking improvements]

UNRESOLVED:
- [opposite opinions needing user decisions]


## Configuration

- Model: gpt-5.4
- Effort: high
- Timeout: 3600s
- Timestamp: 2026-03-29_21-18-52
- Tool: codex
