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
const fs::path kReplayTraceFixture =
    fs::path(ORACLE_DIR) / "graphene_compression_simulator" / "imperfection_trace_cpp.dat";
const fs::path kCrunchItBin = fs::path(CRUNCH_IT_BIN);

struct DataRow {
    double load{0.0};
    std::vector<double> values;
};

struct PvdDataset {
    double timestep{0.0};
    std::string file;
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

std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open file: " + path.string());
    }
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::vector<double> parse_numeric_payload(const std::string& payload) {
    std::istringstream in(payload);
    std::vector<double> values;
    std::string token;
    while (in >> token) {
        values.push_back(fce::io::parse_fortran_double(token));
    }
    return values;
}

std::string extract_xml_data_array_payload(const std::string& xml, const std::string& marker) {
    const std::size_t marker_pos = xml.find(marker);
    if (marker_pos == std::string::npos) {
        throw std::runtime_error("cannot find XML marker: " + marker);
    }
    const std::size_t data_begin = xml.find('>', marker_pos);
    const std::size_t data_end = xml.find("</DataArray>", data_begin);
    if (data_begin == std::string::npos || data_end == std::string::npos) {
        throw std::runtime_error("invalid DataArray payload for marker: " + marker);
    }
    return xml.substr(data_begin + 1, data_end - data_begin - 1);
}

double read_vtu_time_value(const fs::path& path) {
    const std::string xml = read_file(path);
    const auto values = parse_numeric_payload(
        extract_xml_data_array_payload(xml, "Name=\"TimeValue\""));
    if (values.size() != 1) {
        throw std::runtime_error("unexpected TimeValue payload size in " + path.string());
    }
    return values[0];
}

std::vector<fce::Vec3> read_vtu_inner_displacement(const fs::path& path) {
    const std::string xml = read_file(path);
    const auto values = parse_numeric_payload(
        extract_xml_data_array_payload(xml, "Name=\"inner_displacement\""));
    if (values.size() % 3 != 0) {
        throw std::runtime_error("inner_displacement payload is not a multiple of 3 in " +
                                 path.string());
    }

    std::vector<fce::Vec3> out(values.size() / 3);
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = fce::Vec3{
            values[3 * i],
            values[3 * i + 1],
            values[3 * i + 2],
        };
    }
    return out;
}

std::vector<fce::Vec3> read_vtu_points(const fs::path& path, const int expected_points) {
    const std::string xml = read_file(path);
    const auto values = parse_numeric_payload(
        extract_xml_data_array_payload(xml, "NumberOfComponents=\"3\" format=\"ascii\""));
    if (static_cast<int>(values.size()) < expected_points * 3) {
        throw std::runtime_error("VTU points payload is shorter than expected in " + path.string());
    }

    std::vector<fce::Vec3> out(static_cast<std::size_t>(expected_points));
    for (int i = 0; i < expected_points; ++i) {
        out[static_cast<std::size_t>(i)] = fce::Vec3{
            values[3 * i],
            values[3 * i + 1],
            values[3 * i + 2],
        };
    }
    return out;
}

std::vector<PvdDataset> read_pvd_datasets(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open pvd file: " + path.string());
    }

    std::vector<PvdDataset> datasets;
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t tag_pos = line.find("<DataSet");
        if (tag_pos == std::string::npos) {
            continue;
        }

        const std::size_t time_pos = line.find("timestep=\"", tag_pos);
        const std::size_t file_pos = line.find("file=\"", tag_pos);
        if (time_pos == std::string::npos || file_pos == std::string::npos) {
            throw std::runtime_error("invalid DataSet row in " + path.string());
        }

        const std::size_t time_begin = time_pos + std::string("timestep=\"").size();
        const std::size_t time_end = line.find('"', time_begin);
        const std::size_t file_begin = file_pos + std::string("file=\"").size();
        const std::size_t file_end = line.find('"', file_begin);
        datasets.push_back(PvdDataset{
            fce::io::parse_fortran_double(line.substr(time_begin, time_end - time_begin)),
            line.substr(file_begin, file_end - file_begin),
        });
    }
    return datasets;
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

void install_replay_trace(const fs::path& case_dir, const fs::path& fixture_path) {
    if (!fs::exists(fixture_path)) {
        throw std::runtime_error("missing replay trace fixture: " + fixture_path.string());
    }
    fs::copy_file(fixture_path,
                  case_dir / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);
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
    ASSERT_TRUE(fs::exists(kReplayTraceFixture)) << "Missing replay trace fixture at " << kReplayTraceFixture;

    install_replay_trace(temp_case_dir_, kReplayTraceFixture);

    const std::string command =
        shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) + " 50";
    ASSERT_EQ(std::system(command.c_str()), 0) << "Failed to execute: " << command;

    const fs::path energy_path = temp_case_dir_ / "energy.dat";
    const fs::path force_path = temp_case_dir_ / "force.dat";
    const fs::path output_path = temp_case_dir_ / "output.dat";
    const fs::path final_config_path = temp_case_dir_ / "nano_final_config.dat";
    const fs::path pvd_path = temp_case_dir_ / "mesh_config_series.pvd";

    ASSERT_TRUE(fs::exists(energy_path));
    ASSERT_TRUE(fs::exists(force_path));
    ASSERT_TRUE(fs::exists(output_path));
    ASSERT_TRUE(fs::exists(final_config_path));
    ASSERT_TRUE(fs::exists(temp_case_dir_ / "mesh_config_0000.vtu"));
    ASSERT_TRUE(fs::exists(temp_case_dir_ / "mesh_config_0050.vtu"));
    ASSERT_TRUE(fs::exists(pvd_path));

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

    const auto actual_pvd = read_pvd_datasets(pvd_path);
    const auto oracle_pvd = read_pvd_datasets(kCaseDir / "mesh_config_series.pvd");
    ASSERT_EQ(actual_pvd.size(), oracle_pvd.size());
    for (std::size_t i = 0; i < oracle_pvd.size(); ++i) {
        EXPECT_NEAR(actual_pvd[i].timestep, oracle_pvd[i].timestep, 1e-12)
            << "pvd dataset timestep " << i;
        EXPECT_EQ(actual_pvd[i].file, oracle_pvd[i].file)
            << "pvd dataset file " << i;
    }

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

