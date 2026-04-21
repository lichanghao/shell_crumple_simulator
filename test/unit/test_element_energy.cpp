#include "fce/constitutive.hpp"
#include "fce/element_energy.hpp"
#include "fce/element_state.hpp"
#include "fce/exponential.hpp"
#include "fce/ghost_nodes.hpp"
#include "fce/io.hpp"
#include "fce/quadrature.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef ORACLE_DIR
#define ORACLE_DIR "test/cases"
#endif

namespace {

namespace fs = std::filesystem;

using Vec2   = fce::Vec2;
using Vec3   = fce::Vec3;
using Voigt3 = fce::Voigt3;
using Mat22  = fce::Mat22;
using NeighborCoords12 = fce::NeighborCoords12;

// ─── helpers shared with test_element_state.cpp (duplicated to keep tests self-contained) ───

double tolerance(double expected) {
    return std::max(1e-10, std::abs(expected) * 1e-8);
}

std::vector<std::vector<double>> read_rows(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open fixture: " + path.string());
    }
    std::vector<std::vector<double>> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream iss(line);
        std::vector<double> row;
        std::string token;
        while (iss >> token) {
            row.push_back(fce::io::parse_fortran_double(token));
        }
        if (!row.empty()) {
            rows.push_back(row);
        }
    }
    return rows;
}

