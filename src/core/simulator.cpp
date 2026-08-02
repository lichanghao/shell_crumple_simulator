#include "fce/simulator.hpp"

#include "fce/ghost_nodes.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <unordered_set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fce {

namespace {

constexpr int kDefaultInnerMaxIter = 1000;

EtaField zero_eta_field(const int numele, const int ngauss) {
    return EtaField(static_cast<std::size_t>(numele),
                    std::vector<Vec2>(static_cast<std::size_t>(ngauss), Vec2{0.0, 0.0}));
}

struct VdwAssembly {
    double total_energy{0.0};
    double reduced_energy{0.0};
    std::vector<double> force{};
};

struct VdwSpatialBins {
    std::array<int, 3> count{1, 1, 1};
    Vec3 min{};
    Vec3 width{1.0, 1.0, 1.0};
    std::vector<std::vector<int>> bins{};
    std::vector<std::array<int, 3>> point_bin{};
};

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

std::vector<Vec3> locate_vdw_gauss_points(const Mesh& mesh,
                                          const VdwData& vdw,
                                          const FlatCoords& coords_with_ghosts) {
    if (vdw.nvdw != 1) {
        return {};
    }
    if (vdw.ngauss_vdw <= 0) {
        throw std::runtime_error("vdW runtime requires positive ngauss_vdw");
    }
    if (static_cast<int>(vdw.shapef.size()) < vdw.ngauss_vdw) {
        throw std::runtime_error("vdW runtime shapef size does not match ngauss_vdw");
    }

    std::vector<Vec3> x(static_cast<std::size_t>(mesh.numele * vdw.ngauss_vdw),
                        Vec3{0.0, 0.0, 0.0});
    for (int ielem = 0; ielem < mesh.numele; ++ielem) {
        const auto& connect = mesh.connect.at(static_cast<std::size_t>(ielem));
        for (int ig = 0; ig < vdw.ngauss_vdw; ++ig) {
            const int idx = ielem * vdw.ngauss_vdw + ig;
            for (int inode = 0; inode < 12; ++inode) {
                const int node_index = connect.neigh_vert[inode];
                const std::size_t base = static_cast<std::size_t>(3 * node_index);
                const double shape = vdw.shapef.at(static_cast<std::size_t>(ig))[inode];
                for (int axis = 0; axis < 3; ++axis) {
                    x.at(static_cast<std::size_t>(idx))[axis] +=
                        shape * coords_with_ghosts.at(base + static_cast<std::size_t>(axis));
                }
            }
        }
    }
    return x;
}

int tube_for_gauss_index(const VdwData& vdw, const int gp_index) {
    for (int tube = 0; tube < static_cast<int>(vdw.tub_partitions.size()); ++tube) {
        const auto [begin, end] = vdw.tub_partitions.at(static_cast<std::size_t>(tube));
        if (gp_index >= begin && gp_index < end) {
            return tube;
        }
    }
    return -1;
}

bool vdw_prelisted_neighbor_excludes(const VdwData& vdw, const int i, const int j) {
    if (i < 0 || i >= static_cast<int>(vdw.near.size())) {
        return false;
    }
    const auto& row = vdw.near.at(static_cast<std::size_t>(i));
    if (row.empty()) {
        return false;
    }
    const int count = std::max(0, std::min(row[0], static_cast<int>(row.size()) - 1));
    for (int k = 1; k <= count; ++k) {
        const int entry = row.at(static_cast<std::size_t>(k));
        if (entry == j || entry - 1 == j) {
            return true;
        }
    }
    return false;
}

int flatten_vdw_bin_index(const VdwSpatialBins& bins, const int ix, const int iy, const int iz) {
    return (ix * bins.count[1] + iy) * bins.count[2] + iz;
}

int locate_vdw_bin_axis(const double x, const double xmin, const double width, const int count) {
    if (count <= 1) {
        return 0;
    }
    int idx = static_cast<int>((x - xmin) / width);
    if (idx < 0) {
        return 0;
    }
    if (idx >= count) {
        return count - 1;
    }
    return idx;
}

VdwSpatialBins build_vdw_spatial_bins(const std::vector<Vec3>& gp, const double cutoff) {
    if (gp.empty()) {
        return {};
    }
    if (cutoff <= 0.0) {
        throw std::runtime_error("vdW runtime requires positive cutoff radius");
    }

    VdwSpatialBins bins;
    bins.min = gp.front();
    Vec3 max = gp.front();
    for (const auto& x : gp) {
        for (int axis = 0; axis < 3; ++axis) {
            bins.min[axis] = std::min(bins.min[axis], x[axis]);
            max[axis] = std::max(max[axis], x[axis]);
        }
    }

    int nbins = 1;
    for (int axis = 0; axis < 3; ++axis) {
        const double range = max[axis] - bins.min[axis];
        bins.count[axis] = std::max(1, static_cast<int>(range / cutoff));
        bins.width[axis] = bins.count[axis] > 0 ? std::max(range / bins.count[axis], cutoff) : cutoff;
        nbins *= bins.count[axis];
    }
    bins.bins.assign(static_cast<std::size_t>(nbins), {});
    bins.point_bin.assign(gp.size(), std::array<int, 3>{0, 0, 0});

    for (int i = 0; i < static_cast<int>(gp.size()); ++i) {
        auto& point_bin = bins.point_bin.at(static_cast<std::size_t>(i));
        for (int axis = 0; axis < 3; ++axis) {
            point_bin[axis] = locate_vdw_bin_axis(
                gp.at(static_cast<std::size_t>(i))[axis],
                bins.min[axis],
                bins.width[axis],
                bins.count[axis]);
        }
        bins.bins.at(static_cast<std::size_t>(
            flatten_vdw_bin_index(bins, point_bin[0], point_bin[1], point_bin[2]))).push_back(i);
    }

    return bins;
}

VdwAssembly assemble_vdw_runtime(const SimulatorInput& input,
                                 const FlatCoords& coords_with_ghosts,
                                 const int element_begin,
                                 const int element_end,
                                 const int element_stride,
                                 const std::unordered_set<int>& ghost_elements) {
    VdwAssembly result;
    result.force.assign(static_cast<std::size_t>(3 * (input.mesh.numnods + input.mesh.nedge)), 0.0);
    if (input.vdw.nvdw != 1) {
        return result;
    }
    if (input.vdw.weight.size() < static_cast<std::size_t>(input.vdw.ngauss_vdw)) {
        throw std::runtime_error("vdW runtime weight size does not match ngauss_vdw");
    }
    if (input.general.mat.s0 == 0.0) {
        throw std::runtime_error("vdW runtime requires nonzero material s0");
    }

    const auto gp = locate_vdw_gauss_points(input.mesh, input.vdw, coords_with_ghosts);
    const int ng_tot = static_cast<int>(gp.size());
    const bool has_rho = static_cast<int>(input.vdw.rho.size()) >= ng_tot;
    const auto bins = build_vdw_spatial_bins(gp, input.vdw.r_cut);

    for (int ielem = element_begin; ielem < element_end; ielem += element_stride) {
        const auto& elem_i = input.mesh.connect.at(static_cast<std::size_t>(ielem));
        double elem_vdw_energy = 0.0;
        for (int ig = 0; ig < input.vdw.ngauss_vdw; ++ig) {
            const int i = ielem * input.vdw.ngauss_vdw + ig;
            const auto& point_bin = bins.point_bin.at(static_cast<std::size_t>(i));
            for (int ix = std::max(0, point_bin[0] - 1);
                 ix <= std::min(bins.count[0] - 1, point_bin[0] + 1);
                 ++ix) {
            for (int iy = std::max(0, point_bin[1] - 1);
                 iy <= std::min(bins.count[1] - 1, point_bin[1] + 1);
                 ++iy) {
            for (int iz = std::max(0, point_bin[2] - 1);
                 iz <= std::min(bins.count[2] - 1, point_bin[2] + 1);
                 ++iz) {
            const auto& candidates = bins.bins.at(
                static_cast<std::size_t>(flatten_vdw_bin_index(bins, ix, iy, iz)));
            for (const int j : candidates) {
                if (j <= i) {
                    continue;
                }
                if (!runtime_vdw_pair_allowed(input.mesh, input.vdw, i, j)) {
                    continue;
                }
                Vec3 vec{
                    gp.at(static_cast<std::size_t>(i))[0] - gp.at(static_cast<std::size_t>(j))[0],
                    gp.at(static_cast<std::size_t>(i))[1] - gp.at(static_cast<std::size_t>(j))[1],
                    gp.at(static_cast<std::size_t>(i))[2] - gp.at(static_cast<std::size_t>(j))[2],
                };
                const double dist =
                    std::sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
                if (dist >= input.vdw.r_cut) {
                    continue;
                }
                if (dist <= std::numeric_limits<double>::epsilon()) {
                    throw std::runtime_error("vdW runtime encountered coincident gauss points");
                }
                const int jelem = j / input.vdw.ngauss_vdw;
                const int jg = j - jelem * input.vdw.ngauss_vdw;
                const auto& elem_j = input.mesh.connect.at(static_cast<std::size_t>(jelem));

                double weight = input.vdw.weight.at(static_cast<std::size_t>(ig)) *
                                input.vdw.weight.at(static_cast<std::size_t>(jg)) *
                                input.ref_config.at(static_cast<std::size_t>(ielem)).J0 *
                                input.ref_config.at(static_cast<std::size_t>(jelem)).J0;
                if (has_rho) {
                    weight *= input.vdw.rho.at(static_cast<std::size_t>(i)) *
                              input.vdw.rho.at(static_cast<std::size_t>(j)) *
                              (input.general.mat.s0 * input.general.mat.s0 / 4.0);
                }

                const auto potential = evaluate_runtime_vdw_cut_potential(input.vdw, dist);
                elem_vdw_energy += potential.energy * weight;
                const double force_scale =
                    potential.derivative / dist * weight /
                    (input.general.mat.s0 * input.general.mat.s0);
                for (int axis = 0; axis < 3; ++axis) {
                    vec[axis] *= force_scale;
                }

                for (int inode = 0; inode < 12; ++inode) {
                    const double shape_i = input.vdw.shapef.at(static_cast<std::size_t>(ig))[inode];
                    const double shape_j = input.vdw.shapef.at(static_cast<std::size_t>(jg))[inode];
                    const std::size_t base_i =
                        static_cast<std::size_t>(3 * elem_i.neigh_vert[inode]);
                    const std::size_t base_j =
                        static_cast<std::size_t>(3 * elem_j.neigh_vert[inode]);
                    for (int axis = 0; axis < 3; ++axis) {
                        result.force.at(base_i + static_cast<std::size_t>(axis)) +=
                            vec[axis] * shape_i;
                        result.force.at(base_j + static_cast<std::size_t>(axis)) -=
                            vec[axis] * shape_j;
                    }
                }
            }
            }
            }
            }
        }
        elem_vdw_energy /= input.general.mat.s0 * input.general.mat.s0;
        result.total_energy += elem_vdw_energy;
        if (ghost_elements.find(ielem) == ghost_elements.end()) {
            result.reduced_energy += elem_vdw_energy;
        }
    }

    return result;
}

std::string format_patch_coords(const NeighborCoords12& xneigh) {
    std::ostringstream out;
    out << std::scientific << std::setprecision(16);
    for (int inode = 0; inode < 12; ++inode) {
        out << " [" << inode << "]"
            << " x=" << xneigh[inode][0]
            << " y=" << xneigh[inode][1]
            << " z=" << xneigh[inode][2];
    }
    return out.str();
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
    if (std::filesystem::exists(base / "nano_crease.dat")) {
        input.crease = io::read_crease((base / "nano_crease.dat").string(),
                                       input.dims.numnods,
                                       input.dims.ngauss);
        if (input.crease.ncrease == 1) {
            input.crease.K0_ref.assign(static_cast<std::size_t>(input.dims.numele),
                                       std::vector<std::array<double, 3>>(
                                           static_cast<std::size_t>(input.dims.ngauss),
                                           std::array<double, 3>{0.0, 0.0, 0.0}));
        }
    }
    if (std::filesystem::exists(base / "nano_vdw.dat")) {
        input.vdw = io::read_vdw((base / "nano_vdw.dat").string(),
                                 input.dims.ng_tot,
                                 input.dims.ngauss_vdw,
                                 input.dims.nneigh);
        if (std::filesystem::exists(base / "nano_tub_loc.dat")) {
            input.vdw.tub_partitions = io::read_tub_loc((base / "nano_tub_loc.dat").string());
        }
    }
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
    return input;
}

RuntimeState make_runtime_state(const SimulatorInput& input) {
    return RuntimeState{
        input.initial_config.coords,
        input.initial_config.eta,
        input.crease.K0_ref,
    };
}

CheckpointResumeResult load_runtime_checkpoint(const SimulatorInput& input,
                                               const std::string& case_dir,
                                               const int current_nprocs,
                                               const RuntimeState& initial_state) {
    CheckpointResumeResult result;
    result.state = initial_state;

    if (input.bcs.nCodeLoad != 30 && input.bcs.nCodeLoad != 31) {
        return result;
    }

    const std::filesystem::path checkpoint_path =
        std::filesystem::path(case_dir) / "nano_checkpoint.dat";
    if (!std::filesystem::exists(checkpoint_path)) {
        return result;
    }

    try {
        const auto checkpoint = io::read_checkpoint(checkpoint_path.string(),
                                                   input.mesh.numnods,
                                                   input.mesh.numele,
                                                   input.dims.ngauss,
                                                   input.crease.ncrease == 1);
        result.checkpoint_nprocs = checkpoint.nprocs;
        if (checkpoint.nprocs > 0 && checkpoint.nprocs != current_nprocs) {
            result.status = CheckpointResumeStatus::rank_count_mismatch;
            return result;
        }

        result.state.coords = checkpoint.config.coords;
        result.state.eta = checkpoint.config.eta;
        if (!checkpoint.K0_ref.empty()) {
            result.state.K0_ref = checkpoint.K0_ref;
        }
        result.iload_start = checkpoint.iload + 1;
        result.status = CheckpointResumeStatus::loaded;
    } catch (const std::exception& ex) {
        result.status = CheckpointResumeStatus::read_failed;
        result.error_detail = ex.what();
    }

    return result;
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

RuntimeVdwPotential evaluate_runtime_vdw_cut_potential(const VdwData& vdw, const double r) {
    if (r >= vdw.r_cut) {
        return {};
    }
    if (r <= std::numeric_limits<double>::epsilon()) {
        throw std::runtime_error("vdW runtime potential requires positive separation");
    }

    const double a1 = vdw.sig / r;
    const double a6 = std::pow(a1, 6);
    const double a12 = a6 * a6;
    const double y06 = std::pow(vdw.y0, 6);
    RuntimeVdwPotential out;
    out.energy = (0.5 * y06 * a12 - a6) * vdw.a / std::pow(vdw.sig, 6);
    out.derivative =
        (a1 * 6.0 / vdw.sig * (-y06 * a12 + a6)) * vdw.a / std::pow(vdw.sig, 6);
    out.energy = out.energy - vdw.Vcut[1] * (r - vdw.r_cut) - vdw.Vcut[0];
    out.derivative -= vdw.Vcut[1];
    return out;
}

bool runtime_vdw_pair_allowed(const Mesh& mesh, const VdwData& vdw, const int gauss_i, const int gauss_j) {
    if (vdw.ngauss_vdw <= 0) {
        throw std::runtime_error("vdW runtime requires positive ngauss_vdw");
    }
    const int ielem = gauss_i / vdw.ngauss_vdw;
    const int jelem = gauss_j / vdw.ngauss_vdw;
    if (ielem < 0 || ielem >= mesh.numele || jelem < 0 || jelem >= mesh.numele) {
        throw std::out_of_range("vdW gauss index maps outside the mesh");
    }
    if (vdw.nneigh > 0) {
        return !vdw_prelisted_neighbor_excludes(vdw, gauss_i, gauss_j);
    }
    if (vdw.nneigh == -2) {
        if (ielem == jelem) {
            return false;
        }
        const auto& elem = mesh.connect.at(static_cast<std::size_t>(ielem));
        for (int k = 0; k < 12; ++k) {
            if (elem.neigh_elem[k] == jelem + 1) {
                return false;
            }
        }
        return true;
    }
    if (vdw.nneigh == -1) {
        const int my_tube = tube_for_gauss_index(vdw, gauss_i);
        const int other_tube = tube_for_gauss_index(vdw, gauss_j);
        if (my_tube < 0 || other_tube < 0) {
            throw std::runtime_error("vdW runtime could not map gauss point to tube partition");
        }
        return other_tube == my_tube + 1;
    }
    return true;
}

AssemblyResult assemble_energy_forces_local(const SimulatorInput& input,
                                            RuntimeState& state,
                                            const int element_begin,
                                            const int element_end,
                                            const int element_stride,
                                            const bool fold_ghost_forces_after_assembly) {
    if (static_cast<int>(state.coords.size()) != input.mesh.numnods) {
        throw std::runtime_error("coordinate field size does not match mesh.numnods");
    }
    if (static_cast<int>(state.eta.size()) != input.mesh.numele) {
        throw std::runtime_error("eta field size does not match mesh.numele");
    }
    if (element_begin < 0 || element_end < element_begin || element_end > input.mesh.numele) {
        throw std::out_of_range("invalid assembly element range");
    }
    if (element_stride <= 0) {
        throw std::out_of_range("invalid assembly element stride");
    }

    FlatCoords coords_with_ghosts = flatten_coords(state.coords);
    coords_with_ghosts.resize(static_cast<std::size_t>(3 * (input.mesh.numnods + input.mesh.nedge)));
    ghost_nodes(input.mesh, coords_with_ghosts);

    AssemblyResult result;
    result.force.assign(static_cast<std::size_t>(3 * (input.mesh.numnods + input.mesh.nedge)), 0.0);
    result.eta_updates = zero_eta_field(input.mesh.numele, input.dims.ngauss);
    const std::unordered_set<int> ghost_elements(input.mesh.elem_ghost.begin(),
                                                 input.mesh.elem_ghost.end());
    std::vector<double> force_accum(result.force.size(), 0.0);
    const char* extended_force_env = std::getenv("FCE_LONG_DOUBLE_FORCE_ACCUM");
    const bool extended_force_accumulation =
        extended_force_env != nullptr && *extended_force_env != '\0' &&
        std::string(extended_force_env) != "0" &&
        std::string(extended_force_env) != "false" &&
        std::string(extended_force_env) != "no" &&
        std::string(extended_force_env) != "off";
    std::vector<long double> force_accum_extended;
    if (extended_force_accumulation) {
        force_accum_extended.assign(force_accum.size(), 0.0L);
    }
    const char* element_force_dump = std::getenv("FCE_TRACE_ELEMENT_FORCE_INITIAL");
    const char* element_force_call_raw = std::getenv("FCE_TRACE_ELEMENT_FORCE_CALL");
    static bool element_force_dumped = false;
    static int element_force_assembly_call = 0;
    std::ofstream element_force_out;
    int requested_element_force_call = 0;
    if (element_force_call_raw != nullptr && *element_force_call_raw != '\0') {
        char* end = nullptr;
        const long parsed = std::strtol(element_force_call_raw, &end, 10);
        if (end != element_force_call_raw && *end == '\0' && parsed >= 0) {
            requested_element_force_call = static_cast<int>(parsed);
        }
    }
    const int this_element_force_call = element_force_assembly_call++;
    const bool dump_element_force = element_force_dump != nullptr &&
                                    *element_force_dump != '\0' &&
                                    (element_force_call_raw == nullptr ||
                                     this_element_force_call == requested_element_force_call) &&
                                    !element_force_dumped;
    if (dump_element_force) {
        std::string element_force_path(element_force_dump);
        if (element_force_call_raw != nullptr) {
            element_force_path += "." + std::to_string(element_begin);
        }
        element_force_out.open(element_force_path, std::ios::out | std::ios::trunc);
        if (!element_force_out) {
            throw std::runtime_error("cannot open element-force trace: " +
                                     element_force_path);
        }
        element_force_out << std::uppercase << std::scientific << std::setprecision(17);
    }
    double total_energy_accum = 0.0;
    double reduced_energy_accum = 0.0;

    for (int ielem = element_begin; ielem < element_end; ielem += element_stride) {
        const auto xneigh = gather_neighbor_patch(input.mesh, coords_with_ghosts, ielem);
        const auto& eta0 = state.eta.at(static_cast<std::size_t>(ielem));
        std::vector<Voigt3> reference_curvature(static_cast<std::size_t>(input.dims.ngauss),
                                                Voigt3{0.0, 0.0, 0.0});
        if (!state.K0_ref.empty()) {
            const auto& elem_k0 = state.K0_ref.at(static_cast<std::size_t>(ielem));
            for (int igauss = 0; igauss < input.dims.ngauss; ++igauss) {
                const auto& kappa = elem_k0.at(static_cast<std::size_t>(igauss));
                reference_curvature[static_cast<std::size_t>(igauss)] =
                    Voigt3{kappa[0], kappa[1], kappa[2]};
            }
        }
        const auto elem = [&]() {
            try {
                return compute_element_energy(
                    xneigh,
                    input.ref_config.at(static_cast<std::size_t>(ielem)).F0,
                    reference_curvature,
                    input.gauss,
                    input.general.mat,
                    input.general.nW_hat,
                    input.general.crit_local,
                    kDefaultInnerMaxIter,
                    eta0);
            } catch (const std::exception& ex) {
                std::ostringstream msg;
                msg << "assemble_energy_forces failed at element " << ielem
                    << " with error: " << ex.what()
                    << " patch:" << format_patch_coords(xneigh);
                throw std::runtime_error(msg.str());
            }
        }();
        state.eta.at(static_cast<std::size_t>(ielem)) = elem.eta;
        result.eta_updates.at(static_cast<std::size_t>(ielem)) = elem.eta;

        if (dump_element_force) {
            element_force_out << std::setw(8) << ielem + 1;
            for (int inode = 0; inode < 12; ++inode) {
                for (int axis = 0; axis < 3; ++axis) {
                    element_force_out << " " << elem.f_elem[inode][axis];
                }
            }
            element_force_out << " " << elem.W_elem << '\n';
        }

        const double J0 = input.ref_config.at(static_cast<std::size_t>(ielem)).J0;
        // Match energy.f90: accumulate W_dens*J0 over all elements and apply
        // the factor 1/2 only after the element loop.  The force path is
        // different in the reference: f_elem is scaled by J0/2 per element.
        total_energy_accum += elem.W_elem * J0;
        if (ghost_elements.find(ielem) == ghost_elements.end()) {
            reduced_energy_accum += elem.W_elem * J0;
        }
        result.inner_fail += elem.inner_fail;

        const auto& connect = input.mesh.connect.at(static_cast<std::size_t>(ielem));
        for (int inode = 0; inode < 12; ++inode) {
            const int node_index = connect.neigh_vert[inode];
            const std::size_t base = static_cast<std::size_t>(3 * node_index);
            for (int axis = 0; axis < 3; ++axis) {
                const std::size_t force_index = base + static_cast<std::size_t>(axis);
                if (extended_force_accumulation) {
                    force_accum_extended[force_index] +=
                        static_cast<long double>(elem.f_elem[inode][axis]) *
                        static_cast<long double>(J0) / 2.0L;
                } else {
                    // energy.f90 scales f_elem before adding it to f_loc.
                    // Keep the intermediate value explicit so the compiler
                    // cannot fold the scale into the accumulation operation.
                    const double scaled_force =
                        (elem.f_elem[inode][axis] * J0) / 2.0;
                    force_accum[force_index] += scaled_force;
                }
            }
        }
    }
    if (dump_element_force) {
        element_force_dumped = true;
    }

    total_energy_accum /= 2.0;
    reduced_energy_accum /= 2.0;

    const auto vdw = assemble_vdw_runtime(input,
                                          coords_with_ghosts,
                                          element_begin,
                                          element_end,
                                          element_stride,
                                          ghost_elements);
    total_energy_accum += vdw.total_energy;
    for (std::size_t i = 0; i < force_accum.size(); ++i) {
        if (extended_force_accumulation) {
            force_accum_extended[i] += static_cast<long double>(vdw.force[i]);
            force_accum[i] = static_cast<double>(force_accum_extended[i]);
        } else {
            force_accum[i] += vdw.force[i];
        }
    }

    result.total_energy = total_energy_accum;
    result.reduced_energy = reduced_energy_accum;
    result.vdw_reduced_energy = vdw.reduced_energy;
    for (std::size_t i = 0; i < result.force.size(); ++i) {
        result.force[i] = force_accum[i];
    }

    if (fold_ghost_forces_after_assembly) {
        fold_ghost_forces(result.force, input.mesh);
    }

    return result;
}

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      RuntimeState& state,
                                      const int element_begin,
                                      const int element_end,
                                      const int element_stride) {
    return assemble_energy_forces_local(input,
                                         state,
                                         element_begin,
                                         element_end,
                                         element_stride,
                                         /*fold_ghost_forces_after_assembly=*/true);
}

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      const Coords& coords,
                                      const int element_begin,
                                      const int element_end,
                                      const int element_stride) {
    RuntimeState state = make_runtime_state(input);
    state.coords = coords;
    return assemble_energy_forces(input, state, element_begin, element_end, element_stride);
}

AssemblyResult assemble_energy_forces(const SimulatorInput& input,
                                      RuntimeState& state,
                                      const MpiEnv& mpi) {
    // Match energy.f90: reduce raw f_loc first, then fold ghost forces on
    // rank 0, and finally broadcast the real-node force vector.
    auto local = assemble_energy_forces_local(input,
                                              state,
                                              mpi.rank(),
                                              input.mesh.numele,
                                              mpi.size(),
                                              /*fold_ghost_forces_after_assembly=*/false);

    local.total_energy = mpi.allreduce_sum(local.total_energy);
    local.reduced_energy = mpi.allreduce_sum(local.reduced_energy);
    local.vdw_reduced_energy = mpi.allreduce_sum(local.vdw_reduced_energy);
    mpi.reduce_sum_to_root(local.force);
    if (mpi.is_root()) {
        fold_ghost_forces(local.force, input.mesh);
    }
    mpi.bcast_doubles(local.force, 0);

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
