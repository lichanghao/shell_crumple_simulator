#include "oracle_compare.hpp"

#include "fce/io.hpp"
#include "fce/preprocessor.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

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
        kPreproDir.string()));
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
        kCyclicDir.string()));
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
