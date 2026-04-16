#include "fce/simulator.hpp"
#include "fce/solver.hpp"
#include "fce/mpi_env.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string step_filename(const int step) {
    if (step < 0) {
        throw std::invalid_argument("step must be non-negative");
    }

    std::ostringstream out;
    out << "mesh_config_" << std::setw(4) << std::setfill('0') << step << ".vtu";
    return out.str();
}

}  // namespace

int main(int argc, char** argv) {
    try {
        fce::MpiEnv mpi(argc, argv);

        if (argc < 2) {
            if (mpi.is_root()) {
                std::cerr << "usage: crunch_it <case_dir> [stop_step] [--single-step <step>]\n";
            }
            return 1;
        }

        const std::string case_dir = argv[1];

        bool single_step_mode = false;
        int step = 1;
        std::optional<int> requested_stop_step;
        for (int i = 2; i < argc; ++i) {
            const std::string arg(argv[i]);
            if (arg == "--single-step") {
                if (i + 1 >= argc) {
                    throw std::invalid_argument("--single-step requires an integer step");
                }
                single_step_mode = true;
                step = std::stoi(argv[i + 1]);
                ++i;
                continue;
            }

            if (requested_stop_step.has_value()) {
                throw std::invalid_argument("unexpected extra argument: " + arg);
            }
            requested_stop_step = std::stoi(arg);
        }

        const auto input = fce::load_simulator_input(case_dir);
        if (requested_stop_step.has_value()) {
            if (*requested_stop_step <= 0) {
                throw std::invalid_argument("stop_step must be positive");
            }
            if (*requested_stop_step > input.bcs.nloadstep) {
                throw std::invalid_argument("stop_step exceeds BCs%nloadstep from nano_BCs.dat");
            }
        }

        if (single_step_mode) {
            const auto coords = fce::read_vtu_points(case_dir + "/" + step_filename(step),
                                                     input.mesh.numnods);
            const auto result = fce::assemble_energy_forces(input, coords, mpi);

            if (mpi.is_root()) {
                std::cout << "assembled_energy " << std::setprecision(17) << result.total_energy << "\n";
                std::cout << "inner_fail " << result.inner_fail << "\n";
                std::cout << "force_dofs " << result.force.size() << "\n";
            }
        } else {
            auto state = fce::make_runtime_state(input);
            int iload_start = 1;
            if (input.bcs.nCodeLoad == 30 || input.bcs.nCodeLoad == 31) {
                const std::filesystem::path checkpoint_path =
                    std::filesystem::path(case_dir) / "nano_checkpoint.dat";
                int checkpoint_found = 0;
                int checkpoint_status = 0;
                int checkpoint_nprocs = 0;
                if (mpi.is_root() && std::filesystem::exists(checkpoint_path)) {
                    try {
                        const auto checkpoint = fce::io::read_checkpoint(checkpoint_path.string(),
                                                                         input.mesh.numnods,
                                                                         input.mesh.numele,
                                                                         input.dims.ngauss,
                                                                         input.crease.ncrease == 1);
                        checkpoint_nprocs = checkpoint.nprocs;
                        if (checkpoint.nprocs > 0 && checkpoint.nprocs != mpi.size()) {
                            checkpoint_status = -1;
                        } else {
                            state.coords = checkpoint.config.coords;
                            state.eta = checkpoint.config.eta;
                            if (!checkpoint.K0_ref.empty()) {
                                state.K0_ref = checkpoint.K0_ref;
                            }
                            iload_start = checkpoint.iload + 1;
                            checkpoint_found = 1;
                        }
                    } catch (const std::exception&) {
                        checkpoint_status = -2;
                    }
                }

                std::vector<int> meta{
                    checkpoint_status,
                    checkpoint_found,
                    iload_start,
                    checkpoint_nprocs,
                };
                mpi.bcast_ints(meta, 0);
                checkpoint_status = meta[0];
                checkpoint_found = meta[1];
                iload_start = meta[2];
                checkpoint_nprocs = meta[3];

                if (checkpoint_status < 0) {
                    if (checkpoint_status == -2) {
                        throw std::runtime_error("failed to read checkpoint");
                    }
                    throw std::runtime_error("checkpoint rank count mismatch: file was written with " +
                                             std::to_string(checkpoint_nprocs) +
                                             " ranks, current run uses " +
                                             std::to_string(mpi.size()));
                }

                if (checkpoint_found == 1) {
                    std::vector<double> coord_flat;
                    if (mpi.is_root()) {
                        coord_flat.reserve(state.coords.size() * 3);
                        for (const auto& p : state.coords) {
                            coord_flat.push_back(p[0]);
                            coord_flat.push_back(p[1]);
                            coord_flat.push_back(p[2]);
                        }
                    }
                    mpi.bcast_doubles(coord_flat, 0);
                    if (!mpi.is_root()) {
                        state.coords.resize(static_cast<std::size_t>(input.mesh.numnods));
                        for (int inode = 0; inode < input.mesh.numnods; ++inode) {
                            const std::size_t base = static_cast<std::size_t>(3 * inode);
                            state.coords[static_cast<std::size_t>(inode)] = {
                                coord_flat[base],
                                coord_flat[base + 1],
                                coord_flat[base + 2],
                            };
                        }
                    }

                    std::vector<double> eta_flat;
                    if (mpi.is_root()) {
                        eta_flat.reserve(static_cast<std::size_t>(input.mesh.numele * input.dims.ngauss * 2));
                        for (const auto& elem_eta : state.eta) {
                            for (const auto& eta : elem_eta) {
                                eta_flat.push_back(eta[0]);
                                eta_flat.push_back(eta[1]);
                            }
                        }
                    }
                    mpi.bcast_doubles(eta_flat, 0);
                    if (!mpi.is_root()) {
                        state.eta.assign(static_cast<std::size_t>(input.mesh.numele),
                                         std::vector<fce::Vec2>(static_cast<std::size_t>(input.dims.ngauss)));
                        std::size_t cursor = 0;
                        for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
                            for (int igauss = 0; igauss < input.dims.ngauss; ++igauss) {
                                state.eta[static_cast<std::size_t>(ielem)][static_cast<std::size_t>(igauss)] = {
                                    eta_flat[cursor],
                                    eta_flat[cursor + 1],
                                };
                                cursor += 2;
                            }
                        }
                    }

                    std::vector<double> k0_flat;
                    if (mpi.is_root() && !state.K0_ref.empty()) {
                        k0_flat.reserve(static_cast<std::size_t>(input.mesh.numele * input.dims.ngauss * 3));
                        for (const auto& elem_k0 : state.K0_ref) {
                            for (const auto& kappa : elem_k0) {
                                k0_flat.push_back(kappa[0]);
                                k0_flat.push_back(kappa[1]);
                                k0_flat.push_back(kappa[2]);
                            }
                        }
                    }
                    mpi.bcast_doubles(k0_flat, 0);
                    if (!mpi.is_root() && !k0_flat.empty()) {
                        state.K0_ref.assign(static_cast<std::size_t>(input.mesh.numele),
                                            std::vector<std::array<double, 3>>(
                                                static_cast<std::size_t>(input.dims.ngauss)));
                        std::size_t cursor = 0;
                        for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
                            for (int igauss = 0; igauss < input.dims.ngauss; ++igauss) {
                                state.K0_ref[static_cast<std::size_t>(ielem)][static_cast<std::size_t>(igauss)] = {
                                    k0_flat[cursor],
                                    k0_flat[cursor + 1],
                                    k0_flat[cursor + 2],
                                };
                                cursor += 3;
                            }
                        }
                    }
                }
            }

            const int final_stop_step = requested_stop_step.value_or(input.bcs.nloadstep);
            if (input.general.imperfect &&
                !input.imperfection_trace.empty() &&
                static_cast<int>(input.imperfection_trace.size()) < final_stop_step) {
                throw std::runtime_error("imperfection trace is shorter than the requested stop step");
            }

            const double eps = input.general.crit_global > 0.0 ? input.general.crit_global : 1.0e-8;
            fce::pasapas(input,
                         state,
                         mpi,
                         case_dir,
                         eps,
                         iload_start,
                         final_stop_step);
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
