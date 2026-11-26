// 3Blue1Brown / manim-inspired palette and easing.
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#include "imgui.h"

namespace theme {

// Palette slots. These are runtime values so themes can swap them (see themes.cpp);
// names follow manim's colour constants, which the default theme uses.
// WHITE is the foreground (ink) colour and BG the background in every theme.
inline int BG = 0x0C0E12;
inline int WHITE = 0xFFFFFF;
inline int LIGHT_GREY = 0xBBBBBB;
inline int GREY = 0x888888;
inline int DARK_GREY = 0x444444;
inline int BLUE_A = 0xC7E9F1;
inline int BLUE_B = 0x9CDCEB;
inline int BLUE_C = 0x58C4DD;   // accent
inline int BLUE_D = 0x29ABCA;   // grid lines
inline int BLUE_E = 0x1C758A;
inline int TEAL_C = 0x5CD0B3;
inline int GREEN_C = 0x83C167;
inline int YELLOW_C = 0xFFFF00;
inline int YELLOW_D = 0xF4D345;
inline int GOLD_C = 0xF0AC5F;
inline int RED_C = 0xFC6255;
inline int MAROON_C = 0xC55F73;
inline int PURPLE_C = 0x9A72AC;
inline int PINK = 0xD147BD;

constexpr int kNumSlots = 19;
inline int* slot(int i) {
    static int* s[kNumSlots] = {&BG, &WHITE, &LIGHT_GREY, &GREY, &DARK_GREY, &BLUE_A, &BLUE_B, &BLUE_C, &BLUE_D, &BLUE_E,
                                &TEAL_C, &GREEN_C, &YELLOW_C, &YELLOW_D, &GOLD_C, &RED_C, &MAROON_C, &PURPLE_C, &PINK};
    return s[i];
}
inline const char* slotName(int i) {
    static const char* n[kNumSlots] = {"Background", "Foreground", "Light grey", "Grey", "Dark grey", "Blue A", "Blue B",
                                       "Accent", "Grid", "Blue E", "Teal", "Green", "Yellow", "Yellow D", "Gold", "Red",
                                       "Maroon", "Purple", "Pink"};
    return n[i];
}

struct Theme {
    const char* name;
    int c[kNumSlots];
};
const std::vector<Theme>& presets();
void apply(const Theme& t);
Theme current(const char* name);

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

inline ImVec4 mixv(int a, int b, float t, float alpha = 1.f) {
    ImVec4 x = colv(a), y = colv(b);
    return ImVec4(x.x + (y.x - x.x) * t, x.y + (y.y - x.y) * t, x.z + (y.z - x.z) * t, alpha);
}
// Panel surfaces are a blend of background and foreground; accented surfaces blend in the accent.
inline ImVec4 surface(float t, float alpha = 1.f) { return mixv(BG, WHITE, t, alpha); }
inline ImVec4 accentSurface(float t, float alpha = 1.f) { return mixv(BG, BLUE_C, t, alpha); }

// manim's `smooth` rate function
inline float smooth(float t) {
    const float inflection = 10.f;
    auto sig = [](float x) { return 1.f / (1.f + std::exp(-x)); };
    float err = sig(-inflection / 2.f);
    return std::clamp((sig(inflection * (t - 0.5f)) - err) / (1.f - 2.f * err), 0.f, 1.f);
}

}  // namespace theme
