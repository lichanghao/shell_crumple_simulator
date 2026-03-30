#pragma once

#include <gtest/gtest.h>
#include <string>

namespace fce::test_support {

::testing::AssertionResult compare_preprocessor_outputs(
    const std::string& actual_dir,
    const std::string& oracle_dir,
    double float_abs_tol = 1e-10);

} // namespace fce::test_support
