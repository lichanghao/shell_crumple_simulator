// Integration test: read oracle nano_*.dat, write to a temp file, re-read and compare.
// Verifies round-trip fidelity for all supported file types.

#include "fce/io.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>

#ifndef ORACLE_DIR
#define ORACLE_DIR "test/cases"
#endif

static const std::string kPrepro = std::string(ORACLE_DIR) + "/graphene_compression_prepro/";
static const std::string kTmp    = "/tmp/fce_test_";

static bool near_rel(double a, double b, double rtol = 1e-10, double atol = 1e-12) {
    return std::abs(a - b) <= atol + rtol * std::max(std::abs(a), std::abs(b));
}

// ─── Round-trip: nano_dims.dat ────────────────────────────────────────────────

TEST(RoundTrip, Dims) {
    auto d1 = fce::io::read_dims(kPrepro + "nano_dims.dat");
    std::string tmp = kTmp + "dims.dat";
    fce::io::write_dims(tmp, d1);
    auto d2 = fce::io::read_dims(tmp);
    EXPECT_EQ(d1.numele,       d2.numele);
    EXPECT_EQ(d1.numnods,      d2.numnods);
    EXPECT_EQ(d1.nedge,        d2.nedge);
    EXPECT_EQ(d1.nelem_ghost,  d2.nelem_ghost);
    EXPECT_EQ(d1.nnode_ghost,  d2.nnode_ghost);
    EXPECT_EQ(d1.ngauss,       d2.ngauss);
    EXPECT_EQ(d1.nnodBC,       d2.nnodBC);
    EXPECT_EQ(d1.ndofBC,       d2.ndofBC);
    EXPECT_EQ(d1.ndofOP,       d2.ndofOP);
    EXPECT_EQ(d1.nvdw,         d2.nvdw);
}

// ─── Round-trip: nano_general.dat ─────────────────────────────────────────────

TEST(RoundTrip, General) {
    auto g1 = fce::io::read_general(kPrepro + "nano_general.dat");
    std::string tmp = kTmp + "general.dat";
    fce::io::write_general(tmp, g1);
    auto g2 = fce::io::read_general(tmp);
    EXPECT_NEAR(g1.ylength,      g2.ylength,    1e-13);
    EXPECT_NEAR(g1.mat.A0,       g2.mat.A0,     1e-14);
    EXPECT_EQ  (g1.mat.nCode_Pot, g2.mat.nCode_Pot);
    for (int i = 0; i < 3; ++i) {
        EXPECT_NEAR(g1.mat.E[i][0], g2.mat.E[i][0], 1e-13);
        EXPECT_NEAR(g1.mat.E[i][1], g2.mat.E[i][1], 1e-13);
    }
    EXPECT_NEAR(g1.mat.s0,       g2.mat.s0,     1e-13);
    EXPECT_EQ  (g1.nW_hat,       g2.nW_hat);
    EXPECT_NEAR(g1.crit_global,  g2.crit_global, 1e-20);
    EXPECT_NEAR(g1.crit_local,   g2.crit_local,  1e-20);
    EXPECT_EQ  (g1.imperfect,    g2.imperfect);
    EXPECT_NEAR(g1.fact_imp,     g2.fact_imp,   1e-14);
}

// ─── Round-trip: nano_zero.dat ────────────────────────────────────────────────

TEST(RoundTrip, Zero) {
    auto rc1 = fce::io::read_zero(kPrepro + "nano_zero.dat", 3200);
    std::string tmp = kTmp + "zero.dat";
    fce::io::write_zero(tmp, rc1, 3200);
    auto rc2 = fce::io::read_zero(tmp, 3200);
    ASSERT_EQ(rc1.size(), rc2.size());
    for (int k = 0; k < static_cast<int>(rc1.size()); ++k) {
        EXPECT_NEAR(rc1[k].J0, rc2[k].J0, 1e-12) << "k=" << k;
        for (int r = 0; r < 2; ++r)
            for (int c = 0; c < 2; ++c)
                EXPECT_NEAR(rc1[k].F0[r][c], rc2[k].F0[r][c], 1e-12) << "k=" << k << " r=" << r << " c=" << c;
    }
}

// ─── Round-trip: nano_config.dat ──────────────────────────────────────────────

