// Lesson mode: a lesson is a project plus a list of steps. Each step's target is built by
// applying the steps' `set` functions in order to the lesson's starting state, so any step can
// be reached from anywhere. Moving between steps plays a short transition through the same
// keyframe machinery as the timeline: mode switches start at once, views and sliders ease.
#include <algorithm>
#include <cctype>
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

StepCtx& StepCtx::refit(int i, bool equalAxes) {
    auto it = fits.find(i);
    if (it != fits.end()) fit(i, it->second[0], it->second[1], it->second[2], it->second[3], equalAxes);
    return *this;
}

StepCtx& StepCtx::fit(int i, double x0, double x1, double y0, double y1, bool equalAxes) {
    if (i < 0 || i >= (int)k.panes.size() || i >= (int)app.paneRects_.size()) return *this;
    fits[i] = {x0, x1, y0, y1};
    KeyPane& p = k.panes[i];
    const ImVec4 r = app.paneRects_[i];
    const double w = std::max(10.f, r.z - r.x), h = std::max(10.f, r.w - r.y);
    const double sx = w / std::max(1e-9, x1 - x0), sy = h / std::max(1e-9, y1 - y0);
    p.cx = 0.5 * (x0 + x1);
    p.cy = 0.5 * (y0 + y1);
    if (equalAxes || p.complex || p.threeD || p.unitCircle || p.matrix) { p.scale = std::min(sx, sy); p.aspect = 1; }
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
    if (testing_ && !testDone_) return 104.f;
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
    lessonBack_.clear();
    lessonBase_ = captureKeyframe(0);
    lessonPending_ = true;  // go to step 0 next frame, once pane sizes are known
    lessonStep_ = 0;
}

void App::openLink(const std::string& target) {
    if (target.rfind("lesson:", 0) != 0) return;
    const auto& lib = lessonLibrary();
    for (int i = 0; i < (int)lib.size(); ++i)
        if (lib[i].id == target.substr(7)) {
            std::vector<std::pair<int, int>> back = lessonBack_;
            if (lessonIdx_ >= 0 && lessonIdx_ != i) back.push_back({lessonIdx_, lessonStep_});
            startLesson(i);
            lessonBack_ = back;
            return;
        }
}

void App::exitLesson() {
    lessonBack_.clear();
    lessonIdx_ = -1;
    testing_ = testDone_ = false;
    lessonPlaying_ = false;
    lessonQ_.clear();
}

Keyframe App::lessonTarget(int step, StepCtx* out, const std::function<void(StepCtx&)>* extra) {
    // the starting state with steps 0..step applied in order (and then `extra`, for a test question)
    const LessonDef& L = lessonLibrary()[lessonIdx_];
    Keyframe to = lessonBase_;
    for (int j = 0; j <= step && j < (int)L.steps.size(); ++j) {
        StepCtx ctx{*this, to};
        if (L.steps[j].set) L.steps[j].set(ctx);
        if (j == step && out && !extra) {
            out->dur = ctx.dur; out->play = ctx.play; out->playSum = ctx.playSum;
            out->question = ctx.question; out->answer = ctx.answer; out->hint = ctx.hint; out->tol = ctx.tol;
        }
    }
    if (extra && *extra) {
        StepCtx ctx{*this, to};
        (*extra)(ctx);
        if (out) { out->dur = ctx.dur; out->play = ctx.play; out->playSum = ctx.playSum; }
    }
    for (KeyPane& p : to.panes) p.noteScroll = 0;
    to.sumPos = -1;  // the summation player shows the whole sum unless a step plays it
    return to;
}

void App::lessonText(const std::string& text) {
    const int keep = cur_;
    enterPane(0);
    if (isNote_) {
        noteText_ = autoMath(text);
        noteScroll_ = 0;
    }
    enterPane(keep);
}

void App::lessonMoveTo(Keyframe to, float dur, bool animate) {
    sumShowAll_ = true;
    sumPlaying_ = false;
    for (Param& p : params_) p.playing = false;
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
        f.curveX = t.curveX; f.curveY = t.curveY; f.curveT0 = t.curveT0; f.curveT1 = t.curveT1; f.curveDot = t.curveDot;
        f.fieldP = t.fieldP; f.fieldQ = t.fieldQ; f.fieldOpts = t.fieldOpts; f.shadeQuiet = t.shadeQuiet;
        if (t.matrix && !f.matrix) { for (int m = 0; m < 4; ++m) f.mat[m] = (m == 0 || m == 3) ? 1 : 0; }
    }
    from.t = 0;
    from.ease = 0;
    to.t = std::max(0.05f, dur);
    lessonKeys_ = {from, to};
    lessonT_ = 0;
    lessonPlaying_ = true;
}

