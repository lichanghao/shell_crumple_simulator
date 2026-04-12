#include "fce/io.hpp"
#include "fce/runtime_output.hpp"
#include "fce/simulator.hpp"

#include <gtest/gtest.h>

#include <algorithm>
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
const fs::path kSelfContactCaseDir =
    fs::path(ORACLE_DIR) / "graphene_self_contact" / "prepro_run";
const fs::path kXmlValidatorScript =
    fs::path(ORACLE_DIR).parent_path() / "support" / "validate_vtk_xml.py";
const fs::path kFortranTraceFixture =
    fs::path(ORACLE_DIR) / "graphene_compression_simulator" / "imperfection_trace_fortran.dat";
const fs::path kPostMinimizeFreeFixture =
    fs::path(ORACLE_DIR) / "graphene_compression_simulator" / "post_minimize_free_coords.dat";
const fs::path kCrunchItBin = fs::path(CRUNCH_IT_BIN);
constexpr std::array<double, 6> kArchivedStep1EnergyRow{
    2.0e-2, 5.7210528e-5, 5.7210528e-5, 0.0, 0.0, 9.63126754e-6};
constexpr std::array<double, 4> kArchivedStep1ForceRow{
    2.0e-2, 5.7211e-5, -4.2481e-5, 8.4982e-5};

struct DataRow {
    double load{0.0};
    std::vector<double> values;
};

struct PvdDataset {
    double timestep{0.0};
    std::string file;
};

double relative_error(double actual, double expected, double floor);

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

void expect_xml_loadable(const std::vector<fs::path>& paths) {
    ASSERT_TRUE(fs::exists(kXmlValidatorScript)) << "Missing XML validator at " << kXmlValidatorScript;

    std::string command = "python3 " + shell_quote(kXmlValidatorScript);
    for (const auto& path : paths) {
        ASSERT_TRUE(fs::exists(path)) << "Missing XML file " << path;
        command += " " + shell_quote(path);
    }

    EXPECT_EQ(std::system(command.c_str()), 0) << "Failed XML validation command: " << command;
}

std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open file: " + path.string());
    }
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::vector<double> read_trace_values(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open trace file: " + path.string());
    }

    std::vector<double> values;
    std::string token;
    while (in >> token) {
        values.push_back(fce::io::parse_fortran_double(token));
    }
    return values;
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

std::vector<int> parse_integer_payload(const std::string& payload) {
    std::istringstream in(payload);
    std::vector<int> values;
    std::string token;
    while (in >> token) {
        values.push_back(std::stoi(token));
    }
    return values;
}

std::string extract_xml_section(const std::string& xml,
                                const std::string& start_marker,
                                const std::string& end_marker) {
    const std::size_t start = xml.find(start_marker);
    if (start == std::string::npos) {
        throw std::runtime_error("cannot find XML section: " + start_marker);
    }
    const std::size_t end = xml.find(end_marker, start);
    if (end == std::string::npos) {
        throw std::runtime_error("cannot find XML section terminator: " + end_marker);
    }
    return xml.substr(start, end - start + end_marker.size());
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

std::string extract_first_xml_data_array_payload(const std::string& xml_section) {
    const std::size_t data_array_pos = xml_section.find("<DataArray");
    const std::size_t data_begin = xml_section.find('>', data_array_pos);
    const std::size_t data_end = xml_section.find("</DataArray>", data_begin);
    if (data_array_pos == std::string::npos || data_begin == std::string::npos ||
        data_end == std::string::npos) {
        throw std::runtime_error("invalid XML DataArray payload");
    }
    return xml_section.substr(data_begin + 1, data_end - data_begin - 1);
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
        extract_first_xml_data_array_payload(extract_xml_section(xml, "<Points>", "</Points>")));
    if (static_cast<int>(values.size()) != expected_points * 3) {
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

std::vector<fce::Vec3> read_fortran_coord_dump(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open Fortran coordinate dump: " + path.string());
    }

    std::vector<fce::Vec3> out;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        int node = 0;
        std::string sx;
        std::string sy;
        std::string sz;
        if (!(row >> node >> sx >> sy >> sz)) {
            continue;
        }
        out.push_back(fce::Vec3{
            fce::io::parse_fortran_double(sx),
            fce::io::parse_fortran_double(sy),
            fce::io::parse_fortran_double(sz),
        });
    }
    return out;
}

