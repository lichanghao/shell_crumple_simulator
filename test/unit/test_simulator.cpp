#include "fce/simulator.hpp"
#include "fce/load_controller.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef ORACLE_DIR
#define ORACLE_DIR "test/cases"
#endif

namespace {

namespace fs = std::filesystem;

double relative_tolerance(double expected) {
    return std::max(1e-12, std::abs(expected) * 1e-4);
}

fs::path archived_case_dir() {
    return fs::path(ORACLE_DIR) / "graphene_compression_simulator" / "np1";
}

fs::path bilayer_nwhat_case_dir() {
    return fs::path(ORACLE_DIR) / "graphene_bilayer_twist_vdw_1000" / "prepro_run";
}

fs::path self_contact_case_dir() {
    return fs::path(ORACLE_DIR) / "graphene_self_contact" / "prepro_run";
}

fs::path self_contact_simulator_dir() {
    return fs::path(ORACLE_DIR) / "graphene_self_contact" / "simulator_run";
}

fs::path cyclic_prepro_case_dir() {
    return fs::path(ORACLE_DIR) / "graphene_cyclic_crumple" / "prepro_run";
}

struct OracleEnergyComponents {
    double total{0.0};
    double internal{0.0};
    double vdw{0.0};
};

struct OracleReaction {
    double reaction1{0.0};
    double reaction2{0.0};
};

OracleEnergyComponents read_self_contact_energy_components(const fs::path& energy_path,
                                                           const int step) {
    std::ifstream in(energy_path);
    if (!in) {
        throw std::runtime_error("cannot open energy oracle: " + energy_path.string());
    }

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        std::vector<std::string> tokens;
        std::string token;
        while (row >> token) {
            tokens.push_back(token);
        }
        if (tokens.size() < 8 || tokens.front() != std::to_string(step)) {
            continue;
        }
        return OracleEnergyComponents{
            fce::io::parse_fortran_double(tokens[3]),
            fce::io::parse_fortran_double(tokens[4]),
            fce::io::parse_fortran_double(tokens[5]),
        };
    }

    throw std::runtime_error("missing self-contact energy oracle row for step " +
                             std::to_string(step));
}

OracleReaction read_self_contact_reaction(const fs::path& force_path, const int step) {
    std::ifstream in(force_path);
    if (!in) {
        throw std::runtime_error("cannot open force oracle: " + force_path.string());
    }

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        std::vector<std::string> tokens;
        std::string token;
        while (row >> token) {
            tokens.push_back(token);
        }
        if (tokens.size() < 5 || tokens.front() != std::to_string(step)) {
            continue;
        }
        return OracleReaction{
            fce::io::parse_fortran_double(tokens[3]),
            fce::io::parse_fortran_double(tokens[4]),
        };
    }

    throw std::runtime_error("missing self-contact force oracle row for step " +
                             std::to_string(step));
}

std::vector<double> read_archived_step_energies(const fs::path& energy_path) {
    std::ifstream in(energy_path);
    if (!in) {
        throw std::runtime_error("cannot open energy oracle: " + energy_path.string());
    }

    std::vector<double> energies;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        std::vector<std::string> tokens;
        std::string token;
        while (row >> token) {
            tokens.push_back(token);
        }
        if (tokens.size() < 2) {
            continue;
        }

        double load_step = 0.0;
        try {
            load_step = fce::io::parse_fortran_double(tokens[0]);
        } catch (const std::exception&) {
            continue;
        }
        if (load_step <= 0.0) {
            continue;
        }
        energies.push_back(fce::io::parse_fortran_double(tokens[1]));
    }

    return energies;
}

fs::path step_vtu_path(const fs::path& case_dir, const int step) {
    std::ostringstream name;
    name << "mesh_config_" << std::setw(4) << std::setfill('0') << step << ".vtu";
    return case_dir / name.str();
}

