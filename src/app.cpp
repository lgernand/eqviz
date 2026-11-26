#include "app.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <set>

#include "gl.hpp"
#include "theme.hpp"

static const double PI = 3.14159265358979323846;
static const char* kDefaultEq = "sum(k, 0, n, (-1)^k x^(2k+1) / (2k+1)!)";

// ------------------------------------------------------------------ helpers

double niceStep(double raw) {
    if (!(raw > 0) || !std::isfinite(raw)) return 1;
    double e = std::floor(std::log10(raw)), b = std::pow(10.0, e), m = raw / b;
    double s = m < 1.5 ? 1 : m < 3.5 ? 2 : m < 7.5 ? 5 : 10;
    return s * b;
}

void Func::bind(const std::vector<Param>& ps) {
    map.clear();
    for (const auto& n : prog.slotNames()) {
        int m = -3;
        if (n == "x") m = -1;
        else {
            for (size_t i = 0; i < ps.size(); ++i)
                if (ps[i].name == n) { m = (int)i; break; }
            if (m == -3 && n == "y") m = -2;
        }
        map.push_back(m);
    }
}

cplx evalF(const Func& f, cplx x, double y, const EvalEnv& e) {
    if (!f.ok()) return cplx(NAN, NAN);
    cplx slots[64];
    const int n = std::min(f.prog.numSlots(), 64);
    for (int i = 0; i < n; ++i) {
        int m = f.map[i];
        slots[i] = m == -1 ? x : m == -2 ? cplx(y, 0) : m >= 0 ? cplx(e.params[m], 0) : cplx(0, 0);
    }
    return f.prog.eval(slots, e.sumLimit, e.real, e.maxTerms, e.animTerms);
}

double realPart(cplx v) {
    if (!finiteC(v)) return NAN;
    if (std::fabs(v.imag()) > 1e-7 * std::max(1.0, std::fabs(v.real()))) return NAN;
    return v.real();
}

void Camera::update(ImVec2 cmin, ImVec2 cmax) {
    center = ImVec2((cmin.x + cmax.x) * 0.5f, (cmin.y + cmax.y) * 0.5f);
    focal = (cmax.y - cmin.y) * 0.5 / std::tan(fov * 0.5);
    Vec3 dir(std::cos(s.phi) * std::cos(s.theta), std::cos(s.phi) * std::sin(s.theta), std::sin(s.phi));
    eye = s.target + dir * s.dist;
    fwd = normalize(s.target - eye);
    right = normalize(cross(fwd, Vec3(0, 0, 1)));
    up = cross(right, fwd);
}

bool Camera::project(const Vec3& p, ImVec2& out, float& depth) const {
    Vec3 d = p - eye;
    double zc = dot(d, fwd);
    if (zc < 0.05) return false;
    double xc = dot(d, right), yc = dot(d, up);
    out = ImVec2((float)(center.x + focal * xc / zc), (float)(center.y - focal * yc / zc));
    depth = (float)zc;
    return true;
}

// ------------------------------------------------------------------ lifecycle

void App::init(const Fonts& f, const LaunchOptions& o) {
    fonts_ = f;
    opt_ = o;
    ts::init(f.math, f.mathAlnum);
    setupStyle();
    complex_ = o.complex;
    deriv_ = o.deriv;
    domainColor_ = o.domainColor;
    autoRotate_ = o.autoRotate;
    panes_.resize(1);
    cur_ = focus_ = 0;
    initPane();
    want3D_ = o.threeD;
    std::thread([] { eq::warmZetaZeros(); }).detach();  // tabulate zeta zeros in the background
    if (o.project == "demo:riemann") loadDemoRiemann();
    else if (o.project == "demo:poincare") loadDemoPoincare();
    else if (o.project == "demo:navier-stokes") loadDemoNavierStokes();
    else if (!o.project.empty() && loadProject(o.project)) {}
    else {
        setSingleFunction(o.eq.empty() ? kDefaultEq : o.eq.c_str());
        if (!funcs_[0].error.empty()) setSingleFunction(kDefaultEq);
    }
    if (complex_) { gridT_ = 0; gridAnimP_ = 0; gridAnim_ = true; }
}

void App::shutdown() {
    for (int i = 0; i < (int)panes_.size(); ++i) {
        enterPane(i);
        if (dcTex_) {
            GLuint t = dcTex_;
            glDeleteTextures(1, &t);
            dcTex_ = 0;
        }
    }
}

// ------------------------------------------------------------------ panes

