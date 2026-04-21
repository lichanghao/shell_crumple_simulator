#include "fce/runtime_output.hpp"
#include "fce/element_state.hpp"
#include "fce/ghost_nodes.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fce {

namespace {

double snapshot_time(const BCData& bcs, const int step) {
    if (step <= 0 || bcs.nloadstep <= 0) {
        return 0.0;
    }
    return bcs.value * static_cast<double>(step) / static_cast<double>(bcs.nloadstep);
}

Vec2 average_eta(const EtaField& eta, const int ielem, const int ngauss) {
    Vec2 avg{0.0, 0.0};
    for (int igauss = 0; igauss < ngauss; ++igauss) {
        const auto& eta_gp =
            eta.at(static_cast<std::size_t>(ielem)).at(static_cast<std::size_t>(igauss));
        avg[0] += eta_gp[0];
        avg[1] += eta_gp[1];
    }
    avg[0] /= static_cast<double>(ngauss);
    avg[1] /= static_cast<double>(ngauss);
    return avg;
}

NeighborCoords12 gather_neighbor_patch(const Mesh& mesh,
                                       const FlatCoords& coords_with_ghosts,
                                       const int element_index) {
    NeighborCoords12 xneigh{};
    const auto& element = mesh.connect.at(static_cast<std::size_t>(element_index));
    for (int inode = 0; inode < 12; ++inode) {
        const int node_index = element.neigh_vert[inode];
        const std::size_t base = static_cast<std::size_t>(3 * node_index);
        xneigh[inode] = Vec3{
            coords_with_ghosts.at(base),
            coords_with_ghosts.at(base + 1),
            coords_with_ghosts.at(base + 2),
        };
    }
    return xneigh;
}

std::pair<ShapeGradient12, ShapeCurvature12> gauss_geometry_data(const GaussData& gauss,
                                                                 const int igauss) {
    ShapeGradient12 dn{};
    ShapeCurvature12 ddn{};
    const auto& sf = gauss.shapef.at(static_cast<std::size_t>(igauss));
    for (int inode = 0; inode < 12; ++inode) {
        dn[inode] = Vec2{sf[inode][1], sf[inode][2]};
        ddn[inode] = Voigt3{sf[inode][3], sf[inode][4], sf[inode][5]};
    }
    return {dn, ddn};
}

void validate_runtime_output_state(const SimulatorInput& input,
                                   const RuntimeState& state,
                                   const int step) {
    if (step < 0) {
        throw std::runtime_error("snapshot step must be non-negative");
    }
    if (input.mesh.numnods <= 0 || input.mesh.numele <= 0) {
        throw std::runtime_error("runtime output requires a non-empty mesh");
    }
    if (input.dims.ngauss <= 0) {
        throw std::runtime_error("runtime output requires a positive gauss count");
    }
    if (static_cast<int>(state.coords.size()) != input.mesh.numnods) {
        throw std::runtime_error("runtime output coordinate count does not match mesh.numnods");
    }
    if (static_cast<int>(state.eta.size()) != input.mesh.numele) {
        throw std::runtime_error("runtime output eta count does not match mesh.numele");
    }
    if (static_cast<int>(input.mesh.connect.size()) != input.mesh.numele) {
        throw std::runtime_error("runtime output connectivity count does not match mesh.numele");
    }
    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        if (static_cast<int>(state.eta.at(static_cast<std::size_t>(ielem)).size()) != input.dims.ngauss) {
            throw std::runtime_error("runtime output eta gauss count does not match dims.ngauss");
        }
        const auto& vertices = input.mesh.connect.at(static_cast<std::size_t>(ielem)).vertices;
        for (const int inode : vertices) {
            if (inode < 0 || inode >= input.mesh.numnods) {
                throw std::runtime_error("runtime output connectivity references an invalid node");
            }
        }
    }
}

