#include "fce/io.hpp"
#include "fce/lbfgs.hpp"
#include "fce/load_controller.hpp"
#include "fce/runtime_output.hpp"
#include "fce/simulator.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
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

const fs::path kCaseDir =
    fs::path(kOracleDir) / "graphene_compression_simulator" / "np1";
const fs::path kCyclicCaseDir =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "prepro_run";
const fs::path kCyclicReplayTraceFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_trace.dat";
const fs::path kCyclicReplayStepOneEnergyFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_energy.dat";
const fs::path kCyclicReplayStepOneForceFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_force.dat";
const fs::path kCyclicReplayBeforeFirstEvalFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_before_first_eval.dat";
const fs::path kCyclicReplayBeforeFirstEvalEtaFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_before_first_eval_eta.dat";
const fs::path kCyclicReplayBeforeFirstEvalSummaryFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_before_first_eval_summary.dat";
const fs::path kCyclicReplayBeforeOutputFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_before_output.dat";
const fs::path kCyclicReplayBeforeOutputEtaFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_before_output_eta.dat";
const fs::path kCyclicReplayBeforeOutputSummaryFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_before_output_summary.dat";
const fs::path kCyclicPostMinimizeFreeFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "post_minimize_free_coords.dat";
const fs::path kSelfContactCaseDir =
    fs::path(kOracleDir) / "graphene_self_contact" / "prepro_run";
const fs::path kXmlValidatorScript =
    fs::path(kOracleDir).parent_path() / "support" / "validate_vtk_xml.py";
const fs::path kFortranTraceFixture =
    fs::path(kOracleDir) / "graphene_compression_simulator" / "imperfection_trace_fortran.dat";
const fs::path kReplayStepOneMonitorFixture =
    fs::path(kOracleDir) / "graphene_compression_simulator" / "replay_step1_monitor.dat";
const fs::path kReplayStepOneEvalFixture =
    fs::path(kOracleDir) / "graphene_compression_simulator" / "replay_step1_eval_sequence.dat";
const fs::path kReplayStepOneEnergyFixture =
    fs::path(kOracleDir) / "graphene_compression_simulator" / "replay_step1_energy.dat";
const fs::path kReplayStepOneForceFixture =
    fs::path(kOracleDir) / "graphene_compression_simulator" / "replay_step1_force.dat";
const fs::path kReplayStepOneStdoutFixture =
    fs::path(kOracleDir) / "graphene_compression_simulator" / "replay_step1_stdout.txt";
const fs::path kPostMinimizeFreeFixture =
    fs::path(kOracleDir) / "graphene_compression_simulator" / "post_minimize_free_coords.dat";
const fs::path kCrunchItBin = fs::path(kCrunchItBinPath);

struct DataRow {
    double load{0.0};
    std::vector<double> values;
};

struct PvdDataset {
    double timestep{0.0};
    std::string file;
};

struct MonitorRow {
    int iter{0};
    int nfn{0};
    double func{0.0};
    double gnorm{0.0};
    double steplength{0.0};
};

struct StepMonitorFixture {
    double initial_f{0.0};
    double initial_critc{0.0};
    std::vector<MonitorRow> rows;
};

struct EvalRow {
    int eval_index{0};
    double function_value{0.0};
};

std::vector<fce::Vec3> read_fortran_coord_dump(const fs::path& path);

double relative_error(double actual, double expected, double floor);

std::pair<double, double> expected_reaction_from_get_reac_ncode3(
    const fce::BCData& bcs,
    const std::vector<double>& forces_flat) {
    double reaction1 = 0.0;
    double reaction2 = 0.0;

    for (int i = 0; i < bcs.nnodBC; ++i) {
        const std::size_t mdof_idx = static_cast<std::size_t>(3 * i + 2);
        const int flat_dof = bcs.mdofBC.at(mdof_idx);
        const double force_val = forces_flat.at(static_cast<std::size_t>(flat_dof));
        if (bcs.mnodBC.at(static_cast<std::size_t>(i))[1] == 0) {
            reaction1 += force_val;
        } else {
            reaction2 += force_val;
        }
    }

    return {reaction1, reaction2};
}

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

std::map<std::string, double> read_scalar_dump(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open scalar dump: " + path.string());
    }

    std::map<std::string, double> values;
    std::string key;
    std::string token;
    while (in >> key >> token) {
        values[key] = fce::io::parse_fortran_double(token);
    }
    return values;
}

std::vector<fce::Vec2> read_fortran_eta_dump_flat(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open Fortran eta dump: " + path.string());
    }

    std::vector<fce::Vec2> out;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        int ielem = 0;
        int igauss = 0;
        std::string s1;
        std::string s2;
        if (!(row >> ielem >> igauss >> s1 >> s2)) {
            continue;
        }
        out.push_back(fce::Vec2{
            fce::io::parse_fortran_double(s1),
            fce::io::parse_fortran_double(s2),
        });
    }
    return out;
}

StepMonitorFixture read_replay_step_one_monitor_fixture(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open replay monitor fixture: " + path.string());
    }

    StepMonitorFixture fixture;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line.front() == '#') {
            continue;
        }

        std::istringstream row(line);
        std::string label;
        row >> label;
        if (label == "initial_f") {
            row >> fixture.initial_f;
            continue;
        }
        if (label == "initial_critc") {
            row >> fixture.initial_critc;
            continue;
        }

        MonitorRow parsed;
        parsed.iter = std::stoi(label);
        row >> parsed.nfn >> parsed.func >> parsed.gnorm >> parsed.steplength;
        fixture.rows.push_back(parsed);
    }

    if (fixture.rows.empty()) {
        throw std::runtime_error("replay monitor fixture has no rows: " + path.string());
    }

    return fixture;
}

