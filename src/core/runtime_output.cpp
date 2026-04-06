#include "fce/runtime_output.hpp"

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