void App::exchange(PaneState& p) {
#define EQVIZ_SWAP_FIELD(type, name, init) std::swap(name, p.name);
    EQVIZ_PANE_FIELDS(EQVIZ_SWAP_FIELD)
#undef EQVIZ_SWAP_FIELD
}

void App::enterPane(int i) {
    i = std::clamp(i, 0, (int)panes_.size() - 1);
    if (i != cur_) {
        exchange(panes_[cur_]);  // store the current pane
        exchange(panes_[i]);     // load pane i
        cur_ = i;
    }
    if (i < (int)paneRects_.size()) {
        const ImVec4& r = paneRects_[i];
        canvasMin_ = ImVec2(r.x, r.y);
        canvasMax_ = ImVec2(r.z, r.w);
        view_.cmin = canvasMin_;
        view_.cmax = canvasMax_;
    }
}

void App::initPane() {
    view_.scale = targetScale_ = 72;
    cam_.s.theta = -PI / 2;
    cam_.s.phi = PI / 2 - 1e-3;
}

void App::applyFit() {
    if (!fitPending_) return;
    const float w = canvasMax_.x - canvasMin_.x, h = canvasMax_.y - canvasMin_.y;
    if (w < 10 || h < 10) return;
    const ImVec4 r = fitRange_;  // x0, x1, y0, y1
    double sx = w / std::max(1e-9f, r.y - r.x), sy = h / std::max(1e-9f, r.w - r.z);
    view_.cx = (r.x + r.y) * 0.5;
    view_.cy = (r.z + r.w) * 0.5;
    if (complex_ || threeD_ || want3D_) { view_.scale = std::min(sx, sy); view_.aspect = 1; }
    else { view_.scale = sx; view_.aspect = sy / sx; }
    targetScale_ = view_.scale;
    zooming_ = false;
    fitPending_ = false;
}

void App::frame(float dt) {
    ImGuiIO& io = ImGui::GetIO();
    time_ += dt;
    ImVec2 ds = io.DisplaySize;
    panelW_ = std::min(390.f, std::max(300.f, ds.x * 0.3f));
    computePaneRects();
    enterPane(focus_);
    ++frameNo_;
    if (!opt_.actions.empty()) runScriptedActions();
    handleLayoutInput();

    // clicking a pane focuses it (the panel edits the focused pane)
    if (!io.WantCaptureMouse && !layoutBusy_ && inVisible(io.MousePos) && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1)))
        for (int i = 0; i < (int)paneRects_.size(); ++i) {
            const ImVec4& r = paneRects_[i];
            if (io.MousePos.x >= r.x && io.MousePos.x < r.z && io.MousePos.y >= r.y && io.MousePos.y < r.w) focus_ = i;
        }
    enterPane(focus_);

    if (firstFrame_) {
        firstFrame_ = false;
        if (opt_.sumPos >= 0) {
            sumPos_ = opt_.sumPos;
            sumPlaying_ = false;
        }
    }
    handleKeys();

    // shared animations: parameter sweeps and the sum player
    for (auto& p : params_) {
        if (!p.playing) continue;
        p.phase += dt / 6.0;
        double u = 0.5 - 0.5 * std::cos(2 * PI * p.phase);
        p.value = (float)(p.min + (p.max - p.min) * u);
        if (p.integer) p.value = std::round(p.value);
    }
    updateParamValues();
    {
        int sp = -1;
        for (int i = 0; i < (int)panes_.size() && sp < 0; ++i) {
            enterPane(i == 0 ? focus_ : (i == focus_ ? 0 : i));
            if (hasSum_) sp = cur_;
        }
        if (sp >= 0) { enterPane(sp); updateSum(dt); }
    }

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled(ImVec2(0, 0), ds, theme::col(theme::BG));
    dl->PushClipRect(ImVec2(visRect_.x, visRect_.y), ImVec2(visRect_.z, visRect_.w), false);
    for (int i = 0; i < (int)panes_.size(); ++i) {
        enterPane(i);
        applyFit();
        if (want3D_ && !threeD_ && !dimAnimating_) { want3D_ = false; startDim(true); }
        if (hasSum_) sumCount_ = computeSumCount();
        updatePaneAnimations(dt);
        handleInput(dt);
        const bool onScreen = canvasMax_.x > visRect_.x && canvasMin_.x < visRect_.z && canvasMax_.y > visRect_.y &&
                              canvasMin_.y < visRect_.w;
        if (!onScreen) continue;
        cam_.update(canvasMin_, canvasMax_);
        dl->PushClipRect(canvasMin_, canvasMax_, true);
        if (threeD_) drawScene3D(dl);
        else drawScene2D(dl);
        drawOverlay(dl);
        dl->PopClipRect();
    }
    drawLayoutChrome(dl);
    dl->PopClipRect();
    enterPane(focus_);
    drawPanel();
}