std::size_t count_output_load_steps(const fs::path& output_path) {
    std::ifstream in(output_path);
    if (!in) {
        throw std::runtime_error("cannot open output oracle: " + output_path.string());
    }

    std::size_t count = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("Load Step") != std::string::npos) {
            ++count;
        }
    }
    return count;
}

}  // namespace

TEST(SimulatorAssembly, LoadStepOneEnergyMatchesArchivedCompressionOracle) {
    const fs::path case_dir = archived_case_dir();

    const auto input = fce::load_simulator_input(case_dir.string());
    const auto coords = fce::read_vtu_points(
        step_vtu_path(case_dir, 1).string(),
        input.mesh.numnods);
    const auto result = fce::assemble_energy_forces(input, coords, 0, input.mesh.numele);
    const auto energies = read_archived_step_energies(case_dir / "energy.dat");

    ASSERT_GE(energies.size(), 1U);
    const double expected_energy = energies[0];
    EXPECT_NEAR(result.total_energy, expected_energy, relative_tolerance(expected_energy));
    EXPECT_EQ(result.inner_fail, 0);
    EXPECT_EQ(result.force.size(),
              static_cast<std::size_t>(3 * (input.mesh.numnods + input.mesh.nedge)));
}

TEST(SimulatorAssembly, ArchivedEnergyTrajectoryMatchesOracleFile) {
    const fs::path case_dir = archived_case_dir();

    const auto input = fce::load_simulator_input(case_dir.string());
    const auto energies = read_archived_step_energies(case_dir / "energy.dat");

    ASSERT_EQ(energies.size(), count_output_load_steps(case_dir / "output.dat"));
    for (std::size_t step_index = 0; step_index < energies.size(); ++step_index) {
        const int step = static_cast<int>(step_index + 1);
        const auto coords = fce::read_vtu_points(
            step_vtu_path(case_dir, step).string(),
            input.mesh.numnods);
        const auto result = fce::assemble_energy_forces(input, coords, 0, input.mesh.numele);
        const double expected_energy = energies[step_index];
        EXPECT_NEAR(result.total_energy, expected_energy, relative_tolerance(expected_energy))
            << "step=" << step;
    }
}

TEST(SimulatorAssembly, SplitRangesSumToFullAssembly) {
    const fs::path case_dir = archived_case_dir();

    const auto input = fce::load_simulator_input(case_dir.string());
    const auto coords = fce::read_vtu_points(
        step_vtu_path(case_dir, 1).string(),
        input.mesh.numnods);

    const auto full = fce::assemble_energy_forces(input, coords, 0, input.mesh.numele);
    const auto left = fce::assemble_energy_forces(input, coords, 0, input.mesh.numele / 2);
    const auto right = fce::assemble_energy_forces(
        input, coords, input.mesh.numele / 2, input.mesh.numele);

    EXPECT_NEAR(left.total_energy + right.total_energy, full.total_energy, 1e-12);
    ASSERT_EQ(left.force.size(), full.force.size());
    ASSERT_EQ(right.force.size(), full.force.size());
    for (std::size_t i = 0; i < full.force.size(); ++i) {
        EXPECT_NEAR(left.force[i] + right.force[i], full.force[i], 1e-10) << "force[" << i << "]";
    }
}

TEST(SimulatorAssembly, StridedRangesSumToContiguousAssembly) {
    const fs::path case_dir = archived_case_dir();

    const auto input = fce::load_simulator_input(case_dir.string());
    const auto coords = fce::read_vtu_points(
        step_vtu_path(case_dir, 1).string(),
        input.mesh.numnods);

    constexpr int kRanks = 8;
    const int element_end = std::min(input.mesh.numele, 64);
    const auto contiguous = fce::assemble_energy_forces(input, coords, 0, element_end);

    double total_energy = 0.0;
    std::vector<double> force_sum(contiguous.force.size(), 0.0);
    for (int rank = 0; rank < kRanks; ++rank) {
        const auto part = fce::assemble_energy_forces(input, coords, rank, element_end, kRanks);
        total_energy += part.total_energy;
        ASSERT_EQ(part.force.size(), contiguous.force.size());
        for (std::size_t i = 0; i < force_sum.size(); ++i) {
            force_sum[i] += part.force[i];
        }
    }

    EXPECT_NEAR(total_energy, contiguous.total_energy, 1e-12);
    for (std::size_t i = 0; i < force_sum.size(); ++i) {
        EXPECT_NEAR(force_sum[i], contiguous.force[i], 1e-10) << "force[" << i << "]";
    }
}

