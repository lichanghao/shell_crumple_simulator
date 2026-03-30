#pragma once
// Full preprocessor pipeline declaration.

#include <string>

namespace fce {

// Run the full preprocessor pipeline.
// Reads data.dat from the given directory, produces all nano_*.dat files there.
void run_preprocessor(const std::string& work_dir);

} // namespace fce