// ------------------------------------------------------------------ equation

void App::applyActive(const eq::NodeP& plot, bool sameRow) {
    eq::NodeP e = plot;  // already marked: only the row's own top-level sums animate, not inlined ones
    Func oldDisplay = displayFunc();
    bool hadOld = sameRow && oldDisplay.ok();

    expr_ = e;
    dx_ = eq::derivative(e, "x");
    dy_ = eq::derivative(e, "y");
    f_ = Func{eq::Program(expr_), {}};
    fx_ = Func{eq::Program(dx_), {}};
    fy_ = Func{eq::Program(dy_), {}};

    std::vector<const eq::Node*> sums;
    eq::collectAnimatedSums(expr_, sums);
    hasSum_ = !sums.empty();
    if (hasSum_) {  // bounds that move with x or y (like sum(n, 1, x, ...)) are not a series to step through
        for (const char* v : {"x", "y"})
            if (eq::dependsOn(sums[0]->kids[0], v) || eq::dependsOn(sums[0]->kids[1], v)) hasSum_ = false;
    }
    if (hasSum_) {
        sumLoF_ = Func{eq::Program(sums[0]->kids[0]), {}};
        sumHiF_ = Func{eq::Program(sums[0]->kids[1]), {}};
        sumIndex_ = sums[0]->name;
    } else {
        sumLoF_ = Func{};
        sumHiF_ = Func{};
    }
    sumInfinite_ = false;

    prev_ = Func{};
    rebuildParams();

    if (hasSum_) {
        sumStopAt_ = -1;
        sumHold_ = 0;
        if (sameRow) { sumPos_ = 0; sumPlaying_ = true; sumShowAll_ = false; }
        else { sumPlaying_ = false; sumShowAll_ = true; }  // switching rows: show the finished sum
        morph_ = 1;
        create_ = 1;
    } else if (hadOld) {
        prev_ = oldDisplay;
        prev_.bind(params_);
        morph_ = 0;
        create_ = 1;
    } else {
        create_ = sameRow ? 0.f : 1.f;
        morph_ = 1;
    }
    derivCreate_ = sameRow ? 0.f : 1.f;
    if (threeD_) autoZScale();
}

void App::rebuildParams() {
    std::set<std::string> fv, intNames;
    eq::freeVars(expr_, fv);
    eq::sumBoundNames(expr_, intNames);
    for (const auto& d : funcs_)
        if (d.plot) { eq::freeVars(d.plot, fv); eq::sumBoundNames(d.plot, intNames); }
    fv.erase("x");
    if (threeD_ && !complex_) fv.erase("y");
    std::vector<Param> np;
    for (const auto& name : fv) {
        Param p;
        bool found = false;
        for (const auto& old : params_)
            if (old.name == name) { p = old; found = true; break; }
        bool isInt = intNames.count(name) > 0;
        if (!found) {
            p.name = name;
            if (isInt) { p.value = 10; p.min = 0; p.max = 40; }
            else if (name == "y") { p.value = 0; }
        }
        if (isInt && !p.integer) {
            p.min = std::max(0.f, std::round(p.min));
            p.max = std::max(p.min + 1, std::round(p.max));
            p.value = std::round(p.value);
        }
        p.integer = isInt;
        np.push_back(p);
    }
    params_ = std::move(np);
    updateParamValues();
    bindAll();
}

void App::bindAll() {
    const int start = cur_;
    for (int i = 0; i < (int)panes_.size(); ++i) {
        enterPane(i);
        for (Func* f : {&f_, &fx_, &fy_, &prev_, &sumLoF_, &sumHiF_}) f->bind(params_);
    }
    enterPane(start);
    for (auto& d : funcs_) d.f.bind(params_);
}

void App::updateParamValues() {
    pvals_.resize(params_.size());
    for (size_t i = 0; i < params_.size(); ++i) pvals_[i] = params_[i].value;
}

EvalEnv App::makeEnv(double limit) const {
    EvalEnv e;
    e.params = pvals_.data();
    e.sumLimit = std::isnan(limit) ? sumLimit() : limit;
    e.maxTerms = maxTerms_;
    e.animTerms = animTerms_;
    e.real = !complex_;
    return e;
}

