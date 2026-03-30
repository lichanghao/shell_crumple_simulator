// Unit tests for MPI environment wrapper (serial mode).
// These tests run single-rank and verify the partition helpers.

#include "fce/mpi_env.hpp"
#include <gtest/gtest.h>

TEST(MpiEnv, SerialRankSize) {
    // MpiEnv was initialized in main; just check values.
    // Cannot re-initialize here; use the global partition function instead.
    auto p = fce::element_partition(100, 1, 0);
    EXPECT_EQ(p.first,  0);
    EXPECT_EQ(p.second, 100);
}

TEST(MpiEnv, PartitionCoverage) {
    for (int size = 1; size <= 8; ++size) {
        int total = 0;
        for (int r = 0; r < size; ++r) {
            auto p = fce::element_partition(3200, size, r);
            EXPECT_GE(p.second, p.first);
            total += p.second - p.first;
        }
        EXPECT_EQ(total, 3200) << "size=" << size;
    }
}

TEST(MpiEnv, PartitionForRank) {
    std::vector<std::pair<int,int>> parts = {{0, 1600}, {1600, 3200}};
    auto p0 = fce::partition_for_rank(parts, 0);
    auto p1 = fce::partition_for_rank(parts, 1);
    EXPECT_EQ(p0.first, 0);
    EXPECT_EQ(p0.second, 1600);
    EXPECT_EQ(p1.first, 1600);
    EXPECT_EQ(p1.second, 3200);
}
