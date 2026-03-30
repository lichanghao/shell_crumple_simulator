#include "fce/vdw_preprocessor.hpp"

#include "fce/bspline.hpp"
#include "fce/ghost_nodes.hpp"

#include <cmath>
#include <stdexcept>

namespace fce {
namespace {

void fill_vdw_gauss_point(std::vector<std::array<double, 12>>& shapef,
                          int ig,
                          double v,
                          double w)
{
    std::array<double, 12> N{};
    BSpline(N, v, w);
    shapef[ig] = N;
}

void compute_gauss_positions(VdwData& vdw, const Mesh& mesh, const FlatCoords& coords)
{
    FlatCoords coords_with_ghosts = coords;
    coords_with_ghosts.resize(3 * (mesh.numnods + mesh.nedge), 0.0);
    ghost_nodes(mesh, coords_with_ghosts);

    vdw.x.assign(vdw.ng_tot, Vec3{0.0, 0.0, 0.0});
    for (int ielem = 0; ielem < mesh.numele; ++ielem) {
        for (int ig = 0; ig < vdw.ngauss_vdw; ++ig) {
            const int idx = ielem * vdw.ngauss_vdw + ig;
            for (int inode = 0; inode < 12; ++inode) {
                const int node = mesh.connect[ielem].neigh_vert[inode];
                const int base = 3 * node;
                for (int dim = 0; dim < 3; ++dim) {
                    vdw.x[idx][dim] += vdw.shapef[ig][inode] * coords_with_ghosts[base + dim];
                }
            }
        }
    }
}

void compute_atomic_density(VdwData& vdw,
                            const Mesh& mesh,
                            const FlatCoords& coords,
                            const MatData& mat,
                            double twist_angle_radians)
{
    compute_gauss_positions(vdw, mesh, coords);

    const double v1x = mat.A0 * (mat.E[0][0] - mat.E[1][0]);
    const double v1y = mat.A0 * (mat.E[0][1] - mat.E[1][1]);
    const double v2x = mat.A0 * (mat.E[0][0] - mat.E[2][0]);
    const double v2y = mat.A0 * (mat.E[0][1] - mat.E[2][1]);
    const double det = v1x * v2y - v1y * v2x;
    const double inv11 = v2y / det;
    const double inv12 = -v2x / det;
    const double inv21 = -v1y / det;
    const double inv22 = v1x / det;

    const double ct = std::cos(-twist_angle_radians);
    const double st = std::sin(-twist_angle_radians);
    const double alpha = vdw.alpha_sharp;
    const double pi = std::acos(-1.0);

    vdw.rho.assign(vdw.ng_tot, 0.0);
    for (int igg = 0; igg < vdw.ng_tot; ++igg) {
        const double rx = vdw.x[igg][0] - vdw.xc0;
        const double ry = vdw.x[igg][1] - vdw.yc0;
        const double dx_orig = rx * ct - ry * st + vdw.xc0;
        const double dy_orig = rx * st + ry * ct + vdw.yc0;

        const double c1 = inv11 * dx_orig + inv12 * dy_orig;
        const double c2 = inv21 * dx_orig + inv22 * dy_orig;
        const int n1_min = static_cast<int>(std::floor(c1 - 5.0));
        const int n1_max = static_cast<int>(std::ceil(c1 + 5.0));
        const int n2_min = static_cast<int>(std::floor(c2 - 5.0));
        const int n2_max = static_cast<int>(std::ceil(c2 + 5.0));

        double w_val = 0.0;
        for (int n1 = n1_min; n1 <= n1_max; ++n1) {
            for (int n2 = n2_min; n2 <= n2_max; ++n2) {
                double xa = n1 * v1x + n2 * v2x;
                double ya = n1 * v1y + n2 * v2y;
                double r2 = (dx_orig - xa) * (dx_orig - xa) + (dy_orig - ya) * (dy_orig - ya);
                if (alpha * r2 < 20.0) {
                    w_val += std::exp(-alpha * r2);
                }

                xa += mat.A0 * mat.E[0][0];
                ya += mat.A0 * mat.E[0][1];
                r2 = (dx_orig - xa) * (dx_orig - xa) + (dy_orig - ya) * (dy_orig - ya);
                if (alpha * r2 < 20.0) {
                    w_val += std::exp(-alpha * r2);
                }
            }
        }
        vdw.rho[igg] = w_val * (alpha / pi);
    }
}

} // namespace

void compute_vdw_cutoff(VdwData& vdw)
{
    const double a1 = vdw.sig / vdw.r_cut;
    const double a6 = std::pow(a1, 6);
    const double a12 = a6 * a6;
    const double y06 = std::pow(vdw.y0, 6);

    vdw.Vcut[0] = (0.5 * y06 * a12 - a6) * vdw.a / std::pow(vdw.sig, 6);
    vdw.Vcut[1] = ((vdw.sig / vdw.r_cut) * 6.0 / vdw.sig * (-y06 * a12 + a6)) *
                  vdw.a / std::pow(vdw.sig, 6);
}

void setup_vdw_quadrature(VdwData& vdw)
{
    vdw.shapef.assign(vdw.ngauss_vdw, std::array<double, 12>{});
    vdw.weight.assign(vdw.ngauss_vdw, 0.0);

    if (vdw.ngauss_vdw == 1) {
        fill_vdw_gauss_point(vdw.shapef, 0, 1.0 / 3.0, 1.0 / 3.0);
        vdw.weight[0] = 1.0;
        return;
    }
    if (vdw.ngauss_vdw == 2) {
        const double pos1 = 1.0 / 6.0;
        const double pos2 = 2.0 / 3.0;
        fill_vdw_gauss_point(vdw.shapef, 0, pos1, pos2);
        fill_vdw_gauss_point(vdw.shapef, 1, pos2, pos1);
        vdw.weight[0] = 0.5;
        vdw.weight[1] = 0.5;
        return;
    }
    if (vdw.ngauss_vdw == 3) {
        const double pos1 = 1.0 / 6.0;
        const double pos2 = 2.0 / 3.0;
        fill_vdw_gauss_point(vdw.shapef, 0, pos1, pos1);
        fill_vdw_gauss_point(vdw.shapef, 1, pos2, pos1);
        fill_vdw_gauss_point(vdw.shapef, 2, pos1, pos2);
        vdw.weight[0] = 1.0 / 3.0;
        vdw.weight[1] = 1.0 / 3.0;
        vdw.weight[2] = 1.0 / 3.0;
        return;
    }

    throw std::runtime_error("setup_vdw_quadrature: unsupported ngauss_vdw=" +
                             std::to_string(vdw.ngauss_vdw));
}

void initialize_preprocessor_vdw(VdwData& vdw,
                                 const Mesh& mesh,
                                 const FlatCoords& coords,
                                 const MatData& mat,
                                 double sheet_xlength,
                                 double sheet_ylength,
                                 double twist_angle_radians,
                                 bool use_atomic_density)
{
    vdw.ng_tot = mesh.numele * vdw.ngauss_vdw;
    vdw.xc0 = sheet_xlength / 2.0;
    vdw.yc0 = sheet_ylength / 2.0;
    setup_vdw_quadrature(vdw);

    if (use_atomic_density) {
        compute_atomic_density(vdw, mesh, coords, mat, twist_angle_radians);
    } else {
        vdw.rho.assign(vdw.ng_tot, 2.0 / mat.s0);
    }

    vdw.nneigh = (vdw.nself_contact == 1) ? -2 : -1;
    vdw.ninrange = 50000;
}

std::vector<std::pair<int, int>> build_tub_partitions(
    const std::vector<int>& elements_per_sheet,
    int ngauss_vdw)
{
    std::vector<std::pair<int, int>> parts;
    int end = 0;
    for (const int numel : elements_per_sheet) {
        const int start = end;
        end += numel * ngauss_vdw;
        parts.emplace_back(start, end);
    }
    return parts;
}

} // namespace fce
