#include "fce/simulator.hpp"

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
                std::cerr << "usage: crunch_it <case_dir> [step]\n";
            }
            return 1;
        }

        const std::string case_dir = argv[1];
        const int step = (argc >= 3) ? std::stoi(argv[2]) : 1;

        const auto input = fce::load_simulator_input(case_dir);
        const auto coords = fce::read_vtu_points(case_dir + "/" + step_filename(step),
                                                 input.mesh.numnods);
        const auto result = fce::assemble_energy_forces(input, coords, mpi);

        if (mpi.is_root()) {
            std::cout << "assembled_energy " << std::setprecision(17) << result.total_energy << "\n";
            std::cout << "inner_fail " << result.inner_fail << "\n";
            std::cout << "force_dofs " << result.force.size() << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