uint64_t App::evalHash() const {
    Hasher h;
    h.u((uint64_t)(uintptr_t)expr_.get());
    h.u((uint64_t)(uintptr_t)prev_.prog.slotNames().data());
    h.u(deriv_ ? 1 : 0);
    h.u(complex_ ? 1 : 0);
    h.d(morph_);
    h.d(sumLimit());
    h.u((uint64_t)maxTerms_);
    h.u((uint64_t)animTerms_);
    for (double p : pvals_) h.d(p);
    return h.h;
}

// ------------------------------------------------------------------ summation

int App::computeSumCount() {
    if (!hasSum_) return 0;
    EvalEnv env = makeEnv(INFINITY);
    double lo = evalF(sumLoF_, 0.0, 0, env).real();
    double hi = evalF(sumHiF_, 0.0, 0, env).real();
    if (std::isnan(lo) || std::isnan(hi)) return 0;
    sumInfinite_ = std::isinf(lo) || std::isinf(hi);
    if (sumInfinite_) return animTerms_ + 1;  // N animated partial sums, then the step to the limit
    lo = std::ceil(lo - 1e-9);
    hi = std::floor(hi + 1e-9);
    return (int)std::clamp(hi - lo + 1, 0.0, 1e7);
}

double App::sumLimit() const {
    if (!hasSum_) return INFINITY;
    if (sumShowAll_ && !sumPlaying_) return INFINITY;
    double n = std::floor(sumPos_), fr = sumPos_ - n;
    return n + theme::smooth((float)std::min(1.0, fr / 0.7));
}

void App::updateSum(float dt) {
    if (!hasSum_) return;
    sumCount_ = computeSumCount();
    if (sumShowAll_ && !sumPlaying_) sumPos_ = sumCount_;
    if (sumPlaying_) {
        if (sumHold_ > 0) {
            sumHold_ -= dt;
            if (sumHold_ <= 0) { sumHold_ = 0; sumPos_ = 0; }
            return;
        }
        double target = sumStopAt_ >= 0 ? std::min<double>(sumStopAt_, sumCount_) : sumCount_;
        double spd = sumStopAt_ >= 0 ? std::max(sumSpeed_, 1.5f) : sumSpeed_;
        double done = std::floor(sumPos_);
        if (sumAccel_) spd *= 1.0 + done / 4.0;              // each term arrives a little faster
        if (sumInfinite_ && done >= sumCount_ - 1) spd = 0.6;  // the final glide to the limit

        sumPos_ += dt * spd;
        if (sumPos_ >= target) {
            sumPos_ = target;
            if (sumStopAt_ >= 0) { sumPlaying_ = false; sumStopAt_ = -1; }
            else if (sumLoop_) sumHold_ = 1.2;
            else { sumPlaying_ = false; sumShowAll_ = true; }
        }
    }
    sumPos_ = std::clamp(sumPos_, 0.0, (double)sumCount_);
}

// ------------------------------------------------------------------ animation

static CamState lerpCam(const CamState& a, const CamState& b, double t) {
    CamState c;
    c.target = a.target + (b.target - a.target) * t;
    c.theta = a.theta + (b.theta - a.theta) * t;
    c.phi = a.phi + (b.phi - a.phi) * t;
    c.dist = std::exp(std::log(a.dist) + (std::log(b.dist) - std::log(a.dist)) * t);
    return c;
}

void App::updatePaneAnimations(float dt) {
    if (create_ < 1) create_ = std::min(1.f, create_ + dt / 1.5f);
    if (morph_ < 1) morph_ = std::min(1.f, morph_ + dt / 1.3f);
    if (derivCreate_ < 1) derivCreate_ = std::min(1.f, derivCreate_ + dt / 1.3f);
    if (gridAnim_) {
        gridAnimP_ += dt / 2.6f;
        if (gridAnimP_ >= 1) { gridAnimP_ = 1; gridAnim_ = false; }
        gridT_ = theme::smooth(gridAnimP_);
    }
    if (dimAnimating_) {
        dimP_ += dt / 1.9f;
        if (dimP_ >= 1) {
            dimP_ = 1;
            dimAnimating_ = false;
            if (!dimTo3D_) {
                threeD_ = false;
                rebuildParams();
            } else {
                targetDist_ = camTo_.dist;
            }
        }
        cam_.s = lerpCam(camFrom_, camTo_, theme::smooth(dimP_));
    }
    if (fadeStage_ == 1) {
        fade_ -= dt / 0.25f;
        if (fade_ <= 0) {
            fade_ = 0;
            complex_ = pendingComplex_;
            if (complex_) { view_.aspect = 1; gridStep_ = 0; }
            morph_ = 1;
            prev_ = Func{};
            rebuildParams();
            if (threeD_) autoZScale();
            fadeStage_ = 2;
            if (complex_ && !threeD_) { gridT_ = 0; gridAnimP_ = 0; gridAnim_ = true; }
        }
    } else if (fadeStage_ == 2) {
        fade_ += dt / 0.4f;
        if (fade_ >= 1) { fade_ = 1; fadeStage_ = 0; }
    }
}

