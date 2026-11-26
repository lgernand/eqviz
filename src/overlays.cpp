// Teaching overlays for real 2D panes:
//   unit circle   - the angle theta, with sin, cos, tan, sec, cot and csc drawn as segments
//   matrix        - a 2x2 linear map: the transformed grid, basis vectors, determinant, eigenvectors, a vector
//   trace         - a vertical line at x = a parameter, with each curve's value there
//   shaded area   - the region under a row between two bounds, with its integral
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

#include "app.hpp"
#include "misc/cpp/imgui_stdlib.h"
#include "theme.hpp"

using namespace theme;

namespace {

constexpr double PI = 3.14159265358979323846;

void arrow(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 c, float thick) {
    const float dx = b.x - a.x, dy = b.y - a.y, len = std::sqrt(dx * dx + dy * dy);
    if (len < 1) return;
    const float ux = dx / len, uy = dy / len, head = std::min(14.f, len * 0.45f);
    const ImVec2 base(b.x - ux * head, b.y - uy * head);
    dl->AddLine(a, base, c, thick);
    dl->AddTriangleFilled(b, ImVec2(base.x - uy * head * 0.45f, base.y + ux * head * 0.45f),
                          ImVec2(base.x + uy * head * 0.45f, base.y - ux * head * 0.45f), c);
}

void label(ImDrawList* dl, const ts::Box& b, ImVec2 at, ImU32 c, bool backdrop = true) {
    if (backdrop)
        dl->AddRectFilled(ImVec2(at.x - 5, at.y - b.asc - 4), ImVec2(at.x + b.w + 5, at.y + b.desc + 4), col(BG, 0.75f), 5);
    ts::draw(dl, b, at, c);
}

// Exact values at multiples of 30 and 45 degrees.
struct Exact {
    bool ok = false;
    int deg = 0;  // the angle in degrees (any integer multiple of 15 that is a multiple of 30 or 45)
    std::string theta, sin, cos, tan;  // expression sources ("" = undefined)
};

std::string signed_(const std::string& v, bool neg) {
    if (v == "0") return v;
    return neg ? "-" + (v.find('/') != std::string::npos || v.find('(') != std::string::npos ? "(" + v + ")" : v) : v;
}

Exact exactFor(double theta) {
    Exact e;
    const double deg = theta * 180.0 / PI;
    const long d = std::lround(deg);
    if (std::fabs(deg - d) > 2e-3 || (d % 30 != 0 && d % 45 != 0)) return e;
    e.ok = true;
    e.deg = (int)d;
    // theta as a multiple of pi
    if (d == 0) e.theta = "0";
    else {
        long g = std::gcd(std::labs(d), 180L), num = d / g, den = 180 / g;
        std::string n = num == 1 ? "pi" : num == -1 ? "-pi" : std::to_string(num) + " pi";
        e.theta = den == 1 ? n : (num < 0 ? "-" + (num == -1 ? std::string("pi") : std::to_string(-num) + " pi") + "/" + std::to_string(den)
                                          : n + "/" + std::to_string(den));
    }
    const int a = (int)(((d % 360) + 360) % 360);
    const int q = a / 90, r = a % 90;  // quadrant, angle into it
    int ref = (q % 2 == 0) ? r : 90 - r;
    if (r == 0) ref = (q % 2 == 0) ? 0 : 90;
    static const char* S[] = {"0", "1/2", "sqrt(2)/2", "sqrt(3)/2", "1"};  // sin of 0, 30, 45, 60, 90
    auto idx = [](int g) { return g == 0 ? 0 : g == 30 ? 1 : g == 45 ? 2 : g == 60 ? 3 : 4; };
    const double sv = std::sin(a * PI / 180), cv = std::cos(a * PI / 180);
    e.sin = signed_(S[idx(ref)], sv < -1e-9);
    e.cos = signed_(S[idx(90 - ref)], cv < -1e-9);
    static const char* T[] = {"0", "sqrt(3)/3", "1", "sqrt(3)", ""};
    const std::string t = T[idx(ref)];
    e.tan = t.empty() ? "" : signed_(t, sv * cv < -1e-9);
    return e;
}

ts::Box exprBox(const std::string& src, float s) {
    eq::ParseResult r = eq::parse(src);
    return r.root ? ts::expr(r.root, s) : ts::text(src, s);
}

// value (number or expression in the sliders); NaN when it can't be evaluated
double evalScalar(const std::string& src, const std::vector<Param>& ps, const std::vector<double>& pv) {
    if (src.empty()) return NAN;
    eq::ParseResult r = eq::parse(src);
    if (!r.root) return NAN;
    eq::Program prog(r.root);
    if (!prog.valid()) return NAN;
    cplx slots[64];
    const int n = std::min(prog.numSlots(), 64);
    for (int i = 0; i < n; ++i) {
        slots[i] = cplx(0, 0);
        for (size_t k = 0; k < ps.size(); ++k)
            if (ps[k].name == prog.slotNames()[i]) slots[i] = cplx(k < pv.size() ? pv[k] : ps[k].value, 0);
    }
    const cplx v = prog.eval(slots, INFINITY, true, 2048);
    return std::fabs(v.imag()) < 1e-9 ? v.real() : NAN;
}

}  // namespace

// ------------------------------------------------------------------ helpers

double App::paneScalar(const std::string& src) const { return evalScalar(src, params_, pvals_); }

