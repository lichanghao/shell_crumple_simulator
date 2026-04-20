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
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace fce {

namespace {

bool lbfgs_monitor_enabled() {
    const char* raw = std::getenv("FCE_LBFGS_MONITOR");
    if (raw == nullptr) {
        return false;
    }

    std::string value(raw);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return !(value.empty() || value == "0" || value == "false" ||
             value == "no" || value == "off");
}

std::string trace_dump_dir() {
    const char* raw = std::getenv("FCE_TRACE_COORD_DUMPS");
    if (raw == nullptr) {
        return {};
    }
    return std::string(raw);
}

std::string trace_stop_stage() {
    const char* raw = std::getenv("FCE_TRACE_STOP_STAGE");
    if (raw == nullptr) {
        return {};
    }
    return std::string(raw);
}

std::set<int> accepted_state_dump_steps() {
    const char* raw = std::getenv("FCE_TRACE_ACCEPTED_STATE_STEPS");
    if (raw == nullptr || *raw == '\0') {
        return {};
    }

    std::set<int> steps;
    std::string text(raw);
    std::size_t start = 0;
    while (start < text.size()) {
        const std::size_t comma = text.find(',', start);
        const std::string token = text.substr(start, comma == std::string::npos ? std::string::npos
                                                                                : comma - start);
        if (!token.empty()) {
            const std::size_t dash = token.find('-');
            if (dash != std::string::npos) {
                const int begin = std::stoi(token.substr(0, dash));
                const int end = std::stoi(token.substr(dash + 1));
                const int lo = std::min(begin, end);
                const int hi = std::max(begin, end);
                for (int value = lo; value <= hi; ++value) {
                    steps.insert(value);
                }
            } else {
                steps.insert(std::stoi(token));
            }
        }
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    return steps;
}

bool trace_enabled_for_step(const int iload, const MpiEnv& mpi) {
    return !trace_dump_dir().empty() && mpi.is_root() && iload == 1;
}

bool should_stop_after_stage(const std::string& stage,
                             const int iload,
                             const MpiEnv& mpi) {
    return trace_enabled_for_step(iload, mpi) && trace_stop_stage() == stage;
}

void write_coord_dump_if_enabled(const RuntimeState& state,
                                 const std::string& stage,
                                 const int iload,
                                 const MpiEnv& mpi) {
    if (!trace_enabled_for_step(iload, mpi)) {
        return;
    }
    const std::string dir = trace_dump_dir();

    const std::string path = dir + "/step" + std::to_string(iload) + "_" + stage + ".dat";
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open " + path);
    }

    out << std::uppercase << std::scientific << std::setprecision(16);
    for (std::size_t inode = 0; inode < state.coords.size(); ++inode) {
        const auto& p = state.coords[inode];
        out << std::setw(8) << inode + 1
            << std::setw(24) << p[0]
            << std::setw(24) << p[1]
            << std::setw(24) << p[2]
            << "\n";
    }
}

void write_eta_dump_if_enabled(const RuntimeState& state,
                               const std::string& stage,
                               const int iload,
                               const MpiEnv& mpi) {
    const std::string dir = trace_dump_dir();
    if (dir.empty() || !mpi.is_root()) {
        return;
    }

    const std::string path = dir + "/step" + std::to_string(iload) + "_" + stage + "_eta.dat";
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open " + path);
    }

    out << std::uppercase << std::scientific << std::setprecision(16);
    for (std::size_t ielem = 0; ielem < state.eta.size(); ++ielem) {
        const auto& elem_eta = state.eta[ielem];
        for (std::size_t igauss = 0; igauss < elem_eta.size(); ++igauss) {
            out << std::setw(8) << ielem + 1
                << std::setw(8) << igauss + 1
                << std::setw(24) << elem_eta[igauss][0]
                << std::setw(24) << elem_eta[igauss][1]
                << "\n";
        }
    }
}

void write_reaction_dump(const std::string& path,
                         const BCData& bcs,
                         const std::vector<double>& forces_flat,
                         const double reaction1,
                         const double reaction2) {
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open " + path);
    }

    out << "# inode side_tag flat_dof fx fy fz bucket contribution\n";
    out << std::uppercase << std::scientific << std::setprecision(16);

    for (int i = 0; i < bcs.nnodBC; ++i) {
        const std::size_t mdof_idx = static_cast<std::size_t>(3 * i + 2);
        const int flat_dof = bcs.mdofBC.at(mdof_idx);
        const int inode = flat_dof / 3;
        const int side_tag = bcs.mnodBC.at(static_cast<std::size_t>(i))[1];
        const std::size_t base = static_cast<std::size_t>(3 * inode);
        const double fx = forces_flat.at(base);
        const double fy = forces_flat.at(base + 1);
        const double fz = forces_flat.at(base + 2);
        const bool to_reaction1 = (side_tag == 0);

        out << std::setw(8) << inode + 1
            << std::setw(8) << side_tag + 1
            << std::setw(8) << flat_dof + 1
            << std::setw(24) << fx
            << std::setw(24) << fy
            << std::setw(24) << fz
            << std::setw(8) << (to_reaction1 ? 1 : 2)
            << std::setw(24) << fz
            << "\n";
    }

    out << "# reaction1 " << reaction1 << "\n";
    out << "# reaction2 " << reaction2 << "\n";
}

