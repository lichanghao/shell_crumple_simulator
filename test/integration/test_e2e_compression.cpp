#include "fce/io.hpp"
#include "fce/element_energy.hpp"
#include "fce/ghost_nodes.hpp"
#include "fce/lbfgs.hpp"
#include "fce/load_controller.hpp"
#include "fce/runtime_output.hpp"
#include "fce/simulator.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
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
const fs::path kCyclicReplayAcceptedLbfgsHeadFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_lbfgs_head.dat";
const fs::path kCyclicReplayAccepted20Fixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_20.dat";
const fs::path kCyclicReplayAccepted20EtaFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_20_eta.dat";
const fs::path kCyclicReplayAccepted1Fixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_1.dat";
const fs::path kCyclicReplayAccepted1EtaFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_1_eta.dat";
const fs::path kCyclicReplayAccepted2Fixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_2.dat";
const fs::path kCyclicReplayAccepted2EtaFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_2_eta.dat";
const fs::path kCyclicReplayAccepted2XfreeFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_2_xfree.dat";
const fs::path kCyclicReplayAccepted2GfreeFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_2_gfree.dat";
const fs::path kCyclicReplayAccepted3Fixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_3.dat";
const fs::path kCyclicReplayAccepted3EtaFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_3_eta.dat";
const fs::path kCyclicReplayAccepted3XfreeFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_3_xfree.dat";
const fs::path kCyclicReplayAccepted3GfreeFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_3_gfree.dat";
const fs::path kCyclicReplayAccepted55Fixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_55.dat";
const fs::path kCyclicReplayAccepted55EtaFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "replay_step1_accepted_55_eta.dat";
const fs::path kCyclicPostMinimizeFreeFixture =
    fs::path(kOracleDir) / "graphene_cyclic_crumple" / "post_minimize_free_coords.dat";
const fs::path kSelfContactCaseDir =
    fs::path(kOracleDir) / "graphene_self_contact" / "prepro_run";
const fs::path kBilayerRuntimeCaseDir =
    fs::path(kOracleDir) / "graphene_bilayer_twist_vdw_1000" / "prepro_run";
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

struct CreaseRow {
    int ielem{0};
    double kappa_mean{0.0};
    double kappa_max{0.0};
    int is_creased{0};
    int n_neigh{0};
    double min_dihedral_deg{0.0};
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

struct ContributionHit {
    double abs_value{0.0};
    double contribution{0.0};
    int element_index{0};  // 1-based
    int local_node{0};     // 1-based
    int node_index{0};     // 1-based
    int axis{0};           // 0=x,1=y,2=z
};

struct GfreeMismatchHit {
    double abs_diff{0.0};
    double actual{0.0};
    double expected{0.0};
    int free_index{0};      // 1-based
    int flat_dof{0};        // 1-based
    int node_index{0};      // 1-based
    int axis{0};            // 0=x,1=y,2=z
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

std::vector<CreaseRow> read_crease_rows(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open crease_map file: " + path.string());
    }

    std::vector<CreaseRow> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '!') {
            continue;
        }
        std::istringstream row(line);
        CreaseRow parsed;
        if (!(row >> parsed.ielem >> parsed.kappa_mean >> parsed.kappa_max >>
              parsed.is_creased >> parsed.n_neigh >> parsed.min_dihedral_deg)) {
            continue;
        }
        rows.push_back(parsed);
    }
    return rows;
}

fce::FlatCoords flatten_coords_for_test(const fce::Coords& coords) {
    fce::FlatCoords flat;
    flat.reserve(coords.size() * 3);
    for (const auto& xyz : coords) {
        flat.push_back(xyz[0]);
        flat.push_back(xyz[1]);
        flat.push_back(xyz[2]);
    }
    return flat;
}

fce::NeighborCoords12 gather_neighbor_patch_for_test(const fce::Mesh& mesh,
                                                     const fce::FlatCoords& coords_with_ghosts,
                                                     const int element_index) {
    fce::NeighborCoords12 xneigh{};
    const auto& element = mesh.connect.at(static_cast<std::size_t>(element_index));
    const int total_nodes = mesh.numnods + mesh.nedge;

    for (int inode = 0; inode < 12; ++inode) {
        const int node_index = element.neigh_vert[inode];
        if (node_index < 0 || node_index >= total_nodes) {
            throw std::runtime_error("neighbor patch references an invalid node index");
        }
        const std::size_t base = static_cast<std::size_t>(3 * node_index);
        xneigh[inode] = fce::Vec3{
            coords_with_ghosts.at(base),
            coords_with_ghosts.at(base + 1),
            coords_with_ghosts.at(base + 2),
        };
    }

    return xneigh;
}

std::vector<ContributionHit> compute_force_contributions_for_target(
    const fce::SimulatorInput& input,
    const fce::Coords& coords,
    const fce::EtaField& eta,
    const int target_node_zero_based,
    const int axis) {
    fce::FlatCoords coords_with_ghosts = flatten_coords_for_test(coords);
    coords_with_ghosts.resize(static_cast<std::size_t>(3 * (input.mesh.numnods + input.mesh.nedge)));
    fce::ghost_nodes(input.mesh, coords_with_ghosts);

    std::vector<ContributionHit> hits;
    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        const auto xneigh = gather_neighbor_patch_for_test(input.mesh, coords_with_ghosts, ielem);
        std::vector<fce::Voigt3> reference_curvature(
            static_cast<std::size_t>(input.dims.ngauss), fce::Voigt3{0.0, 0.0, 0.0});
        const auto elem = fce::compute_element_energy(
            xneigh,
            input.ref_config.at(static_cast<std::size_t>(ielem)).F0,
            reference_curvature,
            input.gauss,
            input.general.mat,
            input.general.nW_hat,
            input.general.crit_local,
            1000,
            eta.at(static_cast<std::size_t>(ielem)));
        const double scale = input.ref_config.at(static_cast<std::size_t>(ielem)).J0 / 2.0;
        const auto& connect = input.mesh.connect.at(static_cast<std::size_t>(ielem));

        for (int inode = 0; inode < 12; ++inode) {
            const int node_index = connect.neigh_vert[inode];
            if (node_index != target_node_zero_based) {
                continue;
            }
            const double contribution = elem.f_elem[inode][axis] * scale;
            hits.push_back(ContributionHit{
                std::abs(contribution),
                contribution,
                ielem + 1,
                inode + 1,
                node_index + 1,
                axis,
            });
        }
    }

    std::sort(hits.begin(), hits.end(), [](const ContributionHit& lhs, const ContributionHit& rhs) {
        return lhs.abs_value > rhs.abs_value;
    });
    return hits;
}

std::vector<GfreeMismatchHit> compute_gfree_mismatch_hits(const fce::SimulatorInput& input,
                                                          const std::vector<double>& assembled_force,
                                                          const std::vector<double>& expected_gfree) {
    std::vector<GfreeMismatchHit> hits;
    hits.reserve(static_cast<std::size_t>(input.bcs.ndofOP));

    for (int i = 0; i < input.bcs.ndofOP; ++i) {
        const int flat_dof_zero_based = input.bcs.mdofOP.at(static_cast<std::size_t>(i));
        const double actual = assembled_force.at(static_cast<std::size_t>(flat_dof_zero_based));
        const double expected = expected_gfree.at(static_cast<std::size_t>(i));
        hits.push_back(GfreeMismatchHit{
            std::abs(actual - expected),
            actual,
            expected,
            i + 1,
            flat_dof_zero_based + 1,
            flat_dof_zero_based / 3 + 1,
            flat_dof_zero_based % 3,
        });
    }

    std::sort(hits.begin(), hits.end(), [](const GfreeMismatchHit& lhs, const GfreeMismatchHit& rhs) {
        return lhs.abs_diff > rhs.abs_diff;
    });
    return hits;
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

bool long_oracle_tests_enabled() {
    const char* raw = std::getenv("FCE_RUN_LONG_ORACLE_TESTS");
    return raw != nullptr && std::string(raw) == "1";
}

bool mpi_tests_enabled() {
    const char* raw = std::getenv("FCE_RUN_MPI_TESTS");
    return raw != nullptr && std::string(raw) == "1";
}

bool deferred_cyclic_replay_enabled() {
    const char* raw = std::getenv("FCE_RUN_DEFERRED_CYCLIC_REPLAY");
    return raw != nullptr && std::string(raw) == "1";
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

fce::EtaField read_fortran_eta_dump(const fs::path& path,
                                    const int numele,
                                    const int ngauss) {
    fce::EtaField eta(
        static_cast<std::size_t>(numele),
        std::vector<fce::Vec2>(static_cast<std::size_t>(ngauss), fce::Vec2{0.0, 0.0}));
    const auto flat = read_fortran_eta_dump_flat(path);
    if (flat.size() != static_cast<std::size_t>(numele * ngauss)) {
        throw std::runtime_error("Fortran eta dump size does not match the expected field size");
    }
    std::size_t index = 0;
    for (int ielem = 0; ielem < numele; ++ielem) {
        for (int igauss = 0; igauss < ngauss; ++igauss) {
            eta.at(static_cast<std::size_t>(ielem))
                .at(static_cast<std::size_t>(igauss)) = flat.at(index++);
        }
    }
    return eta;
}

std::vector<std::vector<double>> read_numeric_table(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open numeric table: " + path.string());
    }
    std::vector<std::vector<double>> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream row(line);
        std::vector<double> parsed;
        std::string token;
        while (row >> token) {
            parsed.push_back(fce::io::parse_fortran_double(token));
        }
        if (!parsed.empty()) {
            rows.push_back(std::move(parsed));
        }
    }
    return rows;
}

std::vector<double> read_indexed_vector_dump(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open indexed vector dump: " + path.string());
    }
    std::vector<double> out;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        int idx = 0;
        std::string value;
        if (!(row >> idx >> value)) {
            continue;
        }
        out.push_back(fce::io::parse_fortran_double(value));
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

int run_mpi_crunch_it(const fs::path& case_dir,
                      const int ranks,
                      const int stop_step,
                      const fs::path& stdout_path,
                      const std::string& env_prefix = {}) {
    std::string command;
    if (!env_prefix.empty()) {
        command += env_prefix + " ";
    }
    command += "mpirun -np " + std::to_string(ranks) + " " +
               shell_quote(kCrunchItBin) + " " + shell_quote(case_dir) + " " +
               std::to_string(stop_step);
    command += " > " + shell_quote(stdout_path) + " 2>&1";
    return std::system(command.c_str());
}

