// B-spline basis — direct translation from BSpline.f90.
// All 12 expressions are divided by 12 (following the Fortran).

#include "fce/bspline.hpp"

#include <stdexcept>

namespace fce {

namespace {

void validate_parametric_coordinates(double v, double w)
{
    constexpr double kTol = 1e-12;
    if (v < -kTol || w < -kTol || v + w > 1.0 + kTol) {
        throw std::invalid_argument("B-spline coordinates must satisfy v>=0, w>=0, and v+w<=1");
    }
}

} // namespace

void BSpline(std::array<double,12>& N, double v, double w)
{
    validate_parametric_coordinates(v, w);
    double t1 = 1.0 - v - w;
    double t2 = t1 * t1;
    double t3 = t2 * t2;
    double t4 = t2 * t1;
    double t5 = t4 * v;
    double t6 = 2.0 * t5;
    double t8 = t4 * w;
    double t9 = 2.0 * t8;
    double t11 = 6.0 * t5;
    double t13 = t2 * v * w;
    double t14 = 6.0 * t13;
    double t15 = v * v;
    double t16 = t2 * t15;
    double t17 = 12.0 * t16;
    double t19 = t1 * t15 * w;
    double t20 = 6.0 * t19;
    double t21 = t15 * v;
    double t22 = t1 * t21;
    double t23 = 6.0 * t22;
    double t24 = t21 * w;
    double t25 = 2.0 * t24;
    double t26 = t15 * t15;
    double t30 = w * w;
    double t31 = t2 * t30;
    double t32 = 24.0 * t31;
    double t33 = t30 * w;
    double t34 = t1 * t33;
    double t36 = t30 * t30;
    double t40 = t1 * v * t30;
    double t41 = 36.0 * t40;
    double t42 = v * t33;
    double t43 = 6.0 * t42;
    double t44 = 24.0 * t16;
    double t45 = 36.0 * t19;
    double t46 = t15 * t30;
    double t47 = 12.0 * t46;
    double t49 = 6.0 * t24;
    double t50 = 6.0*t3 + 24.0*t8 + t32 + 8.0*t34 + t36 + 24.0*t5 + 60.0*t13 + t41 + t43
               + t44 + t45 + t47 + 8.0*t22 + t49 + t26;
    double t51 = 6.0 * t8;
    double t52 = 12.0 * t31;
    double t53 = 6.0 * t34;
    double t54 = 6.0 * t40;
    double t55 = 2.0 * t42;
    double t57 = 2.0 * t22;
    double t60 = 36.0 * t13;
    double t63 = 24.0 * t46;
    double t67 = t3 + t51 + t52 + t53 + t36 + 8.0*t5 + t60 + t41 + 8.0*t42 + t44 + 60.0*t19
               + t63 + 24.0*t22 + 24.0*t24 + 6.0*t26;
    double t74 = t3 + 8.0*t8 + t32 + 24.0*t34 + 6.0*t36 + t11 + t60 + 60.0*t40 + 24.0*t42
               + t17 + t45 + t63 + t23 + 8.0*t24 + t26;
    double t75 = 2.0 * t34;

    N[0]  = t3 + t6;
    N[1]  = t3 + t9;
    N[2]  = t3 + t9 + t11 + t14 + t17 + t20 + t23 + t25 + t26;
    N[3]  = t50;
    N[4]  = t3 + t51 + t52 + t53 + t36 + t6 + t14 + t54 + t55;
    N[5]  = t57 + t26;
    N[6]  = t67;
    N[7]  = t74;
    N[8]  = t75 + t36;
    N[9]  = t25 + t26;
    N[10] = t75 + t36 + t54 + t43 + t20 + t47 + t57 + t49 + t26;
    N[11] = t36 + t55;
    for (double& value : N) value /= 12.0;
}

void DBSpline(std::array<std::array<double,2>,12>& DN, double v, double w)
{
    validate_parametric_coordinates(v, w);
    double t1  = 1.0 - v - w;
    double t2  = t1 * t1;
    double t3  = t2 * t1;
    double t4  = 2.0 * t3;
    double t5  = t2 * v;
    double t6  = 6.0 * t5;
    double t8  = 4.0 * t3;
    double t10 = t2 * w;
    double t11 = 6.0 * t10;
    double t14 = v * v;
    double t15 = t1 * t14;
    double t16 = 6.0 * t15;
    double t17 = t14 * v;
    double t18 = 2.0 * t17;
    double t20 = 12.0 * t5;
    double t22 = t1 * v * w;
    double t23 = 12.0 * t22;
    double t25 = t14 * w;
    double t26 = 6.0 * t25;
    double t27 = 4.0 * t17;
    double t29 = 12.0 * t10;
    double t30 = w * w;
    double t31 = t1 * t30;
    double t32 = 12.0 * t31;
    double t33 = t30 * w;
    double t34 = 2.0 * t33;
    double t35 = 24.0 * t5;
    double t36 = 48.0 * t22;
    double t37 = v * t30;
    double t38 = 12.0 * t37;
    double t39 = 24.0 * t15;
    double t40 = 18.0 * t25;
    double t42 = 24.0 * t10;
    double t43 = 24.0 * t31;
    double t44 = 4.0 * t33;
    double t45 = 18.0 * t37;
    double t46 = 12.0 * t15;
    double t47 = 12.0 * t25;
    double t50 = 6.0 * t37;
    double t52 = 6.0 * t31;

    DN[0][0]  = -t4 - t6;
    DN[0][1]  = -t8 - t6;
    DN[1][0]  = -t8 - t11;
    DN[1][1]  = -t4 - t11;
    DN[2][0]  = t4 + t6 - t16 - t18;
    DN[2][1]  = -t4 - t11 - t20 - t23 - 18.0*t15 - t26 - t27;
    DN[3][0]  = -t29 - t32 - t34 - t35 - t36 - t38 - t39 - t40 - t27;
    DN[3][1]  = -t42 - t43 - t44 - t20 - t36 - t45 - t46 - t47 - t18;
    DN[4][0]  = -t4 - t29 - 18.0*t31 - t44 - t6 - t23 - t50;
    DN[4][1]  = t4 + t11 - t52 - t34;
    DN[5][0]  = t18 + t16;
    DN[5][1]  = -t18;
    DN[6][0]  = t8 + 18.0*t10 + t32 + t34 + t35 + t36 + t38 + t39 + t47;
    DN[6][1]  = t4 + t11 - t52 - t34 + t20 - t38 + t46 - t47;
    DN[7][0]  = t4 + t29 + t32 + t6 - t38 - t16 - t47 - t18;
    DN[7][1]  = t8 + t42 + t43 + 18.0*t5 + t36 + t38 + t46 + t47 + t18;
    DN[8][0]  = -t34;
    DN[8][1]  = t34 + t52;
    DN[9][0]  = t26 + t27;
    DN[9][1]  = t18;
    DN[10][0] = t44 + t45 + t52 + t47 + t23 + t18 + t16;
    DN[10][1] = t34 + t52 + t38 + t23 + t40 + t16 + t27;
    DN[11][0] = t34;
    DN[11][1] = t44 + t50;
    for (auto& row : DN) {
        row[0] /= 12.0;
        row[1] /= 12.0;
    }
}

void DDBSpline(std::array<std::array<double,3>,12>& DDN, double v, double w)
{
    validate_parametric_coordinates(v, w);
    double t1  = 1.0 - v - w;
    double t2  = t1 * v;
    double t3  = 12.0 * t2;
    double t4  = t1 * t1;
    double t6  = 6.0 * t4;
    double t8  = t1 * w;
    double t10 = 12.0 * t8;
    double t12 = 24.0 * t2;
    double t13 = v * w;
    double t14 = v * v;
    double t15 = t8 + t2 + t13 + t14;
    double t16 = 6.0 * t14;
    double t18 = 24.0 * t8;
    double t19 = 24.0 * t4;
    double t20 = 12.0 * t13;
    double t21 = 12.0 * t14;
    double t23 = w * w;
    double t24 = 12.0 * t23;
    double t26 = 12.0 * t4;
    double t27 = 6.0 * t23;
    double t28 = 24.0 * t13;
    double t30 = t8 + t23 + t2 + t13;

    DDN[0][0]  = t3;
    DDN[0][1]  = 12.0*t4 + 12.0*t2;
    DDN[0][2]  = t6 + t3;
    DDN[1][0]  = 12.0*t4 + 12.0*t8;
    DDN[1][1]  = t10;
    DDN[1][2]  = t6 + t10;
    DDN[2][0]  = -t12;
    DDN[2][1]  = 12.0*t15;
    DDN[2][2]  = -t6 - t3 + t16;
    DDN[3][0]  = -t18 - t19 + t20 + t21;
    DDN[3][1]  = -t19 + t24 - t12 + t20;
    DDN[3][2]  = -t26 + t27 + t28 + t16;
    DDN[4][0]  = 12.0*t30;
    DDN[4][1]  = -t18;
    DDN[4][2]  = -t6 - t10 + t27;
    DDN[5][0]  = t3;
    DDN[5][1]  = 0.0;
    DDN[5][2]  = -t16;
    DDN[6][0]  = t26 + t10 - t28 - 24.0*t14;
    DDN[6][1]  = -24.0*t15;
    DDN[6][2]  = t6 - t10 - t27 - t28 - t21;
    DDN[7][0]  = -24.0*t30;
    DDN[7][1]  = t26 - 24.0*t23 + t3 - t28;
    DDN[7][2]  = t6 - t24 - t3 - t28 - t16;
    DDN[8][0]  = 0.0;
    DDN[8][1]  = t10;
    DDN[8][2]  = -t27;
    DDN[9][0]  = 12.0*t13 + 12.0*t14;
    DDN[9][1]  = 0.0;
    DDN[9][2]  = t16;
    DDN[10][0] = 12.0*t30;
    DDN[10][1] = 12.0*t15;
    DDN[10][2] = t27 + t28 + t10 + t16 + t3;
    DDN[11][0] = 0.0;
    DDN[11][1] = 12.0*t23 + 12.0*t13;
    DDN[11][2] = t27;
    for (auto& row : DDN) {
        row[0] /= 12.0;
        row[1] /= 12.0;
        row[2] /= 12.0;
    }
}

} // namespace fce