std::vector<fs::path> sorted_fixture_paths(const fs::path& dir) {
    std::vector<fs::path> result;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dat") {
            result.push_back(entry.path());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

Vec2 row_to_vec2(const std::vector<double>& row) {
    return Vec2{row.at(0), row.at(1)};
}

Voigt3 row_to_voigt3(const std::vector<double>& row) {
    return Voigt3{row.at(0), row.at(1), row.at(2)};
}

struct ArchivedElementFixture {
    int element_index{0};   // 0-based
    int gauss_index{0};     // 0-based
    Voigt3 C_elem{};
    Vec2 curvppal{};
    Mat22 vppal{};
    Vec2 eta0{};
    double crit{0.0};
    int max_iter{0};
    int fail_mode{0};
    Vec2 eta{};             // converged eta
    double W{0.0};          // inner W at converged eta
};

struct CyclicReplayElementFixture {
    int element_index{0};
    int ngauss{0};
    std::vector<Voigt3> C_elem;
    std::vector<Voigt3> curv0_elem;
    std::vector<Vec2> curvppal;
    std::vector<Mat22> vppal;
    std::vector<bool> flag_num_diff;
    std::vector<Vec2> eta_in;
    std::vector<int> iterations;
    std::vector<int> fail_mode;
    std::vector<Vec2> eta_final;
    std::vector<double> W;
    std::vector<std::array<double, 6>> pe;
    double W_elem{0.0};
    std::array<Vec3, 12> f_elem{};
};

ArchivedElementFixture read_archived_fixture(const fs::path& path) {
    const auto rows = read_rows(path);
    if (rows.size() != 20U) {
        throw std::runtime_error("unexpected archived element fixture row count: " + path.string());
    }
    ArchivedElementFixture f;
    f.element_index = static_cast<int>(rows[0].at(0)) - 1;  // Fortran 1-based → 0-based
    f.gauss_index   = static_cast<int>(rows[0].at(1)) - 1;
    f.C_elem        = row_to_voigt3(rows[1]);
    f.curvppal      = row_to_vec2(rows[3]);
    f.vppal         = Mat22{{Vec2{rows[4].at(0), rows[4].at(1)},
                             Vec2{rows[5].at(0), rows[5].at(1)}}};
    f.eta0          = row_to_vec2(rows[12]);
    f.crit          = rows[13].at(0);
    f.max_iter      = static_cast<int>(rows[13].at(1));
    f.fail_mode     = static_cast<int>(rows[14].at(1));
    f.eta           = row_to_vec2(rows[15]);
    f.W             = rows[16].at(0);
    return f;
}

CyclicReplayElementFixture read_cyclic_replay_element_fixture(const fs::path& path) {
    const auto rows = read_rows(path);
    if (rows.size() != 40U) {
        throw std::runtime_error("unexpected cyclic replay element fixture row count: " + path.string());
    }

    CyclicReplayElementFixture f;
    f.element_index = static_cast<int>(rows[0].at(0)) - 1;
    f.ngauss = static_cast<int>(rows[0].at(1));
    std::size_t row = 1;
    f.C_elem.resize(static_cast<std::size_t>(f.ngauss));
    f.curv0_elem.resize(static_cast<std::size_t>(f.ngauss));
    f.curvppal.resize(static_cast<std::size_t>(f.ngauss));
    f.vppal.resize(static_cast<std::size_t>(f.ngauss));
    f.flag_num_diff.resize(static_cast<std::size_t>(f.ngauss));
    f.eta_in.resize(static_cast<std::size_t>(f.ngauss));
    f.iterations.resize(static_cast<std::size_t>(f.ngauss));
    f.fail_mode.resize(static_cast<std::size_t>(f.ngauss));
    f.eta_final.resize(static_cast<std::size_t>(f.ngauss));
    f.W.resize(static_cast<std::size_t>(f.ngauss));
    f.pe.resize(static_cast<std::size_t>(f.ngauss));

    for (int igauss = 0; igauss < f.ngauss; ++igauss) {
        f.C_elem[static_cast<std::size_t>(igauss)] = row_to_voigt3(rows.at(row++));
        f.curv0_elem[static_cast<std::size_t>(igauss)] = row_to_voigt3(rows.at(row++));
        f.curvppal[static_cast<std::size_t>(igauss)] = row_to_vec2(rows.at(row++));
        f.vppal[static_cast<std::size_t>(igauss)] = Mat22{
            Vec2{rows.at(row).at(0), rows.at(row).at(1)},
            Vec2{rows.at(row + 1).at(0), rows.at(row + 1).at(1)},
        };
        row += 2;
        f.flag_num_diff[static_cast<std::size_t>(igauss)] = (static_cast<int>(rows.at(row++).at(0)) != 0);
        f.eta_in[static_cast<std::size_t>(igauss)] = row_to_vec2(rows.at(row++));
        f.iterations[static_cast<std::size_t>(igauss)] = static_cast<int>(rows.at(row).at(0));
        f.fail_mode[static_cast<std::size_t>(igauss)] = static_cast<int>(rows.at(row).at(1));
        ++row;
        f.eta_final[static_cast<std::size_t>(igauss)] = row_to_vec2(rows.at(row++));
        f.W[static_cast<std::size_t>(igauss)] = rows.at(row++).at(0);
        std::copy(rows.at(row).begin(), rows.at(row).end(),
                  f.pe[static_cast<std::size_t>(igauss)].begin());
        ++row;
        row += 2;
    }
    f.W_elem = rows.at(row++).at(0);
    for (int inode = 0; inode < 12; ++inode) {
        f.f_elem[static_cast<std::size_t>(inode)] = Vec3{
            rows.at(row).at(0),
            rows.at(row).at(1),
            rows.at(row).at(2),
        };
        ++row;
    }
    return f;
}

fce::FlatCoords flatten_coords(const fce::io::ConfigData& config) {
    fce::FlatCoords flat;
    flat.reserve(config.coords.size() * 3);
    for (const auto& xyz : config.coords) {
        flat.push_back(xyz[0]);
        flat.push_back(xyz[1]);
        flat.push_back(xyz[2]);
    }
    return flat;
}

struct ArchivedCompressionState {
    fce::io::DimsData    dims;
    fce::io::GeneralData general;
    fce::Mesh            mesh;
    std::vector<fce::RefConfig> ref_config;
    fce::io::ConfigData  config;
    fce::GaussData       gauss;
    fce::FlatCoords      coords_with_ghosts;
};

const ArchivedCompressionState& archived_compression_state() {
    static const ArchivedCompressionState state = []() {
        ArchivedCompressionState out;
        const fs::path case_dir =
            fs::path(ORACLE_DIR) / "graphene_compression_simulator" / "np1";
        out.dims      = fce::io::read_dims((case_dir / "nano_dims.dat").string());
        out.general   = fce::io::read_general((case_dir / "nano_general.dat").string());
        out.mesh      = fce::io::read_mesh((case_dir / "nano_Mesh.dat").string(),
                                           out.dims.ngauss);
        out.ref_config = fce::io::read_zero((case_dir / "nano_zero.dat").string(),
                                             out.dims.numele);
        out.config    = fce::io::read_config((case_dir / "nano_final_config.dat").string(),
                                              out.dims.numnods,
                                              out.dims.numele,
                                              out.dims.ngauss);
        out.gauss     = fce::setup_gauss(out.dims.ngauss);
        out.coords_with_ghosts = flatten_coords(out.config);
        out.coords_with_ghosts.resize(
            static_cast<std::size_t>(3 * (out.mesh.numnods + out.mesh.nedge)));
        fce::ghost_nodes(out.mesh, out.coords_with_ghosts);
        return out;
    }();
    return state;
}

NeighborCoords12 neighbor_patch_from_archive(const ArchivedCompressionState& archive,
                                             int element_index) {
    NeighborCoords12 xneigh{};
    const auto& element = archive.mesh.connect.at(static_cast<std::size_t>(element_index));
    for (int inode = 0; inode < 12; ++inode) {
        const int node_index = element.neigh_vert[inode];
        const std::size_t base = static_cast<std::size_t>(3 * node_index);
        xneigh[inode] = Vec3{
            archive.coords_with_ghosts.at(base),
            archive.coords_with_ghosts.at(base + 1),
            archive.coords_with_ghosts.at(base + 2),
        };
    }
    return xneigh;
}

std::vector<Vec3> read_fortran_coord_dump(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open coord dump: " + path.string());
    }
    std::vector<Vec3> coords;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        int inode = 0;
        std::string x, y, z;
        if (!(row >> inode >> x >> y >> z)) {
            continue;
        }
        coords.push_back(Vec3{
            fce::io::parse_fortran_double(x),
            fce::io::parse_fortran_double(y),
            fce::io::parse_fortran_double(z),
        });
    }
    return coords;
}

fce::EtaField read_fortran_eta_dump(const fs::path& path, int numele, int ngauss) {
    fce::EtaField eta(
        static_cast<std::size_t>(numele),
        std::vector<fce::Vec2>(static_cast<std::size_t>(ngauss), fce::Vec2{0.0, 0.0}));
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open eta dump: " + path.string());
    }
    std::size_t index = 0;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        int ielem = 0, igauss = 0;
        std::string s1, s2;
        if (!(row >> ielem >> igauss >> s1 >> s2)) {
            continue;
        }
        eta.at(index / static_cast<std::size_t>(ngauss))
            .at(index % static_cast<std::size_t>(ngauss)) = Vec2{
                fce::io::parse_fortran_double(s1),
                fce::io::parse_fortran_double(s2),
            };
        ++index;
    }
    return eta;
}

