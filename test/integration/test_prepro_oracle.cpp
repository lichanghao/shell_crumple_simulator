#include "oracle_compare.hpp"

#include "fce/ghost_nodes.hpp"
#include "fce/io.hpp"
#include "fce/preprocessor.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

#ifndef ORACLE_DIR
#define ORACLE_DIR "test/cases"
#endif

namespace {

const std::filesystem::path kPreproDir =
    std::filesystem::path(ORACLE_DIR) / "graphene_compression_prepro";
const std::filesystem::path kCyclicDir =
    std::filesystem::path(ORACLE_DIR) / "graphene_cyclic_crumple" / "prepro_run";

std::filesystem::path make_tmp_dir()
{
    const auto base = std::filesystem::temp_directory_path();
    for (int attempt = 0; attempt < 100; ++attempt) {
        const auto unique = "fce_prepro_oracle_case_" +
                            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                            "_" + std::to_string(attempt);
        const auto dir = base / std::filesystem::path(unique);
        if (std::filesystem::create_directory(dir)) {
            return dir;
        }
    }
    throw std::runtime_error("Failed to create unique temporary oracle directory");
}

std::string read_text_file(const std::filesystem::path& path)
{
    std::ifstream in(path);
    EXPECT_TRUE(in.is_open()) << path.string();
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void write_text_file(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream out(path);
    ASSERT_TRUE(out.is_open()) << path.string();
    out << text;
}

bool replace_once(std::string& text, const std::string& from, const std::string& to)
{
    const auto pos = text.find(from);
    if (pos == std::string::npos) {
        return false;
    }
    text.replace(pos, from.size(), to);
    return true;
}

std::vector<std::array<double, 3>> compute_ghost_coords(const std::filesystem::path& dir)
{
    const auto dims = fce::io::read_dims((dir / "nano_dims.dat").string());
    const auto mesh = fce::io::read_mesh((dir / "nano_Mesh.dat").string(), dims.ngauss);
    const auto config = fce::io::read_config((dir / "nano_config.dat").string(),
                                             dims.numnods,
                                             dims.numele,
                                             dims.ngauss);

    fce::FlatCoords coords;
    coords.reserve((mesh.numnods + mesh.nedge) * 3);
    for (const auto& xyz : config.coords) {
        coords.push_back(xyz[0]);
        coords.push_back(xyz[1]);
        coords.push_back(xyz[2]);
    }
    fce::ghost_nodes(mesh, coords);

    std::vector<std::array<double, 3>> ghosts(mesh.nedge);
    for (int edge = 0; edge < mesh.nedge; ++edge) {
        const int base = (mesh.numnods + edge) * 3;
        ghosts[edge] = {coords[base], coords[base + 1], coords[base + 2]};
    }
    return ghosts;
}

void write_ghost_coords_file(const std::filesystem::path& path,
                             const std::vector<std::array<double, 3>>& ghosts)
{
    std::ofstream out(path);
    ASSERT_TRUE(out.is_open()) << path.string();
    out << std::setprecision(17) << std::scientific;
    for (const auto& xyz : ghosts) {
        out << xyz[0] << " " << xyz[1] << " " << xyz[2] << "\n";
    }
}

} // namespace

TEST(PreprocessorOracle, ArchivedCompressionCaseMatchesOracle)
{
    const auto work_dir = make_tmp_dir();
    std::filesystem::copy_file(kPreproDir / "data.dat",
                               work_dir / "data.dat",
                               std::filesystem::copy_options::overwrite_existing);

    fce::run_preprocessor(work_dir.string());

    EXPECT_TRUE(fce::test_support::compare_preprocessor_outputs(
        work_dir.string(),
        kPreproDir.string(),
        1.0e-12));
}

TEST(PreprocessorOracle, ArchivedCasesIncludeGhostCoordinateArtifacts)
{
    EXPECT_TRUE(std::filesystem::exists(kPreproDir / "ghost_coords.dat"));
    EXPECT_TRUE(std::filesystem::exists(kCyclicDir / "ghost_coords.dat"));
}

TEST(PreprocessorOracle, TempDirFactoryReturnsDistinctDirectories)
{
    const auto dir1 = make_tmp_dir();
    const auto dir2 = make_tmp_dir();

    EXPECT_NE(dir1, dir2);
    EXPECT_TRUE(std::filesystem::exists(dir1));
    EXPECT_TRUE(std::filesystem::exists(dir2));

    std::filesystem::remove_all(dir1);
    std::filesystem::remove_all(dir2);
}

TEST(PreprocessorOracle, ArchivedCyclicPreproInputMatchesOracleOutputs)
{
    const auto work_dir = make_tmp_dir();
    std::filesystem::copy_file(kCyclicDir / "data.dat",
                               work_dir / "data.dat",
                               std::filesystem::copy_options::overwrite_existing);

    EXPECT_NO_THROW(fce::run_preprocessor(work_dir.string()));
    EXPECT_TRUE(std::filesystem::exists(work_dir / "nano_dims.dat"));
    EXPECT_TRUE(std::filesystem::exists(work_dir / "nano_BCs.dat"));
    EXPECT_TRUE(std::filesystem::exists(work_dir / "nano_Mesh.dat"));
    EXPECT_TRUE(std::filesystem::exists(work_dir / "nano_crease.dat"));
    EXPECT_TRUE(fce::test_support::compare_preprocessor_outputs(
        work_dir.string(),
        kCyclicDir.string(),
        1.0e-12));
}

TEST(PreprocessorOracle, InvalidChiralityInputIsRejected)
{
    const auto work_dir = make_tmp_dir();
    std::filesystem::copy_file(kPreproDir / "data.dat",
                               work_dir / "data.dat",
                               std::filesystem::copy_options::overwrite_existing);

    std::string data = read_text_file(work_dir / "data.dat");
    ASSERT_TRUE(replace_once(data, "10 10", "0 0"));
    ASSERT_TRUE(replace_once(data,
                             "\n1\n30.\nWhat potential? (1=Brenner REBO)\n",
                             "\n0\n30.\nWhat potential? (1=Brenner REBO)\n"));
    write_text_file(work_dir / "data.dat", data);

    EXPECT_THROW(fce::run_preprocessor(work_dir.string()), std::runtime_error);
}

TEST(PreprocessorOracle, CorruptedGeneratedMeshIsRejectedByOracleComparator)
{
    const auto work_dir = make_tmp_dir();
    std::filesystem::copy_file(kPreproDir / "data.dat",
                               work_dir / "data.dat",
                               std::filesystem::copy_options::overwrite_existing);

    fce::run_preprocessor(work_dir.string());

    const auto dims = fce::io::read_dims((work_dir / "nano_dims.dat").string());
    auto mesh = fce::io::read_mesh((work_dir / "nano_Mesh.dat").string(), dims.ngauss);
    std::swap(mesh.connect[0].vertices[0], mesh.connect[0].vertices[1]);
    fce::io::write_mesh((work_dir / "nano_Mesh.dat").string(), mesh, dims.ngauss);

    const auto result = fce::test_support::compare_preprocessor_outputs(
        work_dir.string(),
        kPreproDir.string());
    EXPECT_FALSE(result);
}

TEST(PreprocessorOracle, CorruptedGeneratedGhostCoordinatesAreRejectedByOracleComparator)
{
    const auto work_dir = make_tmp_dir();
    std::filesystem::copy_file(kPreproDir / "data.dat",
                               work_dir / "data.dat",
                               std::filesystem::copy_options::overwrite_existing);

    fce::run_preprocessor(work_dir.string());

    auto ghosts = compute_ghost_coords(work_dir);
    ASSERT_FALSE(ghosts.empty());
    ghosts.front()[0] += 1.0e-3;
    write_ghost_coords_file(work_dir / "ghost_coords.dat", ghosts);

    const auto result = fce::test_support::compare_preprocessor_outputs(
        work_dir.string(),
        kPreproDir.string());
    EXPECT_FALSE(result);
}