StepMonitorFixture read_runtime_step_one_monitor(const std::string& stdout_text,
                                                 const std::size_t max_rows) {
    StepMonitorFixture fixture;
    std::istringstream in(stdout_text);
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("F=") != std::string::npos && line.find("CRITC=") != std::string::npos) {
            const auto f_pos = line.find("F=");
            const auto critc_pos = line.find("CRITC=");
            if (f_pos != std::string::npos) {
                fixture.initial_f = std::stod(line.substr(f_pos + 2));
            }
            if (critc_pos != std::string::npos) {
                fixture.initial_critc = std::stod(line.substr(critc_pos + 6));
            }
            continue;
        }

        std::istringstream row(line);
        MonitorRow parsed;
        if (row >> parsed.iter >> parsed.nfn >> parsed.func >> parsed.gnorm >> parsed.steplength) {
            fixture.rows.push_back(parsed);
            if (fixture.rows.size() >= max_rows) {
                break;
            }
        }
    }

    if (fixture.rows.empty()) {
        throw std::runtime_error("runtime stdout monitor excerpt has no parsed rows");
    }
    return fixture;
}

double read_first_step_equilibrium_energy_from_log(const std::string& log_text) {
    std::istringstream in(log_text);
    std::string line;
    bool inside_step_one = false;
    while (std::getline(in, line)) {
        if (line.find("Load Step") != std::string::npos &&
            line.find('1') != std::string::npos) {
            inside_step_one = true;
            continue;
        }
        if (!inside_step_one) {
            continue;
        }
        const auto pos = line.find("Equilibrium energy:");
        if (pos == std::string::npos) {
            continue;
        }
        const std::string value = line.substr(pos + std::string("Equilibrium energy:").size());
        return fce::io::parse_fortran_double(value);
    }

    throw std::runtime_error("step-1 equilibrium energy is missing from simulator log");
}

std::vector<EvalRow> read_replay_eval_fixture(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open replay eval fixture: " + path.string());
    }

    std::vector<EvalRow> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line.front() == '#') {
            continue;
        }
        std::istringstream row(line);
        EvalRow parsed;
        row >> parsed.eval_index >> parsed.function_value;
        rows.push_back(parsed);
    }
    if (rows.empty()) {
        throw std::runtime_error("replay eval fixture has no rows: " + path.string());
    }
    return rows;
}

