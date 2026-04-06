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

#ifndef ORACLE_DIR
#define ORACLE_DIR "test/cases"
#endif

#ifndef CRUNCH_IT_BIN
#define CRUNCH_IT_BIN "build/crunch_it"
#endif

namespace {

namespace fs = std::filesystem;

const fs::path kCaseDir =
    fs::path(ORACLE_DIR) / "graphene_compression_simulator" / "np1";
const fs::path kCrunchItBin = fs::path(CRUNCH_IT_BIN);

struct DataRow {
    double load{0.0};
    std::vector<double> values;
};

std::vector<DataRow> read_numeric_rows(const fs::path& path, const bool skip_header) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open data file: " + path.string());
    }

    std::vector<DataRow> rows;
    std::string line;
    bool first = true;
    while (std::getline(in, line)) {
        if (first && skip_header) {
            first = false;
            continue;
        }
        first = false;

        std::istringstream row(line);
        std::vector<std::string> tokens;
        std::string token;
        while (row >> token) {
            tokens.push_back(token);
        }
        if (tokens.empty()) {
            continue;
        }

        DataRow parsed;
        parsed.load = fce::io::parse_fortran_double(tokens.front());
        parsed.values.reserve(tokens.size());
        for (const auto& entry : tokens) {
            parsed.values.push_back(fce::io::parse_fortran_double(entry));
        }
        rows.push_back(std::move(parsed));
    }

    return rows;
}

std::vector<DataRow> read_positive_load_rows(const fs::path& path, const bool skip_header) {
    std::vector<DataRow> filtered;
    for (auto row : read_numeric_rows(path, skip_header)) {
        if (row.load > 0.0) {
            filtered.push_back(std::move(row));
        }
    }
    return filtered;
}

fs::path make_temp_dir() {
    std::array<char, 64> pattern{};
    const std::string templ = (fs::temp_directory_path() / "fce_e2e_XXXXXX").string();
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

std::string shell_quote(const fs::path& path) {
    return "\"" + path.string() + "\"";
}

void remove_runtime_outputs(const fs::path& case_dir) {
    for (const auto* name : {"energy.dat", "force.dat", "output.dat", "nano_final_config.dat"}) {
        fs::remove(case_dir / name);
    }
}

int count_output_load_steps(const fs::path& output_path) {
    std::ifstream in(output_path);
    if (!in) {
        throw std::runtime_error("cannot open output.dat: " + output_path.string());
    }

    int count = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("Load Step") != std::string::npos) {
            ++count;
        }
    }
    return count;
}

double relative_error(const double actual, const double expected, const double floor) {
    return std::abs(actual - expected) / std::max(std::abs(expected), floor);
}

}  // namespace

class E2ECompression : public ::testing::Test {
protected:
    fs::path temp_case_dir_;

    void SetUp() override {
        const fs::path temp_root = make_temp_dir();
        temp_case_dir_ = temp_root / "np1";
        fs::copy(kCaseDir, temp_case_dir_, fs::copy_options::recursive);
        remove_runtime_outputs(temp_case_dir_);
    }

    void TearDown() override {
        if (!temp_case_dir_.empty()) {
            fs::remove_all(temp_case_dir_.parent_path());
        }
    }
};

TEST_F(E2ECompression, CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    const std::string command =
        shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) + " 50";
    ASSERT_EQ(std::system(command.c_str()), 0) << "Failed to execute: " << command;

    const fs::path energy_path = temp_case_dir_ / "energy.dat";
    const fs::path force_path = temp_case_dir_ / "force.dat";
    const fs::path output_path = temp_case_dir_ / "output.dat";
    const fs::path final_config_path = temp_case_dir_ / "nano_final_config.dat";

    ASSERT_TRUE(fs::exists(energy_path));
    ASSERT_TRUE(fs::exists(force_path));
    ASSERT_TRUE(fs::exists(output_path));
    ASSERT_TRUE(fs::exists(final_config_path));

    const auto actual_energy = read_positive_load_rows(energy_path, /*skip_header=*/true);
    const auto oracle_energy = read_positive_load_rows(kCaseDir / "energy.dat", /*skip_header=*/true);
    ASSERT_EQ(actual_energy.size(), oracle_energy.size());
    for (std::size_t i = 0; i < oracle_energy.size(); ++i) {
        ASSERT_GE(actual_energy[i].values.size(), 2U);
        ASSERT_GE(oracle_energy[i].values.size(), 2U);
        EXPECT_NEAR(actual_energy[i].values[0], oracle_energy[i].values[0], 1e-12) << "energy load row " << i;
        EXPECT_LE(relative_error(actual_energy[i].values[1], oracle_energy[i].values[1], 1e-12), 1e-4)
            << "energy step " << oracle_energy[i].values[0];
    }

    const auto actual_force = read_positive_load_rows(force_path, /*skip_header=*/false);
    const auto oracle_force = read_positive_load_rows(kCaseDir / "force.dat", /*skip_header=*/false);
    ASSERT_EQ(actual_force.size(), oracle_force.size());
    for (std::size_t i = 0; i < oracle_force.size(); ++i) {
        ASSERT_EQ(actual_force[i].values.size(), oracle_force[i].values.size());
        for (std::size_t col = 0; col < oracle_force[i].values.size(); ++col) {
            EXPECT_LE(relative_error(actual_force[i].values[col], oracle_force[i].values[col], 1e-12), 1e-3)
                << "force row " << i << " col " << col;
        }
    }

    EXPECT_EQ(count_output_load_steps(output_path), 50);

    const auto dims = fce::io::read_dims((kCaseDir / "nano_dims.dat").string());
    const auto actual_config = fce::io::read_config(final_config_path.string(),
                                                    dims.numnods,
                                                    dims.numele,
                                                    dims.ngauss);
    const auto oracle_config = fce::io::read_config((kCaseDir / "nano_final_config.dat").string(),
                                                    dims.numnods,
                                                    dims.numele,
                                                    dims.ngauss);

    for (int node = 0; node < dims.numnods; ++node) {
        for (int axis = 0; axis < 3; ++axis) {
            const double expected = oracle_config.coords[static_cast<std::size_t>(node)][axis];
            const double actual = actual_config.coords[static_cast<std::size_t>(node)][axis];
            EXPECT_LE(relative_error(actual, expected, 1e-12), 1e-3)
                << "final_config coords[" << node << "][" << axis << "]";
        }
    }

    for (int elem = 0; elem < dims.numele; ++elem) {
        for (int gauss = 0; gauss < dims.ngauss; ++gauss) {
            for (int axis = 0; axis < 2; ++axis) {
                const double expected =
                    oracle_config.eta[static_cast<std::size_t>(elem)][static_cast<std::size_t>(gauss)][axis];
                const double actual =
                    actual_config.eta[static_cast<std::size_t>(elem)][static_cast<std::size_t>(gauss)][axis];
                EXPECT_LE(relative_error(actual, expected, 1e-12), 1e-3)
                    << "final_config eta[" << elem << "][" << gauss << "][" << axis << "]";
            }
        }
    }
}