int run_mpi_single_step_assembly(const fs::path& case_dir,
                                 const int ranks,
                                 const int step,
                                 const fs::path& stdout_path) {
    std::string command = "mpirun -np " + std::to_string(ranks) + " " +
                          shell_quote(kCrunchItBin) + " " + shell_quote(case_dir) +
                          " --single-step " + std::to_string(step);
    command += " > " + shell_quote(stdout_path) + " 2>&1";
    return std::system(command.c_str());
}

int run_write_initial_snapshot(const fs::path& case_dir,
                               const fs::path& stdout_path = {}) {
    std::string command = shell_quote(kCrunchItBin) + " " + shell_quote(case_dir) +
                          " --write-initial-snapshot";
    if (!stdout_path.empty()) {
        command += " > " + shell_quote(stdout_path) + " 2>&1";
    } else {
        command += " > /dev/null 2>&1";
    }
    return std::system(command.c_str());
}

double read_labeled_double(const fs::path& path, const std::string& label) {
    std::istringstream in(read_file(path));
    std::string line;
    std::string key;
    std::string value;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        if (!(row >> key >> value)) {
            continue;
        }
        if (key == label) {
            return fce::io::parse_fortran_double(value);
        }
    }
    throw std::runtime_error("missing labeled double '" + label + "' in " + path.string());
}

int read_labeled_int(const fs::path& path, const std::string& label) {
    std::istringstream in(read_file(path));
    std::string line;
    std::string key;
    std::string value;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        if (!(row >> key >> value)) {
            continue;
        }
        if (key == label) {
            return std::stoi(value);
        }
    }
    throw std::runtime_error("missing labeled int '" + label + "' in " + path.string());
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

class RuntimeOutputBilayerCase : public ::testing::Test {
protected:
    fs::path temp_case_dir_;

    void SetUp() override {
        const fs::path temp_root = make_temp_dir();
        temp_case_dir_ = temp_root / "prepro_run";
        fs::copy(kBilayerRuntimeCaseDir, temp_case_dir_, fs::copy_options::recursive);
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

void configure_short_cyclic_crease_case(const fs::path& case_dir) {
    configure_short_cyclic_restart_case(case_dir);
    auto crease = fce::io::read_crease((case_dir / "nano_crease.dat").string(), 0, 0);
    crease.kappa_cr = 0.0;
    crease.alpha_lock = 1.0;
    fce::io::write_crease((case_dir / "nano_crease.dat").string(), crease, 0, 0);
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

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, "FCE_CONSTRAINED_LBFGS_MAX_EVAL=1"), 0);

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
        "FCE_CONSTRAINED_LBFGS_MAX_EVAL=1 " +
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

TEST_F(RuntimeOutputBilayerCase, CrunchItRunsArchivedBilayerStepOneCase) {
    if (!long_oracle_tests_enabled()) {
        GTEST_SKIP() << "bilayer runtime vdW assembly is an opt-in long oracle gate; "
                        "set FCE_RUN_LONG_ORACLE_TESTS=1 to run it";
    }

    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, "FCE_CONSTRAINED_LBFGS_MAX_EVAL=1"), 0);

    const fs::path energy_path = temp_case_dir_ / "energy.dat";
    const fs::path force_path = temp_case_dir_ / "force.dat";
    const fs::path output_path = temp_case_dir_ / "output.dat";
    const fs::path pvd_path = temp_case_dir_ / "mesh_config_series.pvd";
    const fs::path step0_vtu = temp_case_dir_ / "mesh_config_0000.vtu";
    const fs::path step1_vtu = temp_case_dir_ / "mesh_config_0001.vtu";

    ASSERT_TRUE(fs::exists(energy_path));
    ASSERT_TRUE(fs::exists(force_path));
    ASSERT_TRUE(fs::exists(output_path));
    ASSERT_TRUE(fs::exists(pvd_path));
    ASSERT_TRUE(fs::exists(step0_vtu));
    ASSERT_TRUE(fs::exists(step1_vtu));

    expect_xml_loadable({pvd_path, step0_vtu, step1_vtu});
    EXPECT_EQ(count_output_load_steps(output_path), 1);

    const auto energy_rows = read_positive_load_rows(energy_path, /*skip_header=*/true);
    const auto force_rows = read_positive_load_rows(force_path, /*skip_header=*/false);
    ASSERT_EQ(energy_rows.size(), 1U);
    ASSERT_EQ(force_rows.size(), 1U);
    for (const double value : energy_rows.front().values) {
        EXPECT_TRUE(std::isfinite(value));
    }
    ASSERT_GE(force_rows.front().values.size(), 4U);
    EXPECT_TRUE(std::isfinite(force_rows.front().values[0]));
    EXPECT_TRUE(std::isfinite(force_rows.front().values[1]));
}

TEST_F(RuntimeOutputBilayerCase, LoadedBilayerCaseWritesNonzeroDensityArrays) {
    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    ASSERT_EQ(input.vdw.nvdw, 1);
    ASSERT_FALSE(input.vdw.rho.empty());
    ASSERT_FALSE(input.vdw.shapef.empty());
    ASSERT_FALSE(input.vdw.tub_partitions.empty());

    const auto state = fce::make_runtime_state(input);
    ASSERT_NO_THROW(fce::write_mesh_snapshot(input, state, temp_case_dir_.string(), 0));

    const fs::path snapshot = temp_case_dir_ / fce::snapshot_filename(0);
    ASSERT_TRUE(fs::exists(snapshot));
    expect_xml_loadable({snapshot});

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

TEST_F(RuntimeOutputBilayerCase, CrunchItWritesInitialSnapshotDensityArrays) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    ASSERT_EQ(input.vdw.nvdw, 1);
    ASSERT_FALSE(input.vdw.rho.empty());
    ASSERT_FALSE(input.vdw.shapef.empty());
    ASSERT_FALSE(input.vdw.tub_partitions.empty());

    ASSERT_EQ(run_write_initial_snapshot(temp_case_dir_), 0);

    const fs::path pvd_path = temp_case_dir_ / "mesh_config_series.pvd";
    const fs::path snapshot = temp_case_dir_ / fce::snapshot_filename(0);
    ASSERT_TRUE(fs::exists(pvd_path));
    ASSERT_TRUE(fs::exists(snapshot));
    expect_xml_loadable({pvd_path, snapshot});

    const auto datasets = read_pvd_datasets(pvd_path);
    ASSERT_EQ(datasets.size(), 1U);
    EXPECT_EQ(datasets.front().file, fce::snapshot_filename(0));
    EXPECT_NEAR(datasets.front().timestep, 0.0, 1e-12);
    EXPECT_NEAR(read_vtu_time_value(snapshot), 0.0, 1e-12);

    const auto generated_points = read_vtu_points(snapshot, input.mesh.numnods);
    ASSERT_EQ(generated_points.size(), static_cast<std::size_t>(input.mesh.numnods));

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

TEST_F(RuntimeOutputBilayerCase, CrunchItRejectsInitialSnapshotIncompatibleModes) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    const fs::path stop_step_log = temp_case_dir_.parent_path() / "snapshot_stop_step.log";
    const std::string stop_step_command =
        shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) +
        " 1 --write-initial-snapshot > " + shell_quote(stop_step_log) + " 2>&1";
    ASSERT_NE(std::system(stop_step_command.c_str()), 0);
    EXPECT_NE(read_file(stop_step_log).find("--write-initial-snapshot cannot be combined with stop_step"),
              std::string::npos);

    const fs::path single_step_log = temp_case_dir_.parent_path() / "snapshot_single_step.log";
    const std::string single_step_command =
        shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) +
        " --single-step 0 --write-initial-snapshot > " +
        shell_quote(single_step_log) + " 2>&1";
    ASSERT_NE(std::system(single_step_command.c_str()), 0);
    EXPECT_NE(read_file(single_step_log).find("--single-step cannot be combined with --write-initial-snapshot"),
              std::string::npos);
}

TEST_F(RuntimeOutputBilayerCase, BilayerSingleStepAssemblyMatchesAcrossEightMpiRanks) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    if (!mpi_tests_enabled()) {
        GTEST_SKIP() << "MPI bilayer vdW assembly is opt-in; set FCE_RUN_MPI_TESTS=1 to run it";
    }

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    ASSERT_EQ(input.vdw.nvdw, 1);
    ASSERT_FALSE(input.vdw.tub_partitions.empty());

    const auto state = fce::make_runtime_state(input);
    ASSERT_NO_THROW(fce::write_mesh_snapshot(input, state, temp_case_dir_.string(), 0));

    const fs::path np1_stdout = temp_case_dir_.parent_path() / "bilayer_np1.log";
    const fs::path np8_stdout = temp_case_dir_.parent_path() / "bilayer_np8.log";
    ASSERT_EQ(run_mpi_single_step_assembly(temp_case_dir_, 1, 0, np1_stdout), 0)
        << read_file(np1_stdout);
    ASSERT_EQ(run_mpi_single_step_assembly(temp_case_dir_, 8, 0, np8_stdout), 0)
        << read_file(np8_stdout);

    const double np1_energy = read_labeled_double(np1_stdout, "assembled_energy");
    const double np8_energy = read_labeled_double(np8_stdout, "assembled_energy");
    EXPECT_TRUE(std::isfinite(np1_energy));
    EXPECT_GT(std::abs(np1_energy), 1e-12);
    EXPECT_LE(relative_error(np8_energy, np1_energy, 1e-12), 1e-10);
    EXPECT_EQ(read_labeled_int(np8_stdout, "inner_fail"),
              read_labeled_int(np1_stdout, "inner_fail"));
    EXPECT_EQ(read_labeled_int(np1_stdout, "force_dofs"),
              3 * (input.mesh.numnods + input.mesh.nedge));
    EXPECT_EQ(read_labeled_int(np8_stdout, "force_dofs"),
              3 * (input.mesh.numnods + input.mesh.nedge));
}

