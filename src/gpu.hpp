// GPU domain colouring: compiles an expression tree to a GLSL fragment shader and
// draws it as a full-pane quad from inside the ImGui draw list (macOS / OpenGL 3.2).
#pragma once
#include <string>
#include <vector>

#include "expr.hpp"
#include "imgui.h"

namespace gpu {

// GLSL for f(z), or "" with `why` set when the expression uses something the shader
// can't do (infinite series, numeric derivatives, number-theory functions, ...).
std::string fragmentSource(const eq::NodeP& f, const std::vector<std::string>& params, std::string& why);

struct DomainColorDraw {
    double cx, cy, scale, aspect;  // view (world centre, px per unit)
    float ccx, ccy;                // canvas centre (logical px)
    float alpha;
    float sumLimit;
    int zetaN;                     // Borwein terms for zeta (0 if unused)
    float params[32];
    int nParams;
};

// Queues a GPU draw covering the current clip rect. Returns false (and draws nothing)
// if the expression can't be compiled for the GPU; the caller then uses the CPU path.
bool drawDomainColoring(ImDrawList* dl, const eq::NodeP& f, const std::vector<std::string>& params,
                        const DomainColorDraw& d);
bool available();
void shutdown();

}  // namespace gpu
