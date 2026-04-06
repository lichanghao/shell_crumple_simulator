CORE_RISKS:
- The draft assumes “exactly the same functionalities” is well-defined, but the Fortran source is not a single clean baseline: it contains patched behavior, backup variants (`*.f90A`, `*_mod2`, `*_mod3`, `*_good`), and post-hoc fixes recorded in docs, so the reference implementation is ambiguous.
- The draft says “graphene codebase,” while the repository context and existing source documentation span four subprojects plus later extensions; scope drift is likely unless the plan freezes what is and is not in scope.
- Numerical equivalence is high risk because the code couples B-spline geometry, exponential Cauchy-Born mapping, Brenner derivatives, nested Newton inner relaxation, and outer L-BFGS; small derivative or ordering mistakes can produce qualitatively wrong physics.
- Boundary-patch and ghost-node logic is fragile; prior notes show element orientation and 12-node patch ordering bugs materially changed energies, so a superficially correct translation can still be physically wrong.
- MPI behavior is a real risk, not an implementation detail: the source docs already describe bugs from rank divergence, reduction on custom data, and parallel termination, so a serial-first plan may hide major failures.
- I/O compatibility is risky because the simulator/preprocessor pipeline depends on multiple `nano_*.dat` contracts and visualization outputs; “same functionality” is underspecified without defining required file compatibility.
- The source already exhibits optimizer and NaN failure modes; translating existing behavior without a strategy for reproducing or intentionally correcting these edge cases could create disagreement about whether differences are bugs or improvements.

MISSING_REQUIREMENTS:
- A frozen source reference is missing: exact repo path is given, but not the required commit, branch, or whether later fixes/extensions documented in `document/` are part of the target behavior.
- Scope boundaries are missing for nanotube, cyclic crumpling, self-contact, bilayer/vdW cases, and extra visualization formats already present in the source repo.
- Acceptance tolerances are missing: no numeric thresholds for energies, forces, relaxed `eta`, reaction forces, or output-file equivalence.
- Platform/toolchain requirements are missing: supported compilers, C++ standard, MPI implementation, OS targets, and whether results must match across `np=1` and multi-rank runs.
- Performance requirements are missing: runtime, memory, and scaling expectations matter for FEM/MPI software, especially if “same functionality” includes practical usability.
- Error-handling requirements are missing: what the C++ code should do on inner Newton nonconvergence, line-search failure, NaN detection, missing files, or inconsistent mesh data.
- Input/output compatibility requirements are missing: whether the C++ preprocessor/simulator must read and write byte-compatible `nano_*.dat`, preserve line ordering, and reproduce VTU/VTK/Ensight/gmsh schemas.
- Verification requirements are too general; the draft says to verify each module numerically, but does not require a Fortran-vs-C++ differential test harness.

TECHNICAL_GAPS:
- There is no target architecture yet for shared kernels versus app-specific frontends; translating preprocessor and simulator separately without a common geometry/material core will duplicate the hardest logic.
- The draft does not address how Fortran global state, module data, and MPI distribution will map into C++ types and ownership boundaries.
- No library choices are specified for linear algebra, optimization, testing, MPI wrappers, or file parsing, yet those decisions strongly affect feasibility and reproducibility.
- There is no plan for a golden-oracle workflow that runs Fortran and C++ on the same cases and compares intermediate quantities, not just final outputs.
- The empty C++ repo has no build system, test harness, data fixtures, or documentation structure beyond placeholders, so the draft understates bootstrap work.
- The source analysis shows subtle mesh-topology conventions and BC partitioning rules; the draft does not require documenting these invariants before implementation starts.
- The plan does not separate “translate behavior” from “repair source bugs”; that gap will cause churn whenever C++ results differ from older Fortran runs.

ALTERNATIVE_DIRECTIONS:
- Translate graphene preprocessor and simulator first, then generalize to nanotube. Tradeoff: lower initial risk and faster feedback, but delayed validation that the architecture truly covers both geometries.
- Build a shared C++ core for geometry, constitutive laws, optimization, and I/O, with thin graphene/nanotube drivers. Tradeoff: better long-term structure, but more upfront design and slower first executable.
- Keep the Fortran executables as a test oracle and implement C++ differential tests from day one. Tradeoff: more initial harness work, but much lower scientific regression risk.
- Port only the preprocessor first and keep the Fortran simulator temporarily. Tradeoff: narrows early scope and validates mesh/BC/I/O contracts, but does not retire the hardest numerical kernel.
- Wrap selected Fortran kernels behind a C/C++ interface as an intermediate milestone. Tradeoff: accelerates validation of surrounding infrastructure, but delays a full native C++ codebase.
- Define “functional parity within tolerance” instead of byte-identical reproduction. Tradeoff: more realistic for floating-point/MPI software, but requires explicit user approval and comparison thresholds.

QUESTIONS_FOR_USER:
- Is the actual scope graphene only, or all four Fortran subprojects?
- Which exact Fortran baseline should be treated as canonical: current working tree, a specific git commit, or only the active files excluding backup variants?
- Are later source extensions such as cyclic crumpling, self-contact, checkpoint/restart, and added VTU/NetCDF output in scope for the C++ translation?
- What does “exactly the same functionalities” mean in practice: same features only, same numeric trajectories within tolerance, or byte-compatible file outputs?
- Do you want the C++ code to preserve known source quirks/bugs when they affect outputs, or should the plan treat documented fixes as mandatory canonical behavior?
- What C++ stack is acceptable: required standard, preferred build system, testing framework, MPI abstraction level, and whether external libraries are allowed?
- What platforms and parallel modes must be supported at minimum?
- Which verification cases are mandatory acceptance cases: small unit checks only, existing graphene compression case, nanotube twist case, vdW cases, cyclic cases, and multi-rank runs?

CANDIDATE_CRITERIA:
- The plan freezes a canonical Fortran reference baseline by repo path plus commit/branch and explicitly lists which subprojects/features are in and out of scope.
- The C++ repo defines a concrete architecture for shared FEM/material kernels, app frontends, build/test layout, and documentation update rules before implementation begins.
- The C++ preprocessor reproduces required `nano_*.dat` outputs for at least one graphene case with either exact formatting compatibility or documented field-wise tolerances.
- Unit-level checks exist for B-spline basis/derivatives, element patch ordering, ghost-node generation, metric/curvature tensors, principal-curvature extraction, and BC partitioning.
- Constitutive tests verify Brenner `W`, first derivatives, and second derivatives against trusted Fortran outputs or finite-difference checks at representative states.
- Inner-relaxation tests verify Newton convergence behavior, failure detection, and `eta` consistency against Fortran on controlled element-level fixtures.
- End-to-end graphene compression runs in C++ and matches Fortran reference outputs within declared tolerances for energy progression, reaction forces, and selected field data.
- If MPI is in scope, `np=1` and multi-rank runs produce consistent results within tolerance and exit cleanly on failure conditions.
- Required output formats are explicitly tested: whichever of `nano_*.dat`, VTU/VTK, Ensight, gmsh, or NetCDF are declared in scope must be generated and validated.
- Documentation is updated continuously with implementation notes, bug learnings, verification evidence, and any project-structure changes, including `AGENT.md` once it exists in the C++ repo.
