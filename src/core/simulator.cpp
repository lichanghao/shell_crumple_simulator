#include "fce/simulator.hpp"

#include "fce/ghost_nodes.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fce {

namespace {

constexpr int kDefaultInnerMaxIter = 100;

EtaField zero_eta_field(const int numele, const int ngauss) {
    return EtaField(static_cast<std::size_t>(numele),
                    std::vector<Vec2>(static_cast<std::size_t>(ngauss), Vec2{0.0, 0.0}));
}

FlatCoords flatten_coords(const Coords& coords) {
    FlatCoords flat;
    flat.reserve(coords.size() * 3);
    for (const auto& xyz : coords) {
        flat.push_back(xyz[0]);
        flat.push_back(xyz[1]);
        flat.push_back(xyz[2]);
    }
    return flat;
}

NeighborCoords12 gather_neighbor_patch(const Mesh& mesh,
                                       const FlatCoords& coords_with_ghosts,
                                       const int element_index) {
    NeighborCoords12 xneigh{};
    const auto& element = mesh.connect.at(static_cast<std::size_t>(element_index));
    const int total_nodes = mesh.numnods + mesh.nedge;

    for (int inode = 0; inode < 12; ++inode) {
        const int node_index = element.neigh_vert[inode];
        if (node_index < 0 || node_index >= total_nodes) {
            throw std::runtime_error("neighbor patch references an invalid node index");
        }
        const std::size_t base = static_cast<std::size_t>(3 * node_index);
        xneigh[inode] = Vec3{
            coords_with_ghosts.at(base),
            coords_with_ghosts.at(base + 1),
            coords_with_ghosts.at(base + 2),
        };
    }

    return xneigh;
}

std::vector<double> flatten_eta(const EtaField& eta, const int numele, const int ngauss) {
    std::vector<double> flat(static_cast<std::size_t>(2 * numele * ngauss), 0.0);
    for (int ielem = 0; ielem < numele; ++ielem) {
        for (int igauss = 0; igauss < ngauss; ++igauss) {
            const std::size_t base = static_cast<std::size_t>(2 * (ielem * ngauss + igauss));
            flat[base] = eta.at(static_cast<std::size_t>(ielem)).at(static_cast<std::size_t>(igauss))[0];
            flat[base + 1] = eta.at(static_cast<std::size_t>(ielem)).at(static_cast<std::size_t>(igauss))[1];
        }
    }
    return flat;
}

EtaField unflatten_eta(const std::vector<double>& flat, const int numele, const int ngauss) {
    EtaField eta = zero_eta_field(numele, ngauss);
    for (int ielem = 0; ielem < numele; ++ielem) {
        for (int igauss = 0; igauss < ngauss; ++igauss) {
            const std::size_t base = static_cast<std::size_t>(2 * (ielem * ngauss + igauss));
            eta.at(static_cast<std::size_t>(ielem)).at(static_cast<std::size_t>(igauss))[0] = flat.at(base);
            eta.at(static_cast<std::size_t>(ielem)).at(static_cast<std::size_t>(igauss))[1] = flat.at(base + 1);
        }
    }
    return eta;
}

std::vector<double> parse_ascii_numbers(const std::string& payload) {
    std::istringstream in(payload);
    std::vector<double> values;
    std::string token;
    while (in >> token) {
        values.push_back(std::stod(token));
    }
    return values;
}

std::vector<double> read_imperfection_trace(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return {};
    }

    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open imperfection trace file: " + path.string());
    }

    std::vector<double> values;
    std::string token;
    while (in >> token) {
        values.push_back(io::parse_fortran_double(token));
    }
    return values;
}

void fold_ghost_forces(std::vector<double>& force, const Mesh& mesh) {
    for (int ighost = 0; ighost < mesh.nedge; ++ighost) {
        const int ghost = mesh.numnods + ighost;
        const auto& row = mesh.nghost_tab.at(static_cast<std::size_t>(ighost));
        const int i1 = row.at(0);
        const int i2 = row.at(1);
        const int i3 = row.at(2);

        for (int axis = 0; axis < 3; ++axis) {
            const double ghost_force = force.at(static_cast<std::size_t>(3 * ghost + axis));
            force.at(static_cast<std::size_t>(3 * i1 + axis)) += ghost_force;
            force.at(static_cast<std::size_t>(3 * i2 + axis)) += ghost_force;
            force.at(static_cast<std::size_t>(3 * i3 + axis)) -= ghost_force;
        }
    }
}

}  // namespace

SimulatorInput load_simulator_input(const std::string& case_dir) {
    const std::filesystem::path base(case_dir);

    SimulatorInput input;
    input.dims = io::read_dims((base / "nano_dims.dat").string());
    input.general = io::read_general((base / "nano_general.dat").string());
    input.mesh = io::read_mesh((base / "nano_Mesh.dat").string(), input.dims.ngauss);
    input.ref_config = io::read_zero((base / "nano_zero.dat").string(), input.dims.numele);
    input.initial_config = io::read_config((base / "nano_config.dat").string(),
                                           input.dims.numnods,
                                           input.dims.numele,
                                           input.dims.ngauss);
    input.imperfection_trace = read_imperfection_trace(base / "imperfection_trace.dat");
    input.bcs = io::read_bcs((base / "nano_BCs.dat").string());
    input.gauss = setup_gauss(input.dims.ngauss);

    if (static_cast<int>(input.ref_config.size()) != input.mesh.numele) {
        throw std::runtime_error("reference configuration size does not match mesh.numele");
    }
    if (static_cast<int>(input.initial_config.coords.size()) != input.mesh.numnods) {
        throw std::runtime_error("initial config point count does not match mesh.numnods");
    }
    if (static_cast<int>(input.initial_config.eta.size()) != input.mesh.numele) {
        throw std::runtime_error("initial eta field size does not match mesh.numele");
    }
    if (input.general.imperfect &&
        !input.imperfection_trace.empty() &&
        static_cast<int>(input.imperfection_trace.size()) < input.bcs.nloadstep) {
        throw std::runtime_error("imperfection trace is shorter than BCs%nloadstep");
    }

    return input;
}

