#include "fce/ghost_nodes.hpp"
#include "fce/mesh_generator.hpp"

#include <gtest/gtest.h>

#include <cmath>

TEST(GhostNodes, AppliesParallelogramExtrapolationFromGhostTable)
{
    fce::Mesh mesh;
    mesh.numnods = 4;
    mesh.nedge = 2;
    mesh.nghost_tab = {
        {0, 1, 2},
        {3, 2, 1},
    };

    fce::FlatCoords coords = {
        0.0, 0.0, 0.0,
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        1.0, 1.0, 0.0,
    };

    fce::ghost_nodes(mesh, coords);

    ASSERT_EQ(coords.size(), 18U);
    EXPECT_DOUBLE_EQ(coords[12], 1.0);
    EXPECT_DOUBLE_EQ(coords[13], -1.0);
    EXPECT_DOUBLE_EQ(coords[14], 0.0);

    EXPECT_DOUBLE_EQ(coords[15], 0.0);
    EXPECT_DOUBLE_EQ(coords[16], 2.0);
    EXPECT_DOUBLE_EQ(coords[17], 0.0);
}

TEST(GhostNodes, WrongAnchorChoiceDoesNotMatchExpectedGhostPosition)
{
    fce::Mesh mesh;
    mesh.numnods = 4;
    mesh.nedge = 1;
    mesh.nghost_tab = {
        {0, 2, 1},
    };

    fce::FlatCoords coords = {
        0.0, 0.0, 0.0,
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        1.0, 1.0, 0.0,
    };

    fce::ghost_nodes(mesh, coords);

    ASSERT_EQ(coords.size(), 15U);
    EXPECT_GT(std::abs(coords[12] - 1.0), 1e-12);
    EXPECT_GT(std::abs(coords[13] + 1.0), 1e-12);
    EXPECT_DOUBLE_EQ(coords[14], 0.0);
}
