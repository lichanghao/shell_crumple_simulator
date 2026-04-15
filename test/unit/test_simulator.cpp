#include "fce/simulator.hpp"

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

fs::path cyclic_prepro_case_dir() {
    return fs::path(ORACLE_DIR) / "graphene_cyclic_crumple" / "prepro_run";
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
