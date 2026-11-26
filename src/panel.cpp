// Control panel (Dear ImGui) and on-canvas equation labels.
#include <algorithm>
#include <cstdio>

#include "app.hpp"
#include "theme.hpp"

using namespace theme;
static const double PI = 3.14159265358979323846;

void App::setupStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding = ImVec2(20, 18);
    s.FramePadding = ImVec2(10, 7);
    s.ItemSpacing = ImVec2(8, 8);
    s.ItemInnerSpacing = ImVec2(8, 6);
    s.WindowRounding = 0;
    s.FrameRounding = 7;
    s.GrabRounding = 7;
    s.PopupRounding = 8;
    s.ScrollbarRounding = 8;
    s.WindowBorderSize = 0;
    s.FrameBorderSize = 0;
    s.PopupBorderSize = 1;
    s.GrabMinSize = 14;
    s.ScrollbarSize = 9;
    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.95f, 1);
    c[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.54f, 0.60f, 1);
    c[ImGuiCol_WindowBg] = ImVec4(0.063f, 0.071f, 0.090f, 0.98f);
    c[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.10f, 0.125f, 0.99f);
    c[ImGuiCol_Border] = ImVec4(1, 1, 1, 0.08f);
    c[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.135f, 0.165f, 1);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.155f, 0.175f, 0.215f, 1);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.205f, 0.255f, 1);
    c[ImGuiCol_Button] = ImVec4(0.135f, 0.15f, 0.185f, 1);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.205f, 0.25f, 1);
    c[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.25f, 0.31f, 1);
    c[ImGuiCol_SliderGrab] = colv(BLUE_C);
    c[ImGuiCol_SliderGrabActive] = colv(BLUE_B);
    c[ImGuiCol_CheckMark] = colv(BLUE_C);
    c[ImGuiCol_Header] = ImVec4(0.15f, 0.17f, 0.21f, 1);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.19f, 0.22f, 0.27f, 1);
    c[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.25f, 0.31f, 1);
    c[ImGuiCol_Separator] = ImVec4(1, 1, 1, 0.08f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(1, 1, 1, 0.12f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1, 1, 1, 0.2f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(1, 1, 1, 0.28f);
    c[ImGuiCol_TextSelectedBg] = colv(BLUE_D, 0.35f);
    c[ImGuiCol_PlotHistogram] = colv(BLUE_C);
}

// ------------------------------------------------------------------ widgets

namespace {

enum class Icon { Play, Pause, Step, Reset, End };

void pushAccent(int hex) {
    ImVec4 a = colv(hex);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(a.x * 0.42f, a.y * 0.42f, a.z * 0.42f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(a.x * 0.55f, a.y * 0.55f, a.z * 0.55f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(a.x * 0.68f, a.y * 0.68f, a.z * 0.68f, 1));
}

bool iconButton(const char* id, Icon ic, ImVec2 size, bool accent = false) {
    if (accent) pushAccent(BLUE_C);
    bool r = ImGui::Button(id, size);
    if (accent) ImGui::PopStyleColor(3);
    ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
    ImVec2 c((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
    float s = std::min(mx.x - mn.x, mx.y - mn.y) * 0.22f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 w = col(WHITE, 0.92f);
    auto tri = [&](float x0, float dir) {
        dl->AddTriangleFilled(ImVec2(x0 - dir * s * 0.75f, c.y - s), ImVec2(x0 - dir * s * 0.75f, c.y + s), ImVec2(x0 + dir * s * 0.9f, c.y), w);
    };
    switch (ic) {
        case Icon::Play: tri(c.x + s * 0.1f, 1); break;
        case Icon::Pause:
            dl->AddRectFilled(ImVec2(c.x - s * 0.8f, c.y - s), ImVec2(c.x - s * 0.25f, c.y + s), w, 1.5f);
            dl->AddRectFilled(ImVec2(c.x + s * 0.25f, c.y - s), ImVec2(c.x + s * 0.8f, c.y + s), w, 1.5f);
            break;
        case Icon::Step:
            tri(c.x - s * 0.25f, 1);
            dl->AddRectFilled(ImVec2(c.x + s * 0.75f, c.y - s), ImVec2(c.x + s * 1.1f, c.y + s), w, 1.f);
            break;
        case Icon::Reset:
            dl->AddRectFilled(ImVec2(c.x - s * 1.1f, c.y - s), ImVec2(c.x - s * 0.75f, c.y + s), w, 1.f);
            tri(c.x + s * 0.25f, -1);
            break;
        case Icon::End:
            tri(c.x - s * 0.6f, 1);
            tri(c.x + s * 0.6f, 1);
            break;
    }
    return r;
}

bool segmented(const char* a, const char* b, bool second, int accent) {
    float w = ImGui::GetContentRegionAvail().x, half = (w - 6) * 0.5f;
    bool clicked = false;
    for (int i = 0; i < 2; ++i) {
        bool sel = (i == 1) == second;
        if (i) ImGui::SameLine(0, 6);
        if (sel) pushAccent(accent);
        if (ImGui::Button(i ? b : a, ImVec2(half, 36)) && !sel) clicked = true;
        if (sel) ImGui::PopStyleColor(3);
    }
    return clicked;
}

}  // namespace

static void sectionHeader(const char* label, ImFont* bold) {
    ImGui::Dummy(ImVec2(0, 8));
    ImGui::PushFont(bold, 12.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, colv(BLUE_C, 0.9f));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y - 3), ImVec2(p.x + w, p.y - 3), col(WHITE, 0.07f));
    ImGui::Dummy(ImVec2(0, 1));
}

static void labelRow(const char* label, float labelW = 120.f) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(-1);
}

// ------------------------------------------------------------------ panel

void App::drawPanel() {
    ImGuiIO& io = ImGui::GetIO();
    if (panelSlide_ > 0.5f) {  // collapsed: a small floating button brings the panel back
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("##expand", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.09f, 0.10f, 0.125f, 0.92f));
        ImGui::PushFont(fonts_.ui, 18.f);
        if (ImGui::Button("\xE2\x98\xB0", ImVec2(40, 40))) togglePanel();
        ImGui::PopFont();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::SetItemTooltip("Show the control panel (Tab)");
        ImGui::End();
    }
    if (panelSlide_ >= 0.999f) return;
    ImGui::SetNextWindowPos(ImVec2(panelW_ - panelFullW_, 0));
    ImGui::SetNextWindowSize(ImVec2(panelFullW_, io.DisplaySize.y));
    ImGui::Begin("##panel", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PushFont(fonts_.ui, 15.5f);

    // ---- title
    {
        ImDrawList* wdl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        ts::Box sig = ts::text("\xE2\x88\x91", 30);
        ts::draw(wdl, sig, ImVec2(p.x, p.y + 27), col(BLUE_C));
        ImGui::SetCursorScreenPos(ImVec2(p.x + sig.w + 10, p.y));
        ImGui::BeginGroup();
        ImGui::PushFont(fonts_.uiBold, 20.f);
        ImGui::TextUnformatted("Equation Visualizer");
        ImGui::PopFont();
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::PushFont(nullptr, 13.f);
        ImGui::TextUnformatted("functions, derivatives & series");
        ImGui::PopFont();
        ImGui::PopStyleColor();
        ImGui::EndGroup();
        // collapse button in the top-right corner
        const float bs = 30;
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - bs - 14, p.y));
        if (ImGui::Button("\xC2\xAB", ImVec2(bs, bs))) togglePanel();
        ImGui::SetItemTooltip("Hide the control panel (Tab)");
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 44));
    }

    // ---- project & functions
    sectionHeader("PROJECT", fonts_.uiBold);
    drawProjectSection();
    sectionHeader("FUNCTIONS", fonts_.uiBold);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));
    ImGui::PushFont(nullptr, 16.f);
    drawFunctionList();
    ImGui::PopFont();
    ImGui::PopStyleVar();

    // ---- view toggles
    sectionHeader(panes_.size() > 1 ? "VIEW  \xC2\xB7  FOCUSED PANE" : "VIEW", fonts_.uiBold);
    {
        // panes: add next to the focused one, close it, let the workspace grow past the window
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Add pane");
        ImGui::SameLine(84);
        const char* labels[] = {"\xE2\x86\x90", "\xE2\x86\x92", "\xE2\x86\x91", "\xE2\x86\x93"};
        const char* tips[] = {"Add a pane to the left of this one", "Add a pane to the right of this one",
                              "Add a pane above this one", "Add a pane below this one"};
        const Side sides[] = {kLeft, kRight, kAbove, kBelow};
        float avail = ImGui::GetContentRegionAvail().x;
        float bw = (avail - 3 * 6 - (panes_.size() > 1 ? 70 + 6 : 0)) / 4;
        for (int k = 0; k < 4; ++k) {
            if (k) ImGui::SameLine(0, 6);
            ImGui::PushID(k);
            if (ImGui::Button(labels[k], ImVec2(bw, 0))) addPane(sides[k]);
            ImGui::SetItemTooltip("%s", tips[k]);
            ImGui::PopID();
        }
        if (panes_.size() > 1) {
            ImGui::SameLine(0, 6);
            if (ImGui::Button("Close", ImVec2(70, 0))) closePane(focus_);
            ImGui::SetItemTooltip("Close this pane");
        }
        ImGui::Checkbox("Let panes extend past the window", &extendWorkspace_);
        if (extendWorkspace_) {
            float half = (ImGui::GetContentRegionAvail().x - 6) * 0.5f;
            ImGui::SetNextItemWidth(half);
            ImGui::SliderFloat("##wsw", &wsW_, 1.f, 5.f, "width %.1f\xC3\x97");
            ImGui::SameLine(0, 6);
            ImGui::SetNextItemWidth(half);
            ImGui::SliderFloat("##wsh", &wsH_, 1.f, 5.f, "height %.1f\xC3\x97");
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::PushFont(nullptr, 12.5f);
        ImGui::TextWrapped("%s%s", panes_.size() > 1 ? "Click a pane to edit it; drag the gaps between panes to resize. " : "",
                           extendWorkspace_ ? "Scroll with two fingers on a trackpad, or click and drag the scrollbars." : "");
        ImGui::PopFont();
        ImGui::PopStyleColor();
    }
    if (segmented("Real axes", "Complex plane", fadeStage_ ? pendingComplex_ : complex_, BLUE_C)) toggleComplex();
    bool showing3D = dimAnimating_ ? dimTo3D_ : threeD_;
    if (segmented("2D", "3D", showing3D, BLUE_C)) toggle3D();
    {
        const char* lbl = deriv_ ? (complex_ ? "Showing f\xE2\x80\xB2(z)  \xC2\xB7  click to hide" : "Hide first derivative")
                                 : "Show first derivative  f\xE2\x80\xB2";
        if (deriv_) pushAccent(GREEN_C);
        if (ImGui::Button(lbl, ImVec2(-1, 36))) toggleDeriv();
        if (deriv_) ImGui::PopStyleColor(3);
    }

    // ---- variables
    if (!params_.empty()) {
        sectionHeader("VARIABLES", fonts_.uiBold);
        ImDrawList* wdl = ImGui::GetWindowDrawList();
        for (size_t i = 0; i < params_.size(); ++i) {
            Param& p = params_[i];
            ImGui::PushID((int)i);
            float fh = ImGui::GetFrameHeight();
            if (iconButton("##play", p.playing ? Icon::Pause : Icon::Play, ImVec2(fh, fh))) {
                p.playing = !p.playing;
                if (p.playing) {
                    double u = std::clamp((p.value - p.min) / std::max(1e-9f, p.max - p.min), 0.f, 1.f);
                    p.phase = std::acos(1 - 2 * u) / (2 * PI);
                }
            }
            ImGui::SetItemTooltip("Animate %s back and forth", ts::greekOrName(p.name).c_str());
            ImGui::SameLine();
            ImVec2 np = ImGui::GetCursorScreenPos();
            ts::Box nb = ts::ident(p.name, 21);
            float nameW = std::max(34.f, nb.w + 8);
            ts::draw(wdl, nb, ImVec2(np.x + 2, np.y + fh * 0.5f + 7), col(WHITE));
            ImGui::Dummy(ImVec2(nameW, fh));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1);
            if (p.integer) {
                int v = (int)std::lround(p.value);
                if (ImGui::SliderInt("##v", &v, (int)p.min, (int)p.max)) { p.value = (float)v; p.playing = false; }
            } else if (ImGui::SliderFloat("##v", &p.value, p.min, p.max, "%.3f")) {
                p.playing = false;
            }
            if (editRanges_) {
                ImGui::Indent(fh + 8);
                float w = (ImGui::GetContentRegionAvail().x - 6) * 0.5f;
                ImGui::SetNextItemWidth(w);
                ImGui::DragFloat("##min", &p.min, 0.05f, -1e6f, p.max - 1e-3f, "min %.2f");
                ImGui::SameLine(0, 6);
                ImGui::SetNextItemWidth(w);
                ImGui::DragFloat("##max", &p.max, 0.05f, p.min + 1e-3f, 1e6f, "max %.2f");
                if (p.integer) { p.min = std::round(p.min); p.max = std::max(p.min + 1, std::round(p.max)); }
                ImGui::Unindent(fh + 8);
            }
            ImGui::PopID();
        }
        ImGui::Checkbox("Edit slider ranges", &editRanges_);
    }

    // ---- summation player
    if (hasSum_) {
        sectionHeader(sumInfinite_ ? "INFINITE SERIES" : "SUMMATION", fonts_.uiBold);
        double lim = std::min<double>(sumLimit(), sumCount_);
        int shown = (int)std::floor(lim + 1e-9);
        ImGui::Text(sumInfinite_ && shown >= sumCount_ ? "Converged to the limit" : "Terms added");
        char buf[64];
        if (!sumInfinite_) snprintf(buf, sizeof buf, "%d / %d", shown, sumCount_);
        else if (shown >= sumCount_) snprintf(buf, sizeof buf, "\xE2\x88\x9E");
        else snprintf(buf, sizeof buf, "%d / %d  \xE2\x86\x92 \xE2\x88\x9E", shown, sumCount_ - 1);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - ImGui::CalcTextSize(buf).x - ImGui::GetStyle().ItemSpacing.x);
        ImGui::TextUnformatted(buf);
        {
            ImVec2 p = ImGui::GetCursorScreenPos();
            float w = ImGui::GetContentRegionAvail().x;
            ImDrawList* wdl = ImGui::GetWindowDrawList();
            wdl->AddRectFilled(p, ImVec2(p.x + w, p.y + 6), col(WHITE, 0.08f), 3);
            float fr = sumCount_ > 0 ? (float)(lim / sumCount_) : 0.f;
            wdl->AddRectFilled(p, ImVec2(p.x + w * std::clamp(fr, 0.f, 1.f), p.y + 6), col(BLUE_C), 3);
            ImGui::Dummy(ImVec2(w, 8));
        }
        float bw = (ImGui::GetContentRegionAvail().x - 3 * 6) / 4;
        if (iconButton("##reset", Icon::Reset, ImVec2(bw, 34))) {
            sumPos_ = 0; sumPlaying_ = false; sumShowAll_ = false; sumStopAt_ = -1; sumHold_ = 0;
        }
        ImGui::SetItemTooltip("Back to zero terms");
        ImGui::SameLine(0, 6);
        if (iconButton("##play", sumPlaying_ ? Icon::Pause : Icon::Play, ImVec2(bw, 34), true)) {
            if (sumPlaying_) sumPlaying_ = false;
            else {
                if (sumPos_ >= sumCount_ || (sumShowAll_ && !sumPlaying_)) sumPos_ = 0;
                sumShowAll_ = false; sumStopAt_ = -1; sumPlaying_ = true;
            }
        }
        ImGui::SetItemTooltip("Play / pause (Space)");
        ImGui::SameLine(0, 6);
        if (iconButton("##step", Icon::Step, ImVec2(bw, 34))) {
            if (sumShowAll_) { sumShowAll_ = false; sumPos_ = 0; }
            sumStopAt_ = std::floor(sumPos_ + 1e-9) + 1;
            sumPlaying_ = true;
        }
        ImGui::SetItemTooltip("Add one term");
        ImGui::SameLine(0, 6);
        if (iconButton("##all", Icon::End, ImVec2(bw, 34))) { sumPlaying_ = false; sumShowAll_ = true; sumPos_ = sumCount_; }
        ImGui::SetItemTooltip(sumInfinite_ ? "Jump to the limit" : "Show every term");
        labelRow("Loop speed", 100);
        ImGui::SliderFloat("##speed", &sumSpeed_, 0.1f, 60.f, "%.2f terms/s", ImGuiSliderFlags_Logarithmic);
        ImGui::Checkbox("Loop", &sumLoop_);
        ImGui::SameLine(0, 18);
        ImGui::Checkbox("Accelerate", &sumAccel_);
        ImGui::SetItemTooltip("Each new term arrives a little faster than the last");
        if (!complex_ && !threeD_) ImGui::Checkbox("Show previous partial sums", &sumTrail_);
        if (sumInfinite_) {
            labelRow("Animate first", 100);
            if (ImGui::SliderInt("##anim", &animTerms_, 1, 300, "%d terms", ImGuiSliderFlags_Logarithmic)) {
                if (sumShowAll_ && !sumPlaying_) sumPos_ = animTerms_ + 1;
            }
            ImGui::SetItemTooltip("Partial sums shown one at a time before the final step to the limit");
            labelRow("Max terms", 100);
            ImGui::SliderInt("##maxterms", &maxTerms_, 64, 100000, "%d", ImGuiSliderFlags_Logarithmic);
            ImGui::SetItemTooltip("Most terms used when converging the series. Where it has not\n"
                                  "settled by then (or diverges), the graph is left blank.");
        }
    }

    // ---- mode specific options
    sectionHeader(threeD_ ? "SURFACE" : (complex_ ? "COMPLEX PLANE" : "GRAPH"), fonts_.uiBold);
    if (threeD_) {
        labelRow("Domain  \xC2\xB1R", 110);
        if (ImGui::SliderFloat("##dom", &domainR_, 0.5f, 30.f, "%.2f", ImGuiSliderFlags_Logarithmic)) targetDist_ = domainR_ * 4.7;
        labelRow("Height scale", 110);
        float bwA = 56;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - bwA - 6);
        ImGui::SliderFloat("##zs", &zScale_, 0.005f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::SameLine(0, 6);
        if (ImGui::Button("Auto", ImVec2(bwA, 0))) autoZScale();
        labelRow("Resolution", 110);
        ImGui::SliderInt("##res", &surfRes_, 8, 400);
        ImGui::SetItemTooltip("Mesh resolution per side. Above ~200 the surface may get slow while moving the camera.");
        labelRow("Opacity", 110);
        ImGui::SliderFloat("##op", &surfOpacity_, 0.1f, 1.f, "%.2f");
        if (!complex_) {
            labelRow("Colouring", 110);
            const char* modes[] = {"Checkerboard", "By height"};
            ImGui::Combo("##cm", &colorMode_, modes, 2);
        }
        ImGui::Checkbox("Auto-rotate camera", &autoRotate_);
        if (deriv_ && !complex_) {
            labelRow("Point  x\xE2\x82\x80", 110);
            ImGui::SliderFloat("##px", &probeX_, (float)(domainCx_ - domainR_), (float)(domainCx_ + domainR_), "%.3f");
            labelRow("Point  y\xE2\x82\x80", 110);
            ImGui::SliderFloat("##py", &probeY_, (float)(domainCy_ - domainR_), (float)(domainCy_ + domainR_), "%.3f");
        }
    } else if (complex_) {
        labelRow("Transform  t", 110);
        float t = gridT_;
        if (ImGui::SliderFloat("##gt", &t, 0.f, 1.f, "%.2f")) { gridT_ = t; gridAnim_ = false; }
        if (ImGui::Button("Replay transformation", ImVec2(-1, 32))) { gridT_ = 0; gridAnimP_ = 0; gridAnim_ = true; }
        {
            float half = (ImGui::GetContentRegionAvail().x - 6) * 0.5f;
            if (ImGui::Button("Fit grid to view", ImVec2(half, 30))) fitGridToView();
            ImGui::SetItemTooltip("Rebuild the input grid from the region on screen.\n"
                                  "The grid otherwise stays fixed while you zoom and pan.");
            ImGui::SameLine(0, 6);
            float sp = (float)gridStep_;
            ImGui::SetNextItemWidth(half);
            if (ImGui::SliderFloat("##gsp", &sp, 0.001f, 10.f, "spacing %.3g", ImGuiSliderFlags_Logarithmic) && sp > 0) {
                gridStep_ = sp;
                gridHash_ = 0;
            }
            ImGui::SetItemTooltip("Spacing between input grid lines");
        }
        ImGui::Checkbox("Domain colouring", &domainColor_);
        if (domainColor_) {
            labelRow("Opacity", 110);
            ImGui::SliderFloat("##dco", &dcOpacity_, 0.1f, 1.f, "%.2f");
            ImGui::Checkbox("Render on the GPU", &useGpu_);
            ImGui::SetItemTooltip("Compile the function to a GPU shader for full-resolution colouring.\n"
                                  "Falls back to the CPU when it can't (shown below).");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::PushFont(nullptr, 12.5f);
            if (gpuStatus_.empty()) ImGui::TextUnformatted("on GPU");
            else ImGui::Text("on CPU (%s)", gpuStatus_.c_str());
            ImGui::PopFont();
            ImGui::PopStyleColor();
        }
    } else {
        if (deriv_) {
            double x0, x1, yy;
            view_.s2w(canvasMin_, x0, yy);
            view_.s2w(canvasMax_, x1, yy);
            labelRow("Tangent at  x\xE2\x82\x80", 120);
            ImGui::SliderFloat("##px", &probeX_, (float)x0, (float)x1, "%.3f");
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("Turn on the derivative to drag a tangent line along the curve.");
            ImGui::PopStyleColor();
        }
        labelRow("Y stretch", 120);
        float asp = (float)view_.aspect, bw11 = 48;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - bw11 - 6);
        if (ImGui::SliderFloat("##asp", &asp, 0.05f, 50.f, "%.2f\xC3\x97", ImGuiSliderFlags_Logarithmic)) view_.aspect = asp;
        ImGui::SameLine(0, 6);
        if (ImGui::Button("1:1", ImVec2(bw11, 0))) view_.aspect = 1;
    }
    if (ImGui::Button("Reset view", ImVec2(-1, 32))) resetView();

    // ---- help
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::PushFont(nullptr, 13.f);
    ImGui::PushTextWrapPos(0);
    ImGui::TextUnformatted(threeD_ ? "Drag to pan \xC2\xB7 right-drag to orbit \xC2\xB7 pinch or mouse wheel to zoom \xC2\xB7 "
                                     "hover for coordinates"
                                   : "Drag or two-finger scroll to pan \xC2\xB7 pinch or mouse wheel to zoom \xC2\xB7 "
                                     "hover a curve for coordinates");
    ImGui::TextUnformatted("Syntax: 2x, x^2, sin x, e^x, sqrt(x), |x|, n!, pi, i, "
                           "sum(k, 1, n, 1/k^2), prod(k, 1, n, ...), inf. Unknown letters become sliders.");
    ImGui::TextUnformatted("Keys: Tab hide panel \xC2\xB7 Space play sum \xC2\xB7 D derivative \xC2\xB7 C complex \xC2\xB7 3 toggle 3D \xC2\xB7 R reset");
    ImGui::PopTextWrapPos();
    ImGui::PopFont();
    ImGui::PopStyleColor();

    ImGui::PopFont();
    ImGui::End();

    if (panelW_ > 1) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(panelW_, 0), ImVec2(panelW_, io.DisplaySize.y), col(WHITE, 0.07f));
}