double compute_runtime_bbox_norm(const fce::Coords& coords) {
    double xmin = coords.front()[0];
    double xmax = coords.front()[0];
    double ymin = coords.front()[1];
    double ymax = coords.front()[1];
    double zmin = coords.front()[2];
    double zmax = coords.front()[2];
    for (const auto& p : coords) {
        xmin = std::min(xmin, p[0]);
        xmax = std::max(xmax, p[0]);
        ymin = std::min(ymin, p[1]);
        ymax = std::max(ymax, p[1]);
        zmin = std::min(zmin, p[2]);
        zmax = std::max(zmax, p[2]);
    }
    const double dx = xmax - xmin;
    const double dy = ymax - ymin;
    const double dz = zmax - zmin;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

fce::RuntimeState build_replay_step_one_entry_state(const fce::SimulatorInput& input,
                                                    const std::vector<double>& trace_values) {
    if (trace_values.empty()) {
        throw std::runtime_error("trace fixture is empty");
    }

    fce::RuntimeState state;
    state.coords = read_fortran_coord_dump(kPostMinimizeFreeFixture);
    state.eta.assign(static_cast<std::size_t>(input.mesh.numele),
                     std::vector<fce::Vec2>(static_cast<std::size_t>(input.dims.ngauss),
                                            fce::Vec2{0.0, 0.0}));

    fce::LoadController load_ctrl(input.bcs);
    load_ctrl.init(state.coords);
    load_ctrl.apply_increment(1, state.coords);

    const double a = trace_values.front();
    const double delta = input.general.mat.A0 * 2.0 * (a - 0.5) * input.general.fact_imp;
    for (auto& xyz : state.coords) {
        xyz[0] += delta;
        xyz[1] += delta;
        xyz[2] += delta;
    }

    return state;
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
    } else {
        command += " > /dev/null 2>&1";
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

std::vector<std::string> last_data_tokens(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open data file: " + path.string());
    }

    std::vector<std::string> tokens;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        std::vector<std::string> current;
        std::string token;
        while (row >> token) {
            current.push_back(token);
        }
        if (!current.empty()) {
            tokens = std::move(current);
        }
    }
    if (tokens.empty()) {
        throw std::runtime_error("no data rows found in " + path.string());
    }
    return tokens;
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

class E2ECyclicRuntime : public ::testing::Test {
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

void configure_short_cyclic_restart_case(const fs::path& case_dir) {
    auto general = fce::io::read_general((case_dir / "nano_general.dat").string());
    general.imperfect = false;
    general.fact_imp = 0.0;
    general.crit_global = 1.0e-2;
    general.crit_local = 1.0e-3;
    fce::io::write_general((case_dir / "nano_general.dat").string(), general);
    fs::remove(case_dir / "imperfection_trace.dat");

    auto bcs = fce::io::read_bcs((case_dir / "nano_BCs.dat").string());
    bcs.ncycles = 2;
    bcs.nloadstep_comp = 1;
    bcs.nloadstep_rel = 1;
    bcs.nloadstep = 4;
    bcs.value = 0.2;
    bcs.value_comp = 0.2;
    bcs.value_rel = 0.2;
    fce::io::write_bcs((case_dir / "nano_BCs.dat").string(), bcs);
}

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

TEST_F(E2ECompression, CrunchItWritesReplayStepOneAsciiArtifacts) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kFortranTraceFixture)) << "Missing Fortran trace fixture at " << kFortranTraceFixture;

    install_replay_trace(temp_case_dir_, kFortranTraceFixture);

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1), 0);

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
    ASSERT_TRUE(fs::exists(temp_case_dir_ / "mesh_config_0001.vtu"));
    ASSERT_TRUE(fs::exists(pvd_path));

    const auto actual_energy = read_positive_load_rows(energy_path, /*skip_header=*/true);
    const auto oracle_energy = read_positive_load_rows(kCaseDir / "energy.dat", /*skip_header=*/true);
    ASSERT_EQ(actual_energy.size(), 1U);
    ASSERT_FALSE(oracle_energy.empty());
    ASSERT_GE(actual_energy.front().values.size(), 6U);
    ASSERT_GE(oracle_energy.front().values.size(), 6U);
    EXPECT_NEAR(actual_energy.front().values[0], oracle_energy.front().values[0], 1e-12);
    for (std::size_t col = 1; col < actual_energy.front().values.size(); ++col) {
        EXPECT_TRUE(std::isfinite(actual_energy.front().values[col]))
            << "energy row 0 col " << col;
    }

    const auto actual_force = read_positive_load_rows(force_path, /*skip_header=*/false);
    const auto oracle_force = read_positive_load_rows(kCaseDir / "force.dat", /*skip_header=*/false);
    ASSERT_EQ(actual_force.size(), 1U);
    ASSERT_FALSE(oracle_force.empty());
    ASSERT_EQ(actual_force.front().values.size(), oracle_force.front().values.size());
    EXPECT_NEAR(actual_force.front().values[0], oracle_force.front().values[0], 1e-12);
    for (std::size_t col = 1; col < actual_force.front().values.size(); ++col) {
        EXPECT_TRUE(std::isfinite(actual_force.front().values[col]))
            << "force row 0 col " << col;
    }

    EXPECT_EQ(count_output_load_steps(output_path), 1);

    const auto actual_pvd = read_pvd_datasets(pvd_path);
    const auto oracle_pvd = read_pvd_datasets(kCaseDir / "mesh_config_series.pvd");
    expect_xml_loadable({pvd_path, kCaseDir / "mesh_config_series.pvd"});
    ASSERT_GE(oracle_pvd.size(), 2U);
    ASSERT_EQ(actual_pvd.size(), 2U);
    for (std::size_t i = 0; i < actual_pvd.size(); ++i) {
        EXPECT_NEAR(actual_pvd[i].timestep, oracle_pvd[i].timestep, 1e-12)
            << "pvd dataset timestep " << i;
        EXPECT_EQ(actual_pvd[i].file, oracle_pvd[i].file)
            << "pvd dataset file " << i;
    }

    const auto dims = fce::io::read_dims((kCaseDir / "nano_dims.dat").string());
    for (const int step : {0, 1}) {
        const fs::path generated_vtu = temp_case_dir_ / fce::snapshot_filename(step);
        const fs::path oracle_vtu = kCaseDir / fce::snapshot_filename(step);
        ASSERT_TRUE(fs::exists(generated_vtu));
        ASSERT_TRUE(fs::exists(oracle_vtu));
        expect_xml_loadable({generated_vtu, oracle_vtu});

        const auto generated_points = read_vtu_points(generated_vtu, dims.numnods);
        const auto generated_eta = read_vtu_inner_displacement(generated_vtu);
        ASSERT_EQ(generated_points.size(), static_cast<std::size_t>(dims.numnods));
        ASSERT_EQ(generated_eta.size(), static_cast<std::size_t>(dims.numele));

        for (const auto& point : generated_points) {
            EXPECT_TRUE(std::isfinite(point[0]));
            EXPECT_TRUE(std::isfinite(point[1]));
            EXPECT_TRUE(std::isfinite(point[2]));
        }
        for (const auto& eta : generated_eta) {
            EXPECT_TRUE(std::isfinite(eta[0]));
            EXPECT_TRUE(std::isfinite(eta[1]));
            EXPECT_TRUE(std::isfinite(eta[2]));
        }

        EXPECT_EQ(read_vtu_integer_array(generated_vtu, "connectivity"),
                  read_vtu_integer_array(oracle_vtu, "connectivity"));
        EXPECT_EQ(read_vtu_integer_array(generated_vtu, "offsets"),
                  read_vtu_integer_array(oracle_vtu, "offsets"));
        EXPECT_EQ(read_vtu_integer_array(generated_vtu, "types"),
                  read_vtu_integer_array(oracle_vtu, "types"));
    }

    const auto actual_config = fce::io::read_config(final_config_path.string(),
                                                    dims.numnods,
                                                    dims.numele,
                                                    dims.ngauss);

    for (int node = 0; node < dims.numnods; ++node) {
        for (int axis = 0; axis < 3; ++axis) {
            const double actual = actual_config.coords[static_cast<std::size_t>(node)][axis];
            EXPECT_TRUE(std::isfinite(actual))
                << "final_config coords[" << node << "][" << axis << "]";
        }
    }

    for (int elem = 0; elem < dims.numele; ++elem) {
        for (int gauss = 0; gauss < dims.ngauss; ++gauss) {
            for (int axis = 0; axis < 2; ++axis) {
                const double actual =
                    actual_config.eta[static_cast<std::size_t>(elem)][static_cast<std::size_t>(gauss)][axis];
                EXPECT_TRUE(std::isfinite(actual))
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

TEST(ReplayOracle, StepOneEvalSequenceMatchesCommittedFortranReplayTrace) {
    ASSERT_TRUE(fs::exists(kFortranTraceFixture)) << "Missing Fortran trace fixture at " << kFortranTraceFixture;
    ASSERT_TRUE(fs::exists(kReplayStepOneEvalFixture))
        << "Missing replay eval fixture at " << kReplayStepOneEvalFixture;
    ASSERT_TRUE(fs::exists(kPostMinimizeFreeFixture))
        << "Missing canonical post-free fixture at " << kPostMinimizeFreeFixture;

    const auto trace_values = read_trace_values(kFortranTraceFixture);
    const auto replay = read_replay_eval_fixture(kReplayStepOneEvalFixture);
    const auto input = fce::load_simulator_input(kCaseDir.string());
    auto state = build_replay_step_one_entry_state(input, trace_values);
    fce::LoadController load_ctrl(input.bcs);
    auto post_free = read_fortran_coord_dump(kPostMinimizeFreeFixture);
    load_ctrl.init(post_free);
    load_ctrl.apply_increment(1, post_free);

    std::vector<double> x_free = load_ctrl.to_free(state.coords);
    const double xnorm0 = compute_runtime_bbox_norm(state.coords);
    fce::LbfgsSolver solver(10, input.general.crit_global, 1.0e-12, 20000, false);

    std::vector<EvalRow> actual;
    struct StopReplayCapture final : std::exception {};

    try {
        solver.minimize(
            x_free,
            xnorm0,
            /*stop_on_first_trial=*/false,
            [&](const std::vector<double>& xv) -> std::pair<double, std::vector<double>> {
                load_ctrl.scatter_all(xv, state.coords);
                auto assembly = fce::assemble_energy_forces(input, state, /*element_begin=*/0, /*element_end=*/input.mesh.numele);
                actual.push_back(EvalRow{static_cast<int>(actual.size()), assembly.total_energy});
                if (actual.size() >= replay.size()) {
                    throw StopReplayCapture{};
                }

                std::vector<double> gradient(static_cast<std::size_t>(input.bcs.ndofOP));
                for (int i = 0; i < input.bcs.ndofOP; ++i) {
                    const int flat_dof = input.bcs.mdofOP.at(static_cast<std::size_t>(i));
                    gradient[static_cast<std::size_t>(i)] = assembly.force.at(static_cast<std::size_t>(flat_dof));
                }
                return {assembly.total_energy, std::move(gradient)};
            });
        FAIL() << "expected replay capture to stop after the committed eval prefix";
    } catch (const StopReplayCapture&) {
    }

    ASSERT_EQ(actual.size(), replay.size());
    for (std::size_t i = 0; i < replay.size(); ++i) {
        EXPECT_EQ(actual[i].eval_index, replay[i].eval_index) << "eval row index " << i;
        EXPECT_LE(relative_error(actual[i].function_value, replay[i].function_value, 1e-12), 1e-4)
            << "eval row " << i;
    }
}

TEST(CompressionCaseFiles, ArchivedOracleAndReplayTraceAreDistinctStepOneContracts) {
    ASSERT_TRUE(fs::exists(kReplayStepOneMonitorFixture))
        << "Missing replay monitor fixture at " << kReplayStepOneMonitorFixture;

    const auto archived_log = read_file(kCaseDir / "simulator.log");
    const auto replay = read_replay_step_one_monitor_fixture(kReplayStepOneMonitorFixture);

    const auto f_pos = archived_log.find("F=  3.956D+01");
    ASSERT_NE(f_pos, std::string::npos) << "archived simulator.log is missing the expected step-1 header";
    EXPECT_GT(relative_error(3.956e+01, replay.initial_f, 1e-12), 1e-4);
}

TEST(CompressionCaseFiles, ArchivedSimulatorLogStepOneEnergyDoesNotMatchArchivedEnergyOracle) {
    const auto archived_log = read_file(kCaseDir / "simulator.log");
    const auto oracle_energy = read_positive_load_rows(kCaseDir / "energy.dat", /*skip_header=*/true);
    ASSERT_FALSE(oracle_energy.empty());
    ASSERT_GE(oracle_energy.front().values.size(), 2U);

    const double logged_energy = read_first_step_equilibrium_energy_from_log(archived_log);
    const double energy_row = oracle_energy.front().values[1];

    EXPECT_GT(relative_error(logged_energy, energy_row, 1e-12), 1e-4)
        << "archived simulator.log unexpectedly matches archived energy.dat";
}

TEST_F(E2ECompression, CrunchItStepOnePreservesArchivedBcNodeGeometry) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kFortranTraceFixture)) << "Missing Fortran trace fixture at " << kFortranTraceFixture;

    install_replay_trace(temp_case_dir_, kFortranTraceFixture);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1), 0);

    const auto dims = fce::io::read_dims((kCaseDir / "nano_dims.dat").string());
    const auto bcs = fce::io::read_bcs((kCaseDir / "nano_BCs.dat").string());
    const auto generated_points = read_vtu_points(temp_case_dir_ / "mesh_config_0001.vtu", dims.numnods);
    const auto oracle_points = read_vtu_points(kCaseDir / "mesh_config_0001.vtu", dims.numnods);

    std::vector<bool> bc_nodes(static_cast<std::size_t>(dims.numnods), false);
    for (const auto& tag : bcs.mnodBC) {
        bc_nodes.at(static_cast<std::size_t>(tag[0])) = true;
    }

    double max_bc_delta = 0.0;
    double max_free_delta = 0.0;
    for (int node = 0; node < dims.numnods; ++node) {
        for (int axis = 0; axis < 3; ++axis) {
            const double delta = std::abs(
                generated_points[static_cast<std::size_t>(node)][axis] -
                oracle_points[static_cast<std::size_t>(node)][axis]);
            if (bc_nodes.at(static_cast<std::size_t>(node))) {
                max_bc_delta = std::max(max_bc_delta, delta);
            } else {
                max_free_delta = std::max(max_free_delta, delta);
            }
        }
    }

    EXPECT_LE(max_bc_delta, 1e-12);
    EXPECT_GT(max_free_delta, 1e-6);
}