std::vector<double> read_vtu_scalar_array(const fs::path& path, const std::string& name) {
    return parse_numeric_payload(
        extract_xml_data_array_payload(read_file(path), "Name=\"" + name + "\""));
}

std::vector<double> read_vtu_scalar_array_or_empty(const fs::path& path, const std::string& name) {
    const std::string xml = read_file(path);
    const std::string marker = "Name=\"" + name + "\"";
    if (xml.find(marker) == std::string::npos) {
        return {};
    }
    return parse_numeric_payload(extract_xml_data_array_payload(xml, marker));
}

std::vector<int> read_vtu_integer_array(const fs::path& path, const std::string& name) {
    return parse_integer_payload(
        extract_xml_data_array_payload(read_file(path), "Name=\"" + name + "\""));
}

fce::RuntimeState replay_state_from_oracle_vtu(const fs::path& path,
                                               const fce::SimulatorInput& input) {
    fce::RuntimeState state;
    state.coords = read_vtu_points(path, input.mesh.numnods);

    const auto averaged_eta = read_vtu_inner_displacement(path);
    if (static_cast<int>(averaged_eta.size()) != input.mesh.numele) {
        throw std::runtime_error("oracle VTU inner_displacement count does not match mesh.numele");
    }

    state.eta.resize(static_cast<std::size_t>(input.mesh.numele));
    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        const fce::Vec2 eta_avg{
            averaged_eta[static_cast<std::size_t>(ielem)][0],
            averaged_eta[static_cast<std::size_t>(ielem)][1],
        };
        state.eta[static_cast<std::size_t>(ielem)].assign(
            static_cast<std::size_t>(input.dims.ngauss), eta_avg);
    }
    return state;
}

std::vector<double> expected_atomic_density_from_loaded_vdw(const fce::SimulatorInput& input) {
    std::vector<double> rho_nodal(static_cast<std::size_t>(input.mesh.numnods), 0.0);
    std::vector<double> weight_nodal(static_cast<std::size_t>(input.mesh.numnods), 0.0);
    if (input.vdw.nvdw != 1 || input.vdw.rho.empty() || input.vdw.shapef.empty()) {
        return rho_nodal;
    }

    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        const auto& connect = input.mesh.connect.at(static_cast<std::size_t>(ielem));
        for (int ig = 0; ig < input.vdw.ngauss_vdw; ++ig) {
            const double rho_gp =
                input.vdw.rho.at(static_cast<std::size_t>(ielem * input.vdw.ngauss_vdw + ig));
            const auto& shape = input.vdw.shapef.at(static_cast<std::size_t>(ig));
            for (int inode = 0; inode < 12; ++inode) {
                const int node_index = connect.neigh_vert[inode];
                if (node_index < 0 || node_index >= input.mesh.numnods) {
                    continue;
                }
                rho_nodal.at(static_cast<std::size_t>(node_index)) += rho_gp * shape[inode];
                weight_nodal.at(static_cast<std::size_t>(node_index)) += shape[inode];
            }
        }
    }

    for (int inode = 0; inode < input.mesh.numnods; ++inode) {
        if (weight_nodal[static_cast<std::size_t>(inode)] > 1.0e-14) {
            rho_nodal[static_cast<std::size_t>(inode)] /=
                weight_nodal[static_cast<std::size_t>(inode)];
        }
    }
    return rho_nodal;
}

std::vector<double> expected_w_density_from_loaded_vdw(const fce::SimulatorInput& input) {
    std::vector<double> w_density(static_cast<std::size_t>(input.mesh.numele), 0.0);
    if (input.vdw.nvdw != 1 || input.vdw.rho.empty()) {
        return w_density;
    }

    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        double avg = 0.0;
        for (int ig = 0; ig < input.vdw.ngauss_vdw; ++ig) {
            avg += input.vdw.rho.at(static_cast<std::size_t>(ielem * input.vdw.ngauss_vdw + ig));
        }
        w_density[static_cast<std::size_t>(ielem)] = avg / static_cast<double>(input.vdw.ngauss_vdw);
    }
    return w_density;
}

bool has_strictly_positive_entry(const std::vector<double>& values) {
    for (const double value : values) {
        if (value > 0.0) {
            return true;
        }
    }
    return false;
}