TEST_F(E2ECompression, CrunchItPostMinimizeFreeStateMatchesCanonicalFortranDump) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kPostMinimizeFreeFixture))
        << "Missing canonical post-free fixture at " << kPostMinimizeFreeFixture;

    install_replay_trace(temp_case_dir_, kFortranTraceFixture);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, "FCE_CONSTRAINED_LBFGS_MAX_EVAL=1"), 0);

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
    if (!fs::exists(kCaseDir / "simulator.log")) {
        GTEST_SKIP() << "archived simulator.log is not present in this checkout";
    }

    const auto archived_log = read_file(kCaseDir / "simulator.log");
    const auto replay = read_replay_step_one_monitor_fixture(kReplayStepOneMonitorFixture);

    const auto f_pos = archived_log.find("F=  3.956D+01");
    ASSERT_NE(f_pos, std::string::npos) << "archived simulator.log is missing the expected step-1 header";
    EXPECT_GT(relative_error(3.956e+01, replay.initial_f, 1e-12), 1e-4);
}

TEST(CompressionCaseFiles, ArchivedSimulatorLogStepOneEnergyDoesNotMatchArchivedEnergyOracle) {
    if (!fs::exists(kCaseDir / "simulator.log")) {
        GTEST_SKIP() << "archived simulator.log is not present in this checkout";
    }

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
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, "FCE_CONSTRAINED_LBFGS_MAX_EVAL=1"), 0);

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

TEST_F(E2ECompression, CrunchItSingleStepAssemblyMatchesAcrossMpiRanks) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    fs::copy_file(kCaseDir / "mesh_config_0001.vtu",
                  temp_case_dir_ / "mesh_config_0001.vtu",
                  fs::copy_options::overwrite_existing);
    ASSERT_TRUE(fs::exists(temp_case_dir_ / "mesh_config_0001.vtu"));

    const auto input = fce::load_simulator_input(temp_case_dir_.string());

    struct MpiSingleStepResult {
        int ranks;
        double energy;
        double reaction1;
        double reaction2;
        int inner_fail;
        int force_dofs;
    };

    std::vector<MpiSingleStepResult> results;
    for (const int ranks : {1, 2, 4}) {
        const fs::path stdout_path =
            temp_case_dir_.parent_path() / ("compression_single_step_np" + std::to_string(ranks) + ".log");
        ASSERT_EQ(run_mpi_single_step_assembly(temp_case_dir_, ranks, 1, stdout_path), 0)
            << read_file(stdout_path);
        results.push_back(MpiSingleStepResult{
            ranks,
            read_labeled_double(stdout_path, "assembled_energy"),
            read_labeled_double(stdout_path, "reaction1"),
            read_labeled_double(stdout_path, "reaction2"),
            read_labeled_int(stdout_path, "inner_fail"),
            read_labeled_int(stdout_path, "force_dofs"),
        });
    }

    ASSERT_FALSE(results.empty());
    const auto& reference = results.front();
    for (const auto& result : results) {
        EXPECT_LE(relative_error(result.energy, reference.energy, 1e-12), 1e-10)
            << "np=" << result.ranks;
        EXPECT_LE(relative_error(result.reaction1, reference.reaction1, 1e-12), 1e-10)
            << "np=" << result.ranks;
        EXPECT_LE(relative_error(result.reaction2, reference.reaction2, 1e-12), 1e-10)
            << "np=" << result.ranks;
        EXPECT_EQ(result.inner_fail, reference.inner_fail) << "np=" << result.ranks;
        EXPECT_EQ(result.force_dofs, 3 * (input.mesh.numnods + input.mesh.nedge))
            << "np=" << result.ranks;
    }
}

TEST_F(E2ECompression, GeneratedStepOneVtuMatchesGeneratedEnergyAndReactionRows) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kFortranTraceFixture)) << "Missing Fortran trace fixture at " << kFortranTraceFixture;
    if (!long_oracle_tests_enabled()) {
        GTEST_SKIP() << "generated compression VTU consistency requires full serial convergence; set "
                        "FCE_RUN_LONG_ORACLE_TESTS=1 to run it";
    }

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
    if (!long_oracle_tests_enabled()) {
        GTEST_SKIP() << "full serial compression replay is an opt-in long oracle gate; set "
                        "FCE_RUN_LONG_ORACLE_TESTS=1 to run it";
    }

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
    if (!deferred_cyclic_replay_enabled()) {
        GTEST_SKIP() << "strict cyclic same-branch replay parity is temporarily deferred; set "
                        "FCE_RUN_DEFERRED_CYCLIC_REPLAY=1 to run this diagnostic gate";
    }
    if (!long_oracle_tests_enabled()) {
        GTEST_SKIP() << "full serial cyclic replay is an opt-in long oracle gate; set "
                        "FCE_RUN_LONG_ORACLE_TESTS=1 to run it";
    }

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
    if (!long_oracle_tests_enabled()) {
        GTEST_SKIP() << "generated cyclic VTU consistency requires full serial convergence; set "
                        "FCE_RUN_LONG_ORACLE_TESTS=1 to run it";
    }

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

    const std::string env_prefix =
        "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir) + " FCE_CONSTRAINED_LBFGS_MAX_EVAL=25";
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
    const fs::path eval_trace = dump_dir / "step1_eval_trace.dat";
    const fs::path accepted_lbfgs = dump_dir / "step1_accepted_lbfgs.dat";
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
                             eval_trace,
                             accepted_lbfgs,
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

    const auto eval_rows = read_numeric_table(eval_trace);
    ASSERT_FALSE(eval_rows.empty());
    EXPECT_NEAR(eval_rows.front().at(1), first_eval_summary_values.at("E_total"), 1e-12);
    EXPECT_NEAR(eval_rows.back().at(1), output_summary.at("assembly_total_energy"), 1e-12);

    const auto accepted_rows = read_numeric_table(accepted_lbfgs);
    ASSERT_FALSE(accepted_rows.empty());
    EXPECT_NEAR(accepted_rows.back().at(2), output_summary.at("assembly_total_energy"), 1e-6);
    EXPECT_NEAR(accepted_rows.back().at(3), output_summary.at("GNORM"), 1e-6);

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

    const std::string env_prefix =
        "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir) + " FCE_CONSTRAINED_LBFGS_MAX_EVAL=25";
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
    if (!long_oracle_tests_enabled()) {
        GTEST_SKIP() << "cyclic before-output divergence requires full serial convergence; set "
                        "FCE_RUN_LONG_ORACLE_TESTS=1 to run it";
    }

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

TEST_F(E2ECyclicRuntime, BeforeFirstEvalFixtureReassemblyMatchesCommittedFortranSummary) {
    ASSERT_TRUE(fs::exists(kCyclicReplayBeforeFirstEvalFixture))
        << "Missing cyclic first-eval coord fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayBeforeFirstEvalEtaFixture))
        << "Missing cyclic first-eval eta fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayBeforeFirstEvalSummaryFixture))
        << "Missing cyclic first-eval summary fixture";

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    auto state = fce::make_runtime_state(input);
    state.coords = read_fortran_coord_dump(kCyclicReplayBeforeFirstEvalFixture);
    state.eta = read_fortran_eta_dump(
        kCyclicReplayBeforeFirstEvalEtaFixture, input.mesh.numele, input.dims.ngauss);

    const auto assembly = fce::assemble_energy_forces(
        input, state, /*element_begin=*/0, /*element_end=*/input.mesh.numele);
    const auto expected_summary = read_scalar_dump(kCyclicReplayBeforeFirstEvalSummaryFixture);

    EXPECT_LE(relative_error(assembly.total_energy, expected_summary.at("E_total"), 1e-12), 1e-12);
    EXPECT_LE(relative_error(assembly.reduced_energy, expected_summary.at("E_internal"), 1e-12), 1e-12);
    EXPECT_EQ(assembly.inner_fail, 0);
}