RuntimeState make_runtime_state(const SimulatorInput& input) {
    return RuntimeState{
        input.initial_config.coords,
        input.initial_config.eta,
    };
}

Coords read_vtu_points(const std::string& path, const int expected_points) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open VTU file: " + path);
    }

    const std::string xml((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const std::size_t points_pos = xml.find("<Points>");
    if (points_pos == std::string::npos) {
        throw std::runtime_error("VTU file is missing <Points>: " + path);
    }

    const std::size_t data_array_pos = xml.find("<DataArray", points_pos);
    const std::size_t data_begin = xml.find('>', data_array_pos);
    const std::size_t data_end = xml.find("</DataArray>", data_begin);
    if (data_array_pos == std::string::npos || data_begin == std::string::npos ||
        data_end == std::string::npos) {
        throw std::runtime_error("VTU file has an invalid <Points><DataArray> payload: " + path);
    }

    const auto values = parse_ascii_numbers(xml.substr(data_begin + 1, data_end - data_begin - 1));
    if (static_cast<int>(values.size()) != expected_points * 3) {
        throw std::runtime_error("VTU point payload size does not match the expected point count");
    }

    Coords coords(static_cast<std::size_t>(expected_points));
    for (int i = 0; i < expected_points; ++i) {
        const std::size_t base = static_cast<std::size_t>(3 * i);
        coords[static_cast<std::size_t>(i)] = Vec3{
            values[base],
            values[base + 1],
            values[base + 2],
        };
    }
    return coords;
}

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      RuntimeState& state,
                                      const int element_begin,
                                      const int element_end) {
    if (static_cast<int>(state.coords.size()) != input.mesh.numnods) {
        throw std::runtime_error("coordinate field size does not match mesh.numnods");
    }
    if (static_cast<int>(state.eta.size()) != input.mesh.numele) {
        throw std::runtime_error("eta field size does not match mesh.numele");
    }
    if (element_begin < 0 || element_end < element_begin || element_end > input.mesh.numele) {
        throw std::out_of_range("invalid assembly element range");
    }

    FlatCoords coords_with_ghosts = flatten_coords(state.coords);
    coords_with_ghosts.resize(static_cast<std::size_t>(3 * (input.mesh.numnods + input.mesh.nedge)));
    ghost_nodes(input.mesh, coords_with_ghosts);

    AssemblyResult result;
    result.force.assign(static_cast<std::size_t>(3 * (input.mesh.numnods + input.mesh.nedge)), 0.0);
    result.eta_updates = zero_eta_field(input.mesh.numele, input.dims.ngauss);

    for (int ielem = element_begin; ielem < element_end; ++ielem) {
        const auto xneigh = gather_neighbor_patch(input.mesh, coords_with_ghosts, ielem);
        const auto& eta0 = state.eta.at(static_cast<std::size_t>(ielem));
        const auto elem = compute_element_energy(
            xneigh,
            input.ref_config.at(static_cast<std::size_t>(ielem)).F0,
            Voigt3{0.0, 0.0, 0.0},
            input.gauss,
            input.general.mat,
            input.general.nW_hat,
            input.general.crit_local,
            kDefaultInnerMaxIter,
            eta0);
        state.eta.at(static_cast<std::size_t>(ielem)) = elem.eta;
        result.eta_updates.at(static_cast<std::size_t>(ielem)) = elem.eta;

        const double scale = input.ref_config.at(static_cast<std::size_t>(ielem)).J0 / 2.0;
        result.total_energy += elem.W_elem * scale;
        result.inner_fail += elem.inner_fail;

        const auto& connect = input.mesh.connect.at(static_cast<std::size_t>(ielem));
        for (int inode = 0; inode < 12; ++inode) {
            const int node_index = connect.neigh_vert[inode];
            const std::size_t base = static_cast<std::size_t>(3 * node_index);
            for (int axis = 0; axis < 3; ++axis) {
                result.force[base + static_cast<std::size_t>(axis)] +=
                    elem.f_elem[inode][axis] * scale;
            }
        }
    }

    fold_ghost_forces(result.force, input.mesh);

    return result;
}

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      const Coords& coords,
                                      const int element_begin,
                                      const int element_end) {
    RuntimeState state = make_runtime_state(input);
    state.coords = coords;
    return assemble_energy_forces(input, state, element_begin, element_end);
}

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      RuntimeState& state,
                                      const MpiEnv& mpi) {
    const auto owned = element_partition(input.mesh.numele, mpi.size(), mpi.rank());
    auto local = assemble_energy_forces(input, state, owned.first, owned.second);

    local.total_energy = mpi.allreduce_sum(local.total_energy);
    mpi.allreduce_sum(local.force);

    std::vector<double> fail_count{static_cast<double>(local.inner_fail)};
    mpi.allreduce_sum(fail_count);
    local.inner_fail = static_cast<int>(std::lround(fail_count.front()));

    auto eta_updates = flatten_eta(local.eta_updates, input.mesh.numele, input.dims.ngauss);
    mpi.allreduce_sum(eta_updates);
    state.eta = unflatten_eta(eta_updates, input.mesh.numele, input.dims.ngauss);

    return local;
}

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      const Coords& coords,
                                      const MpiEnv& mpi) {
    RuntimeState state = make_runtime_state(input);
    state.coords = coords;
    return assemble_energy_forces(input, state, mpi);
}

}  // namespace fce