void write_reaction_dump_if_enabled(const std::string& stage,
                                    const BCData& bcs,
                                    const std::vector<double>& forces_flat,
                                    const double reaction1,
                                    const double reaction2,
                                    const int iload,
                                    const MpiEnv& mpi) {
    if (!trace_enabled_for_step(iload, mpi)) {
        return;
    }
    const std::string dir = trace_dump_dir();
    write_reaction_dump(dir + "/step" + std::to_string(iload) + "_" + stage + "_reaction.dat",
                        bcs,
                        forces_flat,
                        reaction1,
                        reaction2);
    if (stage == "before_output") {
        write_reaction_dump(dir + "/step" + std::to_string(iload) + "_reaction.dat",
                            bcs,
                            forces_flat,
                            reaction1,
                            reaction2);
    }
}

void write_summary_dump_if_enabled(const std::string& stage,
                                   const EnergyComponents& energy,
                                   const double total_energy,
                                   const double reduced_energy,
                                   const double gnorm,
                                   const int inner_fail,
                                   const int iload,
                                   const MpiEnv& mpi) {
    if (!trace_enabled_for_step(iload, mpi)) {
        return;
    }

    const std::string path =
        trace_dump_dir() + "/step" + std::to_string(iload) + "_" + stage + "_summary.dat";
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open " + path);
    }

    out << std::uppercase << std::scientific << std::setprecision(16);
    out << "E_total " << energy.E_total << "\n";
    out << "E_internal " << energy.E_internal << "\n";
    out << "E_vdw " << energy.E_vdw << "\n";
    out << "E_external " << energy.E_external << "\n";
    out << "assembly_total_energy " << total_energy << "\n";
    out << "assembly_reduced_energy " << reduced_energy << "\n";
    out << "GNORM " << gnorm << "\n";
    out << "inner_fail " << inner_fail << "\n";
}

void write_lbfgs_step_trace_header_if_enabled(const int iload,
                                              const MpiEnv& mpi) {
    if (!trace_enabled_for_step(iload, mpi)) {
        return;
    }
    const std::string path =
        trace_dump_dir() + "/step" + std::to_string(iload) + "_accepted_lbfgs.dat";
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open " + path);
    }
    out << "# iter nfun f critc stp"
        << " node1_z node5_z node45_z node85_z node125_z node165_z node205_z"
        << " node41_x node1641_y node1681_x node1681_y\n";
}

