// Appearance: colour themes, fonts, sizes, and the settings file that remembers them.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "app.hpp"
#include "theme.hpp"

using namespace theme;
namespace fs = std::filesystem;

static std::string settingsPath() {
    const char* home = std::getenv("HOME");
    fs::path dir = fs::path(home ? home : ".");
#if defined(__APPLE__)
    dir = dir / "Library" / "Application Support" / "eqviz";
#else
    dir = dir / ".config" / "eqviz";
#endif
    std::error_code ec;
    fs::create_directories(dir, ec);
    return (dir / "settings.txt").string();
}

// Recolour rows that used a palette colour, so curves follow the theme.
static void remapRows(std::vector<FuncDef>& rows, const Theme& from, const Theme& to) {
    for (auto& d : rows)
        for (int i = 0; i < kNumSlots; ++i)
            if (d.color == from.c[i]) { d.color = to.c[i]; break; }
}

void App::selectTheme(int idx) {
    const Theme before = current("before");
    if (idx >= 0 && idx < (int)presets().size()) apply(presets()[idx]);
    else apply(customTheme_);
    themeIdx_ = idx;
    remapRows(funcs_, before, current("after"));
    applyThemeColors();
    saveSettings();
}

void App::setThemeSlot(int s, int color) {
    if (themeIdx_ >= 0) customTheme_ = current("Custom");  // start the custom theme from the current one
    const int old = *slot(s);
    customTheme_.c[s] = color;
    themeIdx_ = -1;
    apply(customTheme_);
    for (auto& d : funcs_)
        if (d.color == old) d.color = color;
    applyThemeColors();
    saveSettings();
}

void App::applyPendingFonts() {
    bool changed = false;
    if (pendingUiFont_ >= 0) {
        if (loadUiFont(pendingUiFont_, fonts_)) { uiFontIdx_ = pendingUiFont_; ImGui::GetIO().FontDefault = fonts_.ui; changed = true; }
        pendingUiFont_ = -1;
    }
    if (pendingMathFont_ >= 0) {
        if (loadMathFont(pendingMathFont_, fonts_)) {
            mathFontIdx_ = pendingMathFont_;
            ts::init(fonts_.math, fonts_.mathAlnum, fonts_.mathItalic);
            changed = true;
        }
        pendingMathFont_ = -1;
    }
    ImGui::GetStyle().FontScaleMain = textScale_;
    if (changed) saveSettings();
}

void App::saveSettings() const {
    if (opt_.frames > 0) return;  // scripted/headless runs never touch the user's settings
    std::ofstream out(settingsPath());
    if (!out) return;
    out << "theme\t" << (themeIdx_ >= 0 ? presets()[themeIdx_].name : "Custom") << "\n";
    out << "custom";
    for (int i = 0; i < kNumSlots; ++i) {
        char b[16];
        snprintf(b, sizeof b, "\t%06X", customTheme_.c[i] & 0xFFFFFF);
        out << b;
    }
    out << "\n";
    out << "ui_font\t" << uiFontChoices()[uiFontIdx_].name << "\n";
    out << "math_font\t" << mathFontChoices()[mathFontIdx_].name << "\n";
    out << "text_scale\t" << textScale_ << "\n";
    out << "label_scale\t" << labelScale_ << "\n";
}

void App::loadSettings() {
    customTheme_ = presets()[0];
    customTheme_.name = "Custom";
    if (opt_.frames > 0) return;  // scripted/headless runs use the defaults
    std::ifstream in(settingsPath());
    std::string line, themeName;
    while (std::getline(in, line)) {
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, '\t')) f.push_back(item);
        if (f.size() < 2) continue;
        try {
            if (f[0] == "theme") themeName = f[1];
            else if (f[0] == "custom" && f.size() > (size_t)kNumSlots)
                for (int i = 0; i < kNumSlots; ++i) customTheme_.c[i] = (int)std::strtol(f[i + 1].c_str(), nullptr, 16);
            else if (f[0] == "ui_font") {
                for (int i = 0; i < (int)uiFontChoices().size(); ++i)
                    if (f[1] == uiFontChoices()[i].name && i != 0) pendingUiFont_ = i;
            } else if (f[0] == "math_font") {
                for (int i = 0; i < (int)mathFontChoices().size(); ++i)
                    if (f[1] == mathFontChoices()[i].name && i != 0) pendingMathFont_ = i;
            } else if (f[0] == "text_scale") textScale_ = std::clamp(std::stof(f[1]), 0.7f, 1.6f);
            else if (f[0] == "label_scale") labelScale_ = std::clamp(std::stof(f[1]), 0.5f, 2.f);
        } catch (...) {
        }
    }
    int idx = 0;
    if (themeName == "Custom") idx = -1;
    for (int i = 0; i < (int)presets().size(); ++i)
        if (themeName == presets()[i].name) idx = i;
    themeIdx_ = idx;
    apply(idx >= 0 ? presets()[idx] : customTheme_);
}