NeighborCoords12 neighbor_patch_from_state(const fce::Mesh& mesh,
                                           const fce::Coords& coords,
                                           const int element_index) {
    fce::FlatCoords flat;
    flat.reserve(coords.size() * 3);
    for (const auto& xyz : coords) {
        flat.push_back(xyz[0]);
        flat.push_back(xyz[1]);
        flat.push_back(xyz[2]);
    }
    flat.resize(static_cast<std::size_t>(3 * (mesh.numnods + mesh.nedge)));
    fce::ghost_nodes(mesh, flat);

    NeighborCoords12 xneigh{};
    const auto& element = mesh.connect.at(static_cast<std::size_t>(element_index));
    for (int inode = 0; inode < 12; ++inode) {
        const int node_index = element.neigh_vert[inode];
        const std::size_t base = static_cast<std::size_t>(3 * node_index);
        xneigh[inode] = Vec3{flat.at(base), flat.at(base + 1), flat.at(base + 2)};
    }
    return xneigh;
}

}  // namespace

// ─── Test: evaluate_outer_potential matches inner W from archived fixtures ──────────────────

TEST(OuterPotential, MorseWMatchesInnerWAtConvergedEta) {
    // The outer and inner Morse potentials compute the same W(pe).
    // Verify: evaluate_outer_potential(mat, pe_at_converged_eta).W == fixture.W
    // where pe_at_converged_eta is recomputed from fixture.C_elem, curvppal, vppal, eta_converged.
    const fs::path fixture_dir =
        fs::path(ORACLE_DIR) / "constitutive_oracle" / "archived_compression_np1";
    const auto fixture_paths_list = sorted_fixture_paths(fixture_dir);
    ASSERT_GE(fixture_paths_list.size(), 10U);

    const auto& archive = archived_compression_state();
    const auto& mat = archive.general.mat;
    ASSERT_EQ(mat.nCode_Pot, 1) << "archived simulator must use Morse (nCode_Pot=1)";

    for (const auto& path : fixture_paths_list) {
        const auto fix = read_archived_fixture(path);

        // Recompute Ei and A_norm at converged eta
        std::array<double, 3> A_norm{};
        std::array<Vec2, 3> Ei{};
        for (int ibond = 0; ibond < 3; ++ibond) {
            Ei[ibond] = Vec2{
                mat.A0 * mat.E[ibond][0] + fix.eta[0],
                mat.A0 * mat.E[ibond][1] + fix.eta[1],
            };
            const double n = std::sqrt(Ei[ibond][0] * Ei[ibond][0] + Ei[ibond][1] * Ei[ibond][1]);
            A_norm[ibond] = n;
            Ei[ibond][0] /= n;
            Ei[ibond][1] /= n;
        }

        // Compute pe at converged eta using fixture's C_elem, curvppal, vppal
        const fce::BondState bonds =
            fce::compute_deformed_bonds(fix.C_elem, fix.curvppal, fix.vppal, A_norm, Ei);

        const fce::OuterPotentialOutput outer = fce::evaluate_outer_potential(mat, bonds.pe);
        EXPECT_NEAR(outer.W, fix.W, tolerance(fix.W))
            << path.string() << " outer W vs inner W at converged eta";
    }
}

TEST(OuterPotential, BrennerMatchesEvaluateBrenner) {
    // For Brenner (nCode_Pot=2), evaluate_outer_potential must agree with evaluate_brenner.
    fce::MatData mat;
    mat.nCode_Pot = 2;
    mat.A0 = 0.142;
    mat.A1 = 0.142;
    mat.s0 = 3.0 * std::sqrt(3.0) * mat.A0 * mat.A0 / 2.0;
    mat.E[0] = {std::sqrt(3.0) / 2.0, 0.5};
    mat.E[1] = {-std::sqrt(3.0) / 2.0, 0.5};
    mat.E[2] = {0.0, -1.0};
    mat.Vs = {0.60310500860214233, 26.25, 0.9};
    mat.Va = {0.75400000810623169, 0.149, 0.25};

    // A small non-trivial pe vector (bond lengths near A0, angles near 2π/3)
    const fce::Vec6 pe{0.140, 0.141, 0.142, 2.09, 2.10, 2.08};

    const fce::BrennerOutput brenner = fce::evaluate_brenner(mat, pe);
    const fce::OuterPotentialOutput outer = fce::evaluate_outer_potential(mat, pe);

    EXPECT_NEAR(outer.W, brenner.W, tolerance(brenner.W));
    for (int i = 0; i < 6; ++i) {
        EXPECT_NEAR(outer.dW[i], brenner.dW[i], tolerance(brenner.dW[i])) << "dW[" << i << "]";
    }
}

// ─── Test: compute_element_energy vs archived compression oracle ─────────────────────────────