fce::Vec3 averaged_eta(const fce::EtaField& eta, const int elem, const int ngauss) {
    fce::Vec3 out{0.0, 0.0, 0.0};
    for (int igauss = 0; igauss < ngauss; ++igauss) {
        out[0] += eta.at(static_cast<std::size_t>(elem)).at(static_cast<std::size_t>(igauss))[0];
        out[1] += eta.at(static_cast<std::size_t>(elem)).at(static_cast<std::size_t>(igauss))[1];
    }
    out[0] /= static_cast<double>(ngauss);
    out[1] /= static_cast<double>(ngauss);
    return out;
}

std::vector<double> flatten_vec3_array(const std::vector<fce::Vec3>& values) {
    std::vector<double> flat;
    flat.reserve(values.size() * 3);
    for (const auto& value : values) {
        flat.push_back(value[0]);
        flat.push_back(value[1]);
        flat.push_back(value[2]);
    }
    return flat;
}

double max_relative_error(const std::vector<double>& actual,
                          const std::vector<double>& expected,
                          const double floor) {
    if (actual.size() != expected.size()) {
        throw std::runtime_error("cannot compare vectors with different lengths");
    }
    double max_err = 0.0;
    for (std::size_t i = 0; i < actual.size(); ++i) {
        max_err = std::max(max_err, relative_error(actual[i], expected[i], floor));
    }
    return max_err;
}