void App::paneParamNames(std::set<std::string>& names) const {
    auto add = [&](const std::string& src) {
        if (src.empty()) return;
        eq::ParseResult r = eq::parse(src);
        if (r.root) eq::freeVars(r.root, names);
    };
    auto visit = [&](bool note, bool circle, const std::string& tr, const std::string& lo, const std::string& hi, int shade) {
        if (note) return;
        if (circle) names.insert("theta");
        add(tr);
        if (shade >= 0) { add(lo); add(hi); }
    };
    visit(isNote_, unitCircle_, trace_, shadeLo_, shadeHi_, shadeRow_);
    for (int i = 0; i < (int)panes_.size(); ++i) {
        if (i == cur_) continue;
        const PaneState& p = panes_[i];
        visit(p.isNote_, p.unitCircle_, p.trace_, p.shadeLo_, p.shadeHi_, p.shadeRow_);
    }
}

double App::thetaValue() const {
    for (size_t i = 0; i < params_.size(); ++i)
        if (params_[i].name == "theta") return i < pvals_.size() ? pvals_[i] : params_[i].value;
    return 0;
}

void App::setTheta(double v) {
    for (auto& p : params_)
        if (p.name == "theta") {
            p.value = (float)std::clamp(v, (double)p.min, (double)p.max);
            p.playing = false;
        }
    updateParamValues();
}

void App::effMatrix(double m[4]) const {
    const float t = smooth(matAnim_);
    m[0] = 1 + (matA_ - 1) * t;
    m[1] = matB_ * t;
    m[2] = matC_ * t;
    m[3] = 1 + (matD_ - 1) * t;
}

// ------------------------------------------------------------------ shaded area and trace

void App::drawShade(ImDrawList* dl) {
    if (shadeRow_ < 0 || shadeRow_ >= (int)funcs_.size()) return;
    const FuncDef& d = funcs_[shadeRow_];
    if (!d.f.ok()) return;
    double lo = paneScalar(shadeLo_), hi = paneScalar(shadeHi_);
    if (!std::isfinite(lo) || !std::isfinite(hi)) return;
    const bool flip = hi < lo;
    if (flip) std::swap(lo, hi);
    const EvalEnv env = makeEnv();
    auto F = [&](double x) { return realPart(evalF(d.f, x, 0, env)); };
    // fill: one thin quad per pixel column (handles curves that cross the axis)
    const double x0 = std::max(lo, (double)(view_.cx - (view_.center().x - canvasMin_.x) / view_.scale));
    const double x1 = std::min(hi, (double)(view_.cx + (canvasMax_.x - view_.center().x) / view_.scale));
    const ImU32 fill = col(d.color, 0.28f);
    if (x1 > x0) {
        const int n = std::clamp((int)((x1 - x0) * view_.scale / 2), 2, 2000);
        for (int i = 0; i < n; ++i) {
            const double xa = x0 + (x1 - x0) * i / n, xb = x0 + (x1 - x0) * (i + 1) / n, xm = 0.5 * (xa + xb);
            const double y = F(xm);
            if (!std::isfinite(y)) continue;
            ImVec2 p0 = view_.w2s(xa, 0), p1 = view_.w2s(xb, y);
            dl->AddRectFilled(ImVec2(p0.x, std::min(p0.y, p1.y)), ImVec2(p1.x + 0.5f, std::max(p0.y, p1.y)), fill);
        }
    }
    // edges at the bounds
    for (double xb : {lo, hi}) {
        const double y = F(xb);
        if (std::isfinite(y)) dl->AddLine(view_.w2s(xb, 0), view_.w2s(xb, y), col(d.color, 0.8f), 1.5f);
    }
    // the integral (midpoint rule, fine enough for step functions too)
    const int N = 6000;
    double sum = 0;
    for (int i = 0; i < N; ++i) {
        const double y = F(lo + (hi - lo) * (i + 0.5) / N);
        if (std::isfinite(y)) sum += y;
    }
    double area = sum * (hi - lo) / N;
    if (flip) area = -area;
    const float s = 19.f * labelScale_;
    std::vector<ts::Box> parts = {ts::text("\xE2\x88\xAB", s * 1.2f), ts::space(3)};
    parts.push_back(ts::sup(ts::text(ts::fmtNum(flip ? lo : lo, 3), s * 0.6f), ts::text(ts::fmtNum(hi, 3), s * 0.6f), s * 0.6f));
    parts.push_back(ts::space(4));
    parts.push_back(ts::ident(d.name, s));
    parts.push_back(ts::binop("=", s));
    parts.push_back(ts::text(ts::fmtNum(std::fabs(area) < 5e-10 ? 0 : area, 4), s));
    ts::Box b = ts::hcat(parts);
    const double ym = F(0.5 * (lo + hi));
    ImVec2 at = view_.w2s(0.5 * (lo + hi), std::isfinite(ym) ? ym * 0.45 : 0);
    at.x -= b.w * 0.5f;
    at.x = std::clamp(at.x, canvasMin_.x + 10, canvasMax_.x - b.w - 10);
    at.y = std::clamp(at.y, canvasMin_.y + 40, canvasMax_.y - 40);
    label(dl, b, at, col(d.color));
}

