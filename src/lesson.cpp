// Lesson mode: a lesson is a project plus a list of steps. Each step's target is built by
// applying the steps' `set` functions in order to the lesson's starting state, so any step can
// be reached from anywhere. Moving between steps plays a short transition through the same
// keyframe machinery as the timeline: mode switches start at once, views and sliders ease.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "app.hpp"
#include "theme.hpp"

using namespace theme;

namespace {
constexpr double PI = 3.14159265358979323846;
}

// ------------------------------------------------------------------ step context

KeyPane& StepCtx::pane(int i) {
    static KeyPane dummy;
    return i >= 0 && i < (int)k.panes.size() ? k.panes[i] : dummy;
}

StepCtx& StepCtx::param(const std::string& name, double v) {
    for (auto& [n, val] : k.params)
        if (n == name) { val = (float)v; return *this; }
    k.params.push_back({name, (float)v});
    return *this;
}

StepCtx& StepCtx::fit(int i, double x0, double x1, double y0, double y1) {
    if (i < 0 || i >= (int)k.panes.size() || i >= (int)app.paneRects_.size()) return *this;
    KeyPane& p = k.panes[i];
    const ImVec4 r = app.paneRects_[i];
    const double w = std::max(10.f, r.z - r.x), h = std::max(10.f, r.w - r.y);
    const double sx = w / std::max(1e-9, x1 - x0), sy = h / std::max(1e-9, y1 - y0);
    p.cx = 0.5 * (x0 + x1);
    p.cy = 0.5 * (y0 + y1);
    if (p.complex || p.threeD || p.unitCircle || p.matrix) { p.scale = std::min(sx, sy); p.aspect = 1; }
    else { p.scale = sx; p.aspect = sy / sx; }
    if (p.threeD) {
        p.domainR = (float)std::max(x1 - x0, y1 - y0) * 0.5f;
        p.domainCx = p.cx;
        p.domainCy = p.cy;
        p.cam.target = Vec3(p.cx, p.cy, 0);
        p.cam.theta = -PI / 2 + 0.62;
        p.cam.phi = 0.5;
        p.cam.dist = p.domainR * 4.7;
    }
    return *this;
}

StepCtx& StepCtx::show(int i, std::vector<int> rows) {
    KeyPane& p = pane(i);
    p.hidden.clear();
    for (int r = 0; r < (int)app.funcs_.size(); ++r)
        if (std::find(rows.begin(), rows.end(), r) == rows.end()) p.hidden.insert(app.funcs_[r].id);
    return *this;
}

StepCtx& StepCtx::active(int i, int row) {
    pane(i).active = row;
    return *this;
}

StepCtx& StepCtx::matrix(int i, double a, double b, double c, double d) {
    KeyPane& p = pane(i);
    p.mat[0] = a; p.mat[1] = b; p.mat[2] = c; p.mat[3] = d;
    return *this;
}

StepCtx& StepCtx::ask(const std::string& q, const std::string& ans, const std::string& hintText, double tolerance) {
    question = q;
    answer = ans;
    hint = hintText;
    tol = tolerance;
    return *this;
}

// ------------------------------------------------------------------ running a lesson

float App::lessonBarH() const {
    if (lessonIdx_ < 0 || exporting_) return 0.f;
    return lessonQ_.empty() ? 62.f : 104.f;
}

void App::lessonLayout(int graphs, bool stacked, float noteFrac) {
    // pane 0 (the text) on the left; the graphs to its right, side by side or stacked
    nodes_.clear();
    for (int i = 0; i <= graphs; ++i) {
        LayoutNode leaf;
        leaf.pane = i;
        nodes_.push_back(leaf);
    }
    int right = 1;
    if (graphs >= 2) {
        int prev = 1;
        for (int g = 2; g <= graphs; ++g) {
            LayoutNode sp;
            sp.stacked = stacked;
            sp.a = prev;
            sp.b = g;
            sp.ratio = 1.f - 1.f / g;  // equal shares
            nodes_.push_back(sp);
            prev = (int)nodes_.size() - 1;
        }
        right = prev;
    }
    LayoutNode root;
    root.stacked = false;
    root.a = 0;
    root.b = right;
    root.ratio = noteFrac;
    nodes_.push_back(root);
    rootNode_ = (int)nodes_.size() - 1;
    extendWorkspace_ = false;
    wsW_ = wsH_ = 1.f;
    scroll_ = ImVec2(0, 0);
    enterPane(0);
    isNote_ = true;
    noteText_.clear();
    focus_ = std::min(1, (int)panes_.size() - 1);
}