void expect_vtu_matches_archive(const fs::path& generated,
                                const fs::path& oracle,
                                const fce::io::DimsData& dims,
                                const double tol) {
    expect_xml_loadable({generated, oracle});

    EXPECT_LE(relative_error(read_vtu_time_value(generated), read_vtu_time_value(oracle), 1e-12), tol)
        << "time " << generated.filename();

    EXPECT_LE(max_relative_error(flatten_vec3_array(read_vtu_points(generated, dims.numnods)),
                                 flatten_vec3_array(read_vtu_points(oracle, dims.numnods)),
                                 1e-12),
              tol)
        << "points " << generated.filename();

    EXPECT_EQ(read_vtu_integer_array(generated, "connectivity"),
              read_vtu_integer_array(oracle, "connectivity"))
        << "connectivity " << generated.filename();
    EXPECT_EQ(read_vtu_integer_array(generated, "offsets"),
              read_vtu_integer_array(oracle, "offsets"))
        << "offsets " << generated.filename();
    EXPECT_EQ(read_vtu_integer_array(generated, "types"),
              read_vtu_integer_array(oracle, "types"))
        << "types " << generated.filename();

    EXPECT_LE(max_relative_error(flatten_vec3_array(read_vtu_inner_displacement(generated)),
                                 flatten_vec3_array(read_vtu_inner_displacement(oracle)),
                                 1e-12),
              tol)
        << "inner_displacement " << generated.filename();

    std::vector<double> expected_atomic_density = read_vtu_scalar_array_or_empty(oracle, "atomic_density");
    if (expected_atomic_density.empty()) {
        expected_atomic_density.assign(static_cast<std::size_t>(dims.numnods), 0.0);
    }
    EXPECT_LE(max_relative_error(read_vtu_scalar_array(generated, "atomic_density"),
                                 expected_atomic_density,
                                 1e-12),
              tol)
        << "atomic_density " << generated.filename();

    std::vector<double> expected_w_density = read_vtu_scalar_array_or_empty(oracle, "W_density");
    if (expected_w_density.empty()) {
        expected_w_density.assign(static_cast<std::size_t>(dims.numele), 0.0);
    }
    EXPECT_LE(max_relative_error(read_vtu_scalar_array(generated, "W_density"),
                                 expected_w_density,
                                 1e-12),
              tol)
        << "W_density " << generated.filename();
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

int run_crunch_it(const fs::path& case_dir,
                  const int stop_step,
                  const fs::path& stdout_path = {},
                  const std::string& env_prefix = {}) {
    std::string command;
    if (!env_prefix.empty()) {
        command += env_prefix + " ";
    }
    command += shell_quote(kCrunchItBin) + " " + shell_quote(case_dir) + " " + std::to_string(stop_step);
    if (!stdout_path.empty()) {
        command += " > " + shell_quote(stdout_path) + " 2>&1";
    }
    return std::system(command.c_str());
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

class RuntimeOutputVdwCase : public ::testing::Test {
protected:
    fs::path temp_case_dir_;

    void SetUp() override {
        const fs::path temp_root = make_temp_dir();
        temp_case_dir_ = temp_root / "prepro_run";
        fs::copy(kSelfContactCaseDir, temp_case_dir_, fs::copy_options::recursive);
        remove_runtime_outputs(temp_case_dir_);
    }

    void TearDown() override {
        if (!temp_case_dir_.empty()) {
            fs::remove_all(temp_case_dir_.parent_path());
        }
    }
};

TEST(CompressionCaseFiles, ArchivedFortranImperfectionTraceFixtureIsNonSynthetic) {
    ASSERT_TRUE(fs::exists(kFortranTraceFixture)) << "Missing Fortran trace fixture at " << kFortranTraceFixture;

    const auto values = read_trace_values(kFortranTraceFixture);
    ASSERT_EQ(values.size(), 50U);

    const auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
    ASSERT_NE(min_it, values.end());
    ASSERT_NE(max_it, values.end());
    EXPECT_LT(*min_it, *max_it);
    EXPECT_NE(values.front(), 1.0) << "archived trace unexpectedly reverted to the old all-ones placeholder";
}

TEST_F(E2ECompression, CrunchItMatchesArchivedFortranOracleAndWritesRuntimeArtifacts) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kFortranTraceFixture)) << "Missing Fortran trace fixture at " << kFortranTraceFixture;

    install_replay_trace(temp_case_dir_, kFortranTraceFixture);

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
    expect_xml_loadable({pvd_path, kCaseDir / "mesh_config_series.pvd"});
    ASSERT_EQ(actual_pvd.size(), oracle_pvd.size());
    for (std::size_t i = 0; i < oracle_pvd.size(); ++i) {
        EXPECT_NEAR(actual_pvd[i].timestep, oracle_pvd[i].timestep, 1e-12)
            << "pvd dataset timestep " << i;
        EXPECT_EQ(actual_pvd[i].file, oracle_pvd[i].file)
            << "pvd dataset file " << i;
    }

    const auto dims = fce::io::read_dims((kCaseDir / "nano_dims.dat").string());
    expect_vtu_matches_archive(temp_case_dir_ / "mesh_config_0001.vtu",
                               kCaseDir / "mesh_config_0001.vtu",
                               dims,
                               1e-6);
    expect_vtu_matches_archive(temp_case_dir_ / "mesh_config_0025.vtu",
                               kCaseDir / "mesh_config_0025.vtu",
                               dims,
                               1e-6);
    expect_vtu_matches_archive(temp_case_dir_ / "mesh_config_0050.vtu",
                               kCaseDir / "mesh_config_0050.vtu",
                               dims,
                               1e-6);

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