void append_lbfgs_step_trace_if_enabled(const int iload,
                                        const Coords& coords,
                                        const int iter,
                                        const int nfun,
                                        const double f,
                                        const double critc,
                                        const double stp,
                                        const MpiEnv& mpi) {
    if (!trace_enabled_for_step(iload, mpi)) {
        return;
    }
    const std::string path =
        trace_dump_dir() + "/step" + std::to_string(iload) + "_accepted_lbfgs.dat";
    std::ofstream out(path, std::ios::app);
    if (!out) {
        throw std::runtime_error("cannot open " + path);
    }

    auto coord = [&](int one_based, int axis) -> double {
        return coords.at(static_cast<std::size_t>(one_based - 1))[axis];
    };

    out << std::uppercase << std::scientific << std::setprecision(16)
        << std::setw(6) << iter
        << std::setw(6) << nfun
        << std::setw(24) << f
        << std::setw(24) << critc
        << std::setw(24) << stp
        << std::setw(24) << coord(1, 2)
        << std::setw(24) << coord(5, 2)
        << std::setw(24) << coord(45, 2)
        << std::setw(24) << coord(85, 2)
        << std::setw(24) << coord(125, 2)
        << std::setw(24) << coord(165, 2)
        << std::setw(24) << coord(205, 2)
        << std::setw(24) << coord(41, 0)
        << std::setw(24) << coord(1641, 1)
        << std::setw(24) << coord(1681, 0)
        << std::setw(24) << coord(1681, 1)
        << "\n";
}

void write_full_accepted_state_if_enabled(const int iload,
                                          const int accepted_iter,
                                          const Coords& coords,
                                          const EtaField& eta,
                                          const EnergyComponents& energy,
                                          const double gnorm,
                                          const MpiEnv& mpi) {
    if (!trace_enabled_for_step(iload, mpi)) {
        return;
    }
    static const std::set<int> requested = accepted_state_dump_steps();
    if (requested.find(accepted_iter) == requested.end()) {
        return;
    }

    const std::string base = trace_dump_dir() + "/step" + std::to_string(iload) +
                             "_accepted_" + std::to_string(accepted_iter);

    std::ofstream coord_out(base + ".dat", std::ios::out | std::ios::trunc);
    if (!coord_out) {
        throw std::runtime_error("cannot open " + base + ".dat");
    }
    coord_out << std::uppercase << std::scientific << std::setprecision(16);
    for (std::size_t inode = 0; inode < coords.size(); ++inode) {
        const auto& p = coords[inode];
        coord_out << std::setw(8) << inode + 1
                  << std::setw(24) << p[0]
                  << std::setw(24) << p[1]
                  << std::setw(24) << p[2]
                  << "\n";
    }

    std::ofstream eta_out(base + "_eta.dat", std::ios::out | std::ios::trunc);
    if (!eta_out) {
        throw std::runtime_error("cannot open " + base + "_eta.dat");
    }
    eta_out << std::uppercase << std::scientific << std::setprecision(16);
    for (std::size_t ielem = 0; ielem < eta.size(); ++ielem) {
        for (std::size_t igauss = 0; igauss < eta[ielem].size(); ++igauss) {
            eta_out << std::setw(8) << ielem + 1
                    << std::setw(8) << igauss + 1
                    << std::setw(24) << eta[ielem][igauss][0]
                    << std::setw(24) << eta[ielem][igauss][1]
                    << "\n";
        }
    }

    std::ofstream summary_out(base + "_summary.dat", std::ios::out | std::ios::trunc);
    if (!summary_out) {
        throw std::runtime_error("cannot open " + base + "_summary.dat");
    }
    summary_out << std::uppercase << std::scientific << std::setprecision(16);
    summary_out << "E_total " << energy.E_total << "\n";
    summary_out << "E_internal " << energy.E_internal << "\n";
    summary_out << "E_vdw " << energy.E_vdw << "\n";
    summary_out << "E_external " << energy.E_external << "\n";
    summary_out << "GNORM " << gnorm << "\n";
}

void write_vector_dump(const std::string& path,
                       const std::vector<double>& values) {
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open " + path);
    }
    out << std::uppercase << std::scientific << std::setprecision(16);
    for (std::size_t i = 0; i < values.size(); ++i) {
        out << std::setw(8) << i + 1
            << std::setw(24) << values[i]
            << "\n";
    }
}