void App::startLesson(int idx) {
    const auto& lib = lessonLibrary();
    if (idx < 0 || idx >= (int)lib.size()) return;
    tlPlaying_ = false;
    tlShown_ = false;
    lib[idx].setup(*this);  // a project: rows, sliders, panes (resets any previous lesson)
    for (int i = 0; i < (int)panes_.size(); ++i) {
        enterPane(i);
        fitPending_ = false;  // the steps place every view
        want3D_ = false;
        gridAnim_ = false;
        gridT_ = 1;
    }
    enterPane(focus_);
    lessonIdx_ = idx;
    lessonBase_ = captureKeyframe(0);
    lessonPending_ = true;  // go to step 0 next frame, once pane sizes are known
    lessonStep_ = 0;
}

void App::exitLesson() {
    lessonIdx_ = -1;
    lessonPlaying_ = false;
    lessonQ_.clear();
}

void App::lessonGo(int step, bool animate) {
    if (lessonIdx_ < 0) return;
    const LessonDef& L = lessonLibrary()[lessonIdx_];
    step = std::clamp(step, 0, (int)L.steps.size() - 1);
    computePaneRects();

    // target = the starting state with steps 0..step applied in order
    Keyframe to = lessonBase_;
    float dur = 1.4f;
    std::vector<std::string> play;
    bool playSum = false;
    std::string q, ans, hint;
    double tol = 0.01;
    for (int j = 0; j <= step; ++j) {
        StepCtx ctx{*this, to};
        if (L.steps[j].set) L.steps[j].set(ctx);
        if (j == step) { dur = ctx.dur; play = ctx.play; playSum = ctx.playSum; q = ctx.question; ans = ctx.answer; hint = ctx.hint; tol = ctx.tol; }
    }
    for (KeyPane& p : to.panes) p.noteScroll = 0;
    to.sumPos = -1;  // the summation player shows the whole sum unless a step plays it
    sumShowAll_ = true;

    // the text
    const int keep = cur_;
    enterPane(0);
    if (isNote_) {
        noteText_ = "# " + L.steps[step].title + "\n\n" + L.steps[step].text;
        noteScroll_ = 0;
    }
    enterPane(keep);

    lessonStep_ = step;
    lessonQ_ = q;
    lessonAns_ = ans;
    lessonHint_ = hint;
    lessonTol_ = tol;
    lessonInput_[0] = 0;
    lessonResult_ = 0;
    lessonShowHint_ = false;
    lessonPlay_ = play;
    lessonPlaySum_ = playSum;
    sumPlaying_ = false;
    for (Param& p : params_) p.playing = false;
    {
        int& best = lessonProgress_[L.id];
        if (step + 1 > best) { best = step + 1; saveSettings(); }
    }

    if (!animate) {
        to.t = 0;
        applyKeys({to}, 0, false);
        lessonKeys_ = {to};  // updateLesson finishes it next frame (starts any sweeps)
        lessonT_ = 1e9;
        lessonPlaying_ = true;
        return;
    }
    Keyframe from = captureKeyframe(0);
    // discrete changes happen at the start of the move: give `from` the target's switches
    for (size_t i = 0; i < from.panes.size() && i < to.panes.size(); ++i) {
        KeyPane& f = from.panes[i];
        const KeyPane& t = to.panes[i];
        if (t.threeD && !f.threeD) { f.cam = t.cam; f.domainR = t.domainR; f.domainCx = t.domainCx; f.domainCy = t.domainCy; f.zScale = t.zScale; }
        if (!t.threeD && f.threeD) { f.cx = t.cx; f.cy = t.cy; f.scale = t.scale; f.aspect = t.aspect; }
        f.complex = t.complex; f.threeD = t.threeD; f.deriv = t.deriv; f.domainColor = t.domainColor;
        f.fractal = t.fractal; f.julia = t.julia; f.unitCircle = t.unitCircle; f.matrix = t.matrix;
        f.circleOpts = t.circleOpts; f.matOpts = t.matOpts; f.trace = t.trace; f.shadeRow = t.shadeRow;
        f.shadeLo = t.shadeLo; f.shadeHi = t.shadeHi; f.active = t.active; f.hidden = t.hidden;
        if (t.matrix && !f.matrix) { for (int m = 0; m < 4; ++m) f.mat[m] = (m == 0 || m == 3) ? 1 : 0; }
    }
    from.t = 0;
    from.ease = 0;
    to.t = std::max(0.05f, dur);
    lessonKeys_ = {from, to};
    lessonT_ = 0;
    lessonPlaying_ = true;
}