void App::lessonGo(int step, bool animate) {
    if (lessonIdx_ < 0) return;
    const LessonDef& L = lessonLibrary()[lessonIdx_];
    step = std::clamp(step, 0, (int)L.steps.size() - 1);
    testing_ = testDone_ = false;
    computePaneRects();
    StepCtx out{*this, lessonBase_};
    Keyframe to = lessonTarget(step, &out, nullptr);
    {
        std::string text = "# " + L.steps[step].title + "\n\n";
        if (step == 0 && !L.prereqs.empty()) {  // links to the lessons to know first
            std::string links;
            for (const std::string& id : L.prereqs)
                for (const LessonDef& P : lessonLibrary())
                    if (P.id == id) {  // numbered titles ("1. Limits") need their subject to make sense
                        std::string t = P.title;
                        if (std::isdigit((unsigned char)t[0]) && t.find(". ") != std::string::npos) t = P.subject + ": " + t.substr(t.find(". ") + 2);
                        links += (links.empty() ? "" : "  \xC2\xB7  ") + std::string("[") + t + "](lesson:" + id + ")";
                    }
            if (!links.empty()) text += "**Before this lesson:** " + links + "\n\n";
        }
        text += L.steps[step].text;
        const std::string syms = lessonSymbols(L, step);
        if (!syms.empty()) text += "\n\n" + syms;
        lessonText(text);
    }
    lessonStep_ = step;
    lessonQ_ = out.question;
    lessonAns_ = out.answer;
    lessonHint_ = out.hint;
    lessonTol_ = out.tol;
    lessonInput_[0] = 0;
    lessonResult_ = 0;
    lessonShowHint_ = false;
    lessonPlay_ = out.play;
    lessonPlaySum_ = out.playSum;
    {
        int& best = lessonProgress_[L.id];
        if (step + 1 > best) { best = step + 1; saveSettings(); }
    }
    lessonMoveTo(to, out.dur, animate);
}

// ------------------------------------------------------------------ the test

void App::startTest() {
    if (lessonIdx_ < 0) return;
    const LessonDef& L = lessonLibrary()[lessonIdx_];
    if (L.test.empty()) return;
    testing_ = true;
    testDone_ = false;
    testRes_.assign(L.test.size(), 0);
    testGo(0, true);
}

void App::testGo(int q, bool animate) {
    const LessonDef& L = lessonLibrary()[lessonIdx_];
    q = std::clamp(q, 0, (int)L.test.size() - 1);
    testQ_ = q;
    testPicked_ = -1;
    lessonInput_[0] = 0;
    lessonResult_ = 0;
    lessonQ_.clear();
    lessonFocusInput_ = L.test[q].choices.empty();
    computePaneRects();
    StepCtx out{*this, lessonBase_};
    Keyframe to = lessonTarget((int)L.steps.size() - 1, &out, &L.test[q].set);
    std::string text = "# Test \xC2\xB7 question " + std::to_string(q + 1) + " of " + std::to_string(L.test.size()) + "\n\n" + L.test[q].text;
    if (!L.test[q].choices.empty()) {
        text += "\n\n";
        for (size_t c = 0; c < L.test[q].choices.size(); ++c)
            text += "- **" + std::string(1, (char)('A' + c)) + ".** " + L.test[q].choices[c] + "\n";
    }
    lessonText(text);
    lessonPlay_ = out.play;
    lessonPlaySum_ = out.playSum;
    lessonMoveTo(to, out.dur, animate);
}

void App::testAnswer(int choice) {
    const LessonDef& L = lessonLibrary()[lessonIdx_];
    const TestQuestion& t = L.test[testQ_];
    if (testRes_[testQ_] != 0) return;  // the first answer counts
    if (!t.choices.empty()) {
        if (choice < 0) return;
        testPicked_ = choice;
        testRes_[testQ_] = choice == t.correct ? 1 : -1;
        return;
    }
    const double want = paneScalar(t.answer), got = paneScalar(lessonInput_);
    if (!std::isfinite(got)) { lessonResult_ = -2; return; }  // unreadable: doesn't count as an attempt
    lessonResult_ = 0;
    testRes_[testQ_] = std::fabs(got - want) <= t.tol * std::max(1.0, std::fabs(want)) ? 1 : -1;
}