TEST(SimulatorAssembly, CorruptedMeshInputIsRejected) {
    const fs::path case_dir = archived_case_dir();
    const fs::path temp_dir =
        fs::temp_directory_path() / "fce_simulator_corrupted_mesh";
    fs::create_directories(temp_dir);

    for (const auto* file : {
             "nano_dims.dat",
             "nano_general.dat",
             "nano_zero.dat",
             "nano_config.dat",
             "nano_BCs.dat",
         }) {
        fs::copy_file(case_dir / file,
                      temp_dir / file,
                      fs::copy_options::overwrite_existing);
    }

    {
        std::ifstream in(case_dir / "nano_Mesh.dat");
        ASSERT_TRUE(in.is_open());
        std::ofstream out(temp_dir / "nano_Mesh.dat");
        ASSERT_TRUE(out.is_open());

        std::string line;
        bool corrupted = false;
        while (std::getline(in, line)) {
            if (!corrupted && line.find("        0        1") != std::string::npos) {
                out << "        0   999999\n";
                corrupted = true;
            } else {
                out << line << "\n";
            }
        }
        ASSERT_TRUE(corrupted);
    }

    const auto input = fce::load_simulator_input(temp_dir.string());
    const auto coords = fce::read_vtu_points(
        step_vtu_path(case_dir, 1).string(),
        input.mesh.numnods);

    EXPECT_THROW(
        (void)fce::assemble_energy_forces(input, coords, 0, input.mesh.numele),
        std::runtime_error);

    fs::remove_all(temp_dir);
}

TEST(SimulatorAssembly, StatefulAssemblyUsesRuntimeEtaInsteadOfInitialConfig) {
    auto input = fce::load_simulator_input(bilayer_nwhat_case_dir().string());
    ASSERT_TRUE(input.general.nW_hat);

    // Force the inner solver to accept the provided eta immediately so the test
    // directly checks which eta field the stateful assembly path uses as input.
    input.general.crit_local = 1.0e12;

    auto state = fce::make_runtime_state(input);
    const fce::Vec2 seeded_eta{
        0.01 * input.general.mat.A0,
        -0.015 * input.general.mat.A0,
    };
    state.eta.at(0).at(0) = seeded_eta;

    const auto result = fce::assemble_energy_forces(input, state, 0, 1);

    EXPECT_NEAR(result.eta_updates.at(0).at(0)[0], seeded_eta[0], 1e-15);
    EXPECT_NEAR(result.eta_updates.at(0).at(0)[1], seeded_eta[1], 1e-15);
    EXPECT_NEAR(state.eta.at(0).at(0)[0], seeded_eta[0], 1e-15);
    EXPECT_NEAR(state.eta.at(0).at(0)[1], seeded_eta[1], 1e-15);
}

TEST(SimulatorAssembly, SelfContactRuntimeVdwContributesEnergyAndForces) {
    auto input = fce::load_simulator_input(self_contact_case_dir().string());
    auto no_vdw_input = input;
    no_vdw_input.vdw.nvdw = 0;

    auto state = fce::make_runtime_state(input);
    auto no_vdw_state = fce::make_runtime_state(no_vdw_input);

    const auto internal = fce::assemble_energy_forces(
        no_vdw_input, no_vdw_state, 0, no_vdw_input.mesh.numele);
    const auto with_vdw = fce::assemble_energy_forces(input, state, 0, input.mesh.numele);

    ASSERT_EQ(with_vdw.force.size(), internal.force.size());
    EXPECT_TRUE(std::isfinite(with_vdw.vdw_reduced_energy));
    EXPECT_NE(with_vdw.vdw_reduced_energy, 0.0);
    EXPECT_NEAR(with_vdw.total_energy,
                internal.total_energy + with_vdw.vdw_reduced_energy,
                std::max(1e-12, std::abs(with_vdw.total_energy) * 1e-12));

    double force_delta_norm = 0.0;
    for (std::size_t i = 0; i < with_vdw.force.size(); ++i) {
        const double diff = with_vdw.force[i] - internal.force[i];
        force_delta_norm += diff * diff;
    }
    EXPECT_GT(std::sqrt(force_delta_norm), 0.0);
}