void App::updateLesson(float dt) {
    if (lessonIdx_ < 0) return;
    if (lessonPending_) {
        if (visRect_.w - visRect_.y < 50 || visRect_.z - visRect_.x < 50) return;  // window not laid out yet
        lessonPending_ = false;
        lessonGo(lessonStep_, false);
    }
    if (!lessonPlaying_ || lessonKeys_.empty()) return;
    lessonT_ += dt;
    const double end = lessonKeys_.back().t;
    const bool done = lessonT_ >= end;
    applyKeys(lessonKeys_, std::min(lessonT_, end), true);
    if (done) {
        lessonPlaying_ = false;
        for (const std::string& n : lessonPlay_)
            for (Param& p : params_)
                if (p.name == n) {
                    p.playing = true;
                    const double u = std::clamp((p.value - p.min) / std::max(1e-9f, p.max - p.min), 0.f, 1.f);
                    p.phase = std::acos(1 - 2 * u) / (2 * PI);
                }
        if (lessonPlaySum_) { sumPos_ = 0; sumShowAll_ = false; sumPlaying_ = true; }
    }
}

void App::lessonCheck() {
    if (lessonAns_.empty()) return;
    const double want = paneScalar(lessonAns_), got = paneScalar(lessonInput_);
    if (!std::isfinite(got)) { lessonResult_ = -2; return; }
    lessonResult_ = std::fabs(got - want) <= lessonTol_ * std::max(1.0, std::fabs(want)) ? 1 : -1;
}

// ------------------------------------------------------------------ the lesson bar

void App::drawLessonBar() {
    if (lessonIdx_ < 0 || exporting_) return;
    const LessonDef& L = lessonLibrary()[lessonIdx_];
    const int n = (int)L.steps.size();
    ImGuiIO& io = ImGui::GetIO();
    const float x0 = visRect_.x, w = io.DisplaySize.x - x0, h = lessonBarH();
    ImGui::SetNextWindowPos(ImVec2(x0, io.DisplaySize.y - h));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 12));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, surface(0.05f, 0.98f));
    ImGui::Begin("##lessonbar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PushFont(fonts_.ui, 15.f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    {
        ImVec2 wp = ImGui::GetWindowPos();
        dl->AddLine(wp, ImVec2(wp.x + w, wp.y), col(BLUE_C, 0.35f), 2.f);
    }
    auto accent = [](bool on) {
        if (!on) return;
        ImGui::PushStyleColor(ImGuiCol_Button, mixv(BG, BLUE_C, 0.42f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, mixv(BG, BLUE_C, 0.55f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, mixv(BG, BLUE_C, 0.68f));
    };

    ImGui::BeginDisabled(lessonStep_ == 0);
    if (ImGui::Button("\xE2\x97\x80  Back", ImVec2(86, 0))) lessonGo(lessonStep_ - 1, true);
    ImGui::EndDisabled();
    ImGui::SameLine(0, 14);
    {   // progress dots (click to jump)
        const float r = 5.f, gap = 16.f;
        ImVec2 p = ImGui::GetCursorScreenPos();
        const float fh = ImGui::GetFrameHeight();
        const int best = lessonProgress_.count(L.id) ? lessonProgress_[L.id] : 0;
        for (int i = 0; i < n; ++i) {
            ImVec2 c(p.x + r + i * gap, p.y + fh * 0.5f);
            ImGui::SetCursorScreenPos(ImVec2(c.x - gap * 0.5f, p.y));
            ImGui::PushID(i);
            if (ImGui::InvisibleButton("##dot", ImVec2(gap, fh)) && i != lessonStep_) lessonGo(i, true);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%d. %s", i + 1, L.steps[i].title.c_str());
            ImGui::PopID();
            if (i == lessonStep_) dl->AddCircleFilled(c, r + 1.5f, col(BLUE_C));
            else if (i < best) dl->AddCircleFilled(c, r, col(BLUE_C, 0.45f));
            else dl->AddCircle(c, r, col(WHITE, 0.35f), 16, 1.5f);
        }
        ImGui::SetCursorScreenPos(ImVec2(p.x + n * gap + 6, p.y));
        ImGui::Dummy(ImVec2(0, fh));
    }
    ImGui::SameLine(0, 12);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s  \xC2\xB7  %d / %d", L.title.c_str(), lessonStep_ + 1, n);
    ImGui::SameLine(0, 10);
    ImGui::PushFont(fonts_.uiBold, 15.f);
    ImGui::TextUnformatted(L.steps[lessonStep_].title.c_str());
    ImGui::PopFont();

    const bool last = lessonStep_ == n - 1;
    int nextLesson = -1;
    if (last) {
        const auto& lib = lessonLibrary();
        for (int i = lessonIdx_ + 1; i < (int)lib.size(); ++i)
            if (lib[i].subject == L.subject) { nextLesson = i; break; }
    }
    const float nextW = last ? (nextLesson >= 0 ? 150.f : 96.f) : 96.f;
    ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 12, w - 14 - nextW - 8 - 64));
    accent(true);
    if (!last) {
        if (ImGui::Button("Next  \xE2\x96\xB6", ImVec2(nextW, 0))) lessonGo(lessonStep_ + 1, true);
    } else if (nextLesson >= 0) {
        if (ImGui::Button("Next lesson  \xE2\x96\xB6", ImVec2(nextW, 0))) { ImGui::PopStyleColor(3); startLesson(nextLesson); ImGui::PopFont(); ImGui::End(); ImGui::PopStyleColor(); ImGui::PopStyleVar(2); return; }
    } else {
        if (ImGui::Button("Finish", ImVec2(nextW, 0))) exitLesson();
    }
    ImGui::PopStyleColor(3);
    ImGui::SetItemTooltip(last ? "" : "Next step (\xE2\x86\x92)");
    ImGui::SameLine(0, 8);
    if (ImGui::Button("Exit", ImVec2(64, 0))) exitLesson();
    ImGui::SetItemTooltip("Leave the lesson (the panes stay as they are)");

    if (!lessonQ_.empty() && lessonIdx_ >= 0) {  // check yourself
        ImGui::Spacing();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(colv(GOLD_C), "Check yourself");
        ImGui::SameLine(0, 10);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(lessonQ_.c_str());
        ImGui::SameLine(0, 12);
        ImGui::SetNextItemWidth(130);
        if (lessonFocusInput_) { ImGui::SetKeyboardFocusHere(); lessonFocusInput_ = false; }
        if (ImGui::InputTextWithHint("##ans", "answer", lessonInput_, sizeof lessonInput_, ImGuiInputTextFlags_EnterReturnsTrue))
            lessonCheck();
        ImGui::SetItemTooltip("A number or an expression, like 0.5, sqrt(3)/2 or 2pi/3");
        ImGui::SameLine(0, 6);
        if (ImGui::Button("Check")) lessonCheck();
        ImGui::SameLine(0, 12);
        if (lessonResult_ == 1) ImGui::TextColored(colv(GREEN_C), "\xE2\x9C\x93  Correct!");
        else if (lessonResult_ == -1) ImGui::TextColored(colv(RED_C), "Not quite. Try again.");
        else if (lessonResult_ == -2) ImGui::TextColored(colv(RED_C), "That isn't a number or expression I can read.");
        if (!lessonHint_.empty() && lessonResult_ != 1) {
            ImGui::SameLine(0, 12);
            if (!lessonShowHint_) { if (ImGui::SmallButton("Hint")) lessonShowHint_ = true; }
            else ImGui::TextDisabled("%s", lessonHint_.c_str());
        }
        if (lessonResult_ == -1 || lessonResult_ == 1) {
            ImGui::SameLine(0, 12);
            if (ImGui::SmallButton("Show answer")) {
                std::snprintf(lessonInput_, sizeof lessonInput_, "%s", lessonAns_.c_str());
                lessonResult_ = 1;
            }
        }
    }
    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