std::vector<double> compute_atomic_density(const SimulatorInput& input) {
    std::vector<double> rho_nodal(static_cast<std::size_t>(input.mesh.numnods), 0.0);
    std::vector<double> weight_nodal(static_cast<std::size_t>(input.mesh.numnods), 0.0);
    if (input.vdw.nvdw != 1 || input.vdw.rho.empty() || input.vdw.shapef.empty()) {
        return rho_nodal;
    }

    if (input.vdw.ngauss_vdw <= 0) {
        throw std::runtime_error("runtime output vdW gauss count must be positive");
    }
    if (static_cast<int>(input.vdw.shapef.size()) < input.vdw.ngauss_vdw) {
        throw std::runtime_error("runtime output vdW shapef size does not match ngauss_vdw");
    }
    const int expected_rho_size = input.mesh.numele * input.vdw.ngauss_vdw;
    if (static_cast<int>(input.vdw.rho.size()) < expected_rho_size) {
        throw std::runtime_error("runtime output vdW rho payload is shorter than expected");
    }

    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        const auto& connect = input.mesh.connect.at(static_cast<std::size_t>(ielem));
        for (int ig = 0; ig < input.vdw.ngauss_vdw; ++ig) {
            const double rho_gp =
                input.vdw.rho.at(static_cast<std::size_t>(ielem * input.vdw.ngauss_vdw + ig));
            const auto& shape = input.vdw.shapef.at(static_cast<std::size_t>(ig));
            for (int inode = 0; inode < 12; ++inode) {
                const int node_index = connect.neigh_vert[inode];
                if (node_index < 0 || node_index >= input.mesh.numnods) {
                    continue;
                }
                rho_nodal.at(static_cast<std::size_t>(node_index)) += rho_gp * shape[inode];
                weight_nodal.at(static_cast<std::size_t>(node_index)) += shape[inode];
            }
        }
    }

    for (int inode = 0; inode < input.mesh.numnods; ++inode) {
        if (weight_nodal[static_cast<std::size_t>(inode)] > 1.0e-14) {
            rho_nodal[static_cast<std::size_t>(inode)] /=
                weight_nodal[static_cast<std::size_t>(inode)];
        }
    }
    return rho_nodal;
}

std::vector<double> compute_w_density(const SimulatorInput& input) {
    std::vector<double> w_density(static_cast<std::size_t>(input.mesh.numele), 0.0);
    if (input.vdw.nvdw != 1 || input.vdw.rho.empty()) {
        return w_density;
    }

    if (input.vdw.ngauss_vdw <= 0) {
        throw std::runtime_error("runtime output vdW gauss count must be positive");
    }
    const int expected_rho_size = input.mesh.numele * input.vdw.ngauss_vdw;
    if (static_cast<int>(input.vdw.rho.size()) < expected_rho_size) {
        throw std::runtime_error("runtime output vdW rho payload is shorter than expected");
    }

    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        double avg = 0.0;
        for (int ig = 0; ig < input.vdw.ngauss_vdw; ++ig) {
            avg += input.vdw.rho.at(static_cast<std::size_t>(ielem * input.vdw.ngauss_vdw + ig));
        }
        w_density[static_cast<std::size_t>(ielem)] = avg / static_cast<double>(input.vdw.ngauss_vdw);
    }
    return w_density;
}

}  // namespace

std::string snapshot_filename(const int step) {
    std::ostringstream name;
    name << "mesh_config_" << std::setw(4) << std::setfill('0') << step << ".vtu";
    return name.str();
}

void update_crease_reference(const SimulatorInput& input,
                             RuntimeState& state) {
    if (input.crease.ncrease != 1 || state.K0_ref.empty()) {
        return;
    }

    FlatCoords coords_with_ghosts;
    coords_with_ghosts.reserve(state.coords.size() * 3);
    for (const auto& xyz : state.coords) {
        coords_with_ghosts.push_back(xyz[0]);
        coords_with_ghosts.push_back(xyz[1]);
        coords_with_ghosts.push_back(xyz[2]);
    }
    coords_with_ghosts.resize(static_cast<std::size_t>(3 * (input.mesh.numnods + input.mesh.nedge)));
    ghost_nodes(input.mesh, coords_with_ghosts);

    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        const auto xneigh = gather_neighbor_patch(input.mesh, coords_with_ghosts, ielem);
        for (int igauss = 0; igauss < input.dims.ngauss; ++igauss) {
            const auto [dn, ddn] = gauss_geometry_data(input.gauss, igauss);
            const auto element_state = compute_element_state(
                xneigh,
                dn,
                ddn,
                input.ref_config.at(static_cast<std::size_t>(ielem)).F0,
                Voigt3{0.0, 0.0, 0.0});
            auto& k0 = state.K0_ref.at(static_cast<std::size_t>(ielem)).at(static_cast<std::size_t>(igauss));
            const Voigt3 curv_eff{
                element_state.curv0_elem[0] - k0[0],
                element_state.curv0_elem[1] - k0[1],
                element_state.curv0_elem[2] - k0[2],
            };
            const double kappa_mag = std::sqrt(curv_eff[0] * curv_eff[0] +
                                               curv_eff[1] * curv_eff[1] +
                                               curv_eff[2] * curv_eff[2]);
            if (kappa_mag <= input.crease.kappa_cr) {
                continue;
            }
            for (int axis = 0; axis < 3; ++axis) {
                k0[axis] += input.crease.alpha_lock * curv_eff[axis];
            }
        }
    }
}

