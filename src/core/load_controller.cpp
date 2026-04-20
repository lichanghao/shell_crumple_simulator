// Load controller implementation.
// Translates Fortran load.f90 (nCodeLoad=3), pre_ener.f90 (short/long), get_reac.f90.

#include "fce/load_controller.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace fce {

// ─── constructor ─────────────────────────────────────────────────────────────

LoadController::LoadController(const BCData& bcs)
    : bcs_(bcs) {}

// ─── init ─────────────────────────────────────────────────────────────────────

void LoadController::init(const Coords& coords) {
    // Mirrors Fortran: x0_BC(:) = x0(BCs%mdofBC(:))
    // mdofBC holds flat 3*inode+axis indices (0-based).
    x0_bc_.resize(static_cast<std::size_t>(bcs_.ndofBC));
    for (int k = 0; k < bcs_.ndofBC; ++k) {
        const int flat_dof = bcs_.mdofBC.at(static_cast<std::size_t>(k));
        const int inode = flat_dof / 3;
        const int axis  = flat_dof % 3;
        x0_bc_[static_cast<std::size_t>(k)] = coords.at(static_cast<std::size_t>(inode))[axis];
    }
}

// ─── apply_increment ─────────────────────────────────────────────────────────

void LoadController::apply_increment(int iload, Coords& coords) {
    (void)iload;
    const int ncode = bcs_.nCodeLoad;

    if (ncode == 3) {
        // Compression: decrement x-coordinate of BC nodes with mnodBC[inod][1]==2.
        // Fortran (1-based loop over inod=1..nnodBC):
        //   DL = BCs%value / BCs%nloadstep
        //   if (BCs%mnodBC(inod,2) .eq. 2) x0_BC(3*inod-2) -= DL
        //   x0(BCs%mdofBC(:)) = x0_BC(:)
        //
        // 0-based: x0_bc_[3*inod] -= DL  (because 3*inod-2 in 1-based = 3*inod+0 for 0-based inod)
        // Wait: Fortran 1-based inod=1..nnodBC → 0-based inod=0..nnodBC-1.
        // Fortran x0_BC(3*inod-2) for 1-based inod=1: element index 1 (1-based) = 0 (0-based).
        // For inod=2: 3*2-2=4 (1-based) = 3 (0-based).
        // Pattern: for 0-based inod i, element 3*i.
        // BCData::mnodBC stores 0-based values (Fortran value - 1):
        //   mnodBC[i][1] == 0 → side 1 (fixed)
        //   mnodBC[i][1] == 1 → side 2 (compressed)
        const double dl = bcs_.value / static_cast<double>(bcs_.nloadstep);
        for (int inod = 0; inod < bcs_.nnodBC; ++inod) {
            if (bcs_.mnodBC.at(static_cast<std::size_t>(inod))[1] == 1) {
                x0_bc_[static_cast<std::size_t>(3 * inod)] -= dl;
            }
        }
        // Scatter x0_bc_ back to coords at BC DOFs.
        for (int k = 0; k < bcs_.ndofBC; ++k) {
            const int flat_dof = bcs_.mdofBC.at(static_cast<std::size_t>(k));
            const int inode = flat_dof / 3;
            const int axis  = flat_dof % 3;
            coords.at(static_cast<std::size_t>(inode))[axis] =
                x0_bc_.at(static_cast<std::size_t>(k));
        }
    } else if (ncode == 30 || ncode == 31) {
        const int steps_per_cycle = bcs_.nloadstep_comp + bcs_.nloadstep_rel;
        if (steps_per_cycle <= 0) {
            throw std::runtime_error("cyclic load controller requires positive steps_per_cycle");
        }
        const int iload_in_cycle = ((iload - 1) % steps_per_cycle) + 1;
        const bool compression_phase = iload_in_cycle <= bcs_.nloadstep_comp;
        const double dl = compression_phase
            ? (bcs_.value_comp / static_cast<double>(bcs_.nloadstep_comp))
            : (bcs_.value_rel / static_cast<double>(bcs_.nloadstep_rel));
        const double sign = compression_phase ? -1.0 : 1.0;

        for (int inod = 0; inod < bcs_.nnodBC; ++inod) {
            const int side_tag = bcs_.mnodBC.at(static_cast<std::size_t>(inod))[1];
            if (ncode == 30) {
                if (side_tag == 1) {
                    x0_bc_.at(static_cast<std::size_t>(3 * inod)) += sign * dl;
                }
                continue;
            }

            if (side_tag == 0) {
                continue;
            }
            if (side_tag == 1 || side_tag == 2) {
                const int axis = side_tag - 1;
                x0_bc_.at(static_cast<std::size_t>(3 * inod + axis)) += sign * dl;
                continue;
            }
            if (side_tag == 3) {
                x0_bc_.at(static_cast<std::size_t>(3 * inod)) += sign * dl;
                x0_bc_.at(static_cast<std::size_t>(3 * inod + 1)) += sign * dl;
            }
        }

        for (int k = 0; k < bcs_.ndofBC; ++k) {
            const int flat_dof = bcs_.mdofBC.at(static_cast<std::size_t>(k));
            const int inode = flat_dof / 3;
            const int axis  = flat_dof % 3;
            coords.at(static_cast<std::size_t>(inode))[axis] =
                x0_bc_.at(static_cast<std::size_t>(k));
        }
    } else if (ncode == 222 || ncode == 1000) {
        // The archived bilayer runtime keeps the constrained coordinates fixed
        // while still running the solve/output path.
        for (int k = 0; k < bcs_.ndofBC; ++k) {
            const int flat_dof = bcs_.mdofBC.at(static_cast<std::size_t>(k));
            const int inode = flat_dof / 3;
            const int axis  = flat_dof % 3;
            coords.at(static_cast<std::size_t>(inode))[axis] =
                x0_bc_.at(static_cast<std::size_t>(k));
        }
    } else {
        throw std::runtime_error("LoadController::apply_increment: nCodeLoad " +
                                 std::to_string(ncode) + " not supported");
    }
}