// ------------------------------------------------------------------ the library (panel popup)

void App::drawLessonLibrary() {
    const auto& lib = lessonLibrary();
    std::string subject;
    for (int i = 0; i < (int)lib.size(); ++i) {
        const LessonDef& L = lib[i];
        if (L.subject != subject) {
            subject = L.subject;
            ImGui::Spacing();
            ImGui::PushFont(fonts_.uiBold, 14.f);
            ImGui::TextColored(colv(BLUE_C), "%s", subject.c_str());
            ImGui::PopFont();
        }
        const int best = lessonProgress_.count(L.id) ? lessonProgress_.at(L.id) : 0;
        const int n = (int)L.steps.size();
        char right[32];
        if (best >= n) std::snprintf(right, sizeof right, "\xE2\x9C\x93");
        else if (best > 0) std::snprintf(right, sizeof right, "%d / %d", best, n);
        else std::snprintf(right, sizeof right, "%d steps", n);
        ImGui::PushID(i);
        const float wAvail = 380.f;
        if (ImGui::Selectable(L.title.c_str(), lessonIdx_ == i, 0, ImVec2(wAvail, 0))) startLesson(i);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", L.blurb.c_str());
        ImGui::SameLine(wAvail - ImGui::CalcTextSize(right).x);
        if (best >= n) ImGui::TextColored(colv(GREEN_C), "%s", right);
        else ImGui::TextDisabled("%s", right);
        ImGui::PopID();
    }
}