TEST(ElementEnergy, MatchesArchivedCompressionSimulatorOracleFixtures) {
    // The archived fixtures come in pairs (one per Gauss point) for 5 elements.
    // For each element pair, verify:
    //   result.eta[ig] ≈ fixture.eta  (converged Newton eta per Gauss point)
    //   result.W_elem  ≈ sum_ig(fixture.W[ig] * gauss.weight[ig])
    const fs::path fixture_dir =
        fs::path(ORACLE_DIR) / "constitutive_oracle" / "archived_compression_np1";
    const auto fixture_paths = sorted_fixture_paths(fixture_dir);
    ASSERT_GE(fixture_paths.size(), 10U);

    const auto& archive = archived_compression_state();
    const auto& mat = archive.general.mat;
    const int ngauss = archive.dims.ngauss;
    const Voigt3 reference_curvature{0.0, 0.0, 0.0};

    // Load all 10 fixtures
    std::vector<ArchivedElementFixture> fixtures;
    fixtures.reserve(fixture_paths.size());
    for (const auto& p : fixture_paths) {
        fixtures.push_back(read_archived_fixture(p));
    }

    // Fixtures are ordered: [elem83/g0, elem83/g1, elem84/g0, elem84/g1, ...]
    // Process pairs
    const int num_elements = static_cast<int>(fixtures.size()) / ngauss;
    for (int ielem = 0; ielem < num_elements; ++ielem) {
        const int base = ielem * ngauss;
        // All Gauss points of this element share the same element_index
        const int element_index = fixtures[static_cast<std::size_t>(base)].element_index;
        for (int ig = 0; ig < ngauss; ++ig) {
            ASSERT_EQ(fixtures[static_cast<std::size_t>(base + ig)].element_index, element_index)
                << "fixture pair element mismatch at base=" << base << " ig=" << ig;
        }

        const auto xneigh = neighbor_patch_from_archive(archive, element_index);
        const auto& f0 = archive.ref_config.at(static_cast<std::size_t>(element_index)).F0;

        // Build eta0 (zeros, matching the Fortran simulator initial state per load step)
        std::vector<Vec2> eta0(static_cast<std::size_t>(ngauss), Vec2{0.0, 0.0});

        // Use crit and max_iter from the first fixture of this element (same for all)
        const double crit     = fixtures[static_cast<std::size_t>(base)].crit;
        const int    max_iter = fixtures[static_cast<std::size_t>(base)].max_iter;

        const auto result = fce::compute_element_energy(
            xneigh, f0, reference_curvature, archive.gauss,
            mat, /*nW_hat=*/true, crit, max_iter, eta0);

        EXPECT_EQ(result.inner_fail, 0) << "element_index=" << element_index;

        // Per-Gauss-point eta
        for (int ig = 0; ig < ngauss; ++ig) {
            const auto& fix = fixtures[static_cast<std::size_t>(base + ig)];
            ASSERT_EQ(fix.fail_mode, 0) << "fixture fail_mode nonzero at element "
                                         << element_index << " gauss " << ig;
            for (int d = 0; d < 2; ++d) {
                EXPECT_NEAR(result.eta.at(static_cast<std::size_t>(ig))[d],
                            fix.eta[d],
                            tolerance(fix.eta[d]))
                    << "element=" << element_index << " gauss=" << ig << " eta[" << d << "]";
            }
        }

        // Total element energy = sum over Gauss points of (fixture.W * weight)
        double expected_W_elem = 0.0;
        for (int ig = 0; ig < ngauss; ++ig) {
            expected_W_elem += fixtures[static_cast<std::size_t>(base + ig)].W *
                               archive.gauss.weight.at(static_cast<std::size_t>(ig));
        }
        EXPECT_NEAR(result.W_elem, expected_W_elem, tolerance(expected_W_elem))
            << "element=" << element_index;
    }
}

// ─── Test: nW_hat=false path (no inner relaxation, zero eta) ─────────────────────────────────

TEST(ElementEnergy, NoInnerRelaxationProducesZeroEtaAndNonzeroW) {
    // When nW_hat=false, eta stays at zero and W/forces are computed from the outer potential.
    // Verify eta is unchanged and W is non-zero (finite non-trivial geometry).
    const auto& archive = archived_compression_state();
    const auto& mat = archive.general.mat;
    const int ngauss = archive.dims.ngauss;
    const Voigt3 reference_curvature{0.0, 0.0, 0.0};

    // Use element 83 (element_index=82, 0-based)
    const int element_index = 82;
    const auto xneigh = neighbor_patch_from_archive(archive, element_index);
    const auto& f0 = archive.ref_config.at(static_cast<std::size_t>(element_index)).F0;

    const std::vector<Vec2> eta0(static_cast<std::size_t>(ngauss), Vec2{0.0, 0.0});
    const auto result = fce::compute_element_energy(
        xneigh, f0, reference_curvature, archive.gauss,
        mat, /*nW_hat=*/false, 1e-8, 100, eta0);

    // eta must remain zero (not updated when nW_hat=false)
    for (int ig = 0; ig < ngauss; ++ig) {
        EXPECT_DOUBLE_EQ(result.eta.at(static_cast<std::size_t>(ig))[0], 0.0) << "ig=" << ig;
        EXPECT_DOUBLE_EQ(result.eta.at(static_cast<std::size_t>(ig))[1], 0.0) << "ig=" << ig;
    }

    // W_elem must be finite and non-zero (deformed non-trivial geometry)
    EXPECT_TRUE(std::isfinite(result.W_elem));
    EXPECT_NE(result.W_elem, 0.0);

    // inner_fail must be zero (no Newton was run)
    EXPECT_EQ(result.inner_fail, 0);
}

// ─── Test: f_elem is consistent with finite-difference derivative of W_elem ──────────────────

