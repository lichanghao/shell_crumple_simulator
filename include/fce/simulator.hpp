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
    VdwData vdw;
    std::vector<double> imperfection_trace;
    BCData bcs;
    GaussData gauss;
};

struct RuntimeState {
    Coords coords{};
    EtaField eta{};
};

struct AssemblyResult {
    double total_energy{0.0};
    std::vector<double> force{};
    int inner_fail{0};
    EtaField eta_updates{};
};

SimulatorInput load_simulator_input(const std::string& case_dir);
RuntimeState make_runtime_state(const SimulatorInput& input);

Coords read_vtu_points(const std::string& path, int expected_points);

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      RuntimeState& state,
                                      int element_begin,
                                      int element_end);

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      const Coords& coords,
                                      int element_begin,
                                      int element_end);

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      RuntimeState& state,
                                      const MpiEnv& mpi);

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      const Coords& coords,
                                      const MpiEnv& mpi);

}  // namespace fce
