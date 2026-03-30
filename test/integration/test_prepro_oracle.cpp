#include "oracle_compare.hpp"

#include "fce/preprocessor.hpp"

#include <gtest/gtest.h>

#include <filesystem>
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
    const auto dir =
        std::filesystem::temp_directory_path() / std::filesystem::path("fce_prepro_oracle_case");
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
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

TEST(PreprocessorOracle, ArchivedCyclicPreproInputDoesNotCrash)
{
    const auto work_dir = make_tmp_dir();
    std::filesystem::copy_file(kCyclicDir / "data.dat",
                               work_dir / "data.dat",
                               std::filesystem::copy_options::overwrite_existing);

    EXPECT_NO_THROW(fce::run_preprocessor(work_dir.string()));
    EXPECT_TRUE(std::filesystem::exists(work_dir / "nano_dims.dat"));
    EXPECT_TRUE(std::filesystem::exists(work_dir / "nano_BCs.dat"));
    EXPECT_TRUE(std::filesystem::exists(work_dir / "nano_Mesh.dat"));
}
