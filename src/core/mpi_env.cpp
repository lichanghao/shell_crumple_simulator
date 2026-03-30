#include "fce/mpi_env.hpp"
#include <stdexcept>

namespace fce {

MpiEnv::MpiEnv(int argc, char** argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &size_);
}

MpiEnv::~MpiEnv() {
    MPI_Finalize();
}

void MpiEnv::bcast_doubles(std::vector<double>& v, int root) const {
    int n = static_cast<int>(v.size());
    MPI_Bcast(&n, 1, MPI_INT, root, MPI_COMM_WORLD);
    if (static_cast<int>(v.size()) != n) v.resize(n);
    MPI_Bcast(v.data(), n, MPI_DOUBLE, root, MPI_COMM_WORLD);
}

void MpiEnv::bcast_ints(std::vector<int>& v, int root) const {
    int n = static_cast<int>(v.size());
    MPI_Bcast(&n, 1, MPI_INT, root, MPI_COMM_WORLD);
    if (static_cast<int>(v.size()) != n) v.resize(n);
    MPI_Bcast(v.data(), n, MPI_INT, root, MPI_COMM_WORLD);
}

double MpiEnv::allreduce_sum(double local_val) const {
    double global_val = 0.0;
    MPI_Allreduce(&local_val, &global_val, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    return global_val;
}

void MpiEnv::allreduce_sum(std::vector<double>& v) const {
    std::vector<double> tmp(v.size());
    MPI_Allreduce(v.data(), tmp.data(), static_cast<int>(v.size()),
                  MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    v = std::move(tmp);
}

std::pair<int,int> element_partition(int n, int size, int rank) {
    int base  = n / size;
    int extra = n % size;
    int istart = rank * base + std::min(rank, extra);
    int iend   = istart + base + (rank < extra ? 1 : 0);
    return {istart, iend}; // 0-based half-open [istart, iend)
}

std::pair<int,int> partition_for_rank(
    const std::vector<std::pair<int,int>>& parts, int rank) {
    if (rank < 0 || rank >= static_cast<int>(parts.size()))
        throw std::out_of_range("partition_for_rank: rank out of range");
    return parts[rank];
}

} // namespace fce
