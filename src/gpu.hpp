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
struct FractalDraw {
    float cRe = 0, cIm = 0;  // Julia constant
    int iter = 200;
    bool julia = false;      // z0 = point (else c = point, z0 = 0)
    float density = 1;
    float pal[15] = {};      // five palette stops, RGB 0..1
    float bg[3] = {};        // colour of points that never escape
    bool colorSettled = false;  // colour points that settle on a fixed point by where (Newton's method)
};
// Escape-time fractal of z <- f(z, c), where f uses x for z and the variable c.
std::string fractalSource(const eq::NodeP& f, const std::vector<std::string>& params, std::string& why);
bool drawFractal(ImDrawList* dl, const eq::NodeP& f, const std::vector<std::string>& params, const DomainColorDraw& d,
                 const FractalDraw& fd, std::string& why);
bool available();
void shutdown();

}  // namespace gpu
