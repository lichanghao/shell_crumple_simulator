#include "fce/load_controller.hpp"

#include <gtest/gtest.h>

namespace {

fce::BCData compression_bcs() {
    fce::BCData bcs;
    bcs.nloadstep = 50;
    bcs.nCodeLoad = 3;
    bcs.nnodBC = 2;
    bcs.ndofBC = 6;
    bcs.ndofOP = 0;
    bcs.mdofBC = {0, 1, 2, 3, 4, 5};
    bcs.mnodBC = {
        std::array<int, 2>{0, 0},  // side 1
        std::array<int, 2>{1, 1},  // side 2
    };
    bcs.value = 1.0;
    return bcs;
}

fce::BCData cyclic_corner_bcs() {
    fce::BCData bcs;
    bcs.nloadstep = 200;
    bcs.nCodeLoad = 31;
    bcs.nnodBC = 4;
    bcs.ndofBC = 12;
    bcs.ndofOP = 0;
    bcs.mdofBC = {
        0, 1, 2,
        3, 4, 5,
        6, 7, 8,
        9, 10, 11,
    };
    bcs.mnodBC = {
        std::array<int, 2>{0, 0},  // Fortran side tag 1: reaction1 bucket
        std::array<int, 2>{1, 1},  // Fortran side tag 2: reaction2 bucket
        std::array<int, 2>{2, 2},  // Fortran side tag 3: reaction2 bucket
        std::array<int, 2>{3, 3},  // Fortran side tag 4: reaction2 bucket
    };
    bcs.nloadstep_comp = 20;
    bcs.nloadstep_rel = 20;
    bcs.value = 4.0;
    bcs.value_comp = 4.0;
    bcs.value_rel = 4.0;
    return bcs;
}

}  // namespace

TEST(LoadController, ApplyIncrementUsesStoredNloadstepDenominator) {
    const auto bcs = compression_bcs();
    fce::LoadController load_ctrl(bcs);
    fce::Coords coords = {
        fce::Vec3{0.0, 0.0, 0.0},
        fce::Vec3{10.0, 0.0, 0.0},
    };

    load_ctrl.init(coords);
    load_ctrl.apply_increment(3, coords);

    EXPECT_DOUBLE_EQ(coords[0][0], 0.0);
    EXPECT_DOUBLE_EQ(coords[1][0], 10.0 - (bcs.value / static_cast<double>(bcs.nloadstep)));
}

TEST(LoadController, ComputeReactionMatchesGetReacNCodeLoad3Semantics) {
    const auto bcs = compression_bcs();
    fce::LoadController load_ctrl(bcs);
    const std::vector<double> forces = {
        1.0, 2.0, 3.0,   // node 0
        4.0, 5.0, 6.0,   // node 1
    };

    double reaction1 = 0.0;
    double reaction2 = 0.0;
    load_ctrl.compute_reaction(forces, reaction1, reaction2);

    // Fortran get_reac.f90, nCodeLoad=3:
    // reaction1 += forces(mdofBC(3*i)) for side 1
    // reaction2 += forces(mdofBC(3*i)) for side 2
    EXPECT_DOUBLE_EQ(reaction1, 3.0);
    EXPECT_DOUBLE_EQ(reaction2, 6.0);
}

TEST(LoadController, ComputeReactionMatchesCyclicPasapasGetReacBucketing) {
    const auto bcs = cyclic_corner_bcs();
    fce::LoadController load_ctrl(bcs);
    const std::vector<double> forces = {
        1.0, 2.0, 3.0,      // node 0 -> reaction1 uses z
        4.0, 5.0, 6.0,      // node 1 -> reaction2 uses z
        7.0, 8.0, 9.0,      // node 2 -> reaction2 uses z
        10.0, 11.0, 12.0,   // node 3 -> reaction2 uses z
    };

    double reaction1 = 0.0;
    double reaction2 = 0.0;
    load_ctrl.compute_reaction(forces, reaction1, reaction2);

    // pasapas.f90 uses get_reac(..., 3, ...) for nCodeLoad=30/31.
    // That keeps the nCodeLoad=3 bucketing rule even though the cyclic
    // boundary update moves x/y constrained DOFs.
    EXPECT_DOUBLE_EQ(reaction1, 3.0);
    EXPECT_DOUBLE_EQ(reaction2, 6.0 + 9.0 + 12.0);
}
