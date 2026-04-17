#pragma once
// Load controller: applies boundary-condition increments and manages free/BC DOF splits.
// Translated from load.f90 (nCodeLoad=3 path), pre_ener.f90 (short/long), and
// get_reac.f90 (reaction force accumulation).

#include "fce/types.hpp"

#include <stdexcept>
#include <vector>

namespace fce {

class LoadController {
public:
    explicit LoadController(const BCData& bcs);

    // Initialise x0_bc_ from the global coords (call once before the first load step).
    void init(const Coords& coords);

    // Apply the load increment for step iload (1-based, matching Fortran iload_start..nloadstep).
    // Updates coords at BC DOF positions (mirrors Fortran load_doit nCodeLoad=3).
    // Throws std::runtime_error for nCodeLoad=30/31.
    void apply_increment(int iload, Coords& coords);

    // Extract free-DOF values from coords into a short vector.
    // Mirrors Fortran subroutine short: x_short(i) = x0(mdofOP(i)) for i=1..ndofOP.
    // All indices are 0-based internally; mdofOP elements are flat 3*inode+axis indices.
    std::vector<double> to_free(const Coords& coords) const;

    // Scatter free-DOF values back into coords, keeping BC DOFs fixed.
    // Mirrors Fortran subroutine long.
    void to_full(const std::vector<double>& x_free, Coords& coords) const;

    // Scatter x_free and the stored x0_bc_ values back into coords.
    // This is the full "long" operation that also restores BC nodes.
    void scatter_all(const std::vector<double>& x_free, Coords& coords) const;

    // Compute reaction forces using the Fortran get_reac nCodeLoad=3 rule.
    // The cyclic pasapas path (nCodeLoad=30/31) explicitly calls get_reac(..., 3, ...),
    // so the same z-DOF accumulation rule applies there as well.
    // forces_flat is a flat vector of size 3*numnods (real nodes only, after ghost folding).
    // Mirrors Fortran get_reac (nCodeLoad=3):
    //   reaction1 += forces(mdofBC(3*i)) for mnodBC(i,2)==1
    //   reaction2 += forces(mdofBC(3*i)) for every other boundary tag
    // where i goes 1..nnodBC (1-based Fortran) = mdofBC[3*i-1]=mdofBC[3*(i-1)+2] 0-based.
    void compute_reaction(const std::vector<double>& forces_flat,
                          double& reaction1, double& reaction2) const;

    int ndof_free()    const { return bcs_.ndofOP; }
    int ndof_bc()      const { return bcs_.ndofBC; }
    int nloadstep()    const { return bcs_.nloadstep; }
    int nCodeLoad()    const { return bcs_.nCodeLoad; }
    double bc_value()  const { return bcs_.value; }

private:
    const BCData& bcs_;
    std::vector<double> x0_bc_;  // 3*nnodBC values; mirrors Fortran x0_BC array
};

}  // namespace fce
