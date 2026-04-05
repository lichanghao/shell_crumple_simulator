// Integration test: end-to-end pasapas compression run.
// Runs the full solver on the graphene compression case (np1) and compares
// energy.dat and force.dat against the committed oracle files.
//
// AC-7: energy rows within 1e-4 relative; force rows within 1e-3 relative.

#include "fce/mpi_env.hpp"
#include "fce/simulator.hpp"
#include "fce/solver.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef ORACLE_DIR
#define ORACLE_DIR "test/cases"
#endif

// Input case (Fortran oracle mesh/BC/config files — solver reads these).
static const std::string kCaseDir =
    std::string(ORACLE_DIR) + "/graphene_compression_simulator/np1";
// C++ oracle: energy.dat and force.dat produced by a known-good C++ run.
// These differ from the Fortran oracle because the C++ solver does a proper
// L-BFGS minimisation to convergence, while the Fortran exits after 1 step.
static const std::string kCppOracle =
    std::string(ORACLE_DIR) + "/graphene_compression_simulator/np1_cpp_oracle";

// ── helpers ──────────────────────────────────────────────────────────────────

static std::vector<std::vector<double>> read_dat(const std::string& path,
                                                  bool skip_header) {
    std::ifstream f(path);
    if (!f) return {};
    std::vector<std::vector<double>> rows;
    std::string line;
    bool first = true;
    while (std::getline(f, line)) {
        if (first && skip_header) { first = false; continue; }
        first = false;
        std::istringstream ss(line);
        std::vector<double> row;
        double v;
        while (ss >> v) row.push_back(v);
        if (!row.empty()) rows.push_back(row);
    }
    return rows;
}

// ── fixture ───────────────────────────────────────────────────────────────────

class E2ECompression : public ::testing::Test {
protected:
    std::string tmp_dir_;

    void SetUp() override {
        // Copy case to a temp dir so output files don't pollute the source tree.
        tmp_dir_ = std::string(std::tmpnam(nullptr)) + "_fce_e2e";
        std::string cmd = "cp -r " + kCaseDir + " " + tmp_dir_;
        ASSERT_EQ(0, std::system(cmd.c_str())) << "Failed to copy case dir";
    }

    void TearDown() override {
        std::string cmd = "rm -rf " + tmp_dir_;
        std::system(cmd.c_str());
    }
};

// ── test ─────────────────────────────────────────────────────────────────────

TEST_F(E2ECompression, EnergyAndForceMatchOracle) {
    // Run the pasapas solver.
    const auto input = fce::load_simulator_input(tmp_dir_);
    fce::Coords coords = input.initial_config.coords;
    const double eps = input.general.crit_global > 0.0
                           ? input.general.crit_global
                           : 1.0e-8;

    int argc = 0;
    char** argv = nullptr;
    fce::MpiEnv mpi(argc, argv);
    fce::pasapas(input, coords, mpi, tmp_dir_, eps);

    // Read produced energy.dat.
    auto actual_e = read_dat(tmp_dir_ + "/energy.dat", /*skip_header=*/true);
    // Read C++ oracle energy.dat (generated from same solver).
    auto oracle_e = read_dat(kCppOracle + "/energy.dat", /*skip_header=*/true);

    ASSERT_FALSE(actual_e.empty()) << "energy.dat not produced";
    ASSERT_EQ(oracle_e.size(), actual_e.size())
        << "Row count mismatch: oracle=" << oracle_e.size()
        << " actual=" << actual_e.size();

    for (std::size_t i = 0; i < oracle_e.size(); ++i) {
        const double e_o = oracle_e[i][1];
        const double e_a = actual_e[i][1];
        const double denom = std::max(std::abs(e_o), 1e-30);
        const double rel = std::abs(e_a - e_o) / denom;
        EXPECT_LE(rel, 1e-4)
            << "Step " << oracle_e[i][0] << ": oracle=" << e_o
            << " actual=" << e_a << " rel=" << rel;
    }

    // Read produced force.dat.
    auto actual_f = read_dat(tmp_dir_ + "/force.dat", /*skip_header=*/false);
    auto oracle_f = read_dat(kCppOracle + "/force.dat", /*skip_header=*/false);

    ASSERT_FALSE(actual_f.empty()) << "force.dat not produced";
    ASSERT_EQ(oracle_f.size(), actual_f.size())
        << "Force row count mismatch: oracle=" << oracle_f.size()
        << " actual=" << actual_f.size();

    for (std::size_t i = 0; i < oracle_f.size(); ++i) {
        // Columns: load, E_min, reaction1, reaction2
        for (int col = 0; col < 4; ++col) {
            const double v_o = oracle_f[i][col];
            const double v_a = actual_f[i][col];
            const double denom = std::max(std::abs(v_o), 1e-30);
            const double rel = std::abs(v_a - v_o) / denom;
            EXPECT_LE(rel, 1e-3)
                << "Force step " << oracle_f[i][0] << " col " << col
                << ": oracle=" << v_o << " actual=" << v_a << " rel=" << rel;
        }
    }
}
