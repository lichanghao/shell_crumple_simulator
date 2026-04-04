#pragma once

#include "fce/element_energy.hpp"
#include "fce/io.hpp"
#include "fce/mesh_generator.hpp"
#include "fce/mpi_env.hpp"
#include "fce/quadrature.hpp"

#include <string>

namespace fce {

struct SimulatorInput {
    io::DimsData dims;
    io::GeneralData general;
    Mesh mesh;
    std::vector<RefConfig> ref_config;
    io::ConfigData initial_config;
    BCData bcs;
    GaussData gauss;
};

struct AssemblyResult {
    double total_energy{0.0};
    std::vector<double> force{};
    int inner_fail{0};
};

SimulatorInput load_simulator_input(const std::string& case_dir);

Coords read_vtu_points(const std::string& path, int expected_points);

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      const Coords& coords,
                                      int element_begin,
                                      int element_end);

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      const Coords& coords,
                                      const MpiEnv& mpi);

}  // namespace fce
