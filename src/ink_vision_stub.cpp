// No on-device handwriting recognition outside macOS.
#include "ink_vision.hpp"

namespace inkvision {
bool available() { return false; }
std::vector<Char> readChars(const std::vector<std::vector<ImVec2>>&, int) { return {}; }
std::vector<std::pair<std::string, float>> read(const std::vector<std::vector<ImVec2>>&) { return {}; }
}  // namespace inkvision
