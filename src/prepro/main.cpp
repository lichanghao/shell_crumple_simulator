// Preprocessor entry point.
#include "fce/preprocessor.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv)
{
    std::string work_dir = ".";
    if (argc >= 2) work_dir = argv[1];

    try {
        fce::run_preprocessor(work_dir);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