TEST_F(E2ECyclicRuntime, ConstrainedReplayFromCommittedPostFreeMatchesAcceptedStatesThroughThree) {
    ASSERT_TRUE(fs::exists(kCyclicPostMinimizeFreeFixture)) << "Missing cyclic post-free fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted1Fixture)) << "Missing accepted-state-1 fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2Fixture)) << "Missing accepted-state-2 fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted3Fixture)) << "Missing accepted-state-3 fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2XfreeFixture)) << "Missing accepted-state-2 xfree fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2GfreeFixture)) << "Missing accepted-state-2 gfree fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted3XfreeFixture)) << "Missing accepted-state-3 xfree fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted3GfreeFixture)) << "Missing accepted-state-3 gfree fixture";

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    auto state = fce::make_runtime_state(input);
    state.coords = read_fortran_coord_dump(kCyclicPostMinimizeFreeFixture);

    fce::LoadController load_ctrl(input.bcs);
    load_ctrl.init(state.coords);
    load_ctrl.apply_increment(1, state.coords);

    const auto trace = read_trace_values(kCyclicReplayTraceFixture);
    ASSERT_FALSE(trace.empty());
    const double delta =
        input.general.mat.A0 * 2.0 * (trace.front() - 0.5) * input.general.fact_imp;
    for (auto& coord : state.coords) {
        coord[0] += delta;
        coord[1] += delta;
        coord[2] += delta;
    }

    std::vector<double> x_free = load_ctrl.to_free(state.coords);
    fce::LbfgsSolver solver(10, input.general.crit_global, 1.0e-12, 20000, false);
    const std::map<int, fs::path> accepted_fixtures{
        {1, kCyclicReplayAccepted1Fixture},
        {2, kCyclicReplayAccepted2Fixture},
        {3, kCyclicReplayAccepted3Fixture},
    };
    const std::map<int, fs::path> accepted_xfree_fixtures{
        {2, kCyclicReplayAccepted2XfreeFixture},
        {3, kCyclicReplayAccepted3XfreeFixture},
    };
    const std::map<int, fs::path> accepted_gfree_fixtures{
        {2, kCyclicReplayAccepted2GfreeFixture},
        {3, kCyclicReplayAccepted3GfreeFixture},
    };
    std::set<int> checked;
    struct StopAfterAcceptedThree : std::exception {};
    bool have_lagged_gradient = false;
    std::vector<double> lagged_gradient;

    solver.set_accepted_step_observer(
        [&](const int iter,
            const int,
            const double,
            const double,
            const double,
            const std::vector<double>& x_trial,
            const std::vector<double>& g_trial) {
            const auto fixture_it = accepted_fixtures.find(iter);
            if (fixture_it == accepted_fixtures.end()) {
                return;
            }
            fce::Coords coords = state.coords;
            load_ctrl.scatter_all(x_trial, coords);
            const auto expected = read_fortran_coord_dump(fixture_it->second);
            ASSERT_EQ(coords.size(), expected.size()) << "accepted state " << iter;

            double max_abs = 0.0;
            for (std::size_t inode = 0; inode < coords.size(); ++inode) {
                for (int axis = 0; axis < 3; ++axis) {
                    max_abs = std::max(max_abs, std::abs(coords[inode][axis] - expected[inode][axis]));
                }
            }
            EXPECT_LE(max_abs, 1e-10) << "accepted state " << iter;

            if (const auto xfree_it = accepted_xfree_fixtures.find(iter);
                xfree_it != accepted_xfree_fixtures.end()) {
                const auto expected_xfree = read_indexed_vector_dump(xfree_it->second);
                ASSERT_EQ(x_trial.size(), expected_xfree.size()) << "accepted xfree " << iter;
                double max_xfree_abs = 0.0;
                for (std::size_t i = 0; i < x_trial.size(); ++i) {
                    max_xfree_abs = std::max(
                        max_xfree_abs,
                        std::abs(x_trial[i] - expected_xfree[i]));
                }
                EXPECT_LE(max_xfree_abs, 1e-10) << "accepted xfree " << iter;
            }

            if (const auto gfree_it = accepted_gfree_fixtures.find(iter);
                gfree_it != accepted_gfree_fixtures.end()) {
                const auto& gradient_to_compare =
                    have_lagged_gradient ? lagged_gradient : g_trial;
                const auto expected_gfree = read_indexed_vector_dump(gfree_it->second);
                ASSERT_EQ(gradient_to_compare.size(), expected_gfree.size())
                    << "accepted gfree " << iter;
                double max_gfree_abs = 0.0;
                for (std::size_t i = 0; i < gradient_to_compare.size(); ++i) {
                    max_gfree_abs = std::max(
                        max_gfree_abs,
                        std::abs(gradient_to_compare[i] - expected_gfree[i]));
                }
                EXPECT_LE(max_gfree_abs, 1e-8) << "accepted gfree " << iter;
            }
            lagged_gradient = g_trial;
            have_lagged_gradient = true;
            checked.insert(iter);
            if (checked.size() == accepted_fixtures.size()) {
                throw StopAfterAcceptedThree{};
            }
        });

    try {
        solver.minimize(
            x_free,
            compute_runtime_bbox_norm(state.coords),
            /*stop_on_first_trial=*/false,
            [&](const std::vector<double>& xv) -> std::pair<double, std::vector<double>> {
                load_ctrl.scatter_all(xv, state.coords);
                const auto assembly = fce::assemble_energy_forces(
                    input, state, /*element_begin=*/0, /*element_end=*/input.mesh.numele);
                std::vector<double> gradient(static_cast<std::size_t>(input.bcs.ndofOP));
                for (int i = 0; i < input.bcs.ndofOP; ++i) {
                    const int flat_dof = input.bcs.mdofOP.at(static_cast<std::size_t>(i));
                    gradient[static_cast<std::size_t>(i)] =
                        assembly.force.at(static_cast<std::size_t>(flat_dof));
                }
                if (!have_lagged_gradient) {
                    lagged_gradient = gradient;
                    have_lagged_gradient = true;
                }
                return {assembly.total_energy, gradient};
            });
    } catch (const StopAfterAcceptedThree&) {
    }

    EXPECT_EQ(checked.size(), accepted_fixtures.size());
}

TEST_F(E2ECyclicRuntime, ConstrainedReplayFromCommittedPostFreeMatchesAcceptedState20) {
    ASSERT_TRUE(fs::exists(kCyclicPostMinimizeFreeFixture)) << "Missing cyclic post-free fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted20Fixture)) << "Missing accepted-state-20 fixture";

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    auto state = fce::make_runtime_state(input);
    state.coords = read_fortran_coord_dump(kCyclicPostMinimizeFreeFixture);

    fce::LoadController load_ctrl(input.bcs);
    load_ctrl.init(state.coords);
    load_ctrl.apply_increment(1, state.coords);

    const auto trace = read_trace_values(kCyclicReplayTraceFixture);
    ASSERT_FALSE(trace.empty());
    const double delta =
        input.general.mat.A0 * 2.0 * (trace.front() - 0.5) * input.general.fact_imp;
    for (auto& coord : state.coords) {
        coord[0] += delta;
        coord[1] += delta;
        coord[2] += delta;
    }

    std::vector<double> x_free = load_ctrl.to_free(state.coords);
    fce::LbfgsSolver solver(10, input.general.crit_global, 1.0e-12, 20000, false);

    struct StopAfterAccepted20 : std::exception {};
    bool checked20 = false;
    double accepted20_max_abs = 0.0;

    auto compare_accepted_coords =
        [&](const std::vector<double>& x_trial, const fs::path& fixture) {
            fce::Coords coords = state.coords;
            load_ctrl.scatter_all(x_trial, coords);
            const auto expected = read_fortran_coord_dump(fixture);
            EXPECT_EQ(coords.size(), expected.size());
            double max_abs = 0.0;
            for (std::size_t inode = 0; inode < coords.size(); ++inode) {
                for (int axis = 0; axis < 3; ++axis) {
                    max_abs = std::max(
                        max_abs,
                        std::abs(coords[inode][axis] - expected[inode][axis]));
                }
            }
            return max_abs;
        };

    solver.set_accepted_step_observer(
        [&](const int iter,
            const int,
            const double,
            const double,
            const double,
            const std::vector<double>& x_trial,
            const std::vector<double>&) {
            if (iter == 20) {
                accepted20_max_abs = compare_accepted_coords(x_trial, kCyclicReplayAccepted20Fixture);
                checked20 = true;
                throw StopAfterAccepted20{};
            }
        });

    try {
        solver.minimize(
            x_free,
            compute_runtime_bbox_norm(state.coords),
            /*stop_on_first_trial=*/false,
            [&](const std::vector<double>& xv) -> std::pair<double, std::vector<double>> {
                load_ctrl.scatter_all(xv, state.coords);
                const auto assembly = fce::assemble_energy_forces(
                    input, state, /*element_begin=*/0, /*element_end=*/input.mesh.numele);
                std::vector<double> gradient(static_cast<std::size_t>(input.bcs.ndofOP));
                for (int i = 0; i < input.bcs.ndofOP; ++i) {
                    const int flat_dof = input.bcs.mdofOP.at(static_cast<std::size_t>(i));
                    gradient[static_cast<std::size_t>(i)] =
                        assembly.force.at(static_cast<std::size_t>(flat_dof));
                }
                return {assembly.total_energy, gradient};
            });
    } catch (const StopAfterAccepted20&) {
    }

    EXPECT_TRUE(checked20);
    EXPECT_LE(accepted20_max_abs, 5e-10);
}

TEST_F(E2ECyclicRuntime, AcceptedLbfgsHeadMatchesCommittedFortranReplayFixture) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAcceptedLbfgsHeadFixture))
        << "Missing cyclic accepted-step fixture";

    const fs::path dump_dir = temp_case_dir_.parent_path() / "cyclic_trace_lbfgs";
    fs::create_directories(dump_dir);
    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    const std::string env_prefix =
        "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir) + " FCE_CONSTRAINED_LBFGS_MAX_EVAL=25";
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, env_prefix), 0);

    const auto actual_rows = read_numeric_table(dump_dir / "step1_accepted_lbfgs.dat");
    const auto expected_rows = read_numeric_table(kCyclicReplayAcceptedLbfgsHeadFixture);

    ASSERT_GE(actual_rows.size(), expected_rows.size());
    ASSERT_FALSE(expected_rows.empty());

    for (std::size_t i = 0; i < expected_rows.size(); ++i) {
        ASSERT_EQ(actual_rows[i].size(), expected_rows[i].size()) << "row " << i;
        for (std::size_t col = 0; col < expected_rows[i].size(); ++col) {
            double tol = 1e-5;
            if (col == 2) {
                tol = 5e-3;   // Fortran monitor prints F with only a few significant digits.
            } else if (col == 3) {
                tol = 2e-2;   // CRITC is likewise truncated in the monitor output fixture.
            } else if (col == 4) {
                tol = 1e-6;
            }
            EXPECT_NEAR(actual_rows[i][col], expected_rows[i][col], tol)
                << "row " << i << " col " << col;
        }
    }
}

TEST_F(E2ECyclicRuntime, AcceptedState20MatchesCommittedFortranReplayFixture) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted20Fixture)) << "Missing accepted-state-20 coord fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted20EtaFixture)) << "Missing accepted-state-20 eta fixture";

    const fs::path dump_dir = temp_case_dir_.parent_path() / "cyclic_trace_accept20";
    fs::create_directories(dump_dir);
    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    const std::string env_prefix =
        "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir) +
        " FCE_TRACE_ACCEPTED_STATE_STEPS=20 FCE_CONSTRAINED_LBFGS_MAX_EVAL=25";
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, env_prefix), 0);

    const auto actual_coords = read_fortran_coord_dump(dump_dir / "step1_accepted_20.dat");
    const auto expected_coords = read_fortran_coord_dump(kCyclicReplayAccepted20Fixture);
    ASSERT_EQ(actual_coords.size(), expected_coords.size());
    double max_coord_abs = 0.0;
    for (std::size_t inode = 0; inode < actual_coords.size(); ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            max_coord_abs = std::max(max_coord_abs,
                                     std::abs(actual_coords[inode][axis] - expected_coords[inode][axis]));
        }
    }
    EXPECT_LE(max_coord_abs, 1e-6);

    const auto actual_eta = read_fortran_eta_dump_flat(dump_dir / "step1_accepted_20_eta.dat");
    const auto expected_eta = read_fortran_eta_dump_flat(kCyclicReplayAccepted20EtaFixture);
    ASSERT_EQ(actual_eta.size(), expected_eta.size());
    double max_eta_abs = 0.0;
    for (std::size_t i = 0; i < actual_eta.size(); ++i) {
        for (int axis = 0; axis < 2; ++axis) {
            max_eta_abs = std::max(max_eta_abs,
                                   std::abs(actual_eta[i][axis] - expected_eta[i][axis]));
        }
    }
    EXPECT_LE(max_eta_abs, 1e-12);
}

