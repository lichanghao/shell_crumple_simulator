// Simulator entry point (stub — implementation in Milestone 4)
#include "fce/mpi_env.hpp"
#include <iostream>

int main(int argc, char** argv) {
    fce::MpiEnv mpi(argc, argv);
    if (mpi.is_root()) {
        std::cout << "FCE Simulator (C++17) — stub\n";
        std::cout << "MPI ranks: " << mpi.size() << "\n";
    }
    return 0;
}