void write_mesh_snapshot(const SimulatorInput& input,
                         const RuntimeState& state,
                         const std::string& output_dir,
                         const int step) {
    validate_runtime_output_state(input, state, step);

    const auto atomic_density = compute_atomic_density(input);
    const auto w_density = compute_w_density(input);

    const std::filesystem::path path =
        std::filesystem::path(output_dir) / snapshot_filename(step);
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open " + path.string());
    }

    out << "<?xml version=\"1.0\"?>\n";
    out << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
    out << "  <UnstructuredGrid>\n";
    out << "    <FieldData>\n";
    out << "      <DataArray type=\"Float64\" Name=\"TimeValue\" NumberOfTuples=\"1\" format=\"ascii\">";
    out << std::fixed << std::setprecision(16) << snapshot_time(input.bcs, step);
    out << "</DataArray>\n";
    out << "    </FieldData>\n";
    out << "    <Piece NumberOfPoints=\"" << input.mesh.numnods
        << "\" NumberOfCells=\"" << input.mesh.numele << "\">\n";

    out << "      <Points>\n";
    out << "        <DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">\n";
    out << std::uppercase << std::scientific << std::setprecision(16);
    for (int inode = 0; inode < input.mesh.numnods; ++inode) {
        const auto& xyz = state.coords.at(static_cast<std::size_t>(inode));
        out << " " << std::setw(24) << xyz[0]
            << " " << std::setw(24) << xyz[1]
            << " " << std::setw(24) << xyz[2] << "\n";
    }
    out << "        </DataArray>\n";
    out << "      </Points>\n";

    out << "      <PointData Scalars=\"atomic_density\">\n";
    out << "        <DataArray type=\"Float64\" Name=\"atomic_density\" NumberOfComponents=\"1\" format=\"ascii\">\n";
    for (const double value : atomic_density) {
        out << " " << std::setw(24) << value << "\n";
    }
    out << "        </DataArray>\n";
    out << "      </PointData>\n";

    out << "      <Cells>\n";
    out << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">\n";
    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        const auto& vertices = input.mesh.connect.at(static_cast<std::size_t>(ielem)).vertices;
        out << vertices[0] << " " << vertices[1] << " " << vertices[2] << "\n";
    }
    out << "        </DataArray>\n";

    out << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">\n";
    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        out << 3 * (ielem + 1) << "\n";
    }
    out << "        </DataArray>\n";

    out << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n";
    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        out << 5 << "\n";
    }
    out << "        </DataArray>\n";
    out << "      </Cells>\n";

    out << "      <CellData Vectors=\"inner_displacement\" Scalars=\"W_density\">\n";
    out << "        <DataArray type=\"Float64\" Name=\"inner_displacement\" NumberOfComponents=\"3\" format=\"ascii\">\n";
    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        const Vec2 eta_avg = average_eta(state.eta, ielem, input.dims.ngauss);
        out << " " << std::setw(24) << eta_avg[0]
            << " " << std::setw(24) << eta_avg[1]
            << " " << std::setw(24) << 0.0 << "\n";
    }
    out << "        </DataArray>\n";

    out << "        <DataArray type=\"Float64\" Name=\"W_density\" NumberOfComponents=\"1\" format=\"ascii\">\n";
    for (const double value : w_density) {
        out << " " << std::setw(24) << value << "\n";
    }
    out << "        </DataArray>\n";
    out << "      </CellData>\n";

    out << "    </Piece>\n";
    out << "  </UnstructuredGrid>\n";
    out << "</VTKFile>\n";
}