void App::drawTrace(ImDrawList* dl) {
    if (trace_.empty()) return;
    const double a = paneScalar(trace_);
    if (!std::isfinite(a)) return;
    const ImVec2 top = view_.w2s(a, 0);
    if (top.x < canvasMin_.x || top.x > canvasMax_.x) return;
    dashedLine(dl, ImVec2(top.x, canvasMin_.y), ImVec2(top.x, canvasMax_.y), col(WHITE, 0.45f), 1.5f);
    const EvalEnv env = makeEnv();
    const float s = 16.f * labelScale_;
    for (const FuncDef& d : funcs_) {
        if (!rowVisible(d) || !d.f.ok()) continue;
        const double y = realPart(evalF(d.f, a, 0, env));
        if (!std::isfinite(y)) continue;
        const ImVec2 p = view_.w2s(a, y);
        if (p.y < canvasMin_.y || p.y > canvasMax_.y) continue;
        dl->AddCircleFilled(p, 5.5f, col(d.color), 20);
        dl->AddCircle(p, 7.5f, col(BG, 0.8f), 20, 2.f);
        label(dl, ts::text(ts::fmtNum(y, 4), s), ImVec2(p.x + 10, p.y - 8), col(d.color));
    }
    const float ay = std::clamp(view_.w2s(0, 0).y, canvasMin_.y + 30, canvasMax_.y - 40);
    ts::Box b = ts::hcat({exprBox(trace_, s * 1.1f), ts::binop("=", s * 1.1f), ts::text(ts::fmtNum(a, 4), s * 1.1f)});
    label(dl, b, ImVec2(top.x + 8, ay + 34), col(WHITE));
}

// ------------------------------------------------------------------ unit circle

