#pragma once
// High-level runtime solver: pasapas + minimize + minimize_free.
// Translated from pasapas.f90, minimize.f90, minimize_free.f90.

#include "fce/load_controller.hpp"
#include "fce/lbfgs.hpp"
#include "fce/simulator.hpp"
#include "fce/types.hpp"

#include <string>
#include <vector>

namespace fce {

// Energy components: [E_total, E_internal, E_vdw, E_external] (mirrors Fortran E_out(4)).
struct EnergyComponents {
    double E_total{0.0};
    double E_internal{0.0};
    double E_vdw{0.0};
    double E_external{0.0};
};

// One row written to energy.dat for nCodeLoad=3.
struct EnergyRow {
    double load_param{0.0};  // BCs%value * iload / BCs%nloadstep
    EnergyComponents E{};
    double gnorm{0.0};
};

// One row written to force.dat for nCodeLoad=3.
struct ForceRow {
    double load_param{0.0};
    double E_min{0.0};
    double reaction1{0.0};
    double reaction2{0.0};
};

// ─── free-DOF minimizer ───────────────────────────────────────────────────────
// Mirrors Fortran minimize_free.f90.
// Minimises over all DOFs (free + bc together) that are not in mdofBC.
// xnorm0=1 (flat, as in Fortran: XNORM0=1.0 placeholder).

struct MinimizeFreeResult {
    EnergyComponents E{};
    double gnorm{0.0};
    double E_min{0.0};
};

MinimizeFreeResult minimize_free(const SimulatorInput& input,
                                  RuntimeState& state,
                                  const MpiEnv& mpi,
                                  double eps);

// ─── constrained minimizer ────────────────────────────────────────────────────
// Mirrors Fortran minimize.f90: minimises over free DOFs only (mdofOP).
// Computes XNORM0 from initial coords bbox.

struct MinimizeResult {
    EnergyComponents E{};
    double gnorm{0.0};
    double E_min{0.0};
};

MinimizeResult minimize_constrained(const SimulatorInput& input,
                                     RuntimeState& state,
                                     LoadController& load_ctrl,
                                     const MpiEnv& mpi,
                                     double eps);

// ─── pasapas ─────────────────────────────────────────────────────────────────
// Mirrors Fortran pasapas.f90 for nCodeLoad=3.
// Writes energy.dat and force.dat to output_dir.

void pasapas(const SimulatorInput& input,
             RuntimeState& state,
             const MpiEnv& mpi,
             const std::string& output_dir,
             double eps,
             int iload_start = 1,
             int iload_stop = 0);

}  // namespace fce
