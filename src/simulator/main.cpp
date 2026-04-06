#include "fce/simulator.hpp"
#include "fce/solver.hpp"
#include "fce/mpi_env.hpp"

#include <iomanip>
#include <iostream>
#include <optional>
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
                std::cerr << "usage: crunch_it <case_dir> [stop_step] [--single-step <step>]\n";
            }
            return 1;
        }

        const std::string case_dir = argv[1];

        bool single_step_mode = false;
        int step = 1;
        std::optional<int> requested_stop_step;
        for (int i = 2; i < argc; ++i) {
            const std::string arg(argv[i]);
            if (arg == "--single-step") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("--single-step requires an integer step");
                }
                single_step_mode = true;
                step = std::stoi(argv[i + 1]);
                ++i;
                continue;
            }

            if (requested_stop_step.has_value()) {
                throw std::invalid_argument("unexpected extra argument: " + arg);
            }
            requested_stop_step = std::stoi(arg);
        }

        const auto input = fce::load_simulator_input(case_dir);
        if (requested_stop_step.has_value()) {
            if (*requested_stop_step <= 0) {
                throw std::invalid_argument("stop_step must be positive");
            }
            if (*requested_stop_step > input.bcs.nloadstep) {
                throw std::invalid_argument("stop_step exceeds BCs%nloadstep from nano_BCs.dat");
            }
        }

        if (single_step_mode) {
            const auto coords = fce::read_vtu_points(case_dir + "/" + step_filename(step),
                                                     input.mesh.numnods);
            const auto result = fce::assemble_energy_forces(input, coords, mpi);

            if (mpi.is_root()) {
                std::cout << "assembled_energy " << std::setprecision(17) << result.total_energy << "\n";
                std::cout << "inner_fail " << result.inner_fail << "\n";
                std::cout << "force_dofs " << result.force.size() << "\n";
            }
        } else {
            auto state = fce::make_runtime_state(input);

            const double eps = input.general.crit_global > 0.0 ? input.general.crit_global : 1.0e-8;
            fce::pasapas(input,
                         state,
                         mpi,
                         case_dir,
                         eps,
                         1,
                         requested_stop_step.value_or(input.bcs.nloadstep));
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