TEST(CompressionCaseFiles, ArchivedStepOneVtuMatchesArchivedEnergyAndReactionRows) {
    const auto input = fce::load_simulator_input(kCaseDir.string());
    auto state = replay_state_from_oracle_vtu(kCaseDir / "mesh_config_0001.vtu", input);

    const auto assembly = fce::assemble_energy_forces(
        input, state, /*element_begin=*/0, /*element_end=*/input.mesh.numele);

    const auto oracle_energy = read_positive_load_rows(kCaseDir / "energy.dat", /*skip_header=*/true);
    const auto oracle_force = read_positive_load_rows(kCaseDir / "force.dat", /*skip_header=*/false);
    ASSERT_FALSE(oracle_energy.empty());
    ASSERT_FALSE(oracle_force.empty());

    EXPECT_LE(relative_error(assembly.total_energy, oracle_energy.front().values[1], 1e-12), 1e-4)
        << "archived step-one VTU vs energy.dat total energy";

    std::vector<double> forces_real(
        assembly.force.begin(),
        assembly.force.begin() + 3 * input.mesh.numnods);
    fce::LoadController load_ctrl(input.bcs);
    load_ctrl.init(state.coords);

    double reaction1 = 0.0;
    double reaction2 = 0.0;
    load_ctrl.compute_reaction(forces_real, reaction1, reaction2);

    ASSERT_GE(oracle_force.front().values.size(), 4U);
    EXPECT_LE(relative_error(reaction1, oracle_force.front().values[2], 1e-12), 1e-3)
        << "archived step-one VTU vs force.dat reaction1";
    EXPECT_LE(relative_error(reaction2, oracle_force.front().values[3], 1e-12), 1e-3)
        << "archived step-one VTU vs force.dat reaction2";
}

