#include "fce/lbfgs.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <utility>
#include <vector>

namespace {

// Simple quadratic: f(x) = ||x||^2, grad = 2*x.
auto quadratic_callback(const std::vector<double>& x)
    -> std::pair<double, std::vector<double>>
{
    double f = 0.0;
    std::vector<double> g(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        f += x[i] * x[i];
        g[i] = 2.0 * x[i];
    }
    return {f, g};
}

// Rosenbrock function: f(x,y) = (1-x)^2 + 100*(y-x^2)^2.
auto rosenbrock_callback(const std::vector<double>& x)
    -> std::pair<double, std::vector<double>>
{
    const double a = 1.0, b = 100.0;
    const double xv = x[0], yv = x[1];
    const double f = (a - xv)*(a - xv) + b*(yv - xv*xv)*(yv - xv*xv);
    const double gx = -2.0*(a - xv) + b*2.0*(yv - xv*xv)*(-2.0*xv);
    const double gy = b*2.0*(yv - xv*xv);
    return {f, {gx, gy}};
}

}  // namespace

TEST(LbfgsConvergesOnQuadratic, SmallDimension) {
    // f(x) = ||x||^2 has minimum at x=0 with f=0.
    const int n = 5;
    std::vector<double> x(static_cast<std::size_t>(n), 1.0);
    const double xnorm0 = std::sqrt(static_cast<double>(n));  // bbox diagonal from [0,1]^n

    fce::LbfgsSolver solver(10, 1.0e-8, 1.0e-12, 20000);
    const int flag = solver.minimize(x, xnorm0, /*stop_on_first_trial=*/false, quadratic_callback);

    EXPECT_LE(flag, 0) << "solver did not converge (flag=" << flag << ")";
    EXPECT_LT(solver.gnorm(), 1.0e-6) << "gradient norm too large at convergence";

    for (int i = 0; i < n; ++i) {
        EXPECT_NEAR(x[static_cast<std::size_t>(i)], 0.0, 1.0e-6)
            << "x[" << i << "] not near zero";
    }
}

TEST(LbfgsConvergesOnQuadratic, LargerDimension) {
    const int n = 50;
    std::vector<double> x(static_cast<std::size_t>(n), 1.0);
    const double xnorm0 = std::sqrt(static_cast<double>(n));

    fce::LbfgsSolver solver(10, 1.0e-8, 1.0e-12, 20000);
    const int flag = solver.minimize(x, xnorm0, /*stop_on_first_trial=*/false, quadratic_callback);

    EXPECT_LE(flag, 0) << "solver did not converge";
    EXPECT_LT(solver.gnorm(), 1.0e-5);

    double norm2 = 0.0;
    for (auto v : x) norm2 += v * v;
    EXPECT_LT(std::sqrt(norm2), 1.0e-5) << "solution not near zero";
}

TEST(LbfgsResetClearsHistory, TwoSequentialMinimizations) {
    // After reset(), second call must converge independently.
    const int n = 3;
    fce::LbfgsSolver solver(10, 1.0e-8, 1.0e-12, 20000);

    // First minimization.
    std::vector<double> x1(static_cast<std::size_t>(n), 2.0);
    const double xnorm0 = std::sqrt(static_cast<double>(n) * 4.0);
    int flag1 = solver.minimize(x1, xnorm0, /*stop_on_first_trial=*/false, quadratic_callback);
    EXPECT_LE(flag1, 0) << "first minimization did not converge";

    // Reset history.
    solver.reset();

    // Second minimization from a different starting point.
    std::vector<double> x2(static_cast<std::size_t>(n), -3.0);
    int flag2 = solver.minimize(x2, xnorm0, /*stop_on_first_trial=*/false, quadratic_callback);
    EXPECT_LE(flag2, 0) << "second minimization did not converge after reset";
    EXPECT_LT(solver.gnorm(), 1.0e-5) << "gradient not small after second run";
}

TEST(LbfgsConvergesOnRosenbrock, TwoDimensional) {
    // Rosenbrock minimum at (1,1) with f=0.
    std::vector<double> x = {-1.0, 1.0};
    const double xnorm0 = 2.0;

    fce::LbfgsSolver solver(10, 1.0e-8, 1.0e-12, 20000);
    const int flag = solver.minimize(x, xnorm0, /*stop_on_first_trial=*/false, rosenbrock_callback);

    EXPECT_LE(flag, 0) << "Rosenbrock solver did not converge";
    EXPECT_NEAR(x[0], 1.0, 1.0e-4) << "x[0] not near 1";
    EXPECT_NEAR(x[1], 1.0, 1.0e-4) << "x[1] not near 1";
}

TEST(LbfgsStopOnFirstTrial, OnlyTerminatesWhenEnabled) {
    std::vector<double> x_enabled = {1.0, -1.0, 0.5};
    std::vector<double> x_disabled = x_enabled;
    const double xnorm0 = 1.0;

    int enabled_calls = 0;
    int disabled_calls = 0;

    auto callback_enabled =
        [&](const std::vector<double>& x) -> std::pair<double, std::vector<double>> {
            ++enabled_calls;
            return quadratic_callback(x);
        };
    auto callback_disabled =
        [&](const std::vector<double>& x) -> std::pair<double, std::vector<double>> {
            ++disabled_calls;
            return quadratic_callback(x);
        };

    fce::LbfgsSolver enabled_solver(10, 1.0e-8, 1.0e-12, 20000);
    fce::LbfgsSolver disabled_solver(10, 1.0e-8, 1.0e-12, 20000);

    const int enabled_flag =
        enabled_solver.minimize(x_enabled, xnorm0, /*stop_on_first_trial=*/true, callback_enabled);
    const int disabled_flag =
        disabled_solver.minimize(x_disabled, xnorm0, /*stop_on_first_trial=*/false, callback_disabled);

    EXPECT_EQ(enabled_flag, 0);
    EXPECT_EQ(enabled_calls, 1);
    EXPECT_TRUE(enabled_solver.stopped_on_trial_gnorm_gate());

    EXPECT_LE(disabled_flag, 0);
    EXPECT_GT(disabled_calls, 1);
    EXPECT_FALSE(disabled_solver.stopped_on_trial_gnorm_gate());
}

TEST(LbfgsConvergenceCriterion, IncludesPersistedStepNorm) {
    std::vector<double> x(static_cast<std::size_t>(5), 1.0);
    const double xnorm0 = std::sqrt(5.0);

    fce::LbfgsSolver solver(10, 1.0e-8, 1.0e-12, 20000);
    bool captured_first_accept = false;
    double first_crit = 0.0;
    double first_raw_gnorm = 0.0;

    solver.set_accepted_step_observer(
        [&](const int iter,
            const int,
            const double,
            const double critc,
            const double,
            const std::vector<double>&,
            const std::vector<double>&) {
            if (iter == 1 && !captured_first_accept) {
                captured_first_accept = true;
                first_crit = critc;
                first_raw_gnorm = solver.raw_gnorm();
            }
        });

    const int flag = solver.minimize(x, xnorm0, /*stop_on_first_trial=*/false, quadratic_callback);

    EXPECT_LE(flag, 0);
    ASSERT_TRUE(captured_first_accept);
    EXPECT_GT(first_crit, first_raw_gnorm / 100.0 + 0.1);
}