float App::heightScale() const {
    if (dimAnimating_) {
        float s = theme::smooth(dimP_);
        return dimTo3D_ ? s : 1 - s;
    }
    return threeD_ ? 1.f : 0.f;
}

// ------------------------------------------------------------------ mode switches

void App::toggleComplex() {
    if (fadeStage_ != 0) return;
    pendingComplex_ = !complex_;
    fadeStage_ = 1;
}

void App::toggle3D() {
    if (dimAnimating_) return;
    startDim(!threeD_);
}

void App::toggleDeriv() {
    if (complex_) {
        prev_ = displayFunc();
        deriv_ = !deriv_;
        morph_ = 0;
    } else {
        deriv_ = !deriv_;
        if (deriv_) derivCreate_ = 0;
    }
}

void App::startDim(bool to3D) {
    ImVec2 csz(canvasMax_.x - canvasMin_.x, canvasMax_.y - canvasMin_.y);
    double focal = (csz.y * 0.5) / std::tan(cam_.fov * 0.5);
    view_.aspect = 1;  // the 3D camera needs an undistorted plane
    CamState top;
    top.theta = -PI / 2;
    top.phi = PI / 2 - 1e-3;
    top.target = Vec3(view_.cx, view_.cy, 0);
    top.dist = focal / view_.scale;
    view_.s2w(canvasMin_, planeX0_, planeY1_);
    view_.s2w(canvasMax_, planeX1_, planeY0_);
    if (to3D) {
        double halfH = csz.y * 0.5 / view_.scale;
        domainR_ = (float)std::max(0.5, niceStep(halfH * 0.62));
        double snap = domainR_ / 8.0;
        domainCx_ = std::round(view_.cx / snap) * snap;
        domainCy_ = std::round(view_.cy / snap) * snap;
        camFrom_ = top;
        camTo_.target = Vec3(domainCx_, domainCy_, 0);
        camTo_.theta = -PI / 2 + 0.62;
        camTo_.phi = 0.5;
        camTo_.dist = domainR_ * 4.7;
        threeD_ = true;
        rebuildParams();
        autoZScale();
    } else {
        camFrom_ = cam_.s;
        double th = camFrom_.theta;
        while (th - top.theta > PI) th -= 2 * PI;
        while (th - top.theta < -PI) th += 2 * PI;
        camFrom_.theta = th;
        camTo_ = top;
    }
    cam_.s = camFrom_;
    dimTo3D_ = to3D;
    dimP_ = 0;
    dimAnimating_ = true;
}

void App::autoZScale() {
    if (!f_.ok() || lockZScale_) return;
    EvalEnv env = makeEnv(INFINITY);
    std::vector<double> mags;
    const int N = 24;
    for (int i = 0; i <= N; ++i)
        for (int j = 0; j <= N; ++j) {
            double x = domainCx_ - domainR_ + 2.0 * domainR_ * i / N;
            double y = domainCy_ - domainR_ + 2.0 * domainR_ * j / N;
            double m;
            if (complex_) m = std::abs(evalF(f_, cplx(x, y), 0, env));
            else m = std::fabs(realPart(evalF(f_, x, y, env)));
            if (std::isfinite(m)) mags.push_back(m);
        }
    if (mags.empty()) { zScale_ = 1; return; }
    std::sort(mags.begin(), mags.end());
    double p = mags[(size_t)(mags.size() * 0.9) < mags.size() ? (size_t)(mags.size() * 0.9) : mags.size() - 1];
    if (p < 1e-9) { zScale_ = 1; return; }
    zScale_ = (float)std::clamp(domainR_ * 0.55 / p, 0.01, 1.0);
}

void App::resetView() {
    if (threeD_) {
        cam_.s.theta = -PI / 2 + 0.62;
        cam_.s.phi = 0.5;
        cam_.s.target = Vec3(domainCx_, domainCy_, 0);
        targetDist_ = domainR_ * 4.7;
    } else {
        view_.cx = view_.cy = 0;
        view_.aspect = 1;
        targetScale_ = 72;
        zoomAnchorS_ = view_.center();
        zoomAnchorX_ = zoomAnchorY_ = 0;
        zooming_ = true;
    }
}