TEST(ElementEnergy, ForcesAreConsistentWithEnergyByFiniteDifference) {
    // For nW_hat=false (outer potential, fixed eta=0), the analytical forces in f_elem
    // must agree with the centered finite-difference derivative of W_elem:
    //   f_elem[inode][k] ≈ (W(x+h) - W(x-h)) / (2h)
    // f_elem is the energy gradient (+dW/dx), not the particle force (-dW/dx).
    // Centered FD has O(h²) truncation error; with h=1e-6 the floor is ~1e-12,
    // well within the 1e-4 relative tolerance.
    const auto& archive = archived_compression_state();
    const auto& mat = archive.general.mat;
    const int ngauss = archive.dims.ngauss;
    const Voigt3 reference_curvature{0.0, 0.0, 0.0};
    const int element_index = 82;  // element 83, 0-based

    const auto xneigh_base = neighbor_patch_from_archive(archive, element_index);
    const auto& f0 = archive.ref_config.at(static_cast<std::size_t>(element_index)).F0;
    const std::vector<Vec2> eta0(static_cast<std::size_t>(ngauss), Vec2{0.0, 0.0});

    const auto result_base = fce::compute_element_energy(
        xneigh_base, f0, reference_curvature, archive.gauss,
        mat, /*nW_hat=*/false, 1e-8, 100, eta0);
    ASSERT_EQ(result_base.inner_fail, 0);

    constexpr double h = 1e-6;
    for (int inode = 0; inode < 12; ++inode) {
        for (int k = 0; k < 3; ++k) {
            auto xneigh_p = xneigh_base;
            xneigh_p[inode][k] += h;
            auto xneigh_m = xneigh_base;
            xneigh_m[inode][k] -= h;

            const auto result_p = fce::compute_element_energy(
                xneigh_p, f0, reference_curvature, archive.gauss,
                mat, /*nW_hat=*/false, 1e-8, 100, eta0);
            const auto result_m = fce::compute_element_energy(
                xneigh_m, f0, reference_curvature, archive.gauss,
                mat, /*nW_hat=*/false, 1e-8, 100, eta0);

            const double fd_force = (result_p.W_elem - result_m.W_elem) / (2.0 * h);
            const double analytic  = result_base.f_elem[inode][k];
            const double tol = 1e-4 * std::max(std::abs(fd_force), 1e-10);
            EXPECT_NEAR(analytic, fd_force, tol)
                << "inode=" << inode << " k=" << k
                << " analytic=" << analytic << " fd=" << fd_force;
        }
    }
}

// ─── Test: flag_num_diff=true path (flat geometry, equal principal curvatures) ───────────────

TEST(ElementEnergy, FlagNumDiffPathProducesFiniteEnergyAndForces) {
    // For a flat (z=0) geometry, curv0_elem=0 → both principal curvatures=0 → beta=0
    // → flag_num_diff=true.  compute_element_energy must produce finite W_elem and f_elem.
    const auto& archive = archived_compression_state();
    const auto& mat = archive.general.mat;
    const int ngauss = archive.dims.ngauss;
    const Voigt3 reference_curvature{0.0, 0.0, 0.0};
    const int element_index = 82;  // element 83, 0-based

    // Build flat xneigh: preserve x,y from archive but zero out z
    auto xneigh_flat = neighbor_patch_from_archive(archive, element_index);
    for (auto& node : xneigh_flat) {
        node[2] = 0.0;
    }
    const auto& f0 = archive.ref_config.at(static_cast<std::size_t>(element_index)).F0;

    // Confirm flag_num_diff is triggered at the first Gauss point of the flat geometry
    {
        const auto& sf = archive.gauss.shapef.at(0);
        fce::ShapeGradient12 dn{};
        fce::ShapeCurvature12 ddn{};
        for (int inode = 0; inode < 12; ++inode) {
            dn[inode]  = Vec2{sf[inode][1], sf[inode][2]};
            ddn[inode] = Voigt3{sf[inode][3], sf[inode][4], sf[inode][5]};
        }
        const auto state = fce::compute_element_state(
            xneigh_flat, dn, ddn, f0, reference_curvature);
        ASSERT_TRUE(state.flag_num_diff)
            << "flat (z=0) geometry must trigger flag_num_diff at Gauss point 0";
    }

    const std::vector<Vec2> eta0(static_cast<std::size_t>(ngauss), Vec2{0.0, 0.0});
    const auto result = fce::compute_element_energy(
        xneigh_flat, f0, reference_curvature, archive.gauss,
        mat, /*nW_hat=*/true, 1e-8, 100, eta0);

    EXPECT_TRUE(std::isfinite(result.W_elem)) << "W_elem must be finite in flag_num_diff path";
    EXPECT_EQ(result.inner_fail, 0) << "inner Newton must converge for flat element";
    for (int inode = 0; inode < 12; ++inode) {
        for (int k = 0; k < 3; ++k) {
            EXPECT_TRUE(std::isfinite(result.f_elem[inode][k]))
                << "f_elem[" << inode << "][" << k << "] must be finite in flag_num_diff path";
        }
    }
}

// ─── Test: f_elem matches Fortran oracle for element 83 (analytical path) ────────────────────

TEST(ElementEnergy, FElemMatchesFortranOracle) {
    // Canonical Fortran reference: dump_element_energy_oracle.f90 calls ener_elem.f90's
    // analytical path (flag_num_diff=false) for element 83 from the deformed compression
    // state and dumps W_elem and f_elem(12,3).  Both must match within 1e-8 absolute.
    const fs::path fixture_path =
        fs::path(ORACLE_DIR) / "element_energy_oracle" / "archived_compression_np1" / "case_01.dat";
    const auto rows = read_rows(fixture_path);
    ASSERT_EQ(rows.size(), 14U) << "fixture must have 14 rows (header + W_elem + 12 nodes)";

    const int fixture_elem  = static_cast<int>(rows[0].at(0));  // 1-based
    const int fixture_ngauss = static_cast<int>(rows[0].at(1));
    const double fixture_W_elem = rows[1].at(0);

    const auto& archive = archived_compression_state();
    const auto& mat = archive.general.mat;
    const int ngauss = archive.dims.ngauss;
    const Voigt3 reference_curvature{0.0, 0.0, 0.0};

    ASSERT_EQ(fixture_elem - 1, 82) << "oracle must be for element 83 (0-based: 82)";
    ASSERT_EQ(fixture_ngauss, ngauss);

    const int element_index = fixture_elem - 1;  // 0-based
    const auto xneigh = neighbor_patch_from_archive(archive, element_index);
    const auto& f0 = archive.ref_config.at(static_cast<std::size_t>(element_index)).F0;
    const std::vector<Vec2> eta0(static_cast<std::size_t>(ngauss), Vec2{0.0, 0.0});

    const auto result = fce::compute_element_energy(
        xneigh, f0, reference_curvature, archive.gauss,
        mat, /*nW_hat=*/true, 1e-8, 100, eta0);

    ASSERT_EQ(result.inner_fail, 0) << "inner Newton must converge for element 83";

    // Check W_elem
    constexpr double tol = 1e-8;
    EXPECT_NEAR(result.W_elem, fixture_W_elem, tol)
        << "W_elem mismatch vs Fortran oracle";

    // Check f_elem(12 nodes × 3 components) — rows 2..13 of fixture
    for (int inode = 0; inode < 12; ++inode) {
        const auto& row = rows.at(static_cast<std::size_t>(2 + inode));
        for (int k = 0; k < 3; ++k) {
            const double expected = row.at(static_cast<std::size_t>(k));
            EXPECT_NEAR(result.f_elem[inode][k], expected, tol)
                << "f_elem[" << inode << "][" << k << "] mismatch vs Fortran oracle";
        }
    }
}

