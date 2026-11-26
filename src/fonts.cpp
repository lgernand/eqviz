// Font choices for the interface and for typeset equations (macOS system fonts).
#include <cstdio>
#include <map>

#include "app.hpp"

static const char* kMathFallback = "/System/Library/Fonts/Supplemental/STIXTwoMath.otf";

const std::vector<FontChoice>& uiFontChoices() {
    static const std::vector<FontChoice> v = {
        {"Avenir Next", "/System/Library/Fonts/Avenir Next.ttc", 5, nullptr, 2, false},
        {"Helvetica Neue", "/System/Library/Fonts/HelveticaNeue.ttc", 0, nullptr, 1, false},
        {"Charter (serif)", "/System/Library/Fonts/Supplemental/Charter.ttc", 0, nullptr, 3, false},
        {"Iowan Old Style (serif)", "/System/Library/Fonts/Supplemental/Iowan Old Style.ttc", 0, nullptr, 1, false},
        {"New York (serif)", "/System/Library/Fonts/NewYork.ttf", 0, nullptr, 0, false},
        {"Menlo (monospace)", "/System/Library/Fonts/Menlo.ttc", 0, nullptr, 1, false},
    };
    return v;
}

const std::vector<FontChoice>& mathFontChoices() {
    static const std::vector<FontChoice> v = {
        {"STIX Two (TeX-like)", "/System/Library/Fonts/Supplemental/STIXTwoMath.otf", 0, nullptr, 0, true},
        {"Charter", "/System/Library/Fonts/Supplemental/Charter.ttc", 0, nullptr, 1, false},
        {"Palatino", "/System/Library/Fonts/Palatino.ttc", 0, nullptr, 1, false},
        {"Baskerville", "/System/Library/Fonts/Supplemental/Baskerville.ttc", 0, nullptr, 2, false},
        {"Iowan Old Style", "/System/Library/Fonts/Supplemental/Iowan Old Style.ttc", 0, nullptr, 2, false},
        {"New York", "/System/Library/Fonts/NewYork.ttf", 0, "/System/Library/Fonts/NewYorkItalic.ttf", 0, false},
        {"Georgia", "/System/Library/Fonts/Supplemental/Georgia.ttf", 0, "/System/Library/Fonts/Supplemental/Georgia Italic.ttf", 0, false},
        {"Times New Roman", "/System/Library/Fonts/Supplemental/Times New Roman.ttf", 0,
         "/System/Library/Fonts/Supplemental/Times New Roman Italic.ttf", 0, false},
        {"Avenir Next (sans)", "/System/Library/Fonts/Avenir Next.ttc", 7, nullptr, 4, false},
    };
    return v;
}

static bool exists(const char* p) {
    FILE* f = std::fopen(p, "rb");
    if (!f) return false;
    std::fclose(f);
    return true;
}

// Loads a face, with STIX Two Math merged in for symbols and Greek the face lacks.
static ImFont* loadFace(const char* path, int face, float size) {
    if (!path || !exists(path)) return nullptr;
    ImFontConfig cfg;
    cfg.FontNo = face;
    cfg.OversampleH = 2;
    ImFont* f = ImGui::GetIO().Fonts->AddFontFromFileTTF(path, size, &cfg);
    if (f && exists(kMathFallback) && std::string(path) != kMathFallback) {
        ImFontConfig m;
        m.MergeMode = true;
        ImGui::GetIO().Fonts->AddFontFromFileTTF(kMathFallback, size, &m);
    }
    return f;
}

bool loadUiFont(int index, Fonts& f) {
    static std::map<int, std::pair<ImFont*, ImFont*>> cache;
    const auto& v = uiFontChoices();
    if (index < 0 || index >= (int)v.size()) return false;
    auto it = cache.find(index);
    if (it == cache.end()) {
        const FontChoice& c = v[index];
        ImFont* reg = loadFace(c.path, c.face, 16.f);
        ImFont* bold = reg ? loadFace(c.altPath ? c.altPath : c.path, c.altFace, 16.f) : nullptr;
        if (!reg) return false;
        it = cache.emplace(index, std::make_pair(reg, bold ? bold : reg)).first;
    }
    f.ui = it->second.first;
    f.uiBold = it->second.second;
    return true;
}

bool loadMathFont(int index, Fonts& f) {
    static std::map<int, std::pair<ImFont*, ImFont*>> cache;
    const auto& v = mathFontChoices();
    if (index < 0 || index >= (int)v.size()) return false;
    auto it = cache.find(index);
    if (it == cache.end()) {
        const FontChoice& c = v[index];
        ImFont* reg = loadFace(c.path, c.face, 24.f);
        if (!reg) return false;
        ImFont* ital = c.mathAlnum ? nullptr : loadFace(c.altPath ? c.altPath : c.path, c.altFace, 24.f);
        it = cache.emplace(index, std::make_pair(reg, ital)).first;
    }
    f.math = it->second.first;
    f.mathItalic = it->second.second;
    f.mathAlnum = v[index].mathAlnum;
    return true;
}