// ─── to_free ─────────────────────────────────────────────────────────────────

std::vector<double> LoadController::to_free(const Coords& coords) const {
    // Mirrors Fortran subroutine short:
    //   do i = 1, ndofOP; x_short(i) = x0(mdofOP(i)); end do
    // mdofOP holds flat 3*inode+axis indices (0-based).
    std::vector<double> x_free(static_cast<std::size_t>(bcs_.ndofOP));
    for (int i = 0; i < bcs_.ndofOP; ++i) {
        const int flat_dof = bcs_.mdofOP.at(static_cast<std::size_t>(i));
        const int inode = flat_dof / 3;
        const int axis  = flat_dof % 3;
        x_free[static_cast<std::size_t>(i)] = coords.at(static_cast<std::size_t>(inode))[axis];
    }
    return x_free;
}

// ─── to_full ─────────────────────────────────────────────────────────────────

void LoadController::to_full(const std::vector<double>& x_free, Coords& coords) const {
    // Mirrors Fortran subroutine short (scatter free DOFs only):
    //   do i = 1, ndofOP; x0(mdofOP(i)) = x_short(i); end do
    for (int i = 0; i < bcs_.ndofOP; ++i) {
        const int flat_dof = bcs_.mdofOP.at(static_cast<std::size_t>(i));
        const int inode = flat_dof / 3;
        const int axis  = flat_dof % 3;
        coords.at(static_cast<std::size_t>(inode))[axis] =
            x_free.at(static_cast<std::size_t>(i));
    }
}

// ─── scatter_all ─────────────────────────────────────────────────────────────

void LoadController::scatter_all(const std::vector<double>& x_free, Coords& coords) const {
    // Mirrors Fortran subroutine long: scatter both free and BC DOFs.
    to_full(x_free, coords);
    // Restore BC DOFs.
    for (int k = 0; k < bcs_.ndofBC; ++k) {
        const int flat_dof = bcs_.mdofBC.at(static_cast<std::size_t>(k));
        const int inode = flat_dof / 3;
        const int axis  = flat_dof % 3;
        coords.at(static_cast<std::size_t>(inode))[axis] =
            x0_bc_.at(static_cast<std::size_t>(k));
    }
}

// ─── compute_reaction ────────────────────────────────────────────────────────

void LoadController::compute_reaction(const std::vector<double>& forces_flat,
                                       double& reaction1, double& reaction2) const {
    reaction1 = 0.0;
    reaction2 = 0.0;

    // Fortran get_reac nCodeLoad==3:
    //   do i=1,BCs%nnodBC
    //     if (BCs%mnodBC(i,2)==1) reaction1 += forces(mdofBC(3*i))
    //     else                    reaction2 += forces(mdofBC(3*i))
    //   end do
    //
    // pasapas.f90 uses this same rule for nCodeLoad=30/31 by calling
    // get_reac(mesh0, forces, 3, ...), so the production cyclic path also
    // routes every non-side-1 boundary node into reaction2.
    //
    // Translation to 0-based C++:
    //   Fortran i goes 1..nnodBC (1-based); mdofBC(3*i) is 1-based.
    //   For Fortran i: mdofBC(3*i) → 0-based index mdofBC[3*i - 1].
    //   For 0-based loop i_c = i-1 = 0..nnodBC-1:
    //   mdofBC(3*(i_c+1)) → 0-based: mdofBC[3*(i_c+1) - 1] = mdofBC[3*i_c + 2].
    //
    //   forces_flat is size 3*numnods (real nodes only).
    //   mdofBC values are flat 0-based DOF indices (3*inode + axis).
    for (int i = 0; i < bcs_.nnodBC; ++i) {
        // 0-based index into mdofBC: 3*(i+1)-1 = 3*i+2 (matches Fortran mdofBC(3*i) 1-based).
        const std::size_t mdof_idx = static_cast<std::size_t>(3 * i + 2);
        const int flat_dof = bcs_.mdofBC.at(mdof_idx);
        const double force_val = forces_flat.at(static_cast<std::size_t>(flat_dof));
        if (bcs_.mnodBC.at(static_cast<std::size_t>(i))[1] == 0) {
            reaction1 += force_val;
        } else {
            reaction2 += force_val;
        }
    }
}

}  // namespace fce