TEST_F(E2ECyclicRuntime, AcceptedState1MatchesCommittedFortranReplayFixture) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted1Fixture)) << "Missing accepted-state-1 coord fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted1EtaFixture)) << "Missing accepted-state-1 eta fixture";

    const fs::path dump_dir = temp_case_dir_.parent_path() / "cyclic_trace_accept1";
    fs::create_directories(dump_dir);
    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    const std::string env_prefix =
        "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir) +
        " FCE_TRACE_ACCEPTED_STATE_STEPS=1-3 FCE_CONSTRAINED_LBFGS_MAX_EVAL=6";
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, env_prefix), 0);

    const auto actual_coords = read_fortran_coord_dump(dump_dir / "step1_accepted_1.dat");
    const auto expected_coords = read_fortran_coord_dump(kCyclicReplayAccepted1Fixture);
    ASSERT_EQ(actual_coords.size(), expected_coords.size());
    double max_coord_abs = 0.0;
    for (std::size_t inode = 0; inode < actual_coords.size(); ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            max_coord_abs = std::max(max_coord_abs,
                                     std::abs(actual_coords[inode][axis] - expected_coords[inode][axis]));
        }
    }
    EXPECT_LE(max_coord_abs, 1e-6);

    const auto actual_eta = read_fortran_eta_dump_flat(dump_dir / "step1_accepted_1_eta.dat");
    const auto expected_eta = read_fortran_eta_dump_flat(kCyclicReplayAccepted1EtaFixture);
    ASSERT_EQ(actual_eta.size(), expected_eta.size());
    double max_eta_abs = 0.0;
    for (std::size_t i = 0; i < actual_eta.size(); ++i) {
        for (int axis = 0; axis < 2; ++axis) {
            max_eta_abs = std::max(max_eta_abs,
                                   std::abs(actual_eta[i][axis] - expected_eta[i][axis]));
        }
    }
    EXPECT_LE(max_eta_abs, 1e-12);
}

TEST_F(E2ECyclicRuntime, AcceptedState2MatchesCommittedFortranReplayFixture) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2Fixture)) << "Missing accepted-state-2 coord fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2EtaFixture)) << "Missing accepted-state-2 eta fixture";

    const fs::path dump_dir = temp_case_dir_.parent_path() / "cyclic_trace_accept2";
    fs::create_directories(dump_dir);
    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    const std::string env_prefix =
        "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir) +
        " FCE_TRACE_ACCEPTED_STATE_STEPS=1-3 FCE_CONSTRAINED_LBFGS_MAX_EVAL=6";
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, env_prefix), 0);

    const auto actual_coords = read_fortran_coord_dump(dump_dir / "step1_accepted_2.dat");
    const auto expected_coords = read_fortran_coord_dump(kCyclicReplayAccepted2Fixture);
    ASSERT_EQ(actual_coords.size(), expected_coords.size());
    double max_coord_abs = 0.0;
    for (std::size_t inode = 0; inode < actual_coords.size(); ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            max_coord_abs = std::max(max_coord_abs,
                                     std::abs(actual_coords[inode][axis] - expected_coords[inode][axis]));
        }
    }
    EXPECT_LE(max_coord_abs, 1e-5);

    const auto actual_eta = read_fortran_eta_dump_flat(dump_dir / "step1_accepted_2_eta.dat");
    const auto expected_eta = read_fortran_eta_dump_flat(kCyclicReplayAccepted2EtaFixture);
    ASSERT_EQ(actual_eta.size(), expected_eta.size());
    double max_eta_abs = 0.0;
    for (std::size_t i = 0; i < actual_eta.size(); ++i) {
        for (int axis = 0; axis < 2; ++axis) {
            max_eta_abs = std::max(max_eta_abs,
                                   std::abs(actual_eta[i][axis] - expected_eta[i][axis]));
        }
    }
    EXPECT_LE(max_eta_abs, 1e-12);
}

TEST_F(E2ECyclicRuntime, AcceptedState2FreeGradientMatchesCommittedFortranReplayFixture) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2XfreeFixture)) << "Missing accepted-state-2 xfree fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2GfreeFixture)) << "Missing accepted-state-2 gfree fixture";

    const fs::path dump_dir = temp_case_dir_.parent_path() / "cyclic_trace_accept2_free";
    fs::create_directories(dump_dir);
    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    const std::string env_prefix =
        "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir) +
        " FCE_TRACE_ACCEPTED_STATE_STEPS=1-3 FCE_CONSTRAINED_LBFGS_MAX_EVAL=6";
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, env_prefix), 0);

    const auto actual_xfree = read_indexed_vector_dump(dump_dir / "step1_accepted_2_xfree.dat");
    const auto expected_xfree = read_indexed_vector_dump(kCyclicReplayAccepted2XfreeFixture);
    ASSERT_EQ(actual_xfree.size(), expected_xfree.size());
    double max_xfree_abs = 0.0;
    for (std::size_t i = 0; i < actual_xfree.size(); ++i) {
        max_xfree_abs = std::max(max_xfree_abs, std::abs(actual_xfree[i] - expected_xfree[i]));
    }
    EXPECT_LE(max_xfree_abs, 1e-6);

    const auto actual_gfree = read_indexed_vector_dump(dump_dir / "step1_accepted_2_gfree.dat");
    const auto expected_gfree = read_indexed_vector_dump(kCyclicReplayAccepted2GfreeFixture);
    ASSERT_EQ(actual_gfree.size(), expected_gfree.size());
    double max_gfree_abs = 0.0;
    for (std::size_t i = 0; i < actual_gfree.size(); ++i) {
        max_gfree_abs = std::max(max_gfree_abs, std::abs(actual_gfree[i] - expected_gfree[i]));
    }
    EXPECT_LE(max_gfree_abs, 2e-3);
}

TEST_F(E2ECyclicRuntime, AcceptedState2FixtureReassemblyShowsLaggedFreeGradientContract) {
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2Fixture)) << "Missing accepted-state-2 coord fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2EtaFixture)) << "Missing accepted-state-2 eta fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2GfreeFixture)) << "Missing accepted-state-2 gfree fixture";

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    auto state = fce::make_runtime_state(input);
    state.coords = read_fortran_coord_dump(kCyclicReplayAccepted2Fixture);
    state.eta = read_fortran_eta_dump(
        kCyclicReplayAccepted2EtaFixture, input.mesh.numele, input.dims.ngauss);
    const auto fixture_eta = state.eta;

    const auto assembly = fce::assemble_energy_forces(
        input, state, /*element_begin=*/0, /*element_end=*/input.mesh.numele);
    const auto expected_gfree = read_indexed_vector_dump(kCyclicReplayAccepted2GfreeFixture);

    // The committed gfree fixture follows Fortran reverse-communication state:
    // it is lagged relative to reassembly at the accepted coordinates.
    ASSERT_EQ(expected_gfree.size(), static_cast<std::size_t>(input.bcs.ndofOP));

    double max_eta_abs = 0.0;
    for (std::size_t ielem = 0; ielem < state.eta.size(); ++ielem) {
        for (std::size_t igauss = 0; igauss < state.eta[ielem].size(); ++igauss) {
            for (int axis = 0; axis < 2; ++axis) {
                max_eta_abs = std::max(
                    max_eta_abs,
                    std::abs(state.eta[ielem][igauss][axis] - fixture_eta[ielem][igauss][axis]));
            }
        }
    }
    EXPECT_LE(max_eta_abs, 1e-12);

    double max_gfree_abs = 0.0;
    for (int i = 0; i < input.bcs.ndofOP; ++i) {
        const int flat_dof = input.bcs.mdofOP.at(static_cast<std::size_t>(i));
        const double actual = assembly.force.at(static_cast<std::size_t>(flat_dof));
        max_gfree_abs = std::max(
            max_gfree_abs,
            std::abs(actual - expected_gfree.at(static_cast<std::size_t>(i))));
    }
    EXPECT_GT(max_gfree_abs, 1.0);
}

TEST_F(E2ECyclicRuntime, AcceptedState2FixtureContributionProbeLocalizesTopRightCorner) {
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2Fixture)) << "Missing accepted-state-2 coord fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2EtaFixture)) << "Missing accepted-state-2 eta fixture";

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    const auto coords = read_fortran_coord_dump(kCyclicReplayAccepted2Fixture);
    const auto eta = read_fortran_eta_dump(
        kCyclicReplayAccepted2EtaFixture, input.mesh.numele, input.dims.ngauss);

    const auto node_1639_x = compute_force_contributions_for_target(
        input, coords, eta, /*target_node_zero_based=*/1638, /*axis=*/0);
    const auto node_1639_y = compute_force_contributions_for_target(
        input, coords, eta, /*target_node_zero_based=*/1638, /*axis=*/1);
    const auto node_1680_x = compute_force_contributions_for_target(
        input, coords, eta, /*target_node_zero_based=*/1679, /*axis=*/0);

    ASSERT_FALSE(node_1639_x.empty());
    ASSERT_FALSE(node_1639_y.empty());
    ASSERT_FALSE(node_1680_x.empty());

    EXPECT_EQ(node_1639_x.front().element_index, 3200);
    EXPECT_EQ(node_1639_x.front().local_node, 11);
    EXPECT_GT(node_1639_x.front().abs_value, 20.0);

    EXPECT_EQ(node_1639_y.front().element_index, 3200);
    EXPECT_EQ(node_1639_y.front().local_node, 11);
    EXPECT_GT(node_1639_y.front().abs_value, 10.0);

    EXPECT_EQ(node_1680_x.front().element_index, 3200);
    EXPECT_EQ(node_1680_x.front().local_node, 7);
    EXPECT_GT(node_1680_x.front().abs_value, 15.0);
}