TEST_F(E2ECompression, CrunchItWritesRuntimeVtuSeriesAndValidatesFullDataArrays) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kFortranTraceFixture)) << "Missing Fortran trace fixture at " << kFortranTraceFixture;

    install_replay_trace(temp_case_dir_, kFortranTraceFixture);

    const std::string command =
        shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) + " 1";
    ASSERT_EQ(std::system(command.c_str()), 0) << "Failed to execute: " << command;

    const fs::path generated_step0 = temp_case_dir_ / "mesh_config_0000.vtu";
    const fs::path generated_step1 = temp_case_dir_ / "mesh_config_0001.vtu";
    const fs::path generated_pvd = temp_case_dir_ / "mesh_config_series.pvd";
    const fs::path generated_final_config = temp_case_dir_ / "nano_final_config.dat";

    ASSERT_TRUE(fs::exists(generated_step0));
    ASSERT_TRUE(fs::exists(generated_step1));
    ASSERT_TRUE(fs::exists(generated_pvd));
    ASSERT_TRUE(fs::exists(generated_final_config));

    const fs::path oracle_step0 = kCaseDir / "mesh_config_0000.vtu";
    const fs::path oracle_pvd = kCaseDir / "mesh_config_series.pvd";
    expect_xml_loadable({generated_step0, generated_step1, generated_pvd, oracle_step0, oracle_pvd});

    const auto dims = fce::io::read_dims((kCaseDir / "nano_dims.dat").string());
    // The archived mesh_config_0000.vtu is a pre-pasapas() artifact, so step-0
    // geometry parity against it is physically misleading. Keep step-0 focused
    // on VTU schema/payload integrity and let the executable-path oracle checks
    // assert the real runtime parity at step 1.
    const auto generated_points = read_vtu_points(generated_step0, dims.numnods);
    ASSERT_EQ(generated_points.size(), static_cast<std::size_t>(dims.numnods));
    for (int node = 0; node < dims.numnods; ++node) {
        for (int axis = 0; axis < 3; ++axis) {
            EXPECT_TRUE(std::isfinite(generated_points[static_cast<std::size_t>(node)][axis]))
                << "step0 points[" << node << "][" << axis << "]";
        }
    }

    const auto generated_eta = read_vtu_inner_displacement(generated_step0);
    ASSERT_EQ(generated_eta.size(), static_cast<std::size_t>(dims.numele));
    for (int elem = 0; elem < dims.numele; ++elem) {
        for (int axis = 0; axis < 3; ++axis) {
            EXPECT_TRUE(std::isfinite(generated_eta[static_cast<std::size_t>(elem)][axis]))
                << "step0 inner_displacement[" << elem << "][" << axis << "]";
        }
    }

    EXPECT_EQ(read_vtu_integer_array(generated_step0, "connectivity"),
              read_vtu_integer_array(oracle_step0, "connectivity"));
    EXPECT_EQ(read_vtu_integer_array(generated_step0, "offsets"),
              read_vtu_integer_array(oracle_step0, "offsets"));
    EXPECT_EQ(read_vtu_integer_array(generated_step0, "types"),
              read_vtu_integer_array(oracle_step0, "types"));

    const auto generated_atomic_density0 = read_vtu_scalar_array(generated_step0, "atomic_density");
    const auto generated_atomic_density1 = read_vtu_scalar_array(generated_step1, "atomic_density");
    ASSERT_EQ(generated_atomic_density0.size(), static_cast<std::size_t>(dims.numnods));
    ASSERT_EQ(generated_atomic_density1.size(), static_cast<std::size_t>(dims.numnods));
    for (int node = 0; node < dims.numnods; ++node) {
        EXPECT_NEAR(generated_atomic_density0[static_cast<std::size_t>(node)], 0.0, 1e-12)
            << "step0 atomic_density[" << node << "]";
        EXPECT_NEAR(generated_atomic_density1[static_cast<std::size_t>(node)], 0.0, 1e-12)
            << "step1 atomic_density[" << node << "]";
    }

    const auto generated_w_density0 = read_vtu_scalar_array(generated_step0, "W_density");
    const auto generated_w_density1 = read_vtu_scalar_array(generated_step1, "W_density");
    ASSERT_EQ(generated_w_density0.size(), static_cast<std::size_t>(dims.numele));
    ASSERT_EQ(generated_w_density1.size(), static_cast<std::size_t>(dims.numele));
    for (int elem = 0; elem < dims.numele; ++elem) {
        EXPECT_NEAR(generated_w_density0[static_cast<std::size_t>(elem)], 0.0, 1e-12)
            << "step0 W_density[" << elem << "]";
        EXPECT_NEAR(generated_w_density1[static_cast<std::size_t>(elem)], 0.0, 1e-12)
            << "step1 W_density[" << elem << "]";
    }

    EXPECT_NEAR(read_vtu_time_value(generated_step0), 0.0, 1e-12);
    EXPECT_NEAR(read_vtu_time_value(generated_step1), 0.02, 1e-12);

    const auto final_config = fce::io::read_config(generated_final_config.string(),
                                                   dims.numnods,
                                                   dims.numele,
                                                   dims.ngauss);
    const auto generated_step1_points = read_vtu_points(generated_step1, dims.numnods);
    ASSERT_EQ(generated_step1_points.size(), final_config.coords.size());
    for (int node = 0; node < dims.numnods; ++node) {
        for (int axis = 0; axis < 3; ++axis) {
            EXPECT_NEAR(generated_step1_points[static_cast<std::size_t>(node)][axis],
                        final_config.coords[static_cast<std::size_t>(node)][axis],
                        1e-12)
                << "step1 points[" << node << "][" << axis << "]";
        }
    }

    const auto generated_step1_eta = read_vtu_inner_displacement(generated_step1);
    ASSERT_EQ(generated_step1_eta.size(), static_cast<std::size_t>(dims.numele));
    for (int elem = 0; elem < dims.numele; ++elem) {
        const auto expected = averaged_eta(final_config.eta, elem, dims.ngauss);
        for (int axis = 0; axis < 3; ++axis) {
            EXPECT_NEAR(generated_step1_eta[static_cast<std::size_t>(elem)][axis],
                        expected[axis],
                        1e-12)
                << "step1 inner_displacement[" << elem << "][" << axis << "]";
        }
    }

    const auto generated_datasets = read_pvd_datasets(generated_pvd);
    const auto oracle_datasets = read_pvd_datasets(oracle_pvd);
    ASSERT_GE(oracle_datasets.size(), 2U);
    ASSERT_EQ(generated_datasets.size(), 2U);
    EXPECT_NEAR(generated_datasets[0].timestep, oracle_datasets[0].timestep, 1e-12);
    EXPECT_EQ(generated_datasets[0].file, oracle_datasets[0].file);
    EXPECT_NEAR(generated_datasets[1].timestep, oracle_datasets[1].timestep, 1e-12);
    EXPECT_EQ(generated_datasets[1].file, oracle_datasets[1].file);
}

