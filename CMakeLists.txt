cmake_minimum_required(VERSION 3.18)
project(FiniteCrystalElasticity VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# ──────────────────────────────────────────────────────────────────────────────
# Dependencies
# ──────────────────────────────────────────────────────────────────────────────

# MPI
find_package(MPI REQUIRED)

# Eigen3
find_package(Eigen3 REQUIRED NO_MODULE)

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ──────────────────────────────────────────────────────────────────────────────
# Core library (shared by prepro and simulator)
# ──────────────────────────────────────────────────────────────────────────────
add_library(fce_core
  src/core/bspline.cpp
  src/core/constitutive.cpp
  src/core/element_energy.cpp
  src/core/element_state.cpp
  src/core/exponential.cpp
  src/core/geometry.cpp
  src/core/ghost_nodes.cpp
  src/core/io.cpp
  src/core/lbfgs.cpp
  src/core/load_controller.cpp
  src/core/load_pre.cpp
  src/core/mesh_generator.cpp
  src/core/mpi_env.cpp
  src/core/principal.cpp
  src/core/preprocessor.cpp
  src/core/quadrature.cpp
  src/core/reference_config.cpp
  src/core/runtime_output.cpp
  src/core/simulator.cpp
  src/core/solver.cpp
  src/core/taylor.cpp
  src/core/vdw_preprocessor.cpp
)
target_include_directories(fce_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_link_libraries(fce_core PUBLIC
  MPI::MPI_CXX
  Eigen3::Eigen
)

# ──────────────────────────────────────────────────────────────────────────────
# Preprocessor executable
# ──────────────────────────────────────────────────────────────────────────────
add_executable(PrePro
  src/prepro/main.cpp
)
target_link_libraries(PrePro PRIVATE fce_core)

# ──────────────────────────────────────────────────────────────────────────────
# Simulator executable
# ──────────────────────────────────────────────────────────────────────────────
add_executable(crunch_it
  src/simulator/main.cpp
)
target_link_libraries(crunch_it PRIVATE fce_core)

# ──────────────────────────────────────────────────────────────────────────────
# Tests
# ──────────────────────────────────────────────────────────────────────────────
enable_testing()

add_executable(unit_tests
  test/unit/test_bspline.cpp
  test/unit/test_constitutive.cpp
  test/unit/test_element_energy.cpp
  test/unit/test_element_state.cpp
  test/unit/test_exponential.cpp
  test/unit/test_first_constrained_step_oracle.cpp
  test/unit/test_geometry.cpp
  test/unit/test_ghost_nodes.cpp
  test/unit/test_io.cpp
  test/unit/test_lbfgs.cpp
  test/unit/test_load_controller.cpp
  test/unit/test_mpi_env.cpp
  test/unit/test_principal.cpp
  test/unit/test_reference_config.cpp
  test/unit/test_runtime_output.cpp
  test/unit/test_simulator.cpp
)
target_link_libraries(unit_tests PRIVATE
  fce_core
  GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(unit_tests)

add_executable(integration_tests
  test/support/oracle_compare.cpp
  test/integration/test_first_constrained_step_oracle.cpp
  test/integration/test_oracle_roundtrip.cpp
  test/integration/test_prepro_oracle.cpp
  test/integration/test_e2e_compression.cpp
)
target_link_libraries(integration_tests PRIVATE
  fce_core
  GTest::gtest_main
)
target_include_directories(integration_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/test/support
)
gtest_discover_tests(integration_tests
  TEST_LIST integration_test_list
  PROPERTIES TIMEOUT 180
)

# Pass oracle path to tests
target_compile_definitions(integration_tests PRIVATE
  ORACLE_DIR="${CMAKE_SOURCE_DIR}/test/cases"
  CRUNCH_IT_BIN="$<TARGET_FILE:crunch_it>"
)
target_compile_definitions(unit_tests PRIVATE
  ORACLE_DIR="${CMAKE_SOURCE_DIR}/test/cases"
)
