// A tiny TeX-flavoured math typesetter that lays out expression trees into
// boxes and renders them with ImDrawList, using an OpenType math font.
#pragma once
#include <map>
#include <string>
#include <vector>

#include "expr.hpp"
#include "imgui.h"

namespace ts {

struct Prim {
    enum Kind { Text, Line, Poly, Fill, Strip } kind = Text;  // Strip: outer/inner pairs
    ImVec2 a{0, 0}, b{0, 0};  // Text: a = baseline-left; Line: endpoints
    float size = 0, thick = 0;
    ImFont* font = nullptr;  // Text: nullptr = the main math font
    std::string s;
    std::vector<ImVec2> pts;
};

// Origin is at the left end of the baseline; y grows downward.
struct Box {
    float w = 0, asc = 0, desc = 0;
    std::vector<Prim> prims;
};

using Rename = std::map<std::string, std::string>;

// mathAlphanumerics: the font has the Unicode math-italic alphabet (e.g. STIX Two Math).
// Otherwise variables are set in `italicFont` (when given) and everything else upright.
void init(ImFont* mathFont, bool mathAlphanumerics, ImFont* italicFont = nullptr);

Box text(const std::string& utf8, float size);        // upright
Box ident(const std::string& name, float size);       // italic variable, Greek + subscripts
Box space(float w);
Box hcat(const std::vector<Box>& boxes);
Box frac(const Box& num, const Box& den, float size);
Box sup(const Box& base, const Box& script, float size);
Box parens(const Box& inner, float size, char open = '(', char close = ')');
Box binop(const char* utf8op, float size);
Box expr(const eq::NodeP& n, float size, const Rename& rename = {});

void draw(ImDrawList* dl, const Box& b, ImVec2 baselineLeft, ImU32 col);

std::string fmtNum(double v, int sig = 4);   // uses U+2212 for minus
std::string greekOrName(const std::string& name);  // "omega" -> "ω", "a_1" -> "a₁" (plain, for UI text)
std::string italicName(const std::string& name);   // same, using math-italic code points

}  // namespace ts