TEST_F(E2ECompression, GeneratedStepOneVtuMatchesGeneratedEnergyAndReactionRows) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kFortranTraceFixture)) << "Missing Fortran trace fixture at " << kFortranTraceFixture;

    install_replay_trace(temp_case_dir_, kFortranTraceFixture);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1), 0);

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    auto state = replay_state_from_oracle_vtu(temp_case_dir_ / "mesh_config_0001.vtu", input);
    const auto assembly = fce::assemble_energy_forces(
        input, state, /*element_begin=*/0, /*element_end=*/input.mesh.numele);

    const auto actual_energy = read_positive_load_rows(temp_case_dir_ / "energy.dat", /*skip_header=*/true);
    const auto actual_force = read_positive_load_rows(temp_case_dir_ / "force.dat", /*skip_header=*/false);
    ASSERT_FALSE(actual_energy.empty());
    ASSERT_FALSE(actual_force.empty());

    EXPECT_LE(relative_error(assembly.total_energy, actual_energy.front().values[1], 1e-12), 1e-4)
        << "generated step-one VTU vs generated energy.dat total energy";

    std::vector<double> forces_real(
        assembly.force.begin(),
        assembly.force.begin() + 3 * input.mesh.numnods);
    fce::LoadController load_ctrl(input.bcs);
    load_ctrl.init(state.coords);

    double reaction1 = 0.0;
    double reaction2 = 0.0;
    load_ctrl.compute_reaction(forces_real, reaction1, reaction2);

    ASSERT_GE(actual_force.front().values.size(), 4U);
    EXPECT_LE(relative_error(reaction1, actual_force.front().values[2], 1e-12), 1e-3)
        << "generated step-one VTU vs generated force.dat reaction1";
    EXPECT_LE(relative_error(reaction2, actual_force.front().values[3], 1e-12), 1e-3)
        << "generated step-one VTU vs generated force.dat reaction2";
}

TEST(CompressionCaseFiles, ReplayMonitorFixtureMatchesCommittedRuntimeStdoutExcerpt) {
    ASSERT_TRUE(fs::exists(kReplayStepOneMonitorFixture))
        << "Missing replay monitor fixture at " << kReplayStepOneMonitorFixture;
    ASSERT_TRUE(fs::exists(kReplayStepOneStdoutFixture))
        << "Missing replay stdout fixture at " << kReplayStepOneStdoutFixture;

    const auto expected = read_replay_step_one_monitor_fixture(kReplayStepOneMonitorFixture);
    const auto actual = read_runtime_step_one_monitor(read_file(kReplayStepOneStdoutFixture),
                                                      expected.rows.size());
    EXPECT_LE(relative_error(actual.initial_f, expected.initial_f, 1e-12), 1e-4);
    EXPECT_LE(relative_error(actual.initial_critc, expected.initial_critc, 1e-12), 1e-4);
    ASSERT_EQ(actual.rows.size(), expected.rows.size());
    for (std::size_t i = 0; i < expected.rows.size(); ++i) {
        EXPECT_EQ(actual.rows[i].iter, expected.rows[i].iter) << "monitor row " << i;
        EXPECT_EQ(actual.rows[i].nfn, expected.rows[i].nfn) << "monitor row " << i;
        EXPECT_LE(relative_error(actual.rows[i].func, expected.rows[i].func, 1e-12), 1e-4)
            << "monitor row " << i << " func";
        EXPECT_LE(relative_error(actual.rows[i].gnorm, expected.rows[i].gnorm, 1e-12), 1e-4)
            << "monitor row " << i << " gnorm";
        EXPECT_LE(relative_error(actual.rows[i].steplength, expected.rows[i].steplength, 1e-12), 1e-4)
            << "monitor row " << i << " steplength";
    }
}

TEST_F(E2ECompression, CrunchItStepOneRowsMatchCommittedReplayFixture) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kFortranTraceFixture)) << "Missing Fortran trace fixture at " << kFortranTraceFixture;
    ASSERT_TRUE(fs::exists(kReplayStepOneEnergyFixture))
        << "Missing replay energy fixture at " << kReplayStepOneEnergyFixture;
    ASSERT_TRUE(fs::exists(kReplayStepOneForceFixture))
        << "Missing replay force fixture at " << kReplayStepOneForceFixture;

    install_replay_trace(temp_case_dir_, kFortranTraceFixture);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1), 0);

    const auto actual_energy = read_positive_load_rows(temp_case_dir_ / "energy.dat", /*skip_header=*/true);
    const auto actual_force = read_positive_load_rows(temp_case_dir_ / "force.dat", /*skip_header=*/false);
    const auto replay_energy = read_positive_load_rows(kReplayStepOneEnergyFixture, /*skip_header=*/true);
    const auto replay_force = read_positive_load_rows(kReplayStepOneForceFixture, /*skip_header=*/false);
    ASSERT_EQ(actual_energy.size(), 1U);
    ASSERT_EQ(actual_force.size(), 1U);
    ASSERT_EQ(replay_energy.size(), 1U);
    ASSERT_EQ(replay_force.size(), 1U);
    ASSERT_EQ(actual_energy.front().values.size(), replay_energy.front().values.size());
    ASSERT_EQ(actual_force.front().values.size(), replay_force.front().values.size());

    EXPECT_LE(relative_error(actual_energy.front().load, replay_energy.front().load, 1e-12), 1e-6);
    for (std::size_t i = 1; i < actual_energy.front().values.size(); ++i) {
        EXPECT_LE(relative_error(actual_energy.front().values[i], replay_energy.front().values[i], 1e-12), 1e-4)
            << "energy row column " << i;
    }

    EXPECT_LE(relative_error(actual_force.front().load, replay_force.front().load, 1e-12), 1e-6);
    for (std::size_t i = 1; i < actual_force.front().values.size(); ++i) {
        EXPECT_LE(relative_error(actual_force.front().values[i], replay_force.front().values[i], 1e-12), 1e-3)
            << "force row column " << i;
    }
}

