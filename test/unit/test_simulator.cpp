#include "fce/simulator.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#ifndef ORACLE_DIR
#define ORACLE_DIR "test/cases"
#endif

namespace {

namespace fs = std::filesystem;

double relative_tolerance(double expected) {
    return std::max(1e-12, std::abs(expected) * 1e-4);
}

}  // namespace

TEST(SimulatorAssembly, LoadStepOneEnergyMatchesArchivedCompressionOracle) {
    const fs::path case_dir =
        fs::path(ORACLE_DIR) / "graphene_compression_simulator" / "np1";

    const auto input = fce::load_simulator_input(case_dir.string());
    const auto coords = fce::read_vtu_points(
        (case_dir / "mesh_config_0001.vtu").string(),
        input.mesh.numnods);
    const auto result = fce::assemble_energy_forces(input, coords, 0, input.mesh.numele);

    constexpr double expected_energy = 5.7210528e-05;
    EXPECT_NEAR(result.total_energy, expected_energy, relative_tolerance(expected_energy));
    EXPECT_EQ(result.inner_fail, 0);
    EXPECT_EQ(result.force.size(),
              static_cast<std::size_t>(3 * (input.mesh.numnods + input.mesh.nedge)));
}

TEST(SimulatorAssembly, SplitRangesSumToFullAssembly) {
    const fs::path case_dir =
        fs::path(ORACLE_DIR) / "graphene_compression_simulator" / "np1";

    const auto input = fce::load_simulator_input(case_dir.string());
    const auto coords = fce::read_vtu_points(
        (case_dir / "mesh_config_0001.vtu").string(),
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

TEST(SimulatorAssembly, CorruptedVtuPointCountIsRejected) {
    const fs::path temp_dir =
        fs::temp_directory_path() / "fce_simulator_corrupted_vtu";
    fs::create_directories(temp_dir);
    const fs::path vtu_path = temp_dir / "broken.vtu";

    std::ofstream out(vtu_path);
    ASSERT_TRUE(out.is_open());
    out << "<?xml version=\"1.0\"?>\n";
    out << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
    out << "  <UnstructuredGrid>\n";
    out << "    <Piece NumberOfPoints=\"3\" NumberOfCells=\"1\">\n";
    out << "      <Points>\n";
    out << "        <DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">\n";
    out << "          0 0 0\n";
    out << "          1 0 0\n";
    out << "        </DataArray>\n";
    out << "      </Points>\n";
    out << "    </Piece>\n";
    out << "  </UnstructuredGrid>\n";
    out << "</VTKFile>\n";
    out.close();

    EXPECT_THROW(
        (void)fce::read_vtu_points(vtu_path.string(), 3),
        std::runtime_error);

    fs::remove_all(temp_dir);
}