void write_crease_map(const SimulatorInput& input,
                      const RuntimeState& state,
                      const std::string& output_dir) {
    if (input.crease.ncrease != 1 || state.K0_ref.empty()) {
        return;
    }

    validate_runtime_output_state(input, state, /*step=*/0);

    FlatCoords coords_with_ghosts;
    coords_with_ghosts.reserve(state.coords.size() * 3);
    for (const auto& xyz : state.coords) {
        coords_with_ghosts.push_back(xyz[0]);
        coords_with_ghosts.push_back(xyz[1]);
        coords_with_ghosts.push_back(xyz[2]);
    }
    coords_with_ghosts.resize(static_cast<std::size_t>(3 * (input.mesh.numnods + input.mesh.nedge)));
    ghost_nodes(input.mesh, coords_with_ghosts);

    std::vector<Vec3> elem_norm(static_cast<std::size_t>(input.mesh.numele), Vec3{0.0, 0.0, 0.0});
    std::vector<double> kappa_mean(static_cast<std::size_t>(input.mesh.numele), 0.0);
    std::vector<double> kappa_max(static_cast<std::size_t>(input.mesh.numele), 0.0);
    std::vector<int> is_creased(static_cast<std::size_t>(input.mesh.numele), 0);

    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        const auto xneigh = gather_neighbor_patch(input.mesh, coords_with_ghosts, ielem);
        const auto [dn, ddn] = gauss_geometry_data(input.gauss, /*igauss=*/0);
        const auto element_state = compute_element_state(
            xneigh,
            dn,
            ddn,
            input.ref_config.at(static_cast<std::size_t>(ielem)).F0,
            Voigt3{0.0, 0.0, 0.0});
        elem_norm[static_cast<std::size_t>(ielem)] = element_state.metric.xnor_elem;

        double mean = 0.0;
        double max_value = 0.0;
        for (int igauss = 0; igauss < input.dims.ngauss; ++igauss) {
            const auto& k0 = state.K0_ref.at(static_cast<std::size_t>(ielem)).at(static_cast<std::size_t>(igauss));
            const double magnitude = std::sqrt(k0[0] * k0[0] + k0[1] * k0[1] + k0[2] * k0[2]);
            mean += magnitude;
            max_value = std::max(max_value, magnitude);
        }
        mean /= static_cast<double>(input.dims.ngauss);
        kappa_mean[static_cast<std::size_t>(ielem)] = mean;
        kappa_max[static_cast<std::size_t>(ielem)] = max_value;
        is_creased[static_cast<std::size_t>(ielem)] = mean > input.crease.kappa_cr ? 1 : 0;
    }

    const std::filesystem::path path = std::filesystem::path(output_dir) / "crease_map.dat";
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open " + path.string());
    }

    out << "! crease_map.dat - Module 4 crease detection & facet analysis\n";
    out << "! Columns: ielem  kappa_mean(1/nm)  kappa_max(1/nm)  is_creased  n_neigh  min_dihedral_deg\n";
    out << std::uppercase << std::scientific << std::setprecision(8);

    int n_creased = 0;
    for (int ielem = 0; ielem < input.mesh.numele; ++ielem) {
        const auto& element = input.mesh.connect.at(static_cast<std::size_t>(ielem));
        double min_dihedral = 180.0;
        for (int j = 0; j < element.num_neigh_elem; ++j) {
            const int jelem = element.neigh_elem[j];
            if (jelem <= 0 || jelem > input.mesh.numele) {
                continue;
            }
            const auto& other = elem_norm.at(static_cast<std::size_t>(jelem - 1));
            const auto& normal = elem_norm.at(static_cast<std::size_t>(ielem));
            double dot_nn = normal[0] * other[0] + normal[1] * other[1] + normal[2] * other[2];
            dot_nn = std::max(-1.0, std::min(1.0, dot_nn));
            const double dihedral = std::acos(dot_nn) * 180.0 / 3.14159265358979323846;
            min_dihedral = std::min(min_dihedral, dihedral);
        }
        out << std::setw(8) << ielem + 1
            << std::setw(16) << kappa_mean.at(static_cast<std::size_t>(ielem))
            << std::setw(16) << kappa_max.at(static_cast<std::size_t>(ielem))
            << std::setw(4) << is_creased.at(static_cast<std::size_t>(ielem))
            << std::setw(5) << element.num_neigh_elem
            << std::fixed << std::setprecision(4) << std::setw(12) << min_dihedral
            << std::scientific << std::setprecision(8) << "\n";
        if (is_creased.at(static_cast<std::size_t>(ielem)) == 1) {
            ++n_creased;
        }
    }
    out << "!\n";
    out << "! Creased elements : " << n_creased << " / " << input.mesh.numele << "\n";
    out << std::uppercase << std::scientific << std::setprecision(6)
        << "! kappa_cr (1/nm)  : " << input.crease.kappa_cr << "\n";
}

void write_mesh_series_index(const std::string& output_dir,
                             const BCData& bcs,
                             const int final_step) {
    if (final_step < 0) {
        throw std::runtime_error("PVD final step must be non-negative");
    }

    const std::filesystem::path path =
        std::filesystem::path(output_dir) / "mesh_config_series.pvd";
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open " + path.string());
    }

    out << "<?xml version=\"1.0\"?>\n";
    out << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
    out << "  <Collection>\n";
    for (int step = 0; step <= final_step; ++step) {
        const std::string filename = snapshot_filename(step);
        if (!std::filesystem::exists(std::filesystem::path(output_dir) / filename)) {
            continue;
        }
        out << "    <DataSet timestep=\""
            << std::uppercase << std::scientific << std::setprecision(16)
            << snapshot_time(bcs, step)
            << "\" group=\"\" part=\"0\" file=\"" << filename << "\"/>\n";
    }
    out << "  </Collection>\n";
    out << "</VTKFile>\n";
}

}  // namespace fce