void App::drawUnitCircle(ImDrawList* dl) {
    const double th = thetaValue();
    const double c = std::cos(th), s = std::sin(th);
    const ImVec2 O = view_.w2s(0, 0), P = view_.w2s(c, s);
    const float R = (float)view_.scale;
    const float ls = 18.f * labelScale_;
    dl->AddCircle(O, R, col(WHITE, 0.75f), 160, 2.f);

    if (circleOpts_ & kCircArc) {  // the arc of length theta
        std::vector<ImVec2> arc;
        const int n = std::clamp((int)(std::fabs(th) * 60), 2, 2000);
        for (int i = 0; i <= n; ++i) {
            const double t = th * i / n;
            arc.push_back(view_.w2s(std::cos(t), std::sin(t)));
        }
        dl->AddPolyline(arc.data(), (int)arc.size(), col(YELLOW_C, 0.9f), 4.f, 0);
    }
    {   // small angle marker at the centre
        std::vector<ImVec2> arc;
        const double r = 0.2, tw = std::clamp(th, -2 * PI, 2 * PI);
        const int n = std::clamp((int)(std::fabs(tw) * 20), 2, 200);
        for (int i = 0; i <= n; ++i) arc.push_back(view_.w2s(r * std::cos(tw * i / n), r * std::sin(tw * i / n)));
        dl->AddPolyline(arc.data(), (int)arc.size(), col(YELLOW_C), 2.f, 0);
        const double mid = tw * 0.5;
        ts::draw(dl, ts::ident("theta", ls), view_.w2s(0.34 * std::cos(mid) - 0.04, 0.34 * std::sin(mid) - 0.05), col(YELLOW_C));
    }
    if (circleOpts_ & kCircTan && std::fabs(c) > 1e-3) {  // tangent segment from P to the x-axis (length |tan|), and sec along the axis
        const ImVec2 X = view_.w2s(1 / c, 0);
        dl->AddLine(view_.w2s(0, -0.05), view_.w2s(1 / c, -0.05), col(PURPLE_C, 0.9f), 3.f);
        dl->AddLine(P, X, col(GOLD_C), 3.5f);
        const ImVec2 mid((P.x + X.x) * 0.5f, (P.y + X.y) * 0.5f);
        label(dl, ts::hcat({ts::text("tan", ls), ts::space(3), ts::ident("theta", ls)}), ImVec2(mid.x + 8, mid.y - 6), col(GOLD_C));
        label(dl, ts::hcat({ts::text("sec", ls * 0.85f), ts::space(3), ts::ident("theta", ls * 0.85f)}),
              ImVec2(std::clamp(view_.w2s(0.5 / c, 0).x - 20, canvasMin_.x + 4, canvasMax_.x - 80), O.y + 40), col(PURPLE_C));
    }
    if (circleOpts_ & kCircCot && std::fabs(s) > 1e-3) {  // segment from P to the y-axis (length |cot|), and csc along the axis
        const ImVec2 Y = view_.w2s(0, 1 / s);
        dl->AddLine(view_.w2s(-0.05, 0), view_.w2s(-0.05, 1 / s), col(PINK, 0.9f), 3.f);
        dl->AddLine(P, Y, col(TEAL_C), 3.5f);
        const ImVec2 mid((P.x + Y.x) * 0.5f, (P.y + Y.y) * 0.5f);
        label(dl, ts::hcat({ts::text("cot", ls), ts::space(3), ts::ident("theta", ls)}), ImVec2(mid.x + 6, mid.y - 10), col(TEAL_C));
        label(dl, ts::hcat({ts::text("csc", ls * 0.85f), ts::space(3), ts::ident("theta", ls * 0.85f)}),
              ImVec2(O.x - 80, std::clamp(view_.w2s(0, 0.5 / s).y, canvasMin_.y + 20, canvasMax_.y - 20)), col(PINK));
    }
    dl->AddLine(O, P, col(WHITE, 0.9f), 2.5f);
    if (circleOpts_ & kCircSinCos) {
        const ImVec2 C = view_.w2s(c, 0);
        dl->AddLine(O, C, col(BLUE_C), 5.f);
        dl->AddLine(C, P, col(RED_C), 5.f);
        dashedLine(dl, P, view_.w2s(0, s), col(RED_C, 0.45f), 1.5f);
        label(dl, ts::hcat({ts::text("cos", ls), ts::space(3), ts::ident("theta", ls)}), ImVec2((O.x + C.x) * 0.5f - 30, O.y + (s >= 0 ? 26 : -12)),
              col(BLUE_C));
        label(dl, ts::hcat({ts::text("sin", ls), ts::space(3), ts::ident("theta", ls)}),
              ImVec2(C.x + (c >= 0 ? 10 : -78), (C.y + P.y) * 0.5f + 6), col(RED_C));
    }
    // the point, draggable
    const ImVec2 m = ImGui::GetIO().MousePos;
    const bool hot = draggingAngle_ || (std::hypot(m.x - P.x, m.y - P.y) < 14 && canHover());
    dl->AddCircleFilled(P, hot ? 9.f : 7.f, col(YELLOW_C), 24);
    dl->AddCircle(P, hot ? 13.f : 11.f, col(BG, 0.9f), 24, 2.5f);
    circleHandle_ = P;
    {
        ts::Box pb = ts::hcat({ts::ident("P", ls * 0.9f), ts::space(4),
                               ts::parens(ts::hcat({ts::text(ts::fmtNum(c, 3), ls * 0.8f), ts::text(",", ls * 0.8f), ts::space(5),
                                                    ts::text(ts::fmtNum(s, 3), ls * 0.8f)}), ls * 0.8f)});
        label(dl, pb, ImVec2(P.x + (c >= 0 ? 14 : -pb.w - 14), P.y + (s >= 0 ? -14 : 26)), col(WHITE));
    }

    // readout card (exact values at special angles)
    const Exact ex = exactFor(th);
    const float cs = 20.f * labelScale_;
    std::vector<std::pair<ts::Box, ImU32>> rows;
    auto value = [&](const std::string& exact, double v) {
        if (ex.ok) {
            if (exact.empty()) return ts::text("undefined", cs * 0.85f);
            ts::Box b = exprBox(exact, cs);
            if (exact.find("sqrt") != std::string::npos)
                b = ts::hcat({b, ts::space(8), ts::text("\xE2\x89\x88 " + ts::fmtNum(v, 4), cs * 0.75f)});
            return b;
        }
        return ts::text(std::fabs(v) > 1e6 ? "undefined" : ts::fmtNum(v, 4), cs);
    };
    {
        std::vector<ts::Box> t = {ts::ident("theta", cs), ts::binop("=", cs)};
        const double degv = th * 180 / PI;
        if (circleOpts_ & kCircDegrees) t.push_back(ts::text(ts::fmtNum(degv, 4) + "\xC2\xB0", cs));
        else {
            t.push_back(ex.ok ? exprBox(ex.theta, cs) : ts::text(ts::fmtNum(th, 4), cs));
            t.push_back(ts::space(8));
            t.push_back(ts::text("(" + ts::fmtNum(degv, 4) + "\xC2\xB0)", cs * 0.75f));
        }
        rows.push_back({ts::hcat(t), col(YELLOW_C)});
    }
    auto fn = [&](const char* name, const std::string& exact, double v, int color) {
        rows.push_back({ts::hcat({ts::text(name, cs), ts::space(4), ts::ident("theta", cs), ts::binop("=", cs), value(exact, v)}), col(color)});
    };
    if (circleOpts_ & kCircSinCos) { fn("cos", ex.cos, c, BLUE_C); fn("sin", ex.sin, s, RED_C); }
    if (circleOpts_ & kCircTan) fn("tan", ex.tan, std::fabs(c) < 1e-12 ? 1e300 : s / c, GOLD_C);
    if (circleOpts_ & kCircCot) fn("cot", "", std::fabs(s) < 1e-12 ? 1e300 : c / s, TEAL_C);
    float w = 0, h = 0;
    for (auto& r : rows) { w = std::max(w, r.first.w); h += r.first.asc + r.first.desc + 10; }
    const float x = canvasMax_.x - w - 30, y = canvasMin_.y + 22;
    dl->AddRectFilled(ImVec2(x - 14, y - 4), ImVec2(canvasMax_.x - 14, y + h + 8), col(BG, 0.9f), 10);
    float yy = y + 6;
    for (auto& r : rows) {
        ts::draw(dl, r.first, ImVec2(x, yy + r.first.asc), r.second);
        yy += r.first.asc + r.first.desc + 10;
    }
}

// ------------------------------------------------------------------ matrix

