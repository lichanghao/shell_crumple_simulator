#include "fce/mesh_generator.hpp"
#include "fce/reference_config.hpp"

#include <gtest/gtest.h>

TEST(ReferenceConfig, ComputesJacobianAndInverseEdgeMapPerTriangle)
{
    fce::Mesh mesh;
    fce::FlatCoords coords;
    fce::mesh_gen_square(1, 1, 2.0, 3.0, coords, mesh);

    const auto ref = fce::compute_ref_config(mesh, coords);

    ASSERT_EQ(ref.size(), 2U);

    EXPECT_NEAR(ref[0].J0, 6.0, 1e-12);
    EXPECT_NEAR(ref[0].F0[0][0], 0.5, 1e-12);
    EXPECT_NEAR(ref[0].F0[0][1], 0.0, 1e-12);
    EXPECT_NEAR(ref[0].F0[1][0], 0.0, 1e-12);
    EXPECT_NEAR(ref[0].F0[1][1], 1.0 / 3.0, 1e-12);

    EXPECT_NEAR(ref[1].J0, 6.0, 1e-12);
    EXPECT_NEAR(ref[1].F0[0][0], -0.5, 1e-12);
    EXPECT_NEAR(ref[1].F0[0][1], 0.0, 1e-12);
    EXPECT_NEAR(ref[1].F0[1][0], 0.0, 1e-12);
    EXPECT_NEAR(ref[1].F0[1][1], -1.0 / 3.0, 1e-12);
}
