// Runtime solver: minimize_free, minimize_constrained, pasapas.
// Translated from minimize_free.f90, minimize.f90, pasapas.f90.

#include "fce/solver.hpp"

#include "fce/lbfgs.hpp"
#include "fce/load_controller.hpp"
#include "fce/mpi_env.hpp"
#include "fce/runtime_output.hpp"
#include "fce/simulator.hpp"
#include "fce/types.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace fce {

namespace {

// ─── XNORM0 computation ───────────────────────────────────────────────────────
// Mirrors Fortran minimize.f90 lines 43-46:
//   dx1 = maxval(x0(1:3*numnods:3)) - minval(x0(1:3*numnods:3))
//   dx2 = maxval(x0(2:3*numnods:3)) - minval(x0(2:3*numnods:3))
//   dx3 = maxval(x0(3:3*numnods:3)) - minval(x0(3:3*numnods:3))
//   XNORM0 = sqrt(dx1^2 + dx2^2 + dx3^2)
double compute_xnorm0(const Coords& coords) {
    if (coords.empty()) return 1.0;
    double xmin = coords[0][0], xmax = coords[0][0];
    double ymin = coords[0][1], ymax = coords[0][1];
    double zmin = coords[0][2], zmax = coords[0][2];
    for (const auto& p : coords) {
        xmin = std::min(xmin, p[0]); xmax = std::max(xmax, p[0]);
        ymin = std::min(ymin, p[1]); ymax = std::max(ymax, p[1]);
        zmin = std::min(zmin, p[2]); zmax = std::max(zmax, p[2]);
    }
    const double dx = xmax - xmin;
    const double dy = ymax - ymin;
    const double dz = zmax - zmin;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

// ─── energy components from AssemblyResult ────────────────────────────────────
// The Fortran E_out(4) = [E_total, E_internal, E_vdw, E_external].
// Currently AssemblyResult only has total_energy. We map it to E_total; the
// sub-components are not decomposed in the current C++ assembly (VdW not active).
EnergyComponents to_energy_components(const AssemblyResult& res) {
    EnergyComponents e;
    e.E_total    = res.total_energy;
    e.E_internal = res.total_energy;  // no vdw/external separation yet
    e.E_vdw      = 0.0;
    e.E_external = 0.0;
    return e;
}

// ─── extract forces at free DOFs ─────────────────────────────────────────────
// Mirrors Fortran short: g_short(i) = forces(mdofOP(i)).
std::vector<double> extract_free_gradient(const AssemblyResult& res,
                                           const BCData& bcs) {
    std::vector<double> g(static_cast<std::size_t>(bcs.ndofOP));
    for (int i = 0; i < bcs.ndofOP; ++i) {
        const int flat_dof = bcs.mdofOP.at(static_cast<std::size_t>(i));
        g[static_cast<std::size_t>(i)] = res.force.at(static_cast<std::size_t>(flat_dof));
    }
    return g;
}

// ─── extract forces at all (free+bc) DOFs ────────────────────────────────────
// Mirrors Fortran minimize_free gather loop.
std::vector<double> extract_all_gradient(const AssemblyResult& res,
                                          const BCData& bcs) {
    const int ndof = bcs.ndofOP + bcs.ndofBC;
    std::vector<double> g(static_cast<std::size_t>(ndof));
    // Free DOFs.
    for (int i = 0; i < bcs.ndofOP; ++i) {
        const int flat_dof = bcs.mdofOP.at(static_cast<std::size_t>(i));
        g[static_cast<std::size_t>(i)] = res.force.at(static_cast<std::size_t>(flat_dof));
    }
    // BC DOFs (appended after free DOFs in the combined vector).
    for (int i = 0; i < bcs.ndofBC; ++i) {
        const int flat_dof = bcs.mdofBC.at(static_cast<std::size_t>(i));
        g[static_cast<std::size_t>(bcs.ndofOP + i)] = res.force.at(static_cast<std::size_t>(flat_dof));
    }
    return g;
}

// ─── scatter combined (free+bc) DOFs to coords ────────────────────────────────
// Mirrors Fortran minimize_free scatter loop: x0(mdofOP(i)) = x_short(i).
void scatter_all_dofs(const std::vector<double>& x_all,
                       const BCData& bcs,
                       Coords& coords) {
    for (int i = 0; i < bcs.ndofOP; ++i) {
        const int flat_dof = bcs.mdofOP.at(static_cast<std::size_t>(i));
        coords.at(static_cast<std::size_t>(flat_dof / 3))[flat_dof % 3] =
            x_all.at(static_cast<std::size_t>(i));
    }
    for (int i = 0; i < bcs.ndofBC; ++i) {
        const int flat_dof = bcs.mdofBC.at(static_cast<std::size_t>(i));
        coords.at(static_cast<std::size_t>(flat_dof / 3))[flat_dof % 3] =
            x_all.at(static_cast<std::size_t>(bcs.ndofOP + i));
    }
}

// ─── gather combined (free+bc) DOFs from coords ───────────────────────────────
std::vector<double> gather_all_dofs(const Coords& coords, const BCData& bcs) {
    const int ndof = bcs.ndofOP + bcs.ndofBC;
    std::vector<double> x(static_cast<std::size_t>(ndof));
    for (int i = 0; i < bcs.ndofOP; ++i) {
        const int flat_dof = bcs.mdofOP.at(static_cast<std::size_t>(i));
        x[static_cast<std::size_t>(i)] = coords.at(static_cast<std::size_t>(flat_dof / 3))[flat_dof % 3];
    }
    for (int i = 0; i < bcs.ndofBC; ++i) {
        const int flat_dof = bcs.mdofBC.at(static_cast<std::size_t>(i));
        x[static_cast<std::size_t>(bcs.ndofOP + i)] = coords.at(static_cast<std::size_t>(flat_dof / 3))[flat_dof % 3];
    }
    return x;
}

// ─── MPI broadcast of x and gnorm ─────────────────────────────────────────────
// Mirrors Fortran: MPI_BCAST(IFLAG,GNORM,X) from rank 0 to all.
void bcast_solver_state(const MpiEnv& mpi,
                         int& iflag,
                         double& gnorm,
                         std::vector<double>& x) {
    std::vector<int> iflag_v = {iflag};
    mpi.bcast_ints(iflag_v, 0);
    iflag = iflag_v[0];

    std::vector<double> gnorm_v = {gnorm};
    mpi.bcast_doubles(gnorm_v, 0);
    gnorm = gnorm_v[0];

    mpi.bcast_doubles(x, 0);
}

// ─── real-node forces (trim ghost contributions) ──────────────────────────────
// After assemble_energy_forces, result.force has size 3*(numnods+nedge).
// We only pass the first 3*numnods to reaction computation.
std::vector<double> real_node_forces(const AssemblyResult& res, int numnods) {
    return std::vector<double>(res.force.begin(),
                                res.force.begin() + 3 * numnods);
}

void apply_imperfections(const SimulatorInput& input,
                         RuntimeState& state,
                         const int iload) {
    if (!input.general.imperfect) {
        return;
    }

    double a = 0.0;
    if (!input.imperfection_trace.empty()) {
        const std::size_t step_index = static_cast<std::size_t>(iload - 1);
        if (step_index >= input.imperfection_trace.size()) {
            throw std::runtime_error("imperfection trace is missing a value for load step " +
                                     std::to_string(iload));
        }
        a = input.imperfection_trace[step_index];
    } else {
        // Match the checked-in graphene pasapas.f90 structure:
        //   call random_seed()
        //   call random_number(a)
        // before each constrained minimization step.
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        a = dist(rng);
    }
    const double delta = input.general.mat.A0 * 2.0 * (a - 0.5) * input.general.fact_imp;

    for (int inode = 0; inode < input.mesh.numnods; ++inode) {
        (void)iload;
        state.coords[static_cast<std::size_t>(inode)][0] += delta;
        state.coords[static_cast<std::size_t>(inode)][1] += delta;
        state.coords[static_cast<std::size_t>(inode)][2] += delta;
    }
}

void write_output_header(const std::string& output_dir, const double initial_energy) {
    const std::string output_path = output_dir + "/output.dat";
    std::ofstream out(output_path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open " + output_path);
    }

    out << std::uppercase << std::scientific << std::setprecision(16);
    out << "  Initial energy    :   " << initial_energy << "\n";
    out << " ***************************************************\n";
}

void append_output_step(const std::string& output_dir, const int iload, const double energy) {
    const std::string output_path = output_dir + "/output.dat";
    std::ofstream out(output_path, std::ios::app);
    if (!out) {
        throw std::runtime_error("cannot open " + output_path);
    }

    out << "\n";
    out << "  Load Step         : " << std::setw(11) << iload << "\n";
    out << "  =========         : " << std::setw(11) << iload << "\n";
    out << "\n";
    out << std::uppercase << std::scientific << std::setprecision(16);
    out << "  Equilibrium energy:   " << energy << "\n";
    out << " ***************************************************\n";
}

void write_final_config(const SimulatorInput& input,
                        const RuntimeState& state,
                        const std::string& output_dir) {
    io::ConfigData final_config;
    final_config.coords = state.coords;
    final_config.eta = state.eta;
    io::write_config(output_dir + "/nano_final_config.dat",
                     final_config,
                     input.mesh.numnods,
                     input.mesh.numele,
                     input.dims.ngauss);
}

}  // namespace

// ─── minimize_free ────────────────────────────────────────────────────────────

MinimizeFreeResult minimize_free(const SimulatorInput& input,
                                  RuntimeState& state,
                                  const MpiEnv& mpi,
                                  double eps) {
    const BCData& bcs = input.bcs;

    // Fortran minimize_free operates on FREE DOFs only (ndofOP), not all DOFs.
    // Mirror: CALL LBFGS(ndofOP, ...) with x_short = x0(mdofOP(:)).
    std::vector<double> x_free(static_cast<std::size_t>(bcs.ndofOP));
    for (int i = 0; i < bcs.ndofOP; ++i) {
        const int flat_dof = bcs.mdofOP.at(static_cast<std::size_t>(i));
        x_free[static_cast<std::size_t>(i)] =
            state.coords.at(static_cast<std::size_t>(flat_dof / 3))[flat_dof % 3];
    }

    // XNORM0 = 1.0 (Fortran minimize_free uses 1.0 as placeholder).
    const double xnorm0 = 1.0;

    LbfgsSolver solver(10, eps, 1.0e-12, 10000, mpi.is_root());

    EnergyComponents final_E{};
    AssemblyResult final_asm{};

    auto callback = [&](const std::vector<double>& xv)
        -> std::pair<double, std::vector<double>>
    {
        // Scatter free DOFs back to coords (BC DOFs stay fixed).
        for (int i = 0; i < bcs.ndofOP; ++i) {
            const int flat_dof = bcs.mdofOP.at(static_cast<std::size_t>(i));
            state.coords.at(static_cast<std::size_t>(flat_dof / 3))[flat_dof % 3] =
                xv.at(static_cast<std::size_t>(i));
        }

        // Assemble energy and forces.
        const auto res = assemble_energy_forces(input, state, mpi);
        final_asm = res;
        final_E = to_energy_components(res);

        // Gather gradient at free DOFs only (mirrors Fortran g_short).
        return {res.total_energy, extract_free_gradient(res, bcs)};
    };

    int flag = solver.minimize(x_free, xnorm0, callback);

    // Broadcast and scatter final state.
    double gnorm = solver.gnorm();
    bcast_solver_state(mpi, flag, gnorm, x_free);

    // Scatter final free DOFs to coords.
    for (int i = 0; i < bcs.ndofOP; ++i) {
        const int flat_dof = bcs.mdofOP.at(static_cast<std::size_t>(i));
        state.coords.at(static_cast<std::size_t>(flat_dof / 3))[flat_dof % 3] =
            x_free.at(static_cast<std::size_t>(i));
    }

    if (flag > 0) {
        final_asm = assemble_energy_forces(input, state, mpi);
        final_E = to_energy_components(final_asm);
    }

    MinimizeFreeResult result;
    result.E     = final_E;
    result.gnorm = gnorm;
    result.E_min = final_E.E_total;
    result.assembly = final_asm;

    (void)flag;
    return result;
}

// ─── minimize_constrained ────────────────────────────────────────────────────

MinimizeResult minimize_constrained(const SimulatorInput& input,
                                     RuntimeState& state,
                                     LoadController& load_ctrl,
                                     const MpiEnv& mpi,
                                     double eps) {
    const BCData& bcs = input.bcs;

    // Compute XNORM0 from initial coords bbox (mirrors Fortran minimize.f90 lines 43-46).
    const double xnorm0 = compute_xnorm0(state.coords);

    // Extract free DOFs.
    std::vector<double> x_free = load_ctrl.to_free(state.coords);

    LbfgsSolver solver(10, eps, 1.0e-12, 20000, mpi.is_root());

    EnergyComponents final_E{};
    AssemblyResult final_asm{};

    auto callback = [&](const std::vector<double>& xv)
        -> std::pair<double, std::vector<double>>
    {
        // Mirror Fortran long(...): scatter free DOFs and restore BC DOFs
        // from x0_BC before each energy evaluation.
        load_ctrl.scatter_all(xv, state.coords);

        // Assemble.
        const auto res = assemble_energy_forces(input, state, mpi);
        final_asm = res;
        final_E = to_energy_components(res);

        // Gradient at free DOFs.
        return {res.total_energy, extract_free_gradient(res, bcs)};
    };

    int flag = solver.minimize(x_free, xnorm0, callback);

    // MPI broadcast (mirrors Fortran MPI_BCAST of IFLAG, GNORM, X).
    double gnorm = solver.gnorm();
    bcast_solver_state(mpi, flag, gnorm, x_free);

    // Final scatter mirrors Fortran long(...).
    load_ctrl.scatter_all(x_free, state.coords);

    if (flag > 0) {
        final_asm = assemble_energy_forces(input, state, mpi);
        final_E = to_energy_components(final_asm);
    }

    MinimizeResult result;
    result.E     = final_E;
    result.gnorm = gnorm;
    result.E_min = final_E.E_total;
    result.assembly = final_asm;

    (void)flag;
    return result;
}

// ─── pasapas ──────────────────────────────────────────────────────────────────

void pasapas(const SimulatorInput& input,
             RuntimeState& state,
             const MpiEnv& mpi,
             const std::string& output_dir,
             double eps,
             int iload_start,
             int iload_stop) {
    const BCData& bcs = input.bcs;

    if (bcs.nCodeLoad != 3) {
        throw std::runtime_error("pasapas: only nCodeLoad=3 is supported (got " +
                                 std::to_string(bcs.nCodeLoad) + ")");
    }
    const int final_load = iload_stop > 0 ? iload_stop : bcs.nloadstep;
    if (final_load < iload_start || final_load > bcs.nloadstep) {
        throw std::runtime_error("pasapas: invalid load-step range");
    }

    // Build load controller and initialise BC positions.
    LoadController load_ctrl(bcs);
    load_ctrl.init(state.coords);

    // ── Step 0: free minimisation (mirrors Fortran pasapas.f90 lines 55-62) ───
    auto step0 = minimize_free(input, state, mpi, eps);

    // Mirrors Fortran: write(*,*) 'enforce the boundary change' then x0_BC=x0(mdofBC).
    if (mpi.is_root()) {
        std::cout << "enforce the boundary change\n";
    }

    // Snap BC positions from the minimised state.
    load_ctrl.init(state.coords);

    // Write energy.dat: header + step 0 (mirrors Fortran Optim.f90 + pasapas.f90 line 66).
    // Also truncate force.dat so a fresh run doesn't append to a stale file.
    if (mpi.is_root() && iload_start == 1) {
        std::ofstream ff(output_dir + "/force.dat", std::ios::out | std::ios::trunc);
        (void)ff;
        write_output_header(output_dir, step0.E.E_total);
    }
    if (mpi.is_root()) {
        const std::string energy_path = output_dir + "/energy.dat";
        std::ofstream ef(energy_path, std::ios::out | std::ios::trunc);
        if (!ef) throw std::runtime_error("cannot open " + energy_path);
        // Header: 'Load_Step', 'E_total', 'E_internal', 'E_vdw', 'E_external', 'GNORM'
        ef << std::setw(14) << "Load_Step"
           << std::setw(16) << "E_total"
           << std::setw(16) << "E_internal"
           << std::setw(16) << "E_vdw"
           << std::setw(16) << "E_external"
           << std::setw(16) << "GNORM"
           << "\n";
        // Step 0 row: format '(e14.5, 5e16.8)'.
        ef << std::scientific << std::setprecision(5)
           << std::setw(14) << 0.0
           << std::setprecision(8)
           << std::setw(16) << step0.E.E_total
           << std::setw(16) << step0.E.E_internal
           << std::setw(16) << step0.E.E_vdw
           << std::setw(16) << step0.E.E_external
           << std::setw(16) << step0.gnorm
           << "\n";
        write_mesh_snapshot(input, state, output_dir, 0);
    }

    // ── Load steps ────────────────────────────────────────────────────────────
    for (int iload = iload_start; iload <= final_load; ++iload) {
        if (mpi.is_root()) {
            std::cout << "***\n Load Step: " << iload << "\n";
        }

        // Apply load increment (moves BC nodes, updates coords).
        load_ctrl.apply_increment(iload, state.coords);
        apply_imperfections(input, state, iload);

        // Constrained minimisation.
        auto min_res = minimize_constrained(input, state, load_ctrl, mpi, eps);

        // Reuse the last converged assembly, matching the Fortran runtime path.
        const auto forces_real = real_node_forces(min_res.assembly, input.mesh.numnods);

        double reaction1 = 0.0, reaction2 = 0.0;
        load_ctrl.compute_reaction(forces_real, reaction1, reaction2);

        const double load_param = bcs.value * static_cast<double>(iload) /
                                  static_cast<double>(bcs.nloadstep);

        if (mpi.is_root()) {
            std::cout << " Equilibrium energy: " << min_res.E.E_total << "\n";

            // Write energy row.
            // Fortran format: '(e14.5,4e16.8,d17.9)'
            const std::string energy_path = output_dir + "/energy.dat";
            std::ofstream ef(energy_path, std::ios::app);
            ef << std::scientific << std::setprecision(5)
               << std::setw(14) << load_param
               << std::setprecision(8)
               << std::setw(16) << min_res.E.E_total
               << std::setw(16) << min_res.E.E_internal
               << std::setw(16) << min_res.E.E_vdw
               << std::setw(16) << min_res.E.E_external
               << std::scientific << std::setprecision(9)
               << std::setw(17) << min_res.gnorm
               << "\n";

            // Write force row.
            // Fortran format: '(4f17.9)'
            const std::string force_path = output_dir + "/force.dat";
            std::ofstream ff(force_path, std::ios::app);
            ff << std::fixed << std::setprecision(9)
               << std::setw(17) << load_param
               << std::setw(17) << min_res.E_min
               << std::setw(17) << reaction1
               << std::setw(17) << reaction2
               << "\n";

            append_output_step(output_dir, iload, min_res.E.E_total);
            write_mesh_snapshot(input, state, output_dir, iload);
        }
    }

    if (mpi.is_root()) {
        write_final_config(input, state, output_dir);
        write_mesh_series_index(output_dir, bcs, final_load);
    }
}

}  // namespace fce
