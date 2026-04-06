#pragma once

#include "fce/simulator.hpp"

#include <string>

namespace fce {

std::string snapshot_filename(int step);

void write_mesh_snapshot(const SimulatorInput& input,
                         const RuntimeState& state,
                         const std::string& output_dir,
                         int step);

void write_mesh_series_index(const std::string& output_dir,
                             const BCData& bcs,
                             int final_step);

}  // namespace fce