void App::drawMatrixScene(ImDrawList* dl) {
    double M[4];
    effMatrix(M);
    const double a = M[0], b = M[1], c = M[2], d = M[3];
    const double det = a * d - b * c;
    auto map = [&](double x, double y) { return view_.w2s(a * x + b * y, c * x + d * y); };
    const float ls = 20.f * labelScale_;

    // pre-image range of the visible region
    double wx0, wy0, wx1, wy1;
    view_.s2w(canvasMin_, wx0, wy1);
    view_.s2w(canvasMax_, wx1, wy0);
    double ux0 = -20, ux1 = 20, uy0 = -20, uy1 = 20;
    if (std::fabs(det) > 1e-6) {
        ux0 = uy0 = 1e300;
        ux1 = uy1 = -1e300;
        for (double px : {wx0, wx1})
            for (double py : {wy0, wy1}) {
                const double ix = (d * px - b * py) / det, iy = (-c * px + a * py) / det;
                ux0 = std::min(ux0, ix); ux1 = std::max(ux1, ix);
                uy0 = std::min(uy0, iy); uy1 = std::max(uy1, iy);
            }
    } else {
        const double r = std::max({std::fabs(wx0), std::fabs(wx1), std::fabs(wy0), std::fabs(wy1)}) * 2 + 4;
        ux0 = uy0 = -r;
        ux1 = uy1 = r;
    }
    if (matOpts_ & kMatGrid) {
        auto lines = [&](double lo, double hi, bool vertical, double olo, double ohi) {
            double step = 1;
            while ((hi - lo) / step > 120) step *= 2;
            for (double k = std::ceil(lo / step) * step; k <= hi; k += step) {
                const bool axis = std::fabs(k) < 1e-9;
                ImVec2 p0 = vertical ? map(k, olo - 1) : map(olo - 1, k), p1 = vertical ? map(k, ohi + 1) : map(ohi + 1, k);
                dl->AddLine(p0, p1, axis ? col(WHITE, 0.85f) : col(BLUE_D, 0.75f), axis ? 2.2f : 1.3f);
            }
        };
        lines(ux0, ux1, true, uy0, uy1);
        lines(uy0, uy1, false, ux0, ux1);
    }
    if (matOpts_ & kMatEigen) {
        const double tr = a + d, disc = tr * tr - 4 * det;
        auto eigLine = [&](double lam, double ex, double ey, int color, const char* name) {
            const double n = std::hypot(ex, ey);
            if (n < 1e-12) return;
            ex /= n; ey /= n;
            const double L = 1e4;
            dl->AddLine(view_.w2s(-ex * L, -ey * L), view_.w2s(ex * L, ey * L), col(color, 0.7f), 3.f);
            ImVec2 at = view_.w2s(ex * 2.6, ey * 2.6);
            label(dl, ts::hcat({ts::ident(name, ls * 0.9f), ts::binop("=", ls * 0.9f), ts::text(ts::fmtNum(lam, 3), ls * 0.9f)}),
                  ImVec2(at.x + 8, at.y - 8), col(color));
        };
        auto vecFor = [&](double lam, double& ex, double& ey) {
            if (std::fabs(b) > 1e-9) { ex = b; ey = lam - a; }
            else if (std::fabs(c) > 1e-9) { ex = lam - d; ey = c; }
            else if (std::fabs(lam - a) < 1e-9) { ex = 1; ey = 0; }
            else { ex = 0; ey = 1; }
        };
        if (disc > 1e-9) {
            const double l1 = (tr + std::sqrt(disc)) / 2, l2 = (tr - std::sqrt(disc)) / 2;
            double ex, ey;
            vecFor(l1, ex, ey);
            eigLine(l1, ex, ey, PINK, "lambda_1");
            if (std::fabs(b) < 1e-9 && std::fabs(c) < 1e-9) { ex = std::fabs(l2 - a) < 1e-9 ? 1 : 0; ey = 1 - ex; }
            else vecFor(l2, ex, ey);
            eigLine(l2, ex, ey, TEAL_C, "lambda_2");
        } else if (disc > -1e-9) {
            const double l = tr / 2;
            if (std::fabs(b) < 1e-9 && std::fabs(c) < 1e-9) {
                label(dl, ts::text("every vector is an eigenvector (\xCE\xBB = " + ts::fmtNum(l, 3) + ")", ls * 0.8f),
                      ImVec2(canvasMin_.x + 24, canvasMax_.y - 70), col(PINK));
            } else {
                double ex, ey;
                vecFor(l, ex, ey);
                eigLine(l, ex, ey, PINK, "lambda");
            }
        } else {
            label(dl, ts::text("no real eigenvectors: every direction turns", ls * 0.8f), ImVec2(canvasMin_.x + 24, canvasMax_.y - 70),
                  col(PINK));
        }
    }
    if (matOpts_ & kMatDet) {
        ImVec2 q[4] = {map(0, 0), map(1, 0), map(1, 1), map(0, 1)};
        dl->AddConvexPolyFilled(q, 4, col(YELLOW_C, 0.22f));
        if (det < 0) {  // AddConvexPolyFilled wants one winding; draw the other too
            ImVec2 r[4] = {q[0], q[3], q[2], q[1]};
            dl->AddConvexPolyFilled(r, 4, col(YELLOW_C, 0.22f));
        }
        dl->AddPolyline(q, 4, col(YELLOW_C, 0.9f), 2.f, ImDrawFlags_Closed);
        ImVec2 ctr = map(0.5, 0.5);
        ts::Box db = ts::hcat({ts::text("area", ls * 0.8f), ts::binop("=", ls * 0.8f), ts::text(ts::fmtNum(std::fabs(det), 3), ls * 0.8f)});
        label(dl, db, ImVec2(ctr.x - db.w * 0.5f, ctr.y + 6), col(YELLOW_C));
    }
    if (matOpts_ & kMatBasis) {
        const ImVec2 O = map(0, 0);
        const ImVec2 I = map(1, 0), J = map(0, 1);
        arrow(dl, O, I, col(GREEN_C), 4.f);
        arrow(dl, O, J, col(RED_C), 4.f);
        ts::draw(dl, ts::text("\xC3\xAE", ls * 1.1f), ImVec2(I.x + 8, I.y + 18), col(GREEN_C));
        ts::draw(dl, ts::text("\xC4\xB5", ls * 1.1f), ImVec2(J.x + 8, J.y - 6), col(RED_C));
    }
    if (matOpts_ & kMatTarget) {
        const ImVec2 B = view_.w2s(matBx_, matBy_);
        dl->AddCircle(B, 10.f, col(GOLD_C), 24, 3.f);
        dl->AddLine(ImVec2(B.x - 5, B.y), ImVec2(B.x + 5, B.y), col(GOLD_C), 2.f);
        dl->AddLine(ImVec2(B.x, B.y - 5), ImVec2(B.x, B.y + 5), col(GOLD_C), 2.f);
        label(dl, ts::hcat({ts::ident("b", ls), ts::space(4), ts::parens(ts::text(ts::fmtNum(matBx_, 3) + ", " + ts::fmtNum(matBy_, 3), ls * 0.8f), ls * 0.8f)}),
              ImVec2(B.x + 14, B.y + 22), col(GOLD_C));
    }
    if (matOpts_ & kMatVector) {
        const double vx = matVx_, vy = matVy_;
        const double wx = a * vx + b * vy, wy = c * vx + d * vy;
        const ImVec2 O = view_.w2s(0, 0), V = view_.w2s(vx, vy), W = view_.w2s(wx, wy);
        const bool identity = std::fabs(a - 1) + std::fabs(b) + std::fabs(c) + std::fabs(d - 1) < 1e-6;
        if (!identity) {
            dashedLine(dl, O, V, col(WHITE, 0.5f), 2.f);
            dl->AddCircleFilled(V, 4.f, col(WHITE, 0.6f));
            ts::draw(dl, ts::ident("v", ls * 0.9f), ImVec2(V.x + 8, V.y - 6), col(WHITE, 0.7f));
        }
        arrow(dl, O, W, col(YELLOW_C), 4.5f);
        const ImVec2 m = ImGui::GetIO().MousePos;
        const bool hot = draggingVec_ || (std::hypot(m.x - W.x, m.y - W.y) < 14 && canHover());
        dl->AddCircleFilled(W, hot ? 8.f : 6.f, col(YELLOW_C));
        matHandle_ = W;
        std::vector<ts::Box> parts;
        if (identity) parts = {ts::ident("v", ls)};
        else parts = {ts::ident("M", ls), ts::ident("v", ls)};
        parts.push_back(ts::binop("=", ls));
        parts.push_back(ts::parens(ts::text(ts::fmtNum(wx, 3) + ", " + ts::fmtNum(wy, 3), ls * 0.85f), ls * 0.85f));
        if (!identity) {
            parts.push_back(ts::space(10));
            parts.push_back(ts::ident("v", ls * 0.8f));
            parts.push_back(ts::binop("=", ls * 0.8f));
            parts.push_back(ts::parens(ts::text(ts::fmtNum(vx, 3) + ", " + ts::fmtNum(vy, 3), ls * 0.7f), ls * 0.7f));
        }
        if ((matOpts_ & kMatTarget) && std::hypot(wx - matBx_, wy - matBy_) < 0.02) {
            parts.push_back(ts::space(10));
            parts.push_back(ts::text("= b \xE2\x9C\x93", ls * 0.85f));
        }
        label(dl, ts::hcat(parts), ImVec2(W.x + 14, W.y - 14), col(YELLOW_C));
    }

    // matrix card
    {
        const float s = 22.f * labelScale_;
        ts::Box e[4];
        const double vals[4] = {matA_, matB_, matC_, matD_};
        for (int k = 0; k < 4; ++k) e[k] = ts::text(ts::fmtNum(std::fabs(vals[k]) < 5e-4 ? 0 : vals[k], 3), s);
        const float colW0 = std::max(e[0].w, e[2].w), colW1 = std::max(e[1].w, e[3].w);
        const float rowH = s * 1.25f, gap = s * 0.9f;
        ts::Box mlab = ts::hcat({ts::ident("M", s), ts::binop("=", s)});
        const float x = canvasMin_.x + 26, y = canvasMin_.y + 22;
        const float bw = colW0 + gap + colW1 + 24, bh = rowH * 2 + 10;
        float extra = 0;
        ts::Box detBox;
        if (matOpts_ & kMatDet) {
            detBox = ts::hcat({ts::text("det", s * 0.8f), ts::space(4), ts::ident("M", s * 0.8f), ts::binop("=", s * 0.8f),
                               ts::text(ts::fmtNum(std::fabs(matA_ * matD_ - matB_ * matC_) < 5e-4 ? 0 : matA_ * matD_ - matB_ * matC_, 3), s * 0.8f)});
            extra = detBox.asc + detBox.desc + 10;
        }
        dl->AddRectFilled(ImVec2(x - 14, y - 6), ImVec2(x + mlab.w + bw + 16, y + bh + 10 + extra), col(BG, 0.9f), 10);
        ts::draw(dl, mlab, ImVec2(x, y + bh * 0.5f + mlab.asc * 0.35f), col(WHITE));
        const float bx = x + mlab.w + 4;
        const ImU32 wc = col(WHITE);
        dl->AddLine(ImVec2(bx, y), ImVec2(bx, y + bh), wc, 2.f);
        dl->AddLine(ImVec2(bx, y), ImVec2(bx + 7, y), wc, 2.f);
        dl->AddLine(ImVec2(bx, y + bh), ImVec2(bx + 7, y + bh), wc, 2.f);
        dl->AddLine(ImVec2(bx + bw, y), ImVec2(bx + bw, y + bh), wc, 2.f);
        dl->AddLine(ImVec2(bx + bw, y), ImVec2(bx + bw - 7, y), wc, 2.f);
        dl->AddLine(ImVec2(bx + bw, y + bh), ImVec2(bx + bw - 7, y + bh), wc, 2.f);
        const int colors[4] = {GREEN_C, RED_C, GREEN_C, RED_C};  // columns: where i-hat and j-hat land
        for (int k = 0; k < 4; ++k) {
            const int r = k / 2, cc = k % 2;
            const float cx = bx + 12 + (cc == 0 ? colW0 - e[k].w : colW0 + gap + colW1 - e[k].w);
            ts::draw(dl, e[k], ImVec2(cx, y + 5 + rowH * r + rowH * 0.78f), (matOpts_ & kMatBasis) ? col(colors[k]) : wc);
        }
        if (matOpts_ & kMatDet) ts::draw(dl, detBox, ImVec2(x, y + bh + 8 + detBox.asc), col(YELLOW_C));
    }
}