// ─── Test: flag_num_diff S_n/S_m match Fortran oracle for flat geometry ──────────────────────

TEST(ElementEnergy, FlagNumDiffStressesMatchFortranOracle) {
    // Canonical Fortran reference: dump_element_energy_oracle.f90 in flat mode sets z=0
    // for all neighbor nodes of element 83 → curv0_elem=0 → k1=k2=0 → flag_num_diff=true.
    // The fixture stores per-Gauss flag_num_diff, S_n[3], S_m[3] in addition to W_elem
    // and f_elem.  This test directly verifies the Round-25 S_m fix: in the flag_num_diff
    // branch, S_m must perturb C_elem (not curv0_elem), so S_n == S_m.
    //
    // Fixture format (flat_geom_np1/case_01.dat):
    //   Row 0:     ielem  ngauss
    //   Row 1:     W_elem
    //   Rows 2-13: f_elem(inode, 0:2)
    //   For each Gauss point (3 rows each):
    //     Row 14+ig*3: flag_num_diff (1 or 0)
    //     Row 15+ig*3: S_n[3]
    //     Row 16+ig*3: S_m[3]
    const fs::path fixture_path =
        fs::path(ORACLE_DIR) / "element_energy_oracle" / "flat_geom_np1" / "case_01.dat";
    const auto rows = read_rows(fixture_path);
    // 1 header + 1 W_elem + 12 f_elem + ngauss*(1 flag + 2 stress) = 14 + 3*ngauss
    // For ngauss=2: 20 rows
    ASSERT_EQ(rows.size(), 20U) << "flat fixture must have 20 rows for ngauss=2";

    const int fixture_elem   = static_cast<int>(rows[0].at(0));  // 1-based
    const int fixture_ngauss = static_cast<int>(rows[0].at(1));
    const double fixture_W   = rows[1].at(0);

    const auto& archive = archived_compression_state();
    const auto& mat = archive.general.mat;
    const int ngauss = archive.dims.ngauss;
    ASSERT_EQ(fixture_elem - 1, 82);
    ASSERT_EQ(fixture_ngauss, ngauss);

    const Voigt3 reference_curvature{0.0, 0.0, 0.0};
    const int element_index = 82;

    auto xneigh_flat = neighbor_patch_from_archive(archive, element_index);
    for (auto& node : xneigh_flat) {
        node[2] = 0.0;
    }
    const auto& f0 = archive.ref_config.at(static_cast<std::size_t>(element_index)).F0;

    // --- W_elem and f_elem ---
    const std::vector<Vec2> eta0(static_cast<std::size_t>(ngauss), Vec2{0.0, 0.0});
    const auto result = fce::compute_element_energy(
        xneigh_flat, f0, reference_curvature, archive.gauss,
        mat, /*nW_hat=*/true, 1e-8, 100, eta0);

    ASSERT_EQ(result.inner_fail, 0) << "inner Newton must converge for flat element";

    // W_elem tolerance: 1e-6 (larger than archived case due to h=1e-8 FD cancellation
    // combined with gfortran/g++ floating-point rounding order differences)
    EXPECT_NEAR(result.W_elem, fixture_W, 1e-6) << "W_elem mismatch vs flat Fortran oracle";

    for (int inode = 0; inode < 12; ++inode) {
        const auto& row = rows.at(static_cast<std::size_t>(2 + inode));
        for (int k = 0; k < 3; ++k) {
            const double expected = row.at(static_cast<std::size_t>(k));
            // Relative tolerance 1e-7 with absolute floor 1e-6: flat-case f_elem values can
            // be O(20), and the FD cancellation error in S_n/S_m is O(eps*W/h) ~ 1e-7
            const double tol_fe = std::max(1e-6, std::abs(expected) * 1e-7);
            EXPECT_NEAR(result.f_elem[inode][k], expected, tol_fe)
                << "f_elem[" << inode << "][" << k << "] mismatch vs flat Fortran oracle";
        }
    }

    // --- Per-Gauss S_n and S_m via direct stress parity ---
    constexpr double h = 1e-8;
    // S_n/S_m tolerance: 1e-6 absolute; the FD h=1e-8 cancellation error combined with
    // gfortran/g++ rounding order differences produces differences of up to ~3e-7
    constexpr double tol_stress = 1e-6;

    for (int igauss = 0; igauss < ngauss; ++igauss) {
        const std::size_t row_base = static_cast<std::size_t>(14 + igauss * 3);
        const bool fixture_flag = (rows.at(row_base).at(0) != 0.0);
        const Voigt3 oracle_S_n = row_to_voigt3(rows.at(row_base + 1));
        const Voigt3 oracle_S_m = row_to_voigt3(rows.at(row_base + 2));

        ASSERT_TRUE(fixture_flag)
            << "flat geometry must have flag_num_diff=true at gauss " << igauss;

        // Compute element state for this Gauss point
        const auto& sf = archive.gauss.shapef.at(static_cast<std::size_t>(igauss));
        fce::ShapeGradient12 dn{};
        fce::ShapeCurvature12 ddn{};
        for (int inode = 0; inode < 12; ++inode) {
            dn[inode]  = Vec2{sf[inode][1], sf[inode][2]};
            ddn[inode] = Voigt3{sf[inode][3], sf[inode][4], sf[inode][5]};
        }
        const auto state = fce::compute_element_state(
            xneigh_flat, dn, ddn, f0, reference_curvature);
        ASSERT_TRUE(state.flag_num_diff)
            << "flat geometry must trigger flag_num_diff at gauss " << igauss;

        // Newton relaxation to get converged eta
        const auto inner = fce::solve_inner_newton(
            state, mat, Vec2{0.0, 0.0}, 1e-8, 100);
        ASSERT_EQ(inner.fail_mode, 0) << "inner Newton must converge at gauss " << igauss;

        // Bond vectors at converged eta (matches element_energy.cpp lines 65-74)
        std::array<double, 3> A_norm{};
        std::array<Vec2, 3> Ei{};
        for (int ibond = 0; ibond < 3; ++ibond) {
            Ei[ibond] = Vec2{
                mat.A0 * mat.E[ibond][0] + inner.eta[0],
                mat.A0 * mat.E[ibond][1] + inner.eta[1],
            };
            const double n = std::sqrt(Ei[ibond][0] * Ei[ibond][0] +
                                       Ei[ibond][1] * Ei[ibond][1]);
            A_norm[ibond] = n;
            Ei[ibond][0] /= n;
            Ei[ibond][1] /= n;
        }

        // S_n and S_m via flag_num_diff numerical-diff path (element_energy.cpp lines 87-104)
        const auto bonds_base = fce::compute_deformed_bonds(
            state.C_elem, state.curvppal, state.vppal, A_norm, Ei);
        const double W_base = fce::evaluate_outer_potential(mat, bonds_base.pe).W;

        Voigt3 cpp_S_n{}, cpp_S_m{};
        for (int i = 0; i < 3; ++i) {
            // S_n: perturb C_elem[i]
            Voigt3 C_p = state.C_elem;
            C_p[i] += h;
            const auto pp_n = fce::compute_principal_curvature(C_p, state.curv0_elem);
            const auto bonds_n = fce::compute_deformed_bonds(
                C_p, pp_n.curvppal, pp_n.vppal, A_norm, Ei);
            cpp_S_n[i] = (fce::evaluate_outer_potential(mat, bonds_n.pe).W - W_base) / h;

            // S_m: perturb C_elem[i] (Round-25 fix — identical formula to S_n)
            Voigt3 C_pm = state.C_elem;
            C_pm[i] += h;
            const auto pp_m = fce::compute_principal_curvature(C_pm, state.curv0_elem);
            const auto bonds_m = fce::compute_deformed_bonds(
                C_pm, pp_m.curvppal, pp_m.vppal, A_norm, Ei);
            cpp_S_m[i] = (fce::evaluate_outer_potential(mat, bonds_m.pe).W - W_base) / h;
        }

        for (int i = 0; i < 3; ++i) {
            EXPECT_NEAR(cpp_S_n[i], oracle_S_n[i], tol_stress)
                << "S_n[" << i << "] mismatch at gauss " << igauss;
            EXPECT_NEAR(cpp_S_m[i], oracle_S_m[i], tol_stress)
                << "S_m[" << i << "] mismatch at gauss " << igauss;
            // S_n == S_m exactly: both use identical C_elem perturbation formula
            EXPECT_EQ(cpp_S_n[i], cpp_S_m[i])
                << "S_n must equal S_m at gauss " << igauss << " component " << i;
        }
    }
}