void App::runScriptedActions() {
    size_t i = 0;
    const std::string& s = opt_.actions;
    while (i < s.size()) {
        size_t comma = s.find(',', i);
        std::string item = s.substr(i, comma == std::string::npos ? std::string::npos : comma - i);
        i = comma == std::string::npos ? s.size() : comma + 1;
        size_t colon = item.find(':');
        if (colon == std::string::npos || std::atoi(item.c_str()) != frameNo_) continue;
        std::string a = item.substr(colon + 1);
        if (a == "c") toggleComplex();
        else if (a == "3") toggle3D();
        else if (a == "d") toggleDeriv();
        else if (a == "p") { sumPos_ = 0; sumShowAll_ = false; sumPlaying_ = true; }
        else if (!a.empty() && a[0] == 'e') loadExample(std::atoi(a.c_str() + 1));
        else if (a == "aL") addPane(kLeft);
        else if (a == "aR") addPane(kRight);
        else if (a == "aU") addPane(kAbove);
        else if (a == "aD") addPane(kBelow);
        else if (a == "x") closePane(focus_);
        else if (!a.empty() && a[0] == 'P') plat_.magnify += (float)std::atof(a.c_str() + 1);
        else if (!a.empty() && a[0] == 'S') {
            float dx = 0, dy = 0;
            if (std::sscanf(a.c_str() + 1, "%f;%f", &dx, &dy) == 2) { plat_.trackpad = true; plat_.scrollX += dx; plat_.scrollY += dy; }
        }
        else if (a == "W") { extendWorkspace_ = true; wsW_ = 2.f; }
        else if (!a.empty() && a[0] == 'f') focus_ = std::clamp(std::atoi(a.c_str() + 1), 0, (int)panes_.size() - 1);
        else if (!a.empty() && a[0] == 'Z') {  // Zx;y;scale : jump the view (zoom testing)
            double x = 0, y = 0, sc = 72;
            if (std::sscanf(a.c_str() + 1, "%lf;%lf;%lf", &x, &y, &sc) == 3) {
                view_.cx = x; view_.cy = y; view_.scale = targetScale_ = sc; zooming_ = false;
            }
        }
    }
}

bool App::canHover() const {
    const ImGuiIO& io = ImGui::GetIO();
    ImVec2 m = io.MousePos;
    return !io.WantCaptureMouse && !ImGui::IsAnyMouseDown() && !dimAnimating_ && !layoutBusy_ && inVisible(m) && m.x >= canvasMin_.x &&
           m.y >= canvasMin_.y && m.x < canvasMax_.x && m.y < canvasMax_.y;
}

void App::hoverLabel(ImDrawList* dl, ImVec2 at, const ts::Box& b, ImU32 accent) const {
    ImVec2 p(at.x + 16, at.y - 14);
    if (p.x + b.w + 14 > canvasMax_.x) p.x = at.x - 16 - b.w;
    p.y = std::clamp(p.y, canvasMin_.y + b.asc + 10, canvasMax_.y - b.desc - 10);
    dl->AddRectFilled(ImVec2(p.x - 9, p.y - b.asc - 7), ImVec2(p.x + b.w + 9, p.y + b.desc + 7),
                      theme::col(theme::BG, 0.9f), 7);
    dl->AddRect(ImVec2(p.x - 9, p.y - b.asc - 7), ImVec2(p.x + b.w + 9, p.y + b.desc + 7),
                theme::withAlpha(accent, 0.6f), 7, 1.2f, 0);
    ts::draw(dl, b, p, theme::col(theme::WHITE));
}

// ------------------------------------------------------------------ input

