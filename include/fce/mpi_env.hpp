#pragma once
// MPI init/finalize wrapper and rank utilities.
// Encapsulates all MPI state so the rest of the code can query rank/size
// without including mpi.h everywhere.

#include <mpi.h>
#include <vector>
#include <utility>

namespace fce {

// Initialize once at program startup; finalize at shutdown.
// Usage:
//   fce::MpiEnv mpi(argc, argv);
//   if (mpi.rank() == 0) { /* I/O */ }
class MpiEnv {
public:
    MpiEnv(int argc, char** argv);
    ~MpiEnv();

    int rank()  const { return rank_; }
    int size()  const { return size_; }
    bool is_root() const { return rank_ == 0; }

    // Broadcast a vector of doubles from rank 0 to all.
    void bcast_doubles(std::vector<double>& v, int root = 0) const;
    void bcast_ints   (std::vector<int>&    v, int root = 0) const;

    // Global allreduce sum.
    double allreduce_sum(double local_val) const;
    void   allreduce_sum(std::vector<double>& v) const;
    void   reduce_sum_to_root(std::vector<double>& v, int root = 0) const;

    MPI_Comm comm() const { return MPI_COMM_WORLD; }

private:
    int rank_{0};
    int size_{1};
};

// Compute element partition for rank `r` of `size` total ranks over `n` elements.
// Returns [istart, iend) as 0-based half-open interval.
std::pair<int,int> element_partition(int n, int size, int rank);

// Read nano_tub_loc.dat partition and return the slice for this rank.
std::pair<int,int> partition_for_rank(
    const std::vector<std::pair<int,int>>& parts, int rank);

} // namespace fce