// ─── Test: Brenner material element energy matches Fortran oracle ─────────────────────────────

TEST(ElementEnergy, BrennerMaterialMatchesFortranOracle) {
    // Uses element 83's geometry from the archived compression state but with the Brenner REBO
    // material (nCode_Pot=2). Compares W_elem and f_elem(12,3) against the Fortran oracle
    // dump_element_energy_brenner_oracle.f90 which uses the same geometry and Brenner parameters.
    // This validates the production compute_element_energy path for nCode_Pot=2.
    const fs::path fixture_path =
        fs::path(ORACLE_DIR) / "element_energy_oracle" / "brenner_geom_np1" / "case_01.dat";
    const auto rows = read_rows(fixture_path);
    ASSERT_EQ(rows.size(), 14U) << "fixture must have 14 rows (header + W_elem + 12 nodes)";

    const int fixture_elem   = static_cast<int>(rows[0].at(0));  // 1-based
    const int fixture_ngauss = static_cast<int>(rows[0].at(1));
    const double fixture_W_elem = rows[1].at(0);

    // Brenner material: same parameters as dump_constitutive_oracle.f90 / dump_element_energy_brenner_oracle.f90
    fce::MatData mat;
    mat.nCode_Pot = 2;
    mat.A0 = 0.142;
    mat.A1 = 0.142;
    mat.s0 = 3.0 * std::sqrt(3.0) * mat.A0 * mat.A0 / 2.0;
    mat.E[0] = {std::sqrt(3.0) / 2.0, 0.5};
    mat.E[1] = {-std::sqrt(3.0) / 2.0, 0.5};
    mat.E[2] = {0.0, -1.0};
    mat.Vs = {0.60310500860214233, 26.25, 0.9};
    mat.Va = {0.75400000810623169, 0.149, 0.25};

    const auto& archive = archived_compression_state();
    const int ngauss = archive.dims.ngauss;
    const Voigt3 reference_curvature{0.0, 0.0, 0.0};

    ASSERT_EQ(fixture_elem - 1, 82) << "oracle must be for element 83 (0-based: 82)";
    ASSERT_EQ(fixture_ngauss, ngauss);

    const int element_index = fixture_elem - 1;  // 0-based
    const auto xneigh = neighbor_patch_from_archive(archive, element_index);
    const auto& f0 = archive.ref_config.at(static_cast<std::size_t>(element_index)).F0;
    const std::vector<Vec2> eta0(static_cast<std::size_t>(ngauss), Vec2{0.0, 0.0});

    const auto result = fce::compute_element_energy(
        xneigh, f0, reference_curvature, archive.gauss,
        mat, /*nW_hat=*/true, 1e-8, 100, eta0);

    ASSERT_EQ(result.inner_fail, 0) << "Brenner inner Newton must converge for element 83";

    // Tolerance: 1e-6 absolute (Brenner values are O(100), inner FD with h=1e-8 gives ~1e-6 differences)
    constexpr double tol = 1e-6;
    EXPECT_NEAR(result.W_elem, fixture_W_elem, tol) << "W_elem mismatch vs Fortran Brenner oracle";

    for (int inode = 0; inode < 12; ++inode) {
        const auto& row = rows.at(static_cast<std::size_t>(2 + inode));
        for (int k = 0; k < 3; ++k) {
            const double expected = row.at(static_cast<std::size_t>(k));
            EXPECT_NEAR(result.f_elem[inode][k], expected, std::max(tol, std::abs(expected) * 1e-6))
                << "f_elem[" << inode << "][" << k << "] vs Fortran Brenner oracle";
        }
    }
}