TEST_F(E2ECyclicRuntime, CrunchItReplaysCommittedCyclicStepOneTraceDeterministically) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayStepOneEnergyFixture)) << "Missing cyclic replay energy fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayStepOneForceFixture)) << "Missing cyclic replay force fixture";

    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1), 0);

    const auto energy_tokens = last_data_tokens(temp_case_dir_ / "energy.dat");
    const auto force_tokens = last_data_tokens(temp_case_dir_ / "force.dat");
    const auto replay_energy_tokens = last_data_tokens(kCyclicReplayStepOneEnergyFixture);
    const auto replay_force_tokens = last_data_tokens(kCyclicReplayStepOneForceFixture);
    const std::string energy_first = read_file(temp_case_dir_ / "energy.dat");
    const std::string force_first = read_file(temp_case_dir_ / "force.dat");

    ASSERT_GE(energy_tokens.size(), 8U);
    ASSERT_GE(force_tokens.size(), 5U);
    ASSERT_EQ(energy_tokens.size(), replay_energy_tokens.size());
    ASSERT_EQ(force_tokens.size(), replay_force_tokens.size());
    EXPECT_EQ(energy_tokens[0], "1");
    EXPECT_EQ(energy_tokens[1], "1");
    EXPECT_EQ(energy_tokens[2], "1");
    EXPECT_EQ(force_tokens[0], "1");
    EXPECT_EQ(force_tokens[1], "1");
    EXPECT_EQ(force_tokens[2], "1");
    for (std::size_t col = 3; col < energy_tokens.size(); ++col) {
        EXPECT_LE(relative_error(fce::io::parse_fortran_double(energy_tokens[col]),
                                 fce::io::parse_fortran_double(replay_energy_tokens[col]),
                                 1e-12),
                  1e-4)
            << "energy col " << col;
    }
    for (std::size_t col = 3; col < force_tokens.size(); ++col) {
        EXPECT_LE(relative_error(fce::io::parse_fortran_double(force_tokens[col]),
                                 fce::io::parse_fortran_double(replay_force_tokens[col]),
                                 1e-12),
                  1e-3)
            << "force col " << col;
    }
    EXPECT_TRUE(fs::exists(temp_case_dir_ / "mesh_config_0001.vtu"));
    EXPECT_FALSE(fs::exists(temp_case_dir_ / "nano_checkpoint.dat"));

    remove_runtime_outputs(temp_case_dir_);
    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1), 0);
    EXPECT_EQ(read_file(temp_case_dir_ / "energy.dat"), energy_first);
    EXPECT_EQ(read_file(temp_case_dir_ / "force.dat"), force_first);
}

TEST_F(E2ECyclicRuntime, GeneratedStepOneVtuMatchesGeneratedEnergyAndReactionRows) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";

    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1), 0);

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    auto state = replay_state_from_oracle_vtu(temp_case_dir_ / "mesh_config_0001.vtu", input);
    const auto assembly = fce::assemble_energy_forces(
        input, state, /*element_begin=*/0, /*element_end=*/input.mesh.numele);

    const auto energy_tokens = last_data_tokens(temp_case_dir_ / "energy.dat");
    const auto force_tokens = last_data_tokens(temp_case_dir_ / "force.dat");

    ASSERT_GE(energy_tokens.size(), 8U);
    ASSERT_GE(force_tokens.size(), 5U);

    EXPECT_LE(relative_error(assembly.total_energy,
                             fce::io::parse_fortran_double(energy_tokens[3]),
                             1e-12),
              1e-4)
        << "generated cyclic step-one VTU vs generated energy.dat total energy";

    std::vector<double> forces_real(
        assembly.force.begin(),
        assembly.force.begin() + 3 * input.mesh.numnods);
    const auto [reaction1, reaction2] =
        expected_reaction_from_get_reac_ncode3(input.bcs, forces_real);

    EXPECT_LE(relative_error(reaction1,
                             fce::io::parse_fortran_double(force_tokens[3]),
                             1e-12),
              1e-3)
        << "generated cyclic step-one VTU vs generated force.dat reaction1";
    EXPECT_LE(relative_error(reaction2,
                             fce::io::parse_fortran_double(force_tokens[4]),
                             1e-12),
              1e-3)
        << "generated cyclic step-one VTU vs generated force.dat reaction2";
}

TEST_F(E2ECyclicRuntime, TraceDumpsCaptureCyclicReplayCheckpoints) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";

    const fs::path dump_dir = temp_case_dir_.parent_path() / "cyclic_trace";
    fs::create_directories(dump_dir);
    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    const std::string env_prefix = "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, env_prefix), 0);

    const fs::path after_increment = dump_dir / "step1_after_increment.dat";
    const fs::path after_imperfection = dump_dir / "step1_after_imperfection.dat";
    const fs::path before_first_eval = dump_dir / "step1_before_first_eval.dat";
    const fs::path before_first_eval_eta = dump_dir / "step1_before_first_eval_eta.dat";
    const fs::path before_first_eval_summary = dump_dir / "step1_before_first_eval_summary.dat";
    const fs::path before_first_eval_reaction = dump_dir / "step1_before_first_eval_reaction.dat";
    const fs::path before_output = dump_dir / "step1_before_output.dat";
    const fs::path before_output_eta = dump_dir / "step1_before_output_eta.dat";
    const fs::path before_output_summary = dump_dir / "step1_before_output_summary.dat";
    const fs::path before_output_reaction = dump_dir / "step1_before_output_reaction.dat";
    const fs::path legacy_reaction = dump_dir / "step1_reaction.dat";

    for (const auto& path : {after_increment,
                             after_imperfection,
                             before_first_eval,
                             before_first_eval_eta,
                             before_first_eval_summary,
                             before_first_eval_reaction,
                             before_output,
                             before_output_eta,
                             before_output_summary,
                             before_output_reaction,
                             legacy_reaction}) {
        EXPECT_TRUE(fs::exists(path)) << "missing trace artifact " << path;
    }

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    const auto after_increment_coords = read_fortran_coord_dump(after_increment);
    const auto after_imperfection_coords = read_fortran_coord_dump(after_imperfection);
    auto expected_before_first_eval = after_imperfection_coords;
    for (const int dof : input.bcs.mdofBC) {
        const int node = dof / 3;
        const int axis = dof % 3;
        expected_before_first_eval.at(static_cast<std::size_t>(node))[axis] =
            after_increment_coords.at(static_cast<std::size_t>(node))[axis];
    }

    const auto before_first_eval_coords = read_fortran_coord_dump(before_first_eval);
    ASSERT_EQ(expected_before_first_eval.size(), before_first_eval_coords.size());
    for (std::size_t inode = 0; inode < expected_before_first_eval.size(); ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            EXPECT_NEAR(before_first_eval_coords[inode][axis],
                        expected_before_first_eval[inode][axis],
                        1e-12)
                << "inode=" << inode << " axis=" << axis;
        }
    }

    const auto output_summary = read_scalar_dump(before_output_summary);
    const auto first_eval_summary_values = read_scalar_dump(before_first_eval_summary);
    const auto energy_tokens = last_data_tokens(temp_case_dir_ / "energy.dat");
    const auto force_tokens = last_data_tokens(temp_case_dir_ / "force.dat");
    ASSERT_GE(energy_tokens.size(), 8U);
    ASSERT_GE(force_tokens.size(), 5U);

    EXPECT_TRUE(std::isnan(first_eval_summary_values.at("GNORM")));
    EXPECT_LE(relative_error(output_summary.at("E_total"),
                             fce::io::parse_fortran_double(energy_tokens[3]),
                             1e-12),
              1e-6);
    EXPECT_LE(relative_error(output_summary.at("E_internal"),
                             fce::io::parse_fortran_double(energy_tokens[4]),
                             1e-12),
              1e-6);
    EXPECT_LE(relative_error(output_summary.at("GNORM"),
                             fce::io::parse_fortran_double(energy_tokens[7]),
                             1e-12),
              1e-6);

    const auto reaction_lines = read_file(before_output_reaction);
    EXPECT_NE(reaction_lines.find("# reaction1"), std::string::npos);
    EXPECT_NE(reaction_lines.find("# reaction2"), std::string::npos);
    EXPECT_EQ(read_file(before_output_reaction), read_file(legacy_reaction));
}

