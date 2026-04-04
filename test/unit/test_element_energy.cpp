#include "fce/constitutive.hpp"
#include "fce/element_energy.hpp"
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
