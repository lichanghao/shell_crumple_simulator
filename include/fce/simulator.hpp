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
    CreaseData crease;
    VdwData vdw;
    std::vector<double> imperfection_trace;
    BCData bcs;
    GaussData gauss;
};

struct RuntimeState {
    Coords coords{};
    EtaField eta{};
    std::vector<std::vector<std::array<double, 3>>> K0_ref{};
};

enum class CheckpointResumeStatus {
    not_found,
    loaded,
    rank_count_mismatch,
    read_failed,
};

struct CheckpointResumeResult {
    CheckpointResumeStatus status{CheckpointResumeStatus::not_found};
    RuntimeState state{};
    int iload_start{1};
    int checkpoint_nprocs{0};
    std::string error_detail{};
};

struct AssemblyResult {
    double total_energy{0.0};
    double reduced_energy{0.0};
    double vdw_reduced_energy{0.0};
    std::vector<double> force{};
    int inner_fail{0};
    EtaField eta_updates{};
};

struct RuntimeVdwPotential {
    double energy{0.0};
    double derivative{0.0};
};

SimulatorInput load_simulator_input(const std::string& case_dir);
RuntimeState make_runtime_state(const SimulatorInput& input);
CheckpointResumeResult load_runtime_checkpoint(const SimulatorInput& input,
                                              const std::string& case_dir,
                                              int current_nprocs,
                                              const RuntimeState& initial_state);

Coords read_vtu_points(const std::string& path, int expected_points);

RuntimeVdwPotential evaluate_runtime_vdw_cut_potential(const VdwData& vdw, double r);
bool runtime_vdw_pair_allowed(const Mesh& mesh, const VdwData& vdw, int gauss_i, int gauss_j);

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      RuntimeState& state,
                                      int element_begin,
                                      int element_end,
                                      int element_stride = 1);

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      const Coords& coords,
                                      int element_begin,
                                      int element_end,
                                      int element_stride = 1);

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      RuntimeState& state,
                                      const MpiEnv& mpi);

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      const Coords& coords,
                                      const MpiEnv& mpi);

}  // namespace fce