TEST(SimulatorAssembly, SelfContactRuntimeVdwStridedRangesSumToContiguousAssembly) {
    const auto input = fce::load_simulator_input(self_contact_case_dir().string());
    const auto coords = input.initial_config.coords;

    constexpr int kRanks = 8;
    const auto contiguous = fce::assemble_energy_forces(input, coords, 0, input.mesh.numele);

    double total_energy = 0.0;
    double reduced_energy = 0.0;
    double vdw_reduced_energy = 0.0;
    std::vector<double> force_sum(contiguous.force.size(), 0.0);
    for (int rank = 0; rank < kRanks; ++rank) {
        const auto part = fce::assemble_energy_forces(input, coords, rank, input.mesh.numele, kRanks);
        total_energy += part.total_energy;
        reduced_energy += part.reduced_energy;
        vdw_reduced_energy += part.vdw_reduced_energy;
        ASSERT_EQ(part.force.size(), contiguous.force.size());
        for (std::size_t i = 0; i < force_sum.size(); ++i) {
            force_sum[i] += part.force[i];
        }
    }

    EXPECT_NEAR(total_energy, contiguous.total_energy, relative_tolerance(contiguous.total_energy));
    EXPECT_NEAR(reduced_energy, contiguous.reduced_energy, relative_tolerance(contiguous.reduced_energy));
    EXPECT_NEAR(vdw_reduced_energy,
                contiguous.vdw_reduced_energy,
                relative_tolerance(contiguous.vdw_reduced_energy));
    for (std::size_t i = 0; i < force_sum.size(); ++i) {
        EXPECT_NEAR(force_sum[i], contiguous.force[i], 1e-9) << "force[" << i << "]";
    }
}

TEST(SimulatorAssembly, SelfContactStepOneEnergyComponentsMatchFortranOracle) {
    const fs::path case_dir = self_contact_case_dir();
    const fs::path simulator_dir = self_contact_simulator_dir();
    const fs::path step_one_vtu = simulator_dir / "mesh_config_0001.vtu";
    const fs::path energy_path = simulator_dir / "energy.dat";
    ASSERT_TRUE(fs::exists(step_one_vtu)) << "missing fixture: " << step_one_vtu;
    ASSERT_TRUE(fs::exists(energy_path)) << "missing fixture: " << energy_path;

    const auto input = fce::load_simulator_input(case_dir.string());
    const auto coords = fce::read_vtu_points(step_one_vtu.string(), input.mesh.numnods);
    auto state = fce::make_runtime_state(input);
    state.coords = coords;
    const auto result = fce::assemble_energy_forces(input, state, 0, input.mesh.numele);
    const auto expected = read_self_contact_energy_components(energy_path, 1);

    EXPECT_NEAR(result.total_energy, expected.total, relative_tolerance(expected.total));
    EXPECT_NEAR(result.reduced_energy, expected.internal, relative_tolerance(expected.internal));
    EXPECT_NEAR(result.vdw_reduced_energy, expected.vdw, relative_tolerance(expected.vdw));
    EXPECT_EQ(result.inner_fail, 0);
}