TEST_F(E2ECyclicRuntime, BeforeFirstEvalTraceMatchesCommittedFortranReplayFixture) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayBeforeFirstEvalFixture)) << "Missing cyclic first-eval coord fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayBeforeFirstEvalEtaFixture)) << "Missing cyclic first-eval eta fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayBeforeFirstEvalSummaryFixture)) << "Missing cyclic first-eval summary fixture";

    const fs::path dump_dir = temp_case_dir_.parent_path() / "cyclic_trace_fixture";
    fs::create_directories(dump_dir);
    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    const std::string env_prefix = "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, env_prefix), 0);

    const auto actual_coords = read_fortran_coord_dump(dump_dir / "step1_before_first_eval.dat");
    const auto expected_coords = read_fortran_coord_dump(kCyclicReplayBeforeFirstEvalFixture);
    ASSERT_EQ(actual_coords.size(), expected_coords.size());
    double max_coord_abs = 0.0;
    for (std::size_t inode = 0; inode < actual_coords.size(); ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            max_coord_abs = std::max(max_coord_abs,
                                     std::abs(actual_coords[inode][axis] - expected_coords[inode][axis]));
        }
    }
    EXPECT_LE(max_coord_abs, 1e-6);

    const auto actual_eta = read_fortran_eta_dump_flat(dump_dir / "step1_before_first_eval_eta.dat");
    const auto expected_eta = read_fortran_eta_dump_flat(kCyclicReplayBeforeFirstEvalEtaFixture);
    ASSERT_EQ(actual_eta.size(), expected_eta.size());
    double max_eta_abs = 0.0;
    for (std::size_t i = 0; i < actual_eta.size(); ++i) {
        for (int axis = 0; axis < 2; ++axis) {
            max_eta_abs = std::max(max_eta_abs,
                                   std::abs(actual_eta[i][axis] - expected_eta[i][axis]));
        }
    }
    EXPECT_LE(max_eta_abs, 1e-12);

    const auto actual_summary = read_scalar_dump(dump_dir / "step1_before_first_eval_summary.dat");
    const auto expected_summary = read_scalar_dump(kCyclicReplayBeforeFirstEvalSummaryFixture);
    for (const auto& [key, expected_value] : expected_summary) {
        ASSERT_NE(actual_summary.find(key), actual_summary.end()) << "missing summary key " << key;
        EXPECT_LE(relative_error(actual_summary.at(key), expected_value, 1e-12), 1e-6)
            << "summary key " << key;
    }
}

TEST_F(E2ECyclicRuntime, BeforeOutputTraceShowsFirstMaterialReplayDivergence) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayBeforeOutputFixture)) << "Missing cyclic pre-output coord fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayBeforeOutputEtaFixture)) << "Missing cyclic pre-output eta fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayBeforeOutputSummaryFixture)) << "Missing cyclic pre-output summary fixture";

    const fs::path dump_dir = temp_case_dir_.parent_path() / "cyclic_trace_before_output";
    fs::create_directories(dump_dir);
    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    const std::string env_prefix = "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, env_prefix), 0);

    const auto actual_eta = read_fortran_eta_dump_flat(dump_dir / "step1_before_output_eta.dat");
    const auto expected_eta = read_fortran_eta_dump_flat(kCyclicReplayBeforeOutputEtaFixture);
    ASSERT_EQ(actual_eta.size(), expected_eta.size());
    double max_eta_abs = 0.0;
    for (std::size_t i = 0; i < actual_eta.size(); ++i) {
        for (int axis = 0; axis < 2; ++axis) {
            max_eta_abs = std::max(max_eta_abs,
                                   std::abs(actual_eta[i][axis] - expected_eta[i][axis]));
        }
    }
    EXPECT_LE(max_eta_abs, 1e-12);

    const auto actual_coords = read_fortran_coord_dump(dump_dir / "step1_before_output.dat");
    const auto expected_coords = read_fortran_coord_dump(kCyclicReplayBeforeOutputFixture);
    ASSERT_EQ(actual_coords.size(), expected_coords.size());
    double max_coord_abs = 0.0;
    for (std::size_t inode = 0; inode < actual_coords.size(); ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            max_coord_abs = std::max(max_coord_abs,
                                     std::abs(actual_coords[inode][axis] - expected_coords[inode][axis]));
        }
    }
    EXPECT_GT(max_coord_abs, 1e-1);

    const auto actual_summary = read_scalar_dump(dump_dir / "step1_before_output_summary.dat");
    const auto expected_summary = read_scalar_dump(kCyclicReplayBeforeOutputSummaryFixture);
    EXPECT_GT(relative_error(actual_summary.at("E_total"), expected_summary.at("E_total"), 1e-12), 1e-4);
    EXPECT_GT(relative_error(actual_summary.at("E_internal"), expected_summary.at("E_internal"), 1e-12), 1e-4);
    EXPECT_GT(relative_error(actual_summary.at("GNORM"), expected_summary.at("GNORM"), 1e-12), 1e-2);
}