TEST_F(E2ECyclicRuntime, AcceptedState2OracleDifferenceProbeMapsTopOffendersThroughMdofOp) {
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2Fixture)) << "Missing accepted-state-2 coord fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2EtaFixture)) << "Missing accepted-state-2 eta fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted2GfreeFixture)) << "Missing accepted-state-2 gfree fixture";

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    auto state = fce::make_runtime_state(input);
    state.coords = read_fortran_coord_dump(kCyclicReplayAccepted2Fixture);
    state.eta = read_fortran_eta_dump(
        kCyclicReplayAccepted2EtaFixture, input.mesh.numele, input.dims.ngauss);

    const auto assembly = fce::assemble_energy_forces(
        input, state, /*element_begin=*/0, /*element_end=*/input.mesh.numele);
    const auto expected_gfree = read_indexed_vector_dump(kCyclicReplayAccepted2GfreeFixture);
    const auto mismatches = compute_gfree_mismatch_hits(input, assembly.force, expected_gfree);

    ASSERT_GE(mismatches.size(), 3U);

    EXPECT_EQ(mismatches[0].free_index, 4909);
    EXPECT_EQ(mismatches[0].node_index, 1639);
    EXPECT_EQ(mismatches[0].axis, 0);
    EXPECT_GT(mismatches[0].abs_diff, 70.0);

    EXPECT_EQ(mismatches[1].free_index, 4906);
    EXPECT_EQ(mismatches[1].node_index, 1638);
    EXPECT_EQ(mismatches[1].axis, 0);
    EXPECT_GT(mismatches[1].abs_diff, 50.0);

    EXPECT_EQ(mismatches[2].free_index, 4783);
    EXPECT_EQ(mismatches[2].node_index, 1597);
    EXPECT_EQ(mismatches[2].axis, 0);
    EXPECT_GT(mismatches[2].abs_diff, 50.0);
}

TEST_F(E2ECyclicRuntime, AcceptedState3MatchesCommittedFortranReplayFixture) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted3Fixture)) << "Missing accepted-state-3 coord fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted3EtaFixture)) << "Missing accepted-state-3 eta fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted3XfreeFixture)) << "Missing accepted-state-3 xfree fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted3GfreeFixture)) << "Missing accepted-state-3 gfree fixture";

    const fs::path dump_dir = temp_case_dir_.parent_path() / "cyclic_trace_accept3";
    fs::create_directories(dump_dir);
    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    const std::string env_prefix =
        "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir) +
        " FCE_TRACE_ACCEPTED_STATE_STEPS=1-3 FCE_CONSTRAINED_LBFGS_MAX_EVAL=6";
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, env_prefix), 0);

    const auto actual_coords = read_fortran_coord_dump(dump_dir / "step1_accepted_3.dat");
    const auto expected_coords = read_fortran_coord_dump(kCyclicReplayAccepted3Fixture);
    ASSERT_EQ(actual_coords.size(), expected_coords.size());
    double max_coord_abs = 0.0;
    for (std::size_t inode = 0; inode < actual_coords.size(); ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            max_coord_abs = std::max(max_coord_abs,
                                     std::abs(actual_coords[inode][axis] - expected_coords[inode][axis]));
        }
    }
    EXPECT_LE(max_coord_abs, 1e-6);

    const auto actual_eta = read_fortran_eta_dump_flat(dump_dir / "step1_accepted_3_eta.dat");
    const auto expected_eta = read_fortran_eta_dump_flat(kCyclicReplayAccepted3EtaFixture);
    ASSERT_EQ(actual_eta.size(), expected_eta.size());
    double max_eta_abs = 0.0;
    for (std::size_t i = 0; i < actual_eta.size(); ++i) {
        for (int axis = 0; axis < 2; ++axis) {
            max_eta_abs = std::max(max_eta_abs,
                                   std::abs(actual_eta[i][axis] - expected_eta[i][axis]));
        }
    }
    EXPECT_LE(max_eta_abs, 1e-12);

    const auto actual_xfree = read_indexed_vector_dump(dump_dir / "step1_accepted_3_xfree.dat");
    const auto expected_xfree = read_indexed_vector_dump(kCyclicReplayAccepted3XfreeFixture);
    ASSERT_EQ(actual_xfree.size(), expected_xfree.size());
    double max_xfree_abs = 0.0;
    for (std::size_t i = 0; i < actual_xfree.size(); ++i) {
        const double diff = std::abs(actual_xfree[i] - expected_xfree[i]);
        if (diff > max_xfree_abs) {
            max_xfree_abs = diff;
        }
    }
    EXPECT_LE(max_xfree_abs, 1e-6);

    const auto actual_gfree = read_indexed_vector_dump(dump_dir / "step1_accepted_3_gfree.dat");
    const auto expected_gfree = read_indexed_vector_dump(kCyclicReplayAccepted3GfreeFixture);
    ASSERT_EQ(actual_gfree.size(), expected_gfree.size());
    double max_gfree_abs = 0.0;
    for (std::size_t i = 0; i < actual_gfree.size(); ++i) {
        max_gfree_abs = std::max(max_gfree_abs, std::abs(actual_gfree[i] - expected_gfree[i]));
    }
    EXPECT_LE(max_gfree_abs, 1e-3);
}

TEST_F(E2ECyclicRuntime, AcceptedState55CommittedFixtureIsHistoricalBranch) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    ASSERT_TRUE(fs::exists(kCyclicReplayTraceFixture)) << "Missing cyclic replay trace fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted55Fixture)) << "Missing accepted-state-55 coord fixture";
    ASSERT_TRUE(fs::exists(kCyclicReplayAccepted55EtaFixture)) << "Missing accepted-state-55 eta fixture";
    if (!long_oracle_tests_enabled()) {
        GTEST_SKIP() << "accepted-state-55 cyclic replay diagnostic is an opt-in long oracle gate; set "
                        "FCE_RUN_LONG_ORACLE_TESTS=1 to run it";
    }

    const fs::path dump_dir = temp_case_dir_.parent_path() / "cyclic_trace_accept55";
    fs::create_directories(dump_dir);
    fs::copy_file(kCyclicReplayTraceFixture,
                  temp_case_dir_ / "imperfection_trace.dat",
                  fs::copy_options::overwrite_existing);

    const std::string env_prefix =
        "FCE_TRACE_COORD_DUMPS=" + shell_quote(dump_dir) +
        " FCE_TRACE_ACCEPTED_STATE_STEPS=55 FCE_CONSTRAINED_LBFGS_MAX_EVAL=65";
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, env_prefix), 0);

    const auto actual_coords = read_fortran_coord_dump(dump_dir / "step1_accepted_55.dat");
    const auto expected_coords = read_fortran_coord_dump(kCyclicReplayAccepted55Fixture);
    ASSERT_EQ(actual_coords.size(), expected_coords.size());
    double max_coord_abs = 0.0;
    for (std::size_t inode = 0; inode < actual_coords.size(); ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            max_coord_abs = std::max(max_coord_abs,
                                     std::abs(actual_coords[inode][axis] - expected_coords[inode][axis]));
        }
    }
    EXPECT_GT(max_coord_abs, 1e-2);

    const auto actual_eta = read_fortran_eta_dump_flat(dump_dir / "step1_accepted_55_eta.dat");
    const auto expected_eta = read_fortran_eta_dump_flat(kCyclicReplayAccepted55EtaFixture);
    ASSERT_EQ(actual_eta.size(), expected_eta.size());
    double max_eta_abs = 0.0;
    for (std::size_t i = 0; i < actual_eta.size(); ++i) {
        for (int axis = 0; axis < 2; ++axis) {
            max_eta_abs = std::max(max_eta_abs,
                                   std::abs(actual_eta[i][axis] - expected_eta[i][axis]));
        }
    }
    EXPECT_LE(max_eta_abs, 1e-12);
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

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, "FCE_CONSTRAINED_LBFGS_MAX_EVAL=1"), 0);

    const auto energy_rows = read_numeric_rows(temp_case_dir_ / "energy.dat", /*skip_header=*/true);
    ASSERT_FALSE(energy_rows.empty());
    ASSERT_GE(energy_rows.front().values.size(), 6U);
    EXPECT_EQ(energy_rows.front().load, 0.0);
    EXPECT_EQ(energy_rows.front().values[5], 0.0);

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
    EXPECT_LE(max_abs, 2e-7);
}

TEST_F(E2ECyclicRuntime, ShortCyclicStepOneMatchesAcrossMpiRanks) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    if (!mpi_tests_enabled()) {
        GTEST_SKIP() << "MPI launcher parity is opt-in; set FCE_RUN_MPI_TESTS=1 to run it";
    }

    configure_short_cyclic_restart_case(temp_case_dir_);

    struct MpiRunResult {
        int ranks;
        std::vector<std::string> energy_tokens;
        std::vector<std::string> force_tokens;
    };

    const fs::path mpi_root = temp_case_dir_.parent_path() / "mpi_parity";
    fs::create_directories(mpi_root);
    std::vector<MpiRunResult> results;
    for (const int ranks : {1, 2, 4, 8}) {
        const fs::path case_dir = mpi_root / ("np" + std::to_string(ranks));
        fs::copy(temp_case_dir_, case_dir, fs::copy_options::recursive);
        remove_runtime_outputs(case_dir);

        const fs::path stdout_path = mpi_root / ("np" + std::to_string(ranks) + ".log");
        ASSERT_EQ(run_mpi_crunch_it(case_dir,
                                    ranks,
                                    /*stop_step=*/1,
                                    stdout_path,
                                    "FCE_CONSTRAINED_LBFGS_MAX_EVAL=1"),
                  0)
            << read_file(stdout_path);
        results.push_back(MpiRunResult{
            ranks,
            last_data_tokens(case_dir / "energy.dat"),
            last_data_tokens(case_dir / "force.dat"),
        });
    }

    ASSERT_FALSE(results.empty());
    const auto& reference = results.front();
    for (const auto& result : results) {
        ASSERT_EQ(result.energy_tokens.size(), reference.energy_tokens.size())
            << "np=" << result.ranks;
        ASSERT_EQ(result.force_tokens.size(), reference.force_tokens.size())
            << "np=" << result.ranks;

        for (std::size_t i = 0; i < result.energy_tokens.size(); ++i) {
            const double actual = fce::io::parse_fortran_double(result.energy_tokens[i]);
            const double expected = fce::io::parse_fortran_double(reference.energy_tokens[i]);
            EXPECT_LE(relative_error(actual, expected, 1e-12), 1e-10)
                << "np=" << result.ranks << " energy token " << i;
        }
        for (std::size_t i = 0; i < result.force_tokens.size(); ++i) {
            const double actual = fce::io::parse_fortran_double(result.force_tokens[i]);
            const double expected = fce::io::parse_fortran_double(reference.force_tokens[i]);
            EXPECT_LE(relative_error(actual, expected, 1e-12), 1e-10)
                << "np=" << result.ranks << " force token " << i;
        }
    }
}

