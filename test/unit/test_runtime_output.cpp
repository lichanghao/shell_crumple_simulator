#include "fce/runtime_output.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

namespace fs = std::filesystem;

fs::path make_temp_dir() {
    std::array<char, 256> pattern{};
    const std::string templ = (fs::temp_directory_path() / "fce_runtime_output_XXXXXX").string();
    if (templ.size() + 1 > pattern.size()) {
        throw std::runtime_error("temp path template is unexpectedly long");
    }
    std::snprintf(pattern.data(), pattern.size(), "%s", templ.c_str());
    char* created = ::mkdtemp(pattern.data());
    if (created == nullptr) {
        throw std::runtime_error("mkdtemp failed");
    }
    return fs::path(created);
}

std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open file: " + path.string());
    }
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

fce::SimulatorInput make_minimal_input() {
    fce::SimulatorInput input;
    input.dims.numele = 1;
    input.dims.numnods = 3;
    input.dims.ngauss = 2;
    input.dims.nvdw = 0;
    input.mesh.numele = 1;
    input.mesh.numnods = 3;
    input.mesh.connect.resize(1);
    input.mesh.connect[0].vertices = {0, 1, 2};
    input.bcs.nloadstep = 50;
    input.bcs.value = 1.0;
    return input;
}

fce::RuntimeState make_minimal_state() {
    fce::RuntimeState state;
    state.coords = {
        fce::Vec3{0.0, 0.0, 0.0},
        fce::Vec3{1.0, 0.0, 0.0},
        fce::Vec3{0.0, 1.0, 0.0},
    };
    state.eta = {
        std::vector<fce::Vec2>{
            fce::Vec2{1.0, 2.0},
            fce::Vec2{3.0, 4.0},
        },
    };
    return state;
}

}  // namespace

TEST(RuntimeOutput, WritesCanonicalZeroDensityFields) {
    const auto input = make_minimal_input();
    const auto state = make_minimal_state();
    const fs::path temp_dir = make_temp_dir();

    ASSERT_NO_THROW(fce::write_mesh_snapshot(input, state, temp_dir.string(), 1));
    ASSERT_NO_THROW(fce::write_mesh_series_index(temp_dir.string(), input.bcs, 1));

    const std::string xml = read_file(temp_dir / "mesh_config_0001.vtu");
    EXPECT_NE(xml.find("<PointData Scalars=\"atomic_density\">"), std::string::npos);
    EXPECT_NE(xml.find("Name=\"atomic_density\""), std::string::npos);
    EXPECT_NE(xml.find("Name=\"W_density\""), std::string::npos);
    EXPECT_NE(xml.find(" 2.0000000000000000E+00"), std::string::npos);
    EXPECT_NE(xml.find(" 3.0000000000000000E+00"), std::string::npos);
    EXPECT_NE(xml.find(" 0.0000000000000000E+00"), std::string::npos);

    const auto coords = fce::read_vtu_points((temp_dir / "mesh_config_0001.vtu").string(), 3);
    ASSERT_EQ(coords.size(), 3U);
    EXPECT_DOUBLE_EQ(coords[1][0], 1.0);
    EXPECT_DOUBLE_EQ(coords[2][1], 1.0);

    const std::string pvd = read_file(temp_dir / "mesh_config_series.pvd");
    EXPECT_NE(pvd.find("mesh_config_0001.vtu"), std::string::npos);
    EXPECT_NE(pvd.find("2.0000000000000000E-02"), std::string::npos);

    fs::remove_all(temp_dir);
}

TEST(RuntimeOutput, RejectsInvalidRuntimeState) {
    const auto input = make_minimal_input();
    auto state = make_minimal_state();
    state.coords.pop_back();

    const fs::path temp_dir = make_temp_dir();
    EXPECT_THROW(fce::write_mesh_snapshot(input, state, temp_dir.string(), 0), std::runtime_error);
    fs::remove_all(temp_dir);
}
