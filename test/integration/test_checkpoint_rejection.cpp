#include "fce/io.hpp"

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

#if defined(CRUNCH_IT_BIN)
constexpr const char* kCrunchItBinPath = CRUNCH_IT_BIN;
#else
constexpr const char* kCrunchItBinPath = "build/crunch_it";
#endif

#if defined(MPIEXEC_BIN)
constexpr const char* kMpiExecBin = MPIEXEC_BIN;
#else
constexpr const char* kMpiExecBin = "mpirun";
#endif

#if defined(MPIEXEC_NUMPROC_FLAG_VALUE)
constexpr const char* kMpiExecNumprocFlag = MPIEXEC_NUMPROC_FLAG_VALUE;
#else
constexpr const char* kMpiExecNumprocFlag = "-np";
#endif

const fs::path kCyclicCaseDir =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "prepro_run";
const fs::path kCrunchItBin = fs::path(kCrunchItBinPath);

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

std::string shell_quote(const fs::path& path) {
    return "\"" + path.string() + "\"";
}

std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open file: " + path.string());
    }
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
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

int run_mpi_crunch_it_capture(const fs::path& case_dir,
                              const int ranks,
                              const int stop_step,
                              const fs::path& capture_path) {
    const std::string command =
        std::string("OMPI_MCA_rmaps_base_oversubscribe=1 ") +
        shell_quote(kMpiExecBin) + " " + kMpiExecNumprocFlag + " " + std::to_string(ranks) + " " +
        shell_quote(kCrunchItBin) + " " + shell_quote(case_dir) + " " + std::to_string(stop_step) +
        " > " + shell_quote(capture_path) + " 2>&1";
    return std::system(command.c_str());
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
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    const auto dims = fce::io::read_dims((temp_case_dir_ / "nano_dims.dat").string());
    const auto config = fce::io::read_config((temp_case_dir_ / "nano_config.dat").string(),
                                             dims.numnods,
                                             dims.numele,
                                             dims.ngauss);
    fce::io::CheckpointData checkpoint;
    checkpoint.iload = 1;
    checkpoint.icycle = 1;
    checkpoint.nprocs = 1;
    checkpoint.config = config;
    checkpoint.K0_ref.assign(static_cast<std::size_t>(dims.numele),
                             std::vector<std::array<double, 3>>(
                                 static_cast<std::size_t>(dims.ngauss),
                                 std::array<double, 3>{0.0, 0.0, 0.0}));
    fce::io::write_checkpoint((temp_case_dir_ / "nano_checkpoint.dat").string(),
                              checkpoint,
                              dims.numnods,
                              dims.numele,
                              dims.ngauss,
                              /*has_crease_memory=*/true);

    const fs::path stderr_path = temp_case_dir_.parent_path() / "rank_mismatch.log";
    ASSERT_NE(run_mpi_crunch_it_capture(temp_case_dir_, 2, 1, stderr_path), 0);
    const std::string output = read_file(stderr_path);
    EXPECT_NE(output.find("checkpoint rank count mismatch"), std::string::npos);
}

TEST_F(CheckpointRejectionRuntime, CrunchItRejectsMalformedCheckpointAcrossRanks) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

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

    const fs::path stderr_path = temp_case_dir_.parent_path() / "malformed_checkpoint.log";
    ASSERT_NE(run_mpi_crunch_it_capture(temp_case_dir_, 2, 1, stderr_path), 0);
    const std::string output = read_file(stderr_path);
    EXPECT_NE(output.find("failed to read checkpoint"), std::string::npos);
    EXPECT_NE(output.find("checkpoint nodal positions has too few columns"), std::string::npos);
}

}  // namespace