TEST(CompressionCaseFiles, ArchivedAndReplayCyclicStepOneRowsAreDistinctContracts) {
    const auto archived_energy_tokens =
        last_data_tokens(fs::path(kOracleDir) / "graphene_cyclic_crumple" / "simulator_run" / "energy.dat");
    const auto replay_energy_tokens = last_data_tokens(kCyclicReplayStepOneEnergyFixture);
    const auto archived_force_tokens =
        last_data_tokens(fs::path(kOracleDir) / "graphene_cyclic_crumple" / "simulator_run" / "force.dat");
    const auto replay_force_tokens = last_data_tokens(kCyclicReplayStepOneForceFixture);

    ASSERT_EQ(archived_energy_tokens.size(), replay_energy_tokens.size());
    ASSERT_EQ(archived_force_tokens.size(), replay_force_tokens.size());

    EXPECT_GT(relative_error(fce::io::parse_fortran_double(archived_energy_tokens[3]),
                             fce::io::parse_fortran_double(replay_energy_tokens[3]),
                             1e-12),
              1e-4);
    EXPECT_GT(relative_error(fce::io::parse_fortran_double(archived_force_tokens[3]),
                             fce::io::parse_fortran_double(replay_force_tokens[3]),
                             1e-12),
              1e-3);
}

TEST_F(E2ECyclicRuntime, CrunchItPostMinimizeFreeStateMatchesCommittedCyclicOracle) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicPostMinimizeFreeFixture)) << "Missing cyclic post-free fixture";

    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1), 0);

    const auto dims = fce::io::read_dims((temp_case_dir_ / "nano_dims.dat").string());
    const auto actual = read_vtu_points(temp_case_dir_ / "mesh_config_0000.vtu", dims.numnods);
    const auto oracle = read_fortran_coord_dump(kCyclicPostMinimizeFreeFixture);

    ASSERT_EQ(actual.size(), oracle.size());
    double max_abs = 0.0;
    for (std::size_t inode = 0; inode < actual.size(); ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            max_abs = std::max(max_abs,
                               std::abs(actual[inode][axis] - oracle[inode][axis]));
        }
    }
    EXPECT_LE(max_abs, 1e-6);
}

TEST_F(E2ECyclicRuntime, CrunchItRejectsCheckpointWrittenWithDifferentRankCount) {
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
    const std::string command =
        "mpirun -np 2 " + shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) + " 1" +
        " > " + shell_quote(stderr_path) + " 2>&1";

    ASSERT_NE(std::system(command.c_str()), 0);
    const std::string output = read_file(stderr_path);
    EXPECT_NE(output.find("checkpoint rank count mismatch"), std::string::npos);
}

TEST_F(E2ECyclicRuntime, CrunchItRejectsMalformedCheckpointAcrossRanks) {
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
    const std::string command =
        "mpirun -np 2 " + shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) + " 1" +
        " > " + shell_quote(stderr_path) + " 2>&1";

    ASSERT_NE(std::system(command.c_str()), 0);
    const std::string output = read_file(stderr_path);
    EXPECT_NE(output.find("failed to read checkpoint"), std::string::npos);
}

TEST_F(E2ECyclicRuntime, CrunchItRestartMatchesUninterruptedShortCyclicRun) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    configure_short_cyclic_restart_case(temp_case_dir_);

    const fs::path uninterrupted_root = make_temp_dir();
    const fs::path uninterrupted_case = uninterrupted_root / "prepro_run";
    fs::copy(temp_case_dir_, uninterrupted_case, fs::copy_options::recursive);

    ASSERT_EQ(run_crunch_it(uninterrupted_case, 4), 0);

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 2), 0);
    ASSERT_TRUE(fs::exists(temp_case_dir_ / "nano_checkpoint.dat"));
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 4), 0);

    EXPECT_EQ(read_file(temp_case_dir_ / "energy.dat"),
              read_file(uninterrupted_case / "energy.dat"));
    EXPECT_EQ(read_file(temp_case_dir_ / "force.dat"),
              read_file(uninterrupted_case / "force.dat"));
    EXPECT_EQ(read_file(temp_case_dir_ / "output.dat"),
              read_file(uninterrupted_case / "output.dat"));
    EXPECT_EQ(read_file(temp_case_dir_ / "nano_final_config.dat"),
              read_file(uninterrupted_case / "nano_final_config.dat"));

    fs::remove_all(uninterrupted_root);
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

TEST_F(E2ECompression, CrunchItAcceptsStepBoundedImperfectionTrace) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    {
        std::ofstream out(temp_case_dir_ / "imperfection_trace.dat", std::ios::out | std::ios::trunc);
        ASSERT_TRUE(out.good());
        out << std::setprecision(17) << 0.125 << "\n";
    }

    const std::string command =
        shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) + " 1";

    ASSERT_EQ(std::system(command.c_str()), 0) << "Failed to execute: " << command;
    const std::string energy_first = read_file(temp_case_dir_ / "energy.dat");
    const std::string force_first = read_file(temp_case_dir_ / "force.dat");

    remove_runtime_outputs(temp_case_dir_);

    ASSERT_EQ(std::system(command.c_str()), 0) << "Failed to execute: " << command;
    EXPECT_EQ(read_file(temp_case_dir_ / "energy.dat"), energy_first);
    EXPECT_EQ(read_file(temp_case_dir_ / "force.dat"), force_first);
}

TEST_F(E2ECompression, CrunchItRejectsImperfectionTraceShorterThanRequestedStopStep) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    {
        std::ofstream out(temp_case_dir_ / "imperfection_trace.dat", std::ios::out | std::ios::trunc);
        ASSERT_TRUE(out.good());
        out << std::setprecision(17) << 0.125 << "\n";
    }

    const std::string command =
        shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) + " 2";
    EXPECT_NE(std::system(command.c_str()), 0) << "Expected short imperfection trace to be rejected";
}