TEST_F(E2ECompression, CrunchItPostMinimizeFreeStateMatchesCanonicalFortranDump) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kPostMinimizeFreeFixture))
        << "Missing canonical post-free fixture at " << kPostMinimizeFreeFixture;

    install_replay_trace(temp_case_dir_, kFortranTraceFixture);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1), 0);

    const auto dims = fce::io::read_dims((kCaseDir / "nano_dims.dat").string());
    const auto actual = read_vtu_points(temp_case_dir_ / "mesh_config_0000.vtu", dims.numnods);
    const auto oracle = read_fortran_coord_dump(kPostMinimizeFreeFixture);
    ASSERT_EQ(actual.size(), oracle.size());
    for (int node = 0; node < dims.numnods; ++node) {
        for (int axis = 0; axis < 3; ++axis) {
            EXPECT_NEAR(actual[static_cast<std::size_t>(node)][axis],
                        oracle[static_cast<std::size_t>(node)][axis],
                        1.0e-6)
                << "post_free coords[" << node << "][" << axis << "]";
        }
    }
}

TEST_F(E2ECompression, CrunchItStepOneMatchesArchivedFortranOracleWithFortranTrace) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kFortranTraceFixture)) << "Missing Fortran trace fixture at " << kFortranTraceFixture;

    install_replay_trace(temp_case_dir_, kFortranTraceFixture);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1), 0);

    const auto actual_energy = read_positive_load_rows(temp_case_dir_ / "energy.dat", /*skip_header=*/true);
    ASSERT_FALSE(actual_energy.empty());
    ASSERT_GE(actual_energy.front().values.size(), 2U);
    ASSERT_GE(actual_energy.front().values.size(), kArchivedStep1EnergyRow.size());
    EXPECT_NEAR(actual_energy.front().values[0], kArchivedStep1EnergyRow[0], 1e-12);
    EXPECT_LE(relative_error(actual_energy.front().values[1], kArchivedStep1EnergyRow[1], 1e-12), 1e-4);
    EXPECT_LE(relative_error(actual_energy.front().values[5], kArchivedStep1EnergyRow[5], 1e-12), 1e-4);

    const auto actual_force = read_positive_load_rows(temp_case_dir_ / "force.dat", /*skip_header=*/false);
    ASSERT_FALSE(actual_force.empty());
    ASSERT_EQ(actual_force.front().values.size(), kArchivedStep1ForceRow.size());
    for (std::size_t col = 0; col < kArchivedStep1ForceRow.size(); ++col) {
        EXPECT_LE(relative_error(actual_force.front().values[col], kArchivedStep1ForceRow[col], 1e-12), 1e-3)
            << "step1 force col " << col;
    }
}