// ------------------------------------------------------------------ input

bool App::overlayMouse(ImVec2 m, bool clicked) {
    if (complex_ || threeD_) return false;
    if (clicked) {
        if (unitCircle_ && std::hypot(m.x - circleHandle_.x, m.y - circleHandle_.y) < 16) { draggingAngle_ = true; return true; }
        if (matrix_ && (matOpts_ & kMatVector) && std::hypot(m.x - matHandle_.x, m.y - matHandle_.y) < 16) { draggingVec_ = true; return true; }
    }
    if (draggingAngle_) {
        double wx, wy;
        view_.s2w(m, wx, wy);
        double t = std::atan2(wy, wx);
        const double cur = thetaValue();
        t += 2 * PI * std::round((cur - t) / (2 * PI));  // stay on the same turn
        if (circleOpts_ & kCircSnap) {
            const double deg = t * 180 / PI;
            for (int k = (int)std::floor(deg / 15) - 1; k <= (int)std::ceil(deg / 15) + 1; ++k) {
                const int dd = k * 15;
                if ((dd % 30 == 0 || dd % 45 == 0) && std::fabs(deg - dd) < 2.5) t = dd * PI / 180;
            }
        }
        setTheta(t);
        return true;
    }
    if (draggingVec_) {
        double wx, wy;
        view_.s2w(m, wx, wy);
        double M[4];
        effMatrix(M);
        const double det = M[0] * M[3] - M[1] * M[2];
        double vx = wx, vy = wy;
        if (std::fabs(det) > 1e-9) { vx = (M[3] * wx - M[1] * wy) / det; vy = (-M[2] * wx + M[0] * wy) / det; }
        // snap to whole-number inputs when the result is within a few pixels
        const double rx = std::round(vx * 2) / 2, ry = std::round(vy * 2) / 2;
        const ImVec2 sp = view_.w2s(M[0] * rx + M[1] * ry, M[2] * rx + M[3] * ry);
        if (std::hypot(sp.x - m.x, sp.y - m.y) < 9) { vx = rx; vy = ry; }
        matVx_ = vx;
        matVy_ = vy;
        return true;
    }
    return false;
}