void write_selected_free_state_if_enabled(const int iload,
                                          const int accepted_iter,
                                          const std::vector<double>& x_free,
                                          const std::vector<double>& g_free,
                                          const MpiEnv& mpi) {
    if (!trace_enabled_for_step(iload, mpi)) {
        return;
    }
    static const std::set<int> requested = accepted_state_dump_steps();
    if (requested.find(accepted_iter) == requested.end()) {
        return;
    }

    const std::string base = trace_dump_dir() + "/step" + std::to_string(iload) +
                             "_accepted_" + std::to_string(accepted_iter);
    write_vector_dump(base + "_xfree.dat", x_free);
    write_vector_dump(base + "_gfree.dat", g_free);
}

void write_eval_trace_header_if_enabled(const int iload,
                                        const MpiEnv& mpi) {
    if (!trace_enabled_for_step(iload, mpi)) {
        return;
    }
    const std::string path =
        trace_dump_dir() + "/step" + std::to_string(iload) + "_eval_trace.dat";
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open " + path);
    }
    out << "# eval f node1_z node5_z node45_z node85_z node125_z node165_z node205_z"
        << " node41_x node1641_y node1681_x node1681_y\n";
}

void append_eval_trace_if_enabled(const int iload,
                                  const Coords& coords,
                                  const int eval_index,
                                  const double f,
                                  const MpiEnv& mpi) {
    if (!trace_enabled_for_step(iload, mpi)) {
        return;
    }
    const std::string path =
        trace_dump_dir() + "/step" + std::to_string(iload) + "_eval_trace.dat";
    std::ofstream out(path, std::ios::app);
    if (!out) {
        throw std::runtime_error("cannot open " + path);
    }
    auto coord = [&](int one_based, int axis) -> double {
        return coords.at(static_cast<std::size_t>(one_based - 1))[axis];
    };
    out << std::uppercase << std::scientific << std::setprecision(16)
        << std::setw(6) << eval_index
        << std::setw(24) << f
        << std::setw(24) << coord(1, 2)
        << std::setw(24) << coord(5, 2)
        << std::setw(24) << coord(45, 2)
        << std::setw(24) << coord(85, 2)
        << std::setw(24) << coord(125, 2)
        << std::setw(24) << coord(165, 2)
        << std::setw(24) << coord(205, 2)
        << std::setw(24) << coord(41, 0)
        << std::setw(24) << coord(1641, 1)
        << std::setw(24) << coord(1681, 0)
        << std::setw(24) << coord(1681, 1)
        << "\n";
}

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
    e.E_total    = res.reduced_energy;
    e.E_internal = res.reduced_energy;  // no vdw/external separation yet
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
std::vector<double> real_node_forces(const AssemblyResult& res, int numnods) {
    return std::vector<double>(res.force.begin(), res.force.begin() + 3 * numnods);
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

void write_runtime_checkpoint(const SimulatorInput& input,
                              const RuntimeState& state,
                              const std::string& output_dir,
                              const int iload,
                              const int icycle,
                              const int nprocs) {
    io::CheckpointData checkpoint;
    checkpoint.iload = iload;
    checkpoint.icycle = icycle;
    checkpoint.nprocs = nprocs;
    checkpoint.config.coords = state.coords;
    checkpoint.config.eta = state.eta;
    checkpoint.K0_ref = state.K0_ref;
    io::write_checkpoint(output_dir + "/nano_checkpoint.dat",
                         checkpoint,
                         input.mesh.numnods,
                         input.mesh.numele,
                         input.dims.ngauss,
                         input.crease.ncrease == 1);
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

    LbfgsSolver solver(10, eps, 1.0e-12, 10000,
                       mpi.is_root() && lbfgs_monitor_enabled());

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

    int flag = solver.minimize(x_free, xnorm0, /*stop_on_first_trial=*/true, callback);

    // Broadcast and scatter final state.
    double gnorm = solver.gnorm();
    bcast_solver_state(mpi, flag, gnorm, x_free);

    // Scatter final free DOFs to coords.
    for (int i = 0; i < bcs.ndofOP; ++i) {
        const int flat_dof = bcs.mdofOP.at(static_cast<std::size_t>(i));
        state.coords.at(static_cast<std::size_t>(flat_dof / 3))[flat_dof % 3] =
            x_free.at(static_cast<std::size_t>(i));
    }

    // Canonical Fortran minimize_free never performs a post-LBFGS reassembly.
    // It preserves the last in-loop energy/force/eta state, then scatters the
    // final x_short back into x0. Keep that behavior for executable-path parity.

    MinimizeFreeResult result;
    result.E     = final_E;
    result.gnorm = gnorm;
    result.E_min = final_asm.total_energy;
    result.assembly = final_asm;

    (void)flag;
    return result;
}

// ─── minimize_constrained ────────────────────────────────────────────────────

MinimizeResult minimize_constrained(const SimulatorInput& input,
                                     RuntimeState& state,
                                     LoadController& load_ctrl,
                                     const MpiEnv& mpi,
                                     double eps,
                                     const int trace_iload) {
    const BCData& bcs = input.bcs;

    // Compute XNORM0 from initial coords bbox (mirrors Fortran minimize.f90 lines 43-46).
    const double xnorm0 = compute_xnorm0(state.coords);

    // Extract free DOFs.
    std::vector<double> x_free = load_ctrl.to_free(state.coords);

    LbfgsSolver solver(10, eps, 1.0e-12, 20000,
                       mpi.is_root() && lbfgs_monitor_enabled());

    EnergyComponents final_E{};
    AssemblyResult final_asm{};
    bool first_eval_dumped = false;
    int eval_trace_index = 0;
    write_eval_trace_header_if_enabled(trace_iload, mpi);
    write_lbfgs_step_trace_header_if_enabled(trace_iload, mpi);
    solver.set_accepted_step_observer([&](const int iter,
                                          const int nfun,
                                          const double f,
                                          const double critc,
                                          const double stp,
                                          const std::vector<double>& x_trial,
                                          const std::vector<double>& g_trial) {
        Coords traced_coords = state.coords;
        load_ctrl.scatter_all(x_trial, traced_coords);
        append_lbfgs_step_trace_if_enabled(trace_iload,
                                          traced_coords,
                                          iter,
                                          nfun,
                                          f,
                                          critc,
                                          stp,
                                          mpi);
        write_full_accepted_state_if_enabled(trace_iload,
                                             iter,
                                             traced_coords,
                                             state.eta,
                                             final_E,
                                             critc,
                                             mpi);
        write_selected_free_state_if_enabled(trace_iload,
                                             iter,
                                             x_trial,
                                             g_trial,
                                             mpi);
    });

    auto callback = [&](const std::vector<double>& xv)
        -> std::pair<double, std::vector<double>>
    {
        // Mirror Fortran long(...): scatter free DOFs and restore BC DOFs
        // from x0_BC before each energy evaluation.
        load_ctrl.scatter_all(xv, state.coords);
        if (!first_eval_dumped) {
            write_coord_dump_if_enabled(state, "before_first_eval", trace_iload, mpi);
            write_eta_dump_if_enabled(state, "before_first_eval", trace_iload, mpi);
        }

        // Assemble.
        const auto res = assemble_energy_forces(input, state, mpi);
        final_asm = res;
        final_E = to_energy_components(res);
        ++eval_trace_index;
        append_eval_trace_if_enabled(trace_iload, state.coords, eval_trace_index, res.total_energy, mpi);
        if (!first_eval_dumped) {
            const auto forces_real = real_node_forces(res, input.mesh.numnods);
            double reaction1 = 0.0;
            double reaction2 = 0.0;
            load_ctrl.compute_reaction(forces_real, reaction1, reaction2);
            write_summary_dump_if_enabled("before_first_eval",
                                          final_E,
                                          res.total_energy,
                                          res.reduced_energy,
                                          std::numeric_limits<double>::quiet_NaN(),
                                          res.inner_fail,
                                          trace_iload,
                                          mpi);
            write_reaction_dump_if_enabled("before_first_eval",
                                           bcs,
                                           forces_real,
                                           reaction1,
                                           reaction2,
                                           trace_iload,
                                           mpi);
            first_eval_dumped = true;
        }

        // Gradient at free DOFs.
        return {res.total_energy, extract_free_gradient(res, bcs)};
    };

    int flag = solver.minimize(x_free, xnorm0, /*stop_on_first_trial=*/false, callback);

    // MPI broadcast (mirrors Fortran MPI_BCAST of IFLAG, GNORM, X).
    double gnorm = solver.gnorm();
    bcast_solver_state(mpi, flag, gnorm, x_free);

    // Final scatter mirrors Fortran long(...).
    load_ctrl.scatter_all(x_free, state.coords);

    // Canonical Fortran minimize likewise keeps the last in-loop assembly and
    // only scatters the final x vector back into x0 at exit. Do not force a
    // post-LBFGS reassembly here.

    MinimizeResult result;
    result.E     = final_E;
    result.gnorm = gnorm;
    result.E_min = final_asm.total_energy;
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

    if (bcs.nCodeLoad != 3 && bcs.nCodeLoad != 30 && bcs.nCodeLoad != 31) {
        throw std::runtime_error("pasapas: unsupported nCodeLoad " +
                                 std::to_string(bcs.nCodeLoad));
    }
    const int final_load = iload_stop > 0 ? iload_stop : bcs.nloadstep;
    if (final_load < iload_start || final_load > bcs.nloadstep) {
        throw std::runtime_error("pasapas: invalid load-step range");
    }

    // Build load controller and initialise BC positions.
    LoadController load_ctrl(bcs);
    load_ctrl.init(state.coords);

    if (iload_start == 1) {
        // ── Step 0: free minimisation (mirrors Fortran pasapas.f90 lines 55-62) ───
        auto step0 = minimize_free(input, state, mpi, eps);
        write_eta_dump_if_enabled(state, "post_free", 0, mpi);

        if (mpi.is_root()) {
            std::cout << "enforce the boundary change\n";
        }

        load_ctrl.init(state.coords);

        if (mpi.is_root()) {
            std::ofstream ff(output_dir + "/force.dat", std::ios::out | std::ios::trunc);
            (void)ff;
            write_output_header(output_dir, step0.E.E_total);

            const std::string energy_path = output_dir + "/energy.dat";
            std::ofstream ef(energy_path, std::ios::out | std::ios::trunc);
            if (!ef) throw std::runtime_error("cannot open " + energy_path);
            ef << std::setw(14) << "Load_Step"
               << std::setw(16) << "E_total"
               << std::setw(16) << "E_internal"
               << std::setw(16) << "E_vdw"
               << std::setw(16) << "E_external"
               << std::setw(16) << "GNORM"
               << "\n";
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
    } else {
        load_ctrl.init(state.coords);
    }

    // ── Load steps ────────────────────────────────────────────────────────────
    for (int iload = iload_start; iload <= final_load; ++iload) {
        int icycle = 1;
        int iphase = 1;
        int iload_in_cycle = iload;
        if (bcs.nCodeLoad == 30 || bcs.nCodeLoad == 31) {
            const int steps_per_cycle = bcs.nloadstep_comp + bcs.nloadstep_rel;
            icycle = (iload - 1) / steps_per_cycle + 1;
            iload_in_cycle = ((iload - 1) % steps_per_cycle) + 1;
            iphase = iload_in_cycle <= bcs.nloadstep_comp ? 1 : 2;
        }

        if (mpi.is_root()) {
            std::cout << "***\n Load Step: " << iload << "\n";
            if (bcs.nCodeLoad == 30 || bcs.nCodeLoad == 31) {
                std::cout << " Cycle: " << icycle << " Phase: " << iphase << "\n";
            }
        }

        // Apply load increment (moves BC nodes, updates coords).
        load_ctrl.apply_increment(iload, state.coords);
        write_coord_dump_if_enabled(state, "after_increment", iload, mpi);
        if (should_stop_after_stage("after_increment", iload, mpi)) {
            return;
        }
        apply_imperfections(input, state, iload);
        write_coord_dump_if_enabled(state, "after_imperfection", iload, mpi);
        if (should_stop_after_stage("after_imperfection", iload, mpi)) {
            return;
        }

        // Constrained minimisation.
        auto min_res = minimize_constrained(input, state, load_ctrl, mpi, eps, iload);
        write_coord_dump_if_enabled(state, "after_minimize", iload, mpi);
        write_eta_dump_if_enabled(state, "after_minimize", iload, mpi);
        write_coord_dump_if_enabled(state, "before_output", iload, mpi);
        write_eta_dump_if_enabled(state, "before_output", iload, mpi);

        // Reuse the last converged assembly, matching the Fortran runtime path.
        const auto forces_real = real_node_forces(min_res.assembly, input.mesh.numnods);

        double reaction1 = 0.0, reaction2 = 0.0;
        load_ctrl.compute_reaction(forces_real, reaction1, reaction2);
        write_summary_dump_if_enabled("before_output",
                                      min_res.E,
                                      min_res.assembly.total_energy,
                                      min_res.assembly.reduced_energy,
                                      min_res.gnorm,
                                      min_res.assembly.inner_fail,
                                      iload,
                                      mpi);
        write_reaction_dump_if_enabled("before_output",
                                       bcs,
                                       forces_real,
                                       reaction1,
                                       reaction2,
                                       iload,
                                       mpi);

        const double load_param = bcs.value * static_cast<double>(iload) /
                                  static_cast<double>(bcs.nloadstep);

        if (mpi.is_root()) {
            std::cout << " Equilibrium energy: " << min_res.E.E_total << "\n";

            const std::string energy_path = output_dir + "/energy.dat";
            std::ofstream ef(energy_path, std::ios::app);
            const std::string force_path = output_dir + "/force.dat";
            std::ofstream ff(force_path, std::ios::app);
            if (bcs.nCodeLoad == 30 || bcs.nCodeLoad == 31) {
                ef << std::setw(8) << iload
                   << std::setw(6) << icycle
                   << std::setw(4) << iphase
                   << std::scientific << std::setprecision(8)
                   << std::setw(16) << min_res.E.E_total
                   << std::setw(16) << min_res.E.E_internal
                   << std::setw(16) << min_res.E.E_vdw
                   << std::setw(16) << min_res.E.E_external
                   << std::scientific << std::setprecision(9)
                   << std::setw(17) << min_res.gnorm
                   << "\n";
                ff << std::setw(8) << iload
                   << std::setw(6) << icycle
                   << std::setw(4) << iphase
                   << std::fixed << std::setprecision(9)
                   << std::setw(17) << reaction1
                   << std::setw(17) << reaction2
                   << "\n";
            } else {
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
                ff << std::fixed << std::setprecision(9)
                   << std::setw(17) << load_param
                   << std::setw(17) << min_res.E_min
                   << std::setw(17) << reaction1
                   << std::setw(17) << reaction2
                   << "\n";
            }

            append_output_step(output_dir, iload, min_res.E.E_total);
            write_mesh_snapshot(input, state, output_dir, iload);
            if ((bcs.nCodeLoad == 30 || bcs.nCodeLoad == 31) &&
                iload_in_cycle == bcs.nloadstep_comp + bcs.nloadstep_rel) {
                write_runtime_checkpoint(input, state, output_dir, iload, icycle, mpi.size());
            }
        }
    }

    if (mpi.is_root()) {
        write_final_config(input, state, output_dir);
        write_mesh_series_index(output_dir, bcs, final_load);
    }
}

}  // namespace fce