// ------------------------------------------------------------------ panel section

void App::drawAppearanceSection() {
    auto labelRow = [](const char* label, float w = 110.f) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(w);
        ImGui::SetNextItemWidth(-1);
    };

    // theme picker with colour previews
    labelRow("Theme");
    const char* curName = themeIdx_ >= 0 ? presets()[themeIdx_].name : "Custom";
    if (ImGui::BeginCombo("##theme", curName)) {
        for (int i = -1; i < (int)presets().size(); ++i) {
            const Theme& t = i >= 0 ? presets()[i] : customTheme_;
            ImGui::PushID(i);
            ImVec2 p = ImGui::GetCursorScreenPos();
            if (ImGui::Selectable("##t", i == themeIdx_, 0, ImVec2(0, 22))) selectTheme(i);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(ImVec2(p.x, p.y + 2), ImVec2(p.x + 62, p.y + 20), col(t.c[0]), 4);
            const int dots[] = {12, 7, 11, 15, 17};
            for (int k = 0; k < 5; ++k) dl->AddCircleFilled(ImVec2(p.x + 9 + k * 11, p.y + 11), 4, col(t.c[dots[k]]));
            dl->AddText(ImVec2(p.x + 72, p.y + 3), ImGui::GetColorU32(ImGuiCol_Text), i >= 0 ? t.name : "Custom");
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }

    if (ImGui::TreeNode("Customise colours")) {
        struct S { int slot; const char* name; };
        static const S items[] = {{0, "Background"}, {1, "Text & axes"}, {8, "Grid"}, {7, "Accent"}, {9, "Surface shade"},
                                  {12, "Yellow"}, {11, "Green"}, {15, "Red"}, {17, "Purple"}, {14, "Gold"},
                                  {10, "Teal"}, {18, "Pink"}, {3, "Grey"}};
        const int n = (int)(sizeof items / sizeof items[0]);
        const float colX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x * 0.5f;
        for (int k = 0; k < n; ++k) {
            ImGui::PushID(k);
            ImVec4 v = colv(*slot(items[k].slot));
            float c3[3] = {v.x, v.y, v.z};
            if (ImGui::ColorEdit3("##c", c3, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                int hex = ((int)(c3[0] * 255 + 0.5f) << 16) | ((int)(c3[1] * 255 + 0.5f) << 8) | (int)(c3[2] * 255 + 0.5f);
                setThemeSlot(items[k].slot, hex);
            }
            ImGui::SameLine(0, 6);
            ImGui::TextUnformatted(items[k].name);
            if (k % 2 == 0 && k + 1 < n) ImGui::SameLine(colX);
            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    labelRow("Interface font");
    if (ImGui::BeginCombo("##uifont", uiFontChoices()[uiFontIdx_].name)) {
        for (int i = 0; i < (int)uiFontChoices().size(); ++i)
            if (ImGui::Selectable(uiFontChoices()[i].name, i == uiFontIdx_)) pendingUiFont_ = i;
        ImGui::EndCombo();
    }
    labelRow("Equation font");
    if (ImGui::BeginCombo("##mathfont", mathFontChoices()[mathFontIdx_].name)) {
        for (int i = 0; i < (int)mathFontChoices().size(); ++i)
            if (ImGui::Selectable(mathFontChoices()[i].name, i == mathFontIdx_)) pendingMathFont_ = i;
        ImGui::EndCombo();
    }
    {   // live preview of the equation font
        static const eq::NodeP sample = [] {
            auto r = eq::parse("sum(k, 1, n, sin(k omega x) / k^2) + sqrt(pi) e^(-x^2/2)");
            return r.root;
        }();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        ts::Box b = ts::hcat({ts::ident("f", 20), ts::parens(ts::ident("x", 20), 20), ts::binop("=", 20), ts::expr(sample, 20)});
        float s = std::min(1.f, (w - 20) / std::max(1.f, b.w));
        if (s < 1) b = ts::hcat({ts::ident("f", 20 * s), ts::parens(ts::ident("x", 20 * s), 20 * s), ts::binop("=", 20 * s), ts::expr(sample, 20 * s)});
        float h = b.asc + b.desc + 16;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), col(BG), 6);
        ts::draw(dl, b, ImVec2(p.x + 10, p.y + 8 + b.asc), col(YELLOW_C));
        ImGui::Dummy(ImVec2(w, h));
    }
    labelRow("Panel text");
    if (ImGui::SliderFloat("##ts", &textScale_, 0.8f, 1.4f, "%.2f\xC3\x97")) saveSettings();
    labelRow("Equation size");
    if (ImGui::SliderFloat("##ls", &labelScale_, 0.6f, 1.8f, "%.2f\xC3\x97")) saveSettings();
    if (ImGui::Button("Restore default appearance", ImVec2(-1, 30))) {
        textScale_ = labelScale_ = 1.f;
        pendingUiFont_ = 0;
        pendingMathFont_ = 0;
        selectTheme(0);
    }
}
