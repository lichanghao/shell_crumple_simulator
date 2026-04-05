#include "fce/simulator.hpp"
#include "fce/solver.hpp"
#include "fce/mpi_env.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string step_filename(const int step) {
    if (step < 0) {
        throw std::invalid_argument("step must be non-negative");
    }

    std::ostringstream out;
    out << "mesh_config_" << std::setw(4) << std::setfill('0') << step << ".vtu";
    return out.str();
}

}  // namespace

int main(int argc, char** argv) {
    try {
        fce::MpiEnv mpi(argc, argv);

        if (argc < 2) {
            if (mpi.is_root()) {
                std::cerr << "usage: crunch_it <case_dir> [--single-step <step>]\n";
            }
            return 1;
        }

        const std::string case_dir = argv[1];

        // Check if running in legacy single-step assembly mode (--single-step N).
        bool single_step_mode = false;
        int step = 1;
        for (int i = 2; i < argc; ++i) {
            const std::string arg(argv[i]);
            if (arg == "--single-step" && i + 1 < argc) {
                single_step_mode = true;
                step = std::stoi(argv[i + 1]);
                break;
            }
        }
        // Legacy: if second argument is a plain integer, treat as single-step mode.
        if (!single_step_mode && argc >= 3) {
            try {
                step = std::stoi(argv[2]);
                single_step_mode = true;
            } catch (...) {
                single_step_mode = false;
            }
        }

        const auto input = fce::load_simulator_input(case_dir);

        if (single_step_mode) {
            // Legacy single-step assembly mode: load VTU, assemble once, print energy.
            const auto coords = fce::read_vtu_points(case_dir + "/" + step_filename(step),
                                                     input.mesh.numnods);
            const auto result = fce::assemble_energy_forces(input, coords, mpi);

            if (mpi.is_root()) {
                std::cout << "assembled_energy " << std::setprecision(17) << result.total_energy << "\n";
                std::cout << "inner_fail " << result.inner_fail << "\n";
                std::cout << "force_dofs " << result.force.size() << "\n";
            }
        } else {
            // Full pasapas run: load initial config and run all load steps.
            fce::Coords coords = input.initial_config.coords;

            // EPS: use crit_global from general data as convergence criterion.
            const double eps = input.general.crit_global > 0.0 ? input.general.crit_global : 1.0e-8;

            fce::pasapas(input, coords, mpi, case_dir, eps);
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