void App::handleInput(float dt) {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 m = io.MousePos;
    bool overCanvas = !io.WantCaptureMouse && !layoutBusy_ && inVisible(m) && m.x >= canvasMin_.x &&
                      m.y >= canvasMin_.y && m.x < canvasMax_.x && m.y < canvasMax_.y;

    if (!threeD_) {
        if (overCanvas && ImGui::IsMouseClicked(0)) {
            float dx = m.x - probeScreen_.x, dy = m.y - probeScreen_.y;
            if (probeVisible_ && dx * dx + dy * dy < 16 * 16) draggingProbe_ = true;
            else panning_ = true;
        }
        if (!ImGui::IsMouseDown(0)) draggingProbe_ = panning_ = false;
        if (draggingProbe_) {
            double wx, wy;
            view_.s2w(m, wx, wy);
            probeX_ = (float)wx;
        } else if (panning_ && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)) {
            view_.cx -= io.MouseDelta.x / view_.scale;
            view_.cy += io.MouseDelta.y / view_.yscale();
            if (zooming_) view_.s2w(zoomAnchorS_, zoomAnchorX_, zoomAnchorY_);
        }
        // zoom: a mouse wheel, or a trackpad pinch (trackpad scrolling never zooms)
        double zoomBy = 1;
        if (overCanvas && io.MouseWheel != 0 && !plat_.trackpad) zoomBy *= std::pow(1.15, (double)io.MouseWheel);
        if (overCanvas && plat_.magnify != 0) zoomBy *= std::max(0.2, 1.0 + plat_.magnify);
        if (zoomBy != 1) {
            zoomAnchorS_ = m;
            view_.s2w(m, zoomAnchorX_, zoomAnchorY_);
            targetScale_ = std::clamp(targetScale_ * zoomBy, 1e-6, 1e8);  // ~1e9 units across .. ~1e-5
            if (plat_.magnify != 0 && io.MouseWheel == 0) {  // pinches track the fingers directly
                view_.scale = targetScale_;
                ImVec2 c = view_.center();
                view_.cx = zoomAnchorX_ - (zoomAnchorS_.x - c.x) / view_.scale;
                view_.cy = zoomAnchorY_ + (zoomAnchorS_.y - c.y) / view_.yscale();
            }
            zooming_ = true;
        }
        // two-finger trackpad scroll pans the pane (when the workspace itself can't scroll)
        if (overCanvas && plat_.trackpad && !extendWorkspace_ && (plat_.scrollX != 0 || plat_.scrollY != 0)) {
            view_.cx -= plat_.scrollX / view_.scale;
            view_.cy += plat_.scrollY / view_.yscale();
            if (zooming_) view_.s2w(zoomAnchorS_, zoomAnchorX_, zoomAnchorY_);
        }
        if (zooming_) {
            double k = 1 - std::exp(-dt * 14.0);
            double ls = std::log(view_.scale) + (std::log(targetScale_) - std::log(view_.scale)) * k;
            view_.scale = std::exp(ls);
            if (std::fabs(std::log(view_.scale / targetScale_)) < 1e-4) {
                view_.scale = targetScale_;
                zooming_ = false;
            }
            ImVec2 c = view_.center();
            view_.cx = zoomAnchorX_ - (zoomAnchorS_.x - c.x) / view_.scale;
            view_.cy = zoomAnchorY_ + (zoomAnchorS_.y - c.y) / view_.yscale();
        }
    } else if (!dimAnimating_) {
        if (overCanvas && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1))) orbiting_ = true;
        if (!ImGui::IsMouseDown(0) && !ImGui::IsMouseDown(1)) orbiting_ = false;
        if (orbiting_) {
            if (ImGui::IsMouseDown(1)) {
                cam_.s.theta -= io.MouseDelta.x * 0.0075;
                cam_.s.phi = std::clamp(cam_.s.phi + io.MouseDelta.y * 0.0065, 0.02, 1.55);
            } else {
                double k = cam_.s.dist / cam_.focal;
                Vec3 f = normalize(Vec3(cam_.fwd.x, cam_.fwd.y, 0));
                cam_.s.target = cam_.s.target - cam_.right * (io.MouseDelta.x * k) + f * (io.MouseDelta.y * k);
            }
        }
        if (overCanvas && io.MouseWheel != 0 && !plat_.trackpad)
            targetDist_ = std::clamp(targetDist_ * std::pow(0.88, (double)io.MouseWheel), 0.05, 1e6);
        if (overCanvas && plat_.magnify != 0) {
            targetDist_ = std::clamp(targetDist_ / std::max(0.2, 1.0 + plat_.magnify), 0.05, 1e6);
            cam_.s.dist = targetDist_;
        }
        if (overCanvas && plat_.trackpad && !extendWorkspace_ && (plat_.scrollX != 0 || plat_.scrollY != 0)) {
            double k = cam_.s.dist / cam_.focal;
            Vec3 f = normalize(Vec3(cam_.fwd.x, cam_.fwd.y, 0));
            cam_.s.target = cam_.s.target - cam_.right * (plat_.scrollX * k) + f * (plat_.scrollY * k);
        }
        double k = 1 - std::exp(-dt * 12.0);
        cam_.s.dist = std::exp(std::log(cam_.s.dist) + (std::log(targetDist_) - std::log(cam_.s.dist)) * k);
        if (autoRotate_ && !orbiting_) cam_.s.theta += dt * 0.16;
    }

}