// ------------------------------------------------------------------ canvas labels

void App::drawOverlay(ImDrawList* dl) {
    // leave room for the floating "show panel" button when the panel is collapsed
    const bool underButton = panelSlide_ > 0.5f && canvasMin_.x < 8 && canvasMin_.y < 8;
    const float x = canvasMin_.x + (underButton ? 76 : 28);
    float y = canvasMin_.y + 24;
    const float maxW = canvasMax_.x - x - 260;
    const char* prime = "\xE2\x80\xB2";

    auto card = [&](const std::function<ts::Box(float)>& lhs, const eq::NodeP& rhs, const ts::Rename& ren, ImU32 c,
                    float size) {
        ts::Box b = ts::hcat({lhs(size), ts::binop("=", size), ts::expr(rhs, size, ren)});
        if (b.w > maxW && maxW > 50) {
            size = std::max(12.f, size * maxW / b.w);
            b = ts::hcat({lhs(size), ts::binop("=", size), ts::expr(rhs, size, ren)});
        }
        float base = y + 10 + b.asc;
        dl->AddRectFilled(ImVec2(x - 14, y), ImVec2(x + b.w + 14, base + b.desc + 10), col(BG, 0.9f), 10);
        ts::draw(dl, b, ImVec2(x, base), c);
        y = base + b.desc + 10 + 8;
    };
    // "(x)", "(z)", "(x, y)" or the row's own parameter names
    auto argsBox = [&](const FuncDef& d, float s) {
        std::vector<std::string> ps = d.params;
        if (d.autoNamed) {
            if (complex_) ps = {"z"};
            else if (threeD_) ps = {"x", "y"};
        }
        std::vector<ts::Box> parts;
        for (size_t i = 0; i < ps.size(); ++i) {
            if (i) { parts.push_back(ts::text(",", s)); parts.push_back(ts::space(0.2f * s)); }
            parts.push_back(ts::ident(ps[i], s));
        }
        return ts::parens(ts::hcat(parts), s);
    };

    const bool multi = funcs_.size() > 1;
    const bool only2D = !complex_ && !threeD_;
    for (size_t i = 0; i < funcs_.size(); ++i) {
        const FuncDef& d = funcs_[i];
        const bool isActive = (int)i == active_;
        if (!d.body) continue;
        if (!isActive && (!rowVisible(d) || !only2D)) continue;  // complex / 3D views show only the active row
        ts::Rename ren;
        if (complex_ && d.autoNamed) ren["x"] = "z";
        ImU32 c = col(d.color, !isActive && multi ? 0.85f : 1.f);
        if (complex_ && isActive) c = col(WHITE, deriv_ ? 0.55f : 1.f);
        else if (threeD_ && isActive && !complex_ && !multi) c = col(BLUE_C);
        const float size = isActive ? (multi ? 26.f : 30.f) : 22.f;
        if (d.label[0]) {  // a named function shows its name instead of the formula
            ts::Box b = ts::text(d.label, size);
            float base = y + 10 + b.asc;
            dl->AddRectFilled(ImVec2(x - 14, y), ImVec2(x + b.w + 14, base + b.desc + 10), col(BG, 0.9f), 10);
            ts::draw(dl, b, ImVec2(x, base), c);
            y = base + b.desc + 10 + 8;
            continue;
        }
        card([&](float s) { return ts::hcat({ts::ident(d.name, s), argsBox(d, s)}); }, d.body, ren, c, size);
    }

    const FuncDef* a = activeDef();
    if (deriv_ && a && dx_) {
        ts::Rename ren;
        if (complex_ && a->autoNamed) ren["x"] = "z";
        else if (!a->autoNamed) {
            if (!a->params.empty()) ren["x"] = a->params[0];
            if (a->params.size() > 1) ren["y"] = a->params[1];
        }
        const float SD = 24.f;
        if (threeD_ && !complex_) {
            auto partial = [&](const char* v) {
                return [&, v](float s) {
                    return ts::frac(ts::hcat({ts::text("\xE2\x88\x82", s * 0.9f), ts::ident(a->name, s * 0.9f)}),
                                    ts::hcat({ts::text("\xE2\x88\x82", s * 0.9f), ts::ident(v, s * 0.9f)}), s);
                };
            };
            card(partial("x"), dx_, ren, col(RED_C), SD);
            card(partial("y"), dy_, ren, col(GREEN_C), SD);
        } else {
            card([&](float s) { return ts::hcat({ts::ident(a->name, s), ts::text(prime, s), argsBox(*a, s)}); }, dx_, ren,
                 col(GREEN_C), SD);
        }
    }

    if (hasSum_) {
        double lim = std::min<double>(sumLimit(), sumCount_);
        int shown = (int)std::floor(lim + 1e-9);
        const bool atLimit = sumInfinite_ && shown >= sumCount_;
        ts::Box big = ts::text(atLimit ? "\xE2\x88\x9E" : std::to_string(shown), 40);
        ts::Box small = ts::text(sumInfinite_ ? (atLimit ? "" : "\xE2\x86\x92 \xE2\x88\x9E") : "/ " + std::to_string(sumCount_), 22);
        ts::Box lbl = ts::hcat({big, ts::space(8), small});
        ts::Box cap = ts::text(atLimit ? "limit of the series" : "terms", 18);
        float w = std::max(lbl.w, cap.w);
        float rx = canvasMax_.x - 28 - w, ty = canvasMin_.y + 24;
        float base = ty + 12 + lbl.asc;
        float bot = base + 8 + cap.asc + cap.desc + 14;
        dl->AddRectFilled(ImVec2(rx - 16, ty), ImVec2(rx + w + 16, bot), col(BG, 0.8f), 10);
        ts::draw(dl, lbl, ImVec2(rx + w - lbl.w, base), col(WHITE));
        ts::draw(dl, cap, ImVec2(rx + w - cap.w, base + 8 + cap.asc), col(GREY));
        float fr = sumCount_ > 0 ? (float)(lim / sumCount_) : 0.f;
        dl->AddRectFilled(ImVec2(rx - 16, bot - 4), ImVec2(rx - 16 + (w + 32) * fr, bot), col(BLUE_C), 2);
    }
}