#if !defined(FCE_EXCLUDE_CHECKPOINT_REJECTION_TESTS)
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
    EXPECT_NE(output.find("checkpoint nodal positions has too few columns"), std::string::npos);
}
#endif

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

TEST_F(E2ECyclicRuntime, DeletingCheckpointForcesFreshShortCyclicRun) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    configure_short_cyclic_restart_case(temp_case_dir_);

    const fs::path uninterrupted_root = make_temp_dir();
    const fs::path uninterrupted_case = uninterrupted_root / "prepro_run";
    fs::copy(temp_case_dir_, uninterrupted_case, fs::copy_options::recursive);

    ASSERT_EQ(run_crunch_it(uninterrupted_case, 4), 0);

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 2), 0);
    ASSERT_TRUE(fs::exists(temp_case_dir_ / "nano_checkpoint.dat"));
    fs::remove(temp_case_dir_ / "nano_checkpoint.dat");
    ASSERT_FALSE(fs::exists(temp_case_dir_ / "nano_checkpoint.dat"));

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

TEST_F(E2ECyclicRuntime, CrunchItRestartMatchesUninterruptedShortCyclicRunAcrossEightMpiRanks) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    if (!mpi_tests_enabled()) {
        GTEST_SKIP() << "MPI launcher restart parity is opt-in; set FCE_RUN_MPI_TESTS=1 to run it";
    }

    configure_short_cyclic_restart_case(temp_case_dir_);

    const fs::path mpi_root = temp_case_dir_.parent_path() / "mpi_restart";
    const fs::path uninterrupted_case = mpi_root / "uninterrupted_np8";
    const fs::path restarted_case = mpi_root / "restarted_np8";
    fs::create_directories(mpi_root);
    fs::copy(temp_case_dir_, uninterrupted_case, fs::copy_options::recursive);
    fs::copy(temp_case_dir_, restarted_case, fs::copy_options::recursive);

    const fs::path uninterrupted_log = mpi_root / "uninterrupted_np8.log";
    ASSERT_EQ(run_mpi_crunch_it(uninterrupted_case,
                                /*ranks=*/8,
                                /*stop_step=*/4,
                                uninterrupted_log),
              0)
        << read_file(uninterrupted_log);

    const fs::path partial_log = mpi_root / "partial_np8.log";
    ASSERT_EQ(run_mpi_crunch_it(restarted_case,
                                /*ranks=*/8,
                                /*stop_step=*/2,
                                partial_log),
              0)
        << read_file(partial_log);
    ASSERT_TRUE(fs::exists(restarted_case / "nano_checkpoint.dat"));

    const fs::path resumed_log = mpi_root / "resumed_np8.log";
    ASSERT_EQ(run_mpi_crunch_it(restarted_case,
                                /*ranks=*/8,
                                /*stop_step=*/4,
                                resumed_log),
              0)
        << read_file(resumed_log);

    EXPECT_EQ(read_file(restarted_case / "energy.dat"),
              read_file(uninterrupted_case / "energy.dat"));
    EXPECT_EQ(read_file(restarted_case / "force.dat"),
              read_file(uninterrupted_case / "force.dat"));
    EXPECT_EQ(read_file(restarted_case / "output.dat"),
              read_file(uninterrupted_case / "output.dat"));
    EXPECT_EQ(read_file(restarted_case / "nano_final_config.dat"),
              read_file(uninterrupted_case / "nano_final_config.dat"));
    ASSERT_TRUE(fs::exists(restarted_case / "nano_checkpoint.dat"));
    ASSERT_TRUE(fs::exists(uninterrupted_case / "nano_checkpoint.dat"));
    EXPECT_EQ(read_file(restarted_case / "nano_checkpoint.dat"),
              read_file(uninterrupted_case / "nano_checkpoint.dat"));
}

TEST_F(E2ECyclicRuntime, ShortCyclicCheckpointCapturesNonzeroCreaseState) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    configure_short_cyclic_crease_case(temp_case_dir_);
    ASSERT_EQ(run_crunch_it(temp_case_dir_, 2), 0);
    ASSERT_TRUE(fs::exists(temp_case_dir_ / "nano_checkpoint.dat"));

    const auto dims = fce::io::read_dims((temp_case_dir_ / "nano_dims.dat").string());
    const auto checkpoint = fce::io::read_checkpoint((temp_case_dir_ / "nano_checkpoint.dat").string(),
                                                     dims.numnods,
                                                     dims.numele,
                                                     dims.ngauss,
                                                     /*has_crease_memory=*/true);
    double max_abs = 0.0;
    for (const auto& elem_k0 : checkpoint.K0_ref) {
        for (const auto& kappa : elem_k0) {
            for (double value : kappa) {
                max_abs = std::max(max_abs, std::abs(value));
            }
        }
    }
    EXPECT_GT(max_abs, 1.0e-6);

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    const auto resume = fce::load_runtime_checkpoint(input,
                                                     temp_case_dir_.string(),
                                                     /*current_nprocs=*/1,
                                                     fce::make_runtime_state(input));
    ASSERT_EQ(resume.status, fce::CheckpointResumeStatus::loaded);
    ASSERT_EQ(resume.iload_start, checkpoint.iload + 1);
    ASSERT_EQ(resume.state.coords.size(), checkpoint.config.coords.size());
    ASSERT_EQ(resume.state.eta.size(), checkpoint.config.eta.size());
    ASSERT_EQ(resume.state.K0_ref.size(), checkpoint.K0_ref.size());

    double max_coord_abs = 0.0;
    for (std::size_t inode = 0; inode < checkpoint.config.coords.size(); ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            max_coord_abs = std::max(max_coord_abs,
                                     std::abs(resume.state.coords[inode][axis] -
                                              checkpoint.config.coords[inode][axis]));
        }
    }
    EXPECT_LE(max_coord_abs, 1e-12);

    double max_eta_abs = 0.0;
    for (std::size_t ielem = 0; ielem < checkpoint.config.eta.size(); ++ielem) {
        for (std::size_t igauss = 0; igauss < checkpoint.config.eta[ielem].size(); ++igauss) {
            for (int axis = 0; axis < 2; ++axis) {
                max_eta_abs = std::max(max_eta_abs,
                                       std::abs(resume.state.eta[ielem][igauss][axis] -
                                                checkpoint.config.eta[ielem][igauss][axis]));
            }
        }
    }
    EXPECT_LE(max_eta_abs, 1e-12);

    double max_k0_abs = 0.0;
    for (std::size_t ielem = 0; ielem < checkpoint.K0_ref.size(); ++ielem) {
        for (std::size_t igauss = 0; igauss < checkpoint.K0_ref[ielem].size(); ++igauss) {
            for (int axis = 0; axis < 3; ++axis) {
                max_k0_abs = std::max(max_k0_abs,
                                      std::abs(resume.state.K0_ref[ielem][igauss][axis] -
                                               checkpoint.K0_ref[ielem][igauss][axis]));
            }
        }
    }
    EXPECT_LE(max_k0_abs, 1e-12);
}

TEST_F(E2ECyclicRuntime, ShortCyclicRestartPreservesCreaseMapAndCheckpointState) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    configure_short_cyclic_crease_case(temp_case_dir_);

    const fs::path uninterrupted_root = make_temp_dir();
    const fs::path uninterrupted_case = uninterrupted_root / "prepro_run";
    fs::copy(temp_case_dir_, uninterrupted_case, fs::copy_options::recursive);

    ASSERT_EQ(run_crunch_it(uninterrupted_case, 4), 0);

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 2), 0);
    ASSERT_TRUE(fs::exists(temp_case_dir_ / "nano_checkpoint.dat"));
    const auto dims = fce::io::read_dims((temp_case_dir_ / "nano_dims.dat").string());
    const auto checkpoint = fce::io::read_checkpoint((temp_case_dir_ / "nano_checkpoint.dat").string(),
                                                     dims.numnods,
                                                     dims.numele,
                                                     dims.ngauss,
                                                     /*has_crease_memory=*/true);
    double max_abs = 0.0;
    for (const auto& elem_k0 : checkpoint.K0_ref) {
        for (const auto& kappa : elem_k0) {
            for (double value : kappa) {
                max_abs = std::max(max_abs, std::abs(value));
            }
        }
    }
    ASSERT_GT(max_abs, 1.0e-6);

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 4), 0);

    EXPECT_EQ(read_file(temp_case_dir_ / "energy.dat"),
              read_file(uninterrupted_case / "energy.dat"));
    EXPECT_EQ(read_file(temp_case_dir_ / "force.dat"),
              read_file(uninterrupted_case / "force.dat"));
    EXPECT_EQ(read_file(temp_case_dir_ / "output.dat"),
              read_file(uninterrupted_case / "output.dat"));
    EXPECT_EQ(read_file(temp_case_dir_ / "nano_final_config.dat"),
              read_file(uninterrupted_case / "nano_final_config.dat"));
    EXPECT_EQ(read_file(temp_case_dir_ / "crease_map.dat"),
              read_file(uninterrupted_case / "crease_map.dat"));
    ASSERT_TRUE(fs::exists(temp_case_dir_ / "nano_checkpoint.dat"));
    ASSERT_TRUE(fs::exists(uninterrupted_case / "nano_checkpoint.dat"));
    EXPECT_EQ(read_file(temp_case_dir_ / "nano_checkpoint.dat"),
              read_file(uninterrupted_case / "nano_checkpoint.dat"));

    fs::remove_all(uninterrupted_root);
}

TEST_F(E2ECyclicRuntime, CrunchItWritesCreaseMapForShortCyclicRun) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    configure_short_cyclic_crease_case(temp_case_dir_);

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 4), 0);

    const fs::path crease_map = temp_case_dir_ / "crease_map.dat";
    ASSERT_TRUE(fs::exists(crease_map));
    const std::string content = read_file(crease_map);
    EXPECT_NE(content.find("crease_map.dat"), std::string::npos);
    EXPECT_NE(content.find("Creased elements"), std::string::npos);
}