TEST_F(E2ECompression, CrunchItWritesRuntimeVtuSeriesAndStepZeroMatchesArchive) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kReplayTraceFixture)) << "Missing replay trace fixture at " << kReplayTraceFixture;

    install_replay_trace(temp_case_dir_, kReplayTraceFixture);

    const std::string command =
        shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) + " 1";
    ASSERT_EQ(std::system(command.c_str()), 0) << "Failed to execute: " << command;

    const fs::path generated_step0 = temp_case_dir_ / "mesh_config_0000.vtu";
    const fs::path generated_step1 = temp_case_dir_ / "mesh_config_0001.vtu";
    const fs::path generated_pvd = temp_case_dir_ / "mesh_config_series.pvd";

    ASSERT_TRUE(fs::exists(generated_step0));
    ASSERT_TRUE(fs::exists(generated_step1));
    ASSERT_TRUE(fs::exists(generated_pvd));

    const fs::path oracle_step0 = kCaseDir / "mesh_config_0000.vtu";
    const fs::path oracle_pvd = kCaseDir / "mesh_config_series.pvd";

    const auto dims = fce::io::read_dims((kCaseDir / "nano_dims.dat").string());
    const auto generated_points = read_vtu_points(generated_step0, dims.numnods);
    const auto oracle_points = read_vtu_points(oracle_step0, dims.numnods);
    ASSERT_EQ(generated_points.size(), oracle_points.size());
    for (std::size_t i = 0; i < oracle_points.size(); ++i) {
        for (int axis = 0; axis < 3; ++axis) {
            EXPECT_NEAR(generated_points[i][axis], oracle_points[i][axis], 1e-12)
                << "step0 points[" << i << "][" << axis << "]";
        }
    }

    const auto generated_eta = read_vtu_inner_displacement(generated_step0);
    const auto oracle_eta = read_vtu_inner_displacement(oracle_step0);
    ASSERT_EQ(generated_eta.size(), oracle_eta.size());
    for (std::size_t i = 0; i < oracle_eta.size(); ++i) {
        for (int axis = 0; axis < 3; ++axis) {
            EXPECT_NEAR(generated_eta[i][axis], oracle_eta[i][axis], 1e-12)
                << "step0 inner_displacement[" << i << "][" << axis << "]";
        }
    }

    EXPECT_NEAR(read_vtu_time_value(generated_step0), 0.0, 1e-12);
    EXPECT_NEAR(read_vtu_time_value(generated_step1), 0.02, 1e-12);

    const auto generated_datasets = read_pvd_datasets(generated_pvd);
    const auto oracle_datasets = read_pvd_datasets(oracle_pvd);
    ASSERT_GE(oracle_datasets.size(), 2U);
    ASSERT_EQ(generated_datasets.size(), 2U);
    EXPECT_NEAR(generated_datasets[0].timestep, oracle_datasets[0].timestep, 1e-12);
    EXPECT_EQ(generated_datasets[0].file, oracle_datasets[0].file);
    EXPECT_NEAR(generated_datasets[1].timestep, oracle_datasets[1].timestep, 1e-12);
    EXPECT_EQ(generated_datasets[1].file, oracle_datasets[1].file);
}

TEST_F(E2ECompression, CrunchItReusesRecordedImperfectionTraceDeterministically) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    {
        std::ofstream out(temp_case_dir_ / "imperfection_trace.dat", std::ios::out | std::ios::trunc);
        ASSERT_TRUE(out.good());
        out << std::setprecision(17);
        for (int step = 0; step < 50; ++step) {
            out << 0.125 << "\n";
        }
    }

    const std::string command =
        shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) + " 1";

    ASSERT_EQ(std::system(command.c_str()), 0) << "Failed to execute: " << command;
    const std::string energy_first = read_file(temp_case_dir_ / "energy.dat");
    const std::string force_first = read_file(temp_case_dir_ / "force.dat");
    const std::string output_first = read_file(temp_case_dir_ / "output.dat");
    const std::string final_first = read_file(temp_case_dir_ / "nano_final_config.dat");

    remove_runtime_outputs(temp_case_dir_);

    ASSERT_EQ(std::system(command.c_str()), 0) << "Failed to execute: " << command;
    EXPECT_EQ(read_file(temp_case_dir_ / "energy.dat"), energy_first);
    EXPECT_EQ(read_file(temp_case_dir_ / "force.dat"), force_first);
    EXPECT_EQ(read_file(temp_case_dir_ / "output.dat"), output_first);
    EXPECT_EQ(read_file(temp_case_dir_ / "nano_final_config.dat"), final_first);
}

TEST_F(E2ECompression, CrunchItRejectsShortImperfectionTrace) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    {
        std::ofstream out(temp_case_dir_ / "imperfection_trace.dat", std::ios::out | std::ios::trunc);
        ASSERT_TRUE(out.good());
        out << std::setprecision(17) << 0.125 << "\n";
    }

    const std::string command =
        shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) + " 1";
    EXPECT_NE(std::system(command.c_str()), 0) << "Expected short imperfection trace to be rejected";
}