void App::handleKeys() {  // keyboard shortcuts act on the focused pane (only when not typing)
    if (!ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_Space) && hasSum_) {
            if (sumPlaying_) sumPlaying_ = false;
            else {
                if (sumPos_ >= sumCount_) sumPos_ = 0;
                sumShowAll_ = false;
                sumPlaying_ = true;
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_D)) toggleDeriv();
        if (ImGui::IsKeyPressed(ImGuiKey_C)) toggleComplex();
        if (ImGui::IsKeyPressed(ImGuiKey_3)) toggle3D();
        if (ImGui::IsKeyPressed(ImGuiKey_R)) resetView();
    }
}

// ------------------------------------------------------------------ examples

const Example kExamples[] = {
    {"Taylor series of sin(x)", "sum(k, 0, n, (-1)^k x^(2k+1) / (2k+1)!)", false, false},
    {"Fourier square wave", "sum(k, 1, n, 4/pi sin((2k-1)x) / (2k-1))", false, false},
    {"Infinite series: e^x = sum x^k / k!", "sum(k, 0, inf, x^k / k!)", false, false},
    {"Infinite series: 1/(1-x), only for |x| < 1", "sum(k, 0, inf, x^k)", false, false, 0, 0, 160},
    {"Infinite series: ln(1+x) on (-1, 1]", "sum(k, 1, inf, (-1)^(k+1) x^k / k)", false, false, 0, 0, 160},
    {"Riemann zeta on the critical line |zeta(1/2 + ix)|", "abs(zeta(1/2 + i x))", false, false, 20, 1.6, 26, 4.5},
    {"Wave  a sin(bx + c)", "a sin(b x + c)", false, false},
    {"Cubic  x^3 - 3x", "x^3 - 3x", false, false},
    {"Bell curve", "e^(-x^2/2) / sqrt(2pi)", false, false},
    {"Travelling ripple (3D) - animate a", "cos(2 sqrt(x^2 + y^2) - a) e^(-(x^2 + y^2)/12)", false, true},
    {"Saddle (3D)", "x^2 - y^2", false, true},
    {"Squaring  z^2 (complex)", "z^2", true, false},
    {"Inversion  1/z (complex)", "1/z", true, false},
    {"Exponential  e^z (complex)", "exp(z)", true, false},
    {"Riemann zeta function zeta(z) (complex)", "zeta(z)", true, false, 1, 0, 80},
    {"Zeta as an infinite series sum 1/n^z (complex)", "sum(n, 1, inf, 1/n^z)", true, false, 2, 0, 80},
    {"Zeta landscape at its first zero 1/2 + 14.13i (3D complex)", "zeta(z)", true, true, 0.5, 14.13, 150},
    {"Sum over all integers 1/(z-n)^2 (complex)", "sum(n, -inf, inf, 1/(z - n)^2)", true, false},
    {"Geometric series sum z^k (complex)", "sum(k, 0, n, z^k)", true, false},
    {"Poles landscape (3D complex)", "(z^2 - 1)(z - 2 - i)^2 / (z^2 + 2 + 2i)", true, true},
};
const int kNumExamples = (int)(sizeof(kExamples) / sizeof(kExamples[0]));

void App::loadExample(int i) {
    if (i < 0 || i >= kNumExamples) return;
    const Example& ex = kExamples[i];
    bool needComplexSwitch = ex.complex != complex_;
    if (needComplexSwitch) {
        // switch instantly so the new equation is parsed against the right mode
        complex_ = ex.complex;
        fade_ = 0;
        fadeStage_ = 2;
        prev_ = Func{};
    }
    if (ex.scale > 0) {
        view_.cx = ex.cx;
        view_.cy = ex.cy;
        view_.scale = targetScale_ = ex.scale;
        view_.aspect = ex.aspect;
        zooming_ = false;
    } else if (!ex.threeD) {
        view_.cx = view_.cy = 0;
        view_.aspect = 1;
        view_.scale = targetScale_ = 72;
        zooming_ = false;
    }
    gridStep_ = 0;
    setSingleFunction(ex.eq);
    if (complex_ && !ex.threeD) { gridT_ = 0; gridAnimP_ = 0; gridAnim_ = true; }
    if (ex.threeD != threeD_ && !dimAnimating_) startDim(ex.threeD);
    else if (threeD_) {
        if (ex.scale > 0) {  // re-frame the surface around the example's region
            double halfH = (canvasMax_.y - canvasMin_.y) * 0.5 / view_.scale;
            domainR_ = (float)std::max(0.5, niceStep(halfH * 0.62));
            domainCx_ = ex.cx;
            domainCy_ = ex.cy;
            resetView();
        }
        autoZScale();
    }
}