TEST(SimulatorAssembly, SelfContactStepOneReactionForcesMatchFortranOracle) {
    const fs::path case_dir = self_contact_case_dir();
    const fs::path simulator_dir = self_contact_simulator_dir();
    const fs::path step_one_vtu = simulator_dir / "mesh_config_0001.vtu";
    const fs::path force_path = simulator_dir / "force.dat";
    ASSERT_TRUE(fs::exists(step_one_vtu)) << "missing fixture: " << step_one_vtu;
    ASSERT_TRUE(fs::exists(force_path)) << "missing fixture: " << force_path;

    const auto input = fce::load_simulator_input(case_dir.string());
    const auto coords = fce::read_vtu_points(step_one_vtu.string(), input.mesh.numnods);
    auto state = fce::make_runtime_state(input);
    state.coords = coords;
    const auto result = fce::assemble_energy_forces(input, state, 0, input.mesh.numele);

    fce::LoadController load_ctrl(input.bcs);
    double reaction1 = 0.0;
    double reaction2 = 0.0;
    load_ctrl.compute_reaction(result.force, reaction1, reaction2);
    const auto expected = read_self_contact_reaction(force_path, 1);

    EXPECT_NEAR(reaction1, expected.reaction1, relative_tolerance(expected.reaction1));
    EXPECT_NEAR(reaction2, expected.reaction2, relative_tolerance(expected.reaction2));
}

TEST(SimulatorInput, CyclicCaseLoadsCreaseMetadataIntoRuntimeState) {
    const auto input = fce::load_simulator_input(cyclic_prepro_case_dir().string());
    ASSERT_EQ(input.crease.ncrease, 1);
    EXPECT_GT(input.crease.kappa_cr, 0.0);
    EXPECT_GE(input.crease.alpha_lock, 0.0);
    ASSERT_EQ(input.crease.K0_ref.size(), static_cast<std::size_t>(input.mesh.numele));
    ASSERT_EQ(input.crease.K0_ref.front().size(), static_cast<std::size_t>(input.dims.ngauss));
    EXPECT_DOUBLE_EQ(input.crease.K0_ref.front().front()[0], 0.0);
    EXPECT_DOUBLE_EQ(input.crease.K0_ref.front().front()[1], 0.0);
    EXPECT_DOUBLE_EQ(input.crease.K0_ref.front().front()[2], 0.0);

    const auto state = fce::make_runtime_state(input);
    ASSERT_EQ(state.K0_ref.size(), input.crease.K0_ref.size());
    ASSERT_EQ(state.K0_ref.front().size(), input.crease.K0_ref.front().size());
    EXPECT_DOUBLE_EQ(state.K0_ref.front().front()[0], 0.0);
    EXPECT_DOUBLE_EQ(state.K0_ref.front().front()[1], 0.0);
    EXPECT_DOUBLE_EQ(state.K0_ref.front().front()[2], 0.0);
}

TEST(SimulatorInput, SelfContactCaseLoadsRuntimeVdwData) {
    const auto input = fce::load_simulator_input(self_contact_case_dir().string());
    EXPECT_EQ(input.vdw.nvdw, 1);
    EXPECT_EQ(input.vdw.nneigh, -2);
    EXPECT_EQ(input.vdw.ngauss_vdw, 2);
    EXPECT_EQ(input.vdw.rho.size(), static_cast<std::size_t>(input.dims.ng_tot));
    ASSERT_EQ(input.vdw.shapef.size(), static_cast<std::size_t>(input.vdw.ngauss_vdw));
    EXPECT_EQ(input.vdw.shapef.front().size(), 12U);
}

TEST(SimulatorInput, BilayerCaseLoadsTubePartitionsIntoRuntimeVdwData) {
    const auto input = fce::load_simulator_input(bilayer_nwhat_case_dir().string());
    ASSERT_EQ(input.vdw.nvdw, 1);
    ASSERT_FALSE(input.vdw.tub_partitions.empty());
    EXPECT_EQ(input.vdw.tub_partitions.front().first, 0);
    EXPECT_GT(input.vdw.tub_partitions.front().second, input.vdw.tub_partitions.front().first);
}