TEST(RoundTrip, Config) {
    auto c1 = fce::io::read_config(kPrepro + "nano_config.dat", 1681, 3200, 2);
    std::string tmp = kTmp + "config.dat";
    fce::io::write_config(tmp, c1, 1681, 3200, 2);
    auto c2 = fce::io::read_config(tmp, 1681, 3200, 2);
    ASSERT_EQ(c1.coords.size(), c2.coords.size());
    for (int i = 0; i < 1681; ++i) {
        for (int d = 0; d < 3; ++d)
            EXPECT_NEAR(c1.coords[i][d], c2.coords[i][d], 1e-12) << "node=" << i << " d=" << d;
    }
    for (int ie = 0; ie < 3200; ++ie)
        for (int ig = 0; ig < 2; ++ig)
            for (int d = 0; d < 2; ++d)
                EXPECT_NEAR(c1.eta[ie][ig][d], c2.eta[ie][ig][d], 1e-14) << "ie=" << ie;
}

// ─── Round-trip: nano_BCs.dat ─────────────────────────────────────────────────

TEST(RoundTrip, BCs) {
    auto bc1 = fce::io::read_bcs(kPrepro + "nano_BCs.dat");
    std::string tmp = kTmp + "BCs.dat";
    fce::io::write_bcs(tmp, bc1);
    auto bc2 = fce::io::read_bcs(tmp);
    EXPECT_EQ(bc1.nloadstep, bc2.nloadstep);
    EXPECT_EQ(bc1.nCodeLoad, bc2.nCodeLoad);
    EXPECT_EQ(bc1.ndofBC,    bc2.ndofBC);
    EXPECT_EQ(bc1.ndofOP,    bc2.ndofOP);
    EXPECT_EQ(bc1.nnodBC,    bc2.nnodBC);
    for (int i = 0; i < bc1.ndofBC; ++i)
        EXPECT_EQ(bc1.mdofBC[i], bc2.mdofBC[i]) << "i=" << i;
    for (int i = 0; i < bc1.nnodBC; ++i)
        EXPECT_EQ(bc1.mnodBC[i][0], bc2.mnodBC[i][0]) << "i=" << i;
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            EXPECT_NEAR(bc1.rotation[row][col], bc2.rotation[row][col], 1e-13);
    for (int i = 0; i < 3; ++i)
        EXPECT_NEAR(bc1.xc[i], bc2.xc[i], 1e-12);
    EXPECT_NEAR(bc1.value, bc2.value, 1e-12);
    EXPECT_EQ(bc1.ncycles,         bc2.ncycles);
    EXPECT_EQ(bc1.nloadstep_comp,  bc2.nloadstep_comp);
    EXPECT_EQ(bc1.nloadstep_rel,   bc2.nloadstep_rel);
    EXPECT_NEAR(bc1.value_comp,    bc2.value_comp, 1e-12);
    EXPECT_NEAR(bc1.value_rel,     bc2.value_rel,  1e-12);
}

// ─── Round-trip: nano_Mesh.dat ────────────────────────────────────────────────

TEST(RoundTrip, Mesh) {
    auto m1 = fce::io::read_mesh(kPrepro + "nano_Mesh.dat", 2);
    std::string tmp = kTmp + "Mesh.dat";
    fce::io::write_mesh(tmp, m1, 2);
    auto m2 = fce::io::read_mesh(tmp, 2);
    EXPECT_EQ(m1.numele, m2.numele);
    EXPECT_EQ(m1.nedge,  m2.nedge);
    for (int ie = 0; ie < m1.numele; ++ie) {
        for (int k = 0; k < 3; ++k)
            EXPECT_EQ(m1.connect[ie].vertices[k], m2.connect[ie].vertices[k]) << "ie=" << ie;
        EXPECT_EQ(m1.connect[ie].num_neigh_elem, m2.connect[ie].num_neigh_elem);
        for (int j = 0; j < 12; ++j) {
            EXPECT_EQ(m1.connect[ie].neigh_elem[j], m2.connect[ie].neigh_elem[j]) << "ie=" << ie << " j=" << j;
            EXPECT_EQ(m1.connect[ie].neigh_vert[j], m2.connect[ie].neigh_vert[j]) << "ie=" << ie << " j=" << j;
        }
        for (int k = 0; k < 3; ++k)
            EXPECT_EQ(m1.connect[ie].code_bc[k], m2.connect[ie].code_bc[k]) << "ie=" << ie;
    }
    for (int i = 0; i < m1.nedge; ++i)
        for (int k = 0; k < 3; ++k)
            EXPECT_EQ(m1.nghost_tab[i][k], m2.nghost_tab[i][k]) << "i=" << i;
}
