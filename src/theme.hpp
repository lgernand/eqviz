// 3Blue1Brown / manim-inspired palette and easing.
#pragma once
#include <algorithm>
#include <cmath>

#include "imgui.h"

namespace theme {

// manim colour constants
constexpr int BG = 0x0C0E12;
constexpr int WHITE = 0xFFFFFF;
constexpr int LIGHT_GREY = 0xBBBBBB;
constexpr int GREY = 0x888888;
constexpr int DARK_GREY = 0x444444;
constexpr int BLUE_A = 0xC7E9F1;
constexpr int BLUE_B = 0x9CDCEB;
constexpr int BLUE_C = 0x58C4DD;
constexpr int BLUE_D = 0x29ABCA;
constexpr int BLUE_E = 0x1C758A;
constexpr int TEAL_C = 0x5CD0B3;
constexpr int GREEN_C = 0x83C167;
constexpr int YELLOW_C = 0xFFFF00;
constexpr int YELLOW_D = 0xF4D345;
constexpr int GOLD_C = 0xF0AC5F;
constexpr int RED_C = 0xFC6255;
constexpr int MAROON_C = 0xC55F73;
constexpr int PURPLE_C = 0x9A72AC;
constexpr int PINK = 0xD147BD;

inline ImU32 col(int hex, float a = 1.f) {
    a = std::clamp(a, 0.f, 1.f);
    return IM_COL32((hex >> 16) & 255, (hex >> 8) & 255, hex & 255, (int)(a * 255.f + 0.5f));
}
inline ImVec4 colv(int hex, float a = 1.f) {
    return ImVec4(((hex >> 16) & 255) / 255.f, ((hex >> 8) & 255) / 255.f, (hex & 255) / 255.f, a);
}
inline ImU32 withAlpha(ImU32 c, float a) {
    int al = (int)(((c >> IM_COL32_A_SHIFT) & 0xFF) * std::clamp(a, 0.f, 1.f) + 0.5f);
    return (c & ~IM_COL32_A_MASK) | ((ImU32)al << IM_COL32_A_SHIFT);
}
inline ImU32 lerpCol(ImU32 a, ImU32 b, float t) {
    auto ch = [&](int s) {
        float x = (float)((a >> s) & 0xFF), y = (float)((b >> s) & 0xFF);
        return (ImU32)(x + (y - x) * t + 0.5f) << s;
    };
    return ch(IM_COL32_R_SHIFT) | ch(IM_COL32_G_SHIFT) | ch(IM_COL32_B_SHIFT) | ch(IM_COL32_A_SHIFT);
}
inline ImU32 shade(ImU32 c, float k) {
    auto ch = [&](int s) { return (ImU32)std::min(255.f, ((c >> s) & 0xFF) * k) << s; };
    return ch(IM_COL32_R_SHIFT) | ch(IM_COL32_G_SHIFT) | ch(IM_COL32_B_SHIFT) | (c & IM_COL32_A_MASK);
}
inline ImU32 hsv(float h, float s, float v, float a = 1.f) {
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(h - std::floor(h), s, v, r, g, b);
    return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(std::clamp(a, 0.f, 1.f) * 255));
}

// manim's `smooth` rate function
inline float smooth(float t) {
    const float inflection = 10.f;
    auto sig = [](float x) { return 1.f / (1.f + std::exp(-x)); };
    float err = sig(-inflection / 2.f);
    return std::clamp((sig(inflection * (t - 0.5f)) - err) / (1.f - 2.f * err), 0.f, 1.f);
}

}  // namespace theme
