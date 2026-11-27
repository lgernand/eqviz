// Apple Vision text recognition of a single handwritten symbol (macOS); a stub elsewhere.
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "imgui.h"

namespace inkvision {
struct Char {
    std::string text;
    float conf = 0;
    int line = 0, rank = 0;                 // which text line Vision found, and which of its readings
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;  // that line's box, in the strokes' coordinates
};
bool available();
// The characters Vision reads in the ink (best `candidates` readings of each line), with boxes.
std::vector<Char> readChars(const std::vector<std::vector<ImVec2>>& strokes, int candidates = 3);
// Candidate readings of the strokes (strings, confidences 0..1), best first; empty if unavailable.
std::vector<std::pair<std::string, float>> read(const std::vector<std::vector<ImVec2>>& strokes);
}  // namespace inkvision