void App::finishTest() {
    const LessonDef& L = lessonLibrary()[lessonIdx_];
    testDone_ = true;
    int score = 0;
    for (int r : testRes_) score += r == 1;
    const int n = (int)L.test.size();
    auto& best = lessonTestBest_[L.id];
    if (best.second != n) best = {score, n};
    else best.first = std::max(best.first, score);
    saveSettings();
    std::string text = "# Test results\n\n";
    text += "**You scored " + std::to_string(score) + " out of " + std::to_string(n) + ".** ";
    text += score == n ? "Every answer right. Well done!" : score * 100 >= n * L.passPct ? "A pass: well done." : "Have another look at the steps below, then try again.";
    text += "\n\n";
    for (int i = 0; i < n; ++i) {
        const TestQuestion& t = L.test[i];
        std::string first = t.text.substr(0, t.text.find('\n'));
        if (first.size() > 140) first = first.substr(0, 137) + "\xE2\x80\xA6";
        text += "- " + std::string(testRes_[i] == 1 ? "\xE2\x9C\x93 " : "\xC3\x97 ") + first;
        if (testRes_[i] != 1) {
            if (!t.choices.empty() && t.correct >= 0) text += " **Answer: " + std::string(1, (char)('A' + t.correct)) + ".**";
            else if (!t.answer.empty()) text += " **Answer:** $" + t.answer + "$.";
            if (t.review >= 0 && t.review < (int)L.steps.size()) text += " Review step " + std::to_string(t.review + 1) + ", " + L.steps[t.review].title + ".";
        }
        text += "\n";
    }
    lessonText(text);
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

    if (testing_) {  // ---------------- the end-of-lesson test
        int act = 0;  // 1 next question, 2 results, 3 retry, 4 back to the lesson, 5 next lesson, 6 exit
        const int nq = (int)L.test.size();
        int nextLesson = -1;
        for (int i = lessonIdx_ + 1; i < (int)lessonLibrary().size(); ++i)
            if (i == nextLessonIndex(lessonIdx_)) { nextLesson = i; break; }
        ImGui::AlignTextToFramePadding();
        ImGui::PushFont(fonts_.uiBold, 15.f);
        ImGui::TextColored(colv(GOLD_C), testDone_ ? "TEST RESULTS" : "TEST");
        ImGui::PopFont();
        ImGui::SameLine(0, 12);
        {   // one square per question: green right, red wrong, outline unanswered
            const float sz = 11.f, gap = 6.f, fh = ImGui::GetFrameHeight();
            ImVec2 p = ImGui::GetCursorScreenPos();
            for (int i = 0; i < nq; ++i) {
                ImVec2 a(p.x + i * (sz + gap), p.y + (fh - sz) * 0.5f), b(a.x + sz, a.y + sz);
                if (testRes_[i] == 1) dl->AddRectFilled(a, b, col(GREEN_C), 2.f);
                else if (testRes_[i] == -1) dl->AddRectFilled(a, b, col(RED_C), 2.f);
                else dl->AddRect(a, b, col(WHITE, i == testQ_ && !testDone_ ? 0.9f : 0.35f), 2.f, 1.5f, 0);
            }
            ImGui::Dummy(ImVec2(nq * (sz + gap), fh));
        }
        ImGui::SameLine(0, 10);
        ImGui::AlignTextToFramePadding();
        int score = 0;
        for (int r : testRes_) score += r == 1;
        if (testDone_) ImGui::TextDisabled("%s  \xC2\xB7  %d / %d correct", L.title.c_str(), score, nq);
        else ImGui::TextDisabled("%s  \xC2\xB7  question %d of %d", L.title.c_str(), testQ_ + 1, nq);

        const bool answered = !testDone_ && testRes_[testQ_] != 0;
        float rightW = 64 + 8;
        if (testDone_) rightW += 110 + 8 + 130 + 8 + (nextLesson >= 0 ? 150 + 8 : 0);
        else rightW += 150 + 8 + 130 + 8;
        ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 12, w - 14 - rightW));
        if (ImGui::Button("Back to lesson", ImVec2(130, 0))) act = 4;
        ImGui::SameLine(0, 8);
        if (testDone_) {
            if (ImGui::Button("Retry test", ImVec2(110, 0))) act = 3;
            if (nextLesson >= 0) {
                ImGui::SameLine(0, 8);
                accent(true);
                if (ImGui::Button("Next lesson  \xE2\x96\xB6", ImVec2(150, 0))) act = 5;
                ImGui::PopStyleColor(3);
            }
        } else {
            ImGui::BeginDisabled(!answered);
            accent(answered);
            const bool lastQ = testQ_ == nq - 1;
            if (ImGui::Button(lastQ ? "See results  \xE2\x96\xB6" : "Next question  \xE2\x96\xB6", ImVec2(150, 0))) act = lastQ ? 2 : 1;
            if (answered) ImGui::PopStyleColor(3);
            ImGui::EndDisabled();
        }
        ImGui::SameLine(0, 8);
        if (ImGui::Button("Exit", ImVec2(64, 0))) act = 6;

        if (!testDone_) {  // answer row
            const TestQuestion& t = L.test[testQ_];
            ImGui::Spacing();
            if (!t.choices.empty()) {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Choose:");
                for (int c = 0; c < (int)t.choices.size(); ++c) {
                    ImGui::SameLine(0, 8);
                    const bool isRight = c == t.correct, picked = c == testPicked_;
                    int pushed = 0;
                    if (answered && (isRight || picked)) {
                        const int hue = isRight ? GREEN_C : RED_C;
                        ImGui::PushStyleColor(ImGuiCol_Button, mixv(BG, hue, 0.5f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, mixv(BG, hue, 0.5f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, mixv(BG, hue, 0.5f));
                        pushed = 3;
                    }
                    char lab[8];
                    std::snprintf(lab, sizeof lab, "  %c  ", 'A' + c);
                    ImGui::PushID(c);
                    if (ImGui::Button(lab) && !answered) testAnswer(c);
                    ImGui::PopID();
                    if (pushed) ImGui::PopStyleColor(pushed);
                }
            } else {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Your answer:");
                ImGui::SameLine(0, 10);
                ImGui::SetNextItemWidth(160);
                if (lessonFocusInput_) { ImGui::SetKeyboardFocusHere(); lessonFocusInput_ = false; }
                ImGui::BeginDisabled(answered);
                if (ImGui::InputTextWithHint("##tans", "number or expression", lessonInput_, sizeof lessonInput_,
                                             ImGuiInputTextFlags_EnterReturnsTrue))
                    testAnswer(-1);
                ImGui::SameLine(0, 6);
                if (ImGui::Button("Submit")) testAnswer(-1);
                ImGui::EndDisabled();
            }
            ImGui::SameLine(0, 14);
            if (testRes_[testQ_] == 1) ImGui::TextColored(colv(GREEN_C), "\xE2\x9C\x93  Correct!");
            else if (testRes_[testQ_] == -1) {
                if (!t.choices.empty()) ImGui::TextColored(colv(RED_C), "Not this time: the answer is %c.", 'A' + t.correct);
                else ImGui::TextColored(colv(RED_C), "Not this time: the answer is %s.", t.answer.c_str());
                if (t.review >= 0 && t.review < n) {
                    ImGui::SameLine(0, 10);
                    ImGui::TextDisabled("(review step %d, %s)", t.review + 1, L.steps[t.review].title.c_str());
                }
            } else if (lessonResult_ == -2) ImGui::TextColored(colv(RED_C), "That isn't a number or expression I can read.");
            else ImGui::TextDisabled("Your first answer counts.");
        }
        ImGui::PopFont();
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        switch (act) {
            case 1: testGo(testQ_ + 1, true); break;
            case 2: finishTest(); break;
            case 3: startTest(); break;
            case 4: lessonGo((int)L.steps.size() - 1, true); break;
            case 5: startLesson(nextLesson); break;
            case 6: exitLesson(); break;
        }
        return;
    }

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
    if (!lessonBack_.empty()) {  // came here by a prerequisite link
        const auto [bi, bs] = lessonBack_.back();
        ImGui::SameLine(0, 12);
        const std::string lbl = "\xE2\x86\xA9 Back to " + lessonLibrary()[bi].title;
        if (ImGui::SmallButton(lbl.c_str())) {
            std::vector<std::pair<int, int>> rest(lessonBack_.begin(), lessonBack_.end() - 1);
            startLesson(bi);
            lessonStep_ = bs;
            lessonBack_ = rest;
            ImGui::PopFont(); ImGui::End(); ImGui::PopStyleColor(); ImGui::PopStyleVar(2);
            return;
        }
        ImGui::SetItemTooltip("Return to the lesson you left, at step %d", bs + 1);
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
            if (i == nextLessonIndex(lessonIdx_)) { nextLesson = i; break; }
    }
    const bool hasTest = !L.test.empty();
    const float nextW = last ? (hasTest || nextLesson >= 0 ? 150.f : 96.f) : 96.f;
    ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 12, w - 14 - nextW - 8 - 64));
    accent(true);
    if (!last) {
        if (ImGui::Button("Next  \xE2\x96\xB6", ImVec2(nextW, 0))) lessonGo(lessonStep_ + 1, true);
    } else if (hasTest) {
        if (ImGui::Button("Take the test  \xE2\x96\xB6", ImVec2(nextW, 0))) startTest();
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
        richTextLine(lessonQ_, ImGui::GetColorU32(ImGuiCol_Text), col(WHITE));
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
            else richTextLine(lessonHint_, ImGui::GetColorU32(ImGuiCol_TextDisabled), col(WHITE, 0.6f));
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

int nextLessonIndex(int idx) {
    const auto& lib = lessonLibrary();
    if (idx < 0 || idx + 1 >= (int)lib.size()) return -1;
    const LessonDef& L = lib[idx];
    for (int i = idx + 1; i < (int)lib.size(); ++i) {
        if (!L.track.empty() && lib[i].track == L.track) return i;
        if (L.track.empty() && lib[i].subject == L.subject) return i;
    }
    return -1;
}

void App::drawLessonLibrary() {
    const auto& lib = lessonLibrary();
    std::string subject;
    bool open = true;
    for (int i = 0; i < (int)lib.size(); ++i) {
        const LessonDef& L = lib[i];
        if (L.subject != subject) {  // one collapsible section per subject
            subject = L.subject;
            int count = 0, passed = 0;
            for (const LessonDef& M : lib) {
                if (M.subject != subject) continue;
                ++count;
                const auto tb = lessonTestBest_.find(M.id);
                if (tb != lessonTestBest_.end() && tb->second.second > 0 && tb->second.first * 100 >= tb->second.second * M.passPct) ++passed;
            }
            const bool here = lessonIdx_ >= 0 && lib[lessonIdx_].subject == subject;
            ImGui::SetNextItemOpen(here || (lessonIdx_ < 0 && i == 0), ImGuiCond_Appearing);
            char head[192];
            std::snprintf(head, sizeof head, "%s   (%d/%d passed)###%s", subject.c_str(), passed, count, subject.c_str());
            ImGui::PushFont(fonts_.uiBold, 14.f);
            ImGui::PushStyleColor(ImGuiCol_Text, colv(BLUE_C));
            open = ImGui::CollapsingHeader(head);
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }
        if (!open) continue;
        const int best = lessonProgress_.count(L.id) ? lessonProgress_.at(L.id) : 0;
        const int n = (int)L.steps.size();
        char right[48];
        bool done = false;
        const auto tb = lessonTestBest_.find(L.id);
        if (tb != lessonTestBest_.end() && tb->second.second > 0) {
            done = tb->second.first * 100 >= tb->second.second * L.passPct;
            std::snprintf(right, sizeof right, "%stest %d/%d", done ? "\xE2\x9C\x93 " : "", tb->second.first, tb->second.second);
        } else if (best >= n) std::snprintf(right, sizeof right, L.test.empty() ? "\xE2\x9C\x93" : "test to take");
        else if (best > 0) std::snprintf(right, sizeof right, "%d / %d", best, n);
        else std::snprintf(right, sizeof right, "%d steps", n);
        if (L.test.empty() && best >= n) done = true;
        ImGui::PushID(i);
        const float wAvail = 380.f;
        if (ImGui::Selectable(L.title.c_str(), lessonIdx_ == i, 0, ImVec2(wAvail, 0))) startLesson(i);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", L.blurb.c_str());
        ImGui::SameLine(wAvail - ImGui::CalcTextSize(right).x);
        if (done) ImGui::TextColored(colv(GREEN_C), "%s", right);
        else ImGui::TextDisabled("%s", right);
        ImGui::PopID();
    }
}
