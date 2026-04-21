#include "fce/io.hpp"
#include "fce/simulator.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

#if defined(ORACLE_DIR)
constexpr const char* kOracleDir = ORACLE_DIR;
#else
constexpr const char* kOracleDir = "test/cases";
#endif

const fs::path kCyclicCaseDir =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "prepro_run";

fs::path make_temp_dir() {
    const std::string templ = (fs::temp_directory_path() / "fce_ckpt_XXXXXX").string();
    std::vector<char> pattern(templ.begin(), templ.end());
    pattern.push_back('\0');
    char* created = ::mkdtemp(pattern.data());
    if (created == nullptr) {
        throw std::runtime_error("mkdtemp failed");
    }
    return fs::path(created);
}

void remove_runtime_outputs(const fs::path& case_dir) {
    for (const auto* name : {"energy.dat", "force.dat", "output.dat", "nano_final_config.dat"}) {
        fs::remove(case_dir / name);
    }
    for (const auto& entry : fs::directory_iterator(case_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string name = entry.path().filename().string();
        if ((name.rfind("mesh_config_", 0) == 0 && entry.path().extension() == ".vtu") ||
            name == "mesh_config_series.pvd") {
            fs::remove(entry.path());
        }
    }
}

class CheckpointRejectionRuntime : public ::testing::Test {
protected:
    fs::path temp_case_dir_;

    void SetUp() override {
        const fs::path temp_root = make_temp_dir();
        temp_case_dir_ = temp_root / "prepro_run";
        fs::copy(kCyclicCaseDir, temp_case_dir_, fs::copy_options::recursive);
        remove_runtime_outputs(temp_case_dir_);
        fs::remove(temp_case_dir_ / "nano_checkpoint.dat");
    }

    void TearDown() override {
        if (!temp_case_dir_.empty()) {
            fs::remove_all(temp_case_dir_.parent_path());
        }
    }
};

TEST_F(CheckpointRejectionRuntime, CrunchItRejectsCheckpointWrittenWithDifferentRankCount) {
    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    fce::io::CheckpointData checkpoint;
    checkpoint.iload = 1;
    checkpoint.icycle = 1;
    checkpoint.nprocs = 1;
    checkpoint.config = input.initial_config;
    checkpoint.K0_ref.assign(static_cast<std::size_t>(input.dims.numele),
                             std::vector<std::array<double, 3>>(
                                 static_cast<std::size_t>(input.dims.ngauss),
                                 std::array<double, 3>{0.0, 0.0, 0.0}));
    fce::io::write_checkpoint((temp_case_dir_ / "nano_checkpoint.dat").string(),
                              checkpoint,
                              input.dims.numnods,
                              input.dims.numele,
                              input.dims.ngauss,
                              /*has_crease_memory=*/true);

    const auto resume = fce::load_runtime_checkpoint(input,
                                                     temp_case_dir_.string(),
                                                     /*current_nprocs=*/2,
                                                     fce::make_runtime_state(input));
    EXPECT_EQ(resume.status, fce::CheckpointResumeStatus::rank_count_mismatch);
    EXPECT_EQ(resume.checkpoint_nprocs, 1);
}

TEST_F(CheckpointRejectionRuntime, CrunchItRejectsMalformedCheckpointAcrossRanks) {
    const auto input = fce::load_simulator_input(temp_case_dir_.string());

    {
        std::ofstream out(temp_case_dir_ / "nano_checkpoint.dat", std::ios::out | std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << " checkpoint_step\n";
        out << "           1\n";
        out << " checkpoint_cycle\n";
        out << "           1\n";
        out << " checkpoint_nprocs\n";
        out << "           2\n";
        out << " Nodal positions\n";
        out << " malformed\n";
    }

    const auto resume = fce::load_runtime_checkpoint(input,
                                                     temp_case_dir_.string(),
                                                     /*current_nprocs=*/2,
                                                     fce::make_runtime_state(input));
    EXPECT_EQ(resume.status, fce::CheckpointResumeStatus::read_failed);
    EXPECT_NE(resume.error_detail.find("checkpoint nodal positions has too few columns"), std::string::npos);
}

}  // namespace