TEST_F(E2ECyclicRuntime, CrunchItSkipsCreaseMapWhenCreaseMemoryIsDisabled) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    configure_short_cyclic_restart_case(temp_case_dir_);
    auto crease = fce::io::read_crease((temp_case_dir_ / "nano_crease.dat").string(), 0, 0);
    crease.ncrease = 0;
    crease.kappa_cr = 0.0;
    crease.alpha_lock = 0.0;
    fce::io::write_crease((temp_case_dir_ / "nano_crease.dat").string(), crease, 0, 0);
    fs::remove(temp_case_dir_ / "crease_map.dat");

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 4), 0);

    EXPECT_FALSE(fs::exists(temp_case_dir_ / "crease_map.dat"));
}

TEST_F(E2ECyclicRuntime, RuntimeCreaseMapMatchesArchivedCyclicOracleFromArchivedFinalState) {
    const auto input = fce::load_simulator_input(kCyclicCaseDir.string());
    const auto archived_final = fce::io::read_config(
        (fs::path(kOracleDir) / "graphene_cyclic_crumple" / "simulator_run" / "nano_final_config.dat").string(),
        input.dims.numnods,
        input.dims.numele,
        input.dims.ngauss);
    const auto archived_checkpoint = fce::io::read_checkpoint(
        (fs::path(kOracleDir) / "graphene_cyclic_crumple" / "simulator_run" / "nano_checkpoint.dat").string(),
        input.dims.numnods,
        input.dims.numele,
        input.dims.ngauss,
        /*has_crease_memory=*/true);

    auto state = fce::make_runtime_state(input);
    state.coords = archived_final.coords;
    state.eta = archived_final.eta;
    state.K0_ref = archived_checkpoint.K0_ref;

    const fs::path temp_dir = make_temp_dir();
    ASSERT_NO_THROW(fce::write_crease_map(input, state, temp_dir.string()));

    const auto actual = read_crease_rows(temp_dir / "crease_map.dat");
    const auto expected = read_crease_rows(
        fs::path(kOracleDir) / "graphene_cyclic_crumple" / "simulator_run" / "crease_map.dat");
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i].ielem, expected[i].ielem) << "row " << i;
        EXPECT_NEAR(actual[i].kappa_mean, expected[i].kappa_mean, 1e-8) << "row " << i;
        EXPECT_NEAR(actual[i].kappa_max, expected[i].kappa_max, 1e-8) << "row " << i;
        EXPECT_EQ(actual[i].is_creased, expected[i].is_creased) << "row " << i;
        EXPECT_EQ(actual[i].n_neigh, expected[i].n_neigh) << "row " << i;
        EXPECT_NEAR(actual[i].min_dihedral_deg, expected[i].min_dihedral_deg, 1e-4) << "row " << i;
    }

    fs::remove_all(temp_dir);
}

TEST_F(E2ECyclicRuntime, ArchivedCyclicCheckpointLoadsThroughSharedResumePath) {
    const fs::path temp_root = make_temp_dir();
    const fs::path temp_case = temp_root / "prepro_run";
    fs::copy(kCyclicCaseDir, temp_case, fs::copy_options::recursive);
    fs::copy_file(fs::path(kOracleDir) / "graphene_cyclic_crumple" / "simulator_run" / "nano_checkpoint.dat",
                  temp_case / "nano_checkpoint.dat",
                  fs::copy_options::overwrite_existing);

    const auto input = fce::load_simulator_input(temp_case.string());
    const auto expected = fce::io::read_checkpoint((temp_case / "nano_checkpoint.dat").string(),
                                                   input.dims.numnods,
                                                   input.dims.numele,
                                                   input.dims.ngauss,
                                                   /*has_crease_memory=*/true);
    const auto resume = fce::load_runtime_checkpoint(input,
                                                     temp_case.string(),
                                                     /*current_nprocs=*/1,
                                                     fce::make_runtime_state(input));
    ASSERT_EQ(resume.status, fce::CheckpointResumeStatus::loaded);
    ASSERT_EQ(resume.iload_start, expected.iload + 1);
    ASSERT_EQ(resume.state.coords.size(), expected.config.coords.size());
    ASSERT_EQ(resume.state.eta.size(), expected.config.eta.size());
    ASSERT_EQ(resume.state.K0_ref.size(), expected.K0_ref.size());

    double max_coord_abs = 0.0;
    for (std::size_t inode = 0; inode < expected.config.coords.size(); ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            max_coord_abs = std::max(max_coord_abs,
                                     std::abs(resume.state.coords[inode][axis] -
                                              expected.config.coords[inode][axis]));
        }
    }
    EXPECT_LE(max_coord_abs, 1e-12);

    double max_eta_abs = 0.0;
    for (std::size_t ielem = 0; ielem < expected.config.eta.size(); ++ielem) {
        for (std::size_t igauss = 0; igauss < expected.config.eta[ielem].size(); ++igauss) {
            for (int axis = 0; axis < 2; ++axis) {
                max_eta_abs = std::max(max_eta_abs,
                                       std::abs(resume.state.eta[ielem][igauss][axis] -
                                                expected.config.eta[ielem][igauss][axis]));
            }
        }
    }
    EXPECT_LE(max_eta_abs, 1e-12);

    double max_k0_abs = 0.0;
    for (std::size_t ielem = 0; ielem < expected.K0_ref.size(); ++ielem) {
        for (std::size_t igauss = 0; igauss < expected.K0_ref[ielem].size(); ++igauss) {
            for (int axis = 0; axis < 3; ++axis) {
                max_k0_abs = std::max(max_k0_abs,
                                      std::abs(resume.state.K0_ref[ielem][igauss][axis] -
                                               expected.K0_ref[ielem][igauss][axis]));
            }
        }
    }
    EXPECT_LE(max_k0_abs, 1e-12);

    fs::remove_all(temp_root);

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

TEST_F(RuntimeOutputVdwCase, CrunchItWritesNonzeroDensityArraysForSelfContactCase) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    ASSERT_EQ(input.vdw.nvdw, 1);
    ASSERT_FALSE(input.vdw.rho.empty());
    ASSERT_FALSE(input.vdw.shapef.empty());

    ASSERT_EQ(run_crunch_it(temp_case_dir_, 1, {}, "FCE_CONSTRAINED_LBFGS_MAX_EVAL=1"), 0);

    const fs::path pvd_path = temp_case_dir_ / "mesh_config_series.pvd";
    const fs::path step0_vtu = temp_case_dir_ / fce::snapshot_filename(0);
    const fs::path step1_vtu = temp_case_dir_ / fce::snapshot_filename(1);
    ASSERT_TRUE(fs::exists(pvd_path));
    ASSERT_TRUE(fs::exists(step0_vtu));
    ASSERT_TRUE(fs::exists(step1_vtu));
    expect_xml_loadable({pvd_path, step0_vtu, step1_vtu});

    const auto datasets = read_pvd_datasets(pvd_path);
    ASSERT_EQ(datasets.size(), 2U);
    EXPECT_EQ(datasets[0].file, fce::snapshot_filename(0));
    EXPECT_EQ(datasets[1].file, fce::snapshot_filename(1));

    const auto expected_atomic_density = expected_atomic_density_from_loaded_vdw(input);
    const auto expected_w_density = expected_w_density_from_loaded_vdw(input);

    for (const fs::path& snapshot : {step0_vtu, step1_vtu}) {
        const auto generated_atomic_density = read_vtu_scalar_array(snapshot, "atomic_density");
        const auto generated_w_density = read_vtu_scalar_array(snapshot, "W_density");

        ASSERT_EQ(generated_atomic_density.size(), expected_atomic_density.size());
        ASSERT_EQ(generated_w_density.size(), expected_w_density.size());
        EXPECT_LE(max_relative_error(generated_atomic_density, expected_atomic_density, 1e-12), 1e-12)
            << snapshot.filename();
        EXPECT_LE(max_relative_error(generated_w_density, expected_w_density, 1e-12), 1e-12)
            << snapshot.filename();
        EXPECT_TRUE(has_strictly_positive_entry(generated_atomic_density)) << snapshot.filename();
        EXPECT_TRUE(has_strictly_positive_entry(generated_w_density)) << snapshot.filename();
    }
}

TEST_F(RuntimeOutputVdwCase, SelfContactSingleStepAssemblyMatchesAcrossEightMpiRanks) {
    ASSERT_TRUE(fs::exists(kCrunchItBin)) << "Missing crunch_it binary at " << kCrunchItBin;
    if (!mpi_tests_enabled()) {
        GTEST_SKIP() << "MPI runtime vdW assembly is opt-in; set FCE_RUN_MPI_TESTS=1 to run it";
    }

    const auto input = fce::load_simulator_input(temp_case_dir_.string());
    const auto state = fce::make_runtime_state(input);
    ASSERT_NO_THROW(fce::write_mesh_snapshot(input, state, temp_case_dir_.string(), 0));

    const fs::path np1_stdout = temp_case_dir_.parent_path() / "self_contact_np1.log";
    const fs::path np8_stdout = temp_case_dir_.parent_path() / "self_contact_np8.log";
    ASSERT_EQ(run_mpi_single_step_assembly(temp_case_dir_, 1, 0, np1_stdout), 0)
        << read_file(np1_stdout);
    ASSERT_EQ(run_mpi_single_step_assembly(temp_case_dir_, 8, 0, np8_stdout), 0)
        << read_file(np8_stdout);

    const double np1_energy = read_labeled_double(np1_stdout, "assembled_energy");
    const double np8_energy = read_labeled_double(np8_stdout, "assembled_energy");
    EXPECT_LE(relative_error(np8_energy, np1_energy, 1e-12), 1e-10);
    EXPECT_EQ(read_labeled_int(np1_stdout, "inner_fail"), 0);
    EXPECT_EQ(read_labeled_int(np8_stdout, "inner_fail"), 0);
    EXPECT_EQ(read_labeled_int(np1_stdout, "force_dofs"),
              3 * (input.mesh.numnods + input.mesh.nedge));
    EXPECT_EQ(read_labeled_int(np8_stdout, "force_dofs"),
              3 * (input.mesh.numnods + input.mesh.nedge));
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
        "FCE_CONSTRAINED_LBFGS_MAX_EVAL=1 " +
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
        "FCE_CONSTRAINED_LBFGS_MAX_EVAL=1 " +
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
        "FCE_CONSTRAINED_LBFGS_MAX_EVAL=1 " +
        shell_quote(kCrunchItBin) + " " + shell_quote(temp_case_dir_) + " 2";
    EXPECT_NE(std::system(command.c_str()), 0) << "Expected short imperfection trace to be rejected";
}