// ------------------------------------------------------------------ panel

void App::drawOverlaySection() {
    auto labelRow = [](const char* label, float w = 110.f) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(w);
        ImGui::SetNextItemWidth(-1);
    };
    auto flag = [](const char* name, int& opts, int bit) {
        bool v = (opts & bit) != 0;
        if (ImGui::Checkbox(name, &v)) opts = v ? (opts | bit) : (opts & ~bit);
    };
    if (ImGui::Checkbox("Unit circle", &unitCircle_)) {
        if (unitCircle_) { matrix_ = false; view_.aspect = 1; }
        rebuildParams();
    }
    ImGui::SetItemTooltip("Draw the unit circle with the angle \xCE\xB8 and the trigonometric functions as lengths");
    if (unitCircle_) {
        ImGui::Indent(12);
        const float half = ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x) * 0.5f;
        flag("sin & cos", circleOpts_, kCircSinCos);
        ImGui::SameLine(half);
        flag("tan & sec", circleOpts_, kCircTan);
        flag("cot & csc", circleOpts_, kCircCot);
        ImGui::SameLine(half);
        flag("Arc length", circleOpts_, kCircArc);
        flag("Snap to special angles", circleOpts_, kCircSnap);
        ImGui::SameLine(0, 12);
        flag("Degrees", circleOpts_, kCircDegrees);
        ImGui::TextDisabled("Drag the yellow point, or the \xCE\xB8 slider.");
        ImGui::Unindent(12);
    }
    if (ImGui::Checkbox("Matrix transformation", &matrix_)) {
        if (matrix_) { unitCircle_ = false; view_.aspect = 1; matAnim_ = 0; }
    }
    ImGui::SetItemTooltip("Apply a 2\xC3\x97" "2 matrix to the whole plane");
    if (matrix_) {
        ImGui::Indent(12);
        double* m[4] = {&matA_, &matB_, &matC_, &matD_};
        const float w = (ImGui::GetContentRegionAvail().x - 6) * 0.5f;
        for (int k = 0; k < 4; ++k) {
            if (k % 2) ImGui::SameLine(0, 6);
            ImGui::PushID(k);
            ImGui::SetNextItemWidth(w);
            float v = (float)*m[k];
            if (ImGui::DragFloat("##m", &v, 0.01f, -10.f, 10.f, "%.2f")) { *m[k] = v; matAnim_ = 1; }
            ImGui::PopID();
        }
        struct Preset { const char* name; double m[4]; };
        static const Preset presets[] = {
            {"Identity", {1, 0, 0, 1}}, {"Rotate 90\xC2\xB0", {0, -1, 1, 0}}, {"Rotate 30\xC2\xB0", {0.866, -0.5, 0.5, 0.866}},
            {"Shear", {1, 1, 0, 1}}, {"Stretch", {2, 0, 0, 0.5}}, {"Reflect in y = x", {0, 1, 1, 0}},
            {"Squash onto a line (det 0)", {1, 2, 0.5, 1}}, {"Two eigen-directions", {3, 1, 0, 2}}};
        ImGui::SetNextItemWidth(w);
        if (ImGui::BeginCombo("##preset", "Presets")) {
            for (const Preset& p : presets)
                if (ImGui::Selectable(p.name)) { matA_ = p.m[0]; matB_ = p.m[1]; matC_ = p.m[2]; matD_ = p.m[3]; matAnim_ = 0; }
            ImGui::EndCombo();
        }
        ImGui::SameLine(0, 6);
        if (ImGui::Button("Play", ImVec2(w, 0))) matAnim_ = 0;
        ImGui::SetItemTooltip("Animate from the identity to this matrix");
        const float half = ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x) * 0.5f;
        flag("Grid", matOpts_, kMatGrid);
        ImGui::SameLine(half);
        flag("Basis \xC3\xAE, \xC4\xB5", matOpts_, kMatBasis);
        flag("Determinant", matOpts_, kMatDet);
        ImGui::SameLine(half);
        flag("Eigenvectors", matOpts_, kMatEigen);
        flag("Vector v", matOpts_, kMatVector);
        ImGui::SameLine(half);
        flag("Target b", matOpts_, kMatTarget);
        if (matOpts_ & kMatVector) {
            labelRow("v", 40);
            double v[2] = {matVx_, matVy_};
            if (ImGui::DragScalarN("##v", ImGuiDataType_Double, v, 2, 0.01f, nullptr, nullptr, "%.2f")) { matVx_ = v[0]; matVy_ = v[1]; }
        }
        if (matOpts_ & kMatTarget) {
            labelRow("b", 40);
            double v[2] = {matBx_, matBy_};
            if (ImGui::DragScalarN("##b", ImGuiDataType_Double, v, 2, 0.01f, nullptr, nullptr, "%.2f")) { matBx_ = v[0]; matBy_ = v[1]; }
        }
        ImGui::Unindent(12);
    }
    {   // trace
        labelRow("Trace  x =", 110);
        if (ImGui::BeginCombo("##trace", trace_.empty() ? "off" : ts::greekOrName(trace_).c_str())) {
            if (ImGui::Selectable("off", trace_.empty())) { trace_.clear(); rebuildParams(); }
            for (const Param& p : params_)
                if (ImGui::Selectable(ts::greekOrName(p.name).c_str(), trace_ == p.name)) { trace_ = p.name; rebuildParams(); }
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("A vertical line at x = this slider, with every curve's value there");
    }
    {   // shaded area
        labelRow("Shade area", 110);
        std::string cur = shadeRow_ >= 0 && shadeRow_ < (int)funcs_.size() ? funcs_[shadeRow_].name : "off";
        if (ImGui::BeginCombo("##shade", cur.c_str())) {
            if (ImGui::Selectable("off", shadeRow_ < 0)) shadeRow_ = -1;
            for (int r = 0; r < (int)funcs_.size(); ++r)
                if (funcs_[r].body && ImGui::Selectable(funcs_[r].name.c_str(), shadeRow_ == r)) {
                    shadeRow_ = r;
                    if (shadeLo_.empty()) shadeLo_ = "0";
                    if (shadeHi_.empty()) shadeHi_ = "1";
                    rebuildParams();
                }
            ImGui::EndCombo();
        }
        if (shadeRow_ >= 0) {
            const float w = (ImGui::GetContentRegionAvail().x - 110 - 6) * 0.5f;
            ImGui::Dummy(ImVec2(0, 0));
            ImGui::SameLine(110);
            ImGui::SetNextItemWidth(w);
            bool ch = ImGui::InputTextWithHint("##slo", "from", &shadeLo_);
            ImGui::SameLine(0, 6);
            ImGui::SetNextItemWidth(w);
            ch |= ImGui::InputTextWithHint("##shi", "to", &shadeHi_);
            if (ch) rebuildParams();
            ImGui::SetItemTooltip("Bounds: numbers or expressions in the sliders, like 0, pi or a");
        }
    }
}