TEST_F(E2ECompression, CrunchItLbfgsMonitorIsOptIn) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kFortranTraceFixture)) << "Missing Fortran trace fixture at " << kFortranTraceFixture;

    install_replay_trace(temp_case_dir_, kFortranTraceFixture);

    const fs::path quiet_stdout = temp_case_dir_ / "quiet.stdout";
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, quiet_stdout), 0);
    EXPECT_EQ(read_file(quiet_stdout).find("NUMBER OF CORRECTIONS"), std::string::npos);

    remove_runtime_outputs(temp_case_dir_);
    install_replay_trace(temp_case_dir_, kFortranTraceFixture);

    const fs::path monitor_stdout = temp_case_dir_ / "monitor.stdout";
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, monitor_stdout, "FCE_LBFGS_MONITOR=1"), 0);
    EXPECT_NE(read_file(monitor_stdout).find("NUMBER OF CORRECTIONS"), std::string::npos);
}

TEST_F(E2ECompression, RuntimeOutputReplaysArchivedCompressionSnapshotsIndependentlyOfSolver) {
    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    const std::array<int, 3> replay_steps{1, 25, 50};

    for (const int step : replay_steps) {
        const fs::path oracle_snapshot = kCaseDir / fce::snapshot_filename(step);
        ASSERT_TRUE(fs::exists(oracle_snapshot)) << "Missing archived VTU " << oracle_snapshot;

        const auto state = replay_state_from_oracle_vtu(oracle_snapshot, input);
        ASSERT_NO_THROW(fce::write_mesh_snapshot(input, state, temp_case_dir_.string(), step));

        const fs::path generated_snapshot = temp_case_dir_ / fce::snapshot_filename(step);
        ASSERT_TRUE(fs::exists(generated_snapshot));
        expect_vtu_matches_archive(generated_snapshot, oracle_snapshot, input.dims, 1e-12);
    }

    ASSERT_NO_THROW(fce::write_mesh_series_index(temp_case_dir_.string(), input.bcs, 50));
    const auto datasets = read_pvd_datasets(temp_case_dir_ / "mesh_config_series.pvd");
    ASSERT_EQ(datasets.size(), replay_steps.size());
    for (std::size_t i = 0; i < replay_steps.size(); ++i) {
        const int step = replay_steps[i];
        EXPECT_EQ(datasets[i].file, fce::snapshot_filename(step));
        EXPECT_NEAR(datasets[i].timestep,
                    read_vtu_time_value(kCaseDir / fce::snapshot_filename(step)),
                    1e-12);
    }
}

TEST_F(RuntimeOutputVdwCase, LoadedVdwCaseWritesNonzeroDensityArrays) {
    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    ASSERT_EQ(input.vdw.nvdw, 1);
    ASSERT_FALSE(input.vdw.rho.empty());
    ASSERT_FALSE(input.vdw.shapef.empty());

    const auto state = fce::make_runtime_state(input);
    ASSERT_NO_THROW(fce::write_mesh_snapshot(input, state, temp_case_dir_.string(), 0));

    const fs::path snapshot = temp_case_dir_ / fce::snapshot_filename(0);
    ASSERT_TRUE(fs::exists(snapshot));
    expect_xml_loadable({snapshot});

    const auto generated_points = read_vtu_points(snapshot, input.mesh.numnods);
    ASSERT_EQ(generated_points.size(), static_cast<std::size_t>(input.mesh.numnods));
    EXPECT_NEAR(read_vtu_time_value(snapshot), 0.0, 1e-12);

    const auto generated_atomic_density = read_vtu_scalar_array(snapshot, "atomic_density");
    const auto generated_w_density = read_vtu_scalar_array(snapshot, "W_density");
    const auto expected_atomic_density = expected_atomic_density_from_loaded_vdw(input);
    const auto expected_w_density = expected_w_density_from_loaded_vdw(input);

    ASSERT_EQ(generated_atomic_density.size(), expected_atomic_density.size());
    ASSERT_EQ(generated_w_density.size(), expected_w_density.size());
    EXPECT_LE(max_relative_error(generated_atomic_density, expected_atomic_density, 1e-12), 1e-12);
    EXPECT_LE(max_relative_error(generated_w_density, expected_w_density, 1e-12), 1e-12);
    EXPECT_TRUE(has_strictly_positive_entry(generated_atomic_density));
    EXPECT_TRUE(has_strictly_positive_entry(generated_w_density));
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
