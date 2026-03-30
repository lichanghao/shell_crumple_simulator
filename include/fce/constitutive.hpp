#pragma once

#include "fce/types.hpp"

#include <array>

namespace fce {

using Vec6 = std::array<double, 6>;
using Voigt3 = std::array<double, 3>;
using Mat66 = std::array<Vec6, 6>;

struct BrennerOutput {
    double W{0.0};
    Vec6 dW{};
    Mat66 ddW{};
};

struct InnerPotentialOutput {
    double W{0.0};
    Vec2 dWdeta{};
    Voigt3 ddWdeta{};
    Vec6 dW_dpe{};
};

struct NewtonInnerOutput {
    Vec2 eta{};
    double W{0.0};
    Vec2 dWdeta{};
    Voigt3 ddWdeta{};
    Vec6 dW_dpe{};
    int iterations{0};
    int fail_mode{0};
};

BrennerOutput evaluate_brenner(const MatData& mat, const Vec6& pe);

InnerPotentialOutput evaluate_inner_potential(const Voigt3& C_elem,
                                              const Vec2& curvppal,
                                              const Mat22& vppal,
                                              const MatData& mat,
                                              const Vec2& eta);

NewtonInnerOutput solve_inner_newton(const Voigt3& C_elem,
                                     const Vec2& curvppal,
                                     const Mat22& vppal,
                                     const MatData& mat,
                                     const Vec2& eta0,
                                     double crit,
                                     int max_iter);

}  // namespace fce