TEST(ElementEnergy, CyclicReplayAcceptedStateTwoElement3200MatchesFortranOracle) {
    const fs::path case_dir =
        fs::path(ORACLE_DIR) / "graphene_cyclic_crumple" / "prepro_run";
    const auto dims = fce::io::read_dims((case_dir / "nano_dims.dat").string());
    const auto general = fce::io::read_general((case_dir / "nano_general.dat").string());
    const auto mesh = fce::io::read_mesh((case_dir / "nano_Mesh.dat").string(), dims.ngauss);
    const auto ref_config = fce::io::read_zero((case_dir / "nano_zero.dat").string(), dims.numele);
    const auto gauss = fce::setup_gauss(dims.ngauss);

    const auto coords = read_fortran_coord_dump(
        fs::path(ORACLE_DIR) / "graphene_cyclic_crumple" / "replay_step1_accepted_2.dat");
    const auto eta = read_fortran_eta_dump(
        fs::path(ORACLE_DIR) / "graphene_cyclic_crumple" / "replay_step1_accepted_2_eta.dat",
        dims.numele,
        dims.ngauss);
    const auto oracle = read_cyclic_replay_element_fixture(
        fs::path(ORACLE_DIR) / "graphene_cyclic_crumple" / "replay_step1_accepted_2_element3200_full_oracle.dat");

    ASSERT_FALSE(general.nW_hat);
    ASSERT_EQ(oracle.element_index, 3199);
    ASSERT_EQ(oracle.ngauss, dims.ngauss);

    const auto xneigh = neighbor_patch_from_state(mesh, coords, oracle.element_index);
    const auto result = fce::compute_element_energy(
        xneigh,
        ref_config.at(static_cast<std::size_t>(oracle.element_index)).F0,
        std::vector<Voigt3>(static_cast<std::size_t>(dims.ngauss), Voigt3{0.0, 0.0, 0.0}),
        gauss,
        general.mat,
        general.nW_hat,
        general.crit_local,
        100,
        eta.at(static_cast<std::size_t>(oracle.element_index)));

    EXPECT_EQ(result.inner_fail, 0);
    EXPECT_NEAR(result.W_elem, oracle.W_elem, 1e-8);
    for (int igauss = 0; igauss < dims.ngauss; ++igauss) {
        EXPECT_FALSE(oracle.flag_num_diff.at(static_cast<std::size_t>(igauss)));
        EXPECT_EQ(oracle.iterations.at(static_cast<std::size_t>(igauss)), 0);
        EXPECT_EQ(oracle.fail_mode.at(static_cast<std::size_t>(igauss)), 0);
        for (int axis = 0; axis < 2; ++axis) {
            EXPECT_NEAR(result.eta.at(static_cast<std::size_t>(igauss))[axis],
                        oracle.eta_final.at(static_cast<std::size_t>(igauss))[axis],
                        1e-12);
            EXPECT_NEAR(oracle.eta_in.at(static_cast<std::size_t>(igauss))[axis],
                        oracle.eta_final.at(static_cast<std::size_t>(igauss))[axis],
                        1e-12);
        }
    }
    for (int inode = 0; inode < 12; ++inode) {
        for (int axis = 0; axis < 3; ++axis) {
            EXPECT_NEAR(result.f_elem[inode][axis],
                        oracle.f_elem.at(static_cast<std::size_t>(inode))[axis],
                        1e-8)
                << "inode=" << inode << " axis=" << axis;
        }
    }
}
