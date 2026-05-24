#include "utils.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::string readFile(const std::filesystem::path &path) {
    // Binary mode: don't let Windows translate CRLF, so the string length matches
    // the bytes on disk. (Harmless for GLSL text, but the right default for a
    // general file reader.)
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        // Fail loud: a missing shader otherwise silently compiles as empty source.
        throw std::runtime_error("readFile: could not open " + path.string());
    }

    std::ostringstream ss;
    ss << file.rdbuf();  // slurp the whole stream buffer in one shot
    return ss.str();
}
