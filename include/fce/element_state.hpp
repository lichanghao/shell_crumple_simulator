#pragma once

#include "fce/constitutive.hpp"
#include "fce/exponential.hpp"
#include "fce/geometry.hpp"
#include "fce/principal.hpp"

namespace fce {

struct PreparedBondState {
    std::array<double, 3> A_norm{};
    std::array<Vec2, 3> Ei{};
    BondState bonds{};
};

struct PreparedBondStateWithDerivatives {
    std::array<double, 3> A_norm{};
    std::array<Vec2, 3> Ei{};
    BondStateWithDerivatives bonds{};
};

struct CanonicalPreparedBondState {
    std::array<double, 3> A_norm{};
    std::array<Vec2, 3> Ei{};
    BondState bonds{};
    BondStateWithDerivatives bonds_with_derivatives{};
};

struct ElementState {
    MetricResult metric{};
    CurvatureResult curvature{};
    Voigt3 C_elem{};
    Voigt3 curv0_elem{};
    Vec2 curvppal{};
    Mat22 vppal{};
    PrincipalDerivativeVector dcurvppaldC{};
    PrincipalDerivativeVector dcurvppaldk{};
    PrincipalDerivativeMatrix dvppaldC{};
    PrincipalDerivativeMatrix dvppaldk{};
    bool flag_num_diff{false};
    CanonicalPreparedBondState prepared_bonds{};
    Vec2 prepared_eta{};
    std::array<Vec2, 3> prepared_material_E{};
    double prepared_material_A0{0.0};
    bool has_prepared_bond_state{false};
};

struct RelaxedElementState {
    ElementState state{};
    NewtonInnerOutput inner{};
};

ElementState compute_element_state(const NeighborCoords12& xneigh,
                                   const ShapeGradient12& dn,
                                   const ShapeCurvature12& ddn,
                                   const Mat22& f0,
                                   const Voigt3& reference_curvature = {});

PreparedBondState prepare_bond_state(const ElementState& state,
                                     const MatData& mat,
                                     const Vec2& eta);

PreparedBondStateWithDerivatives prepare_bond_state_with_derivatives(const ElementState& state,
                                                                     const MatData& mat,
                                                                     const Vec2& eta);

ElementState prepare_element_state(const ElementState& state,
                                   const MatData& mat,
                                   const Vec2& eta);

RelaxedElementState solve_inner_newton_for_element(const NeighborCoords12& xneigh,
                                                   const ShapeGradient12& dn,
                                                   const ShapeCurvature12& ddn,
                                                   const Mat22& f0,
                                                   const Voigt3& reference_curvature,
                                                   const MatData& mat,
                                                   const Vec2& eta0,
                                                   double crit,
                                                   int max_iter);

}  // namespace fce
