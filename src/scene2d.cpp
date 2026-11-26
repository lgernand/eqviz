// 2D rendering: number plane, real graphs with derivative probe, and the
// complex plane shown as a warped grid (plus optional domain colouring).
#include <algorithm>
#include <cstring>

#include "app.hpp"
#include "gl.hpp"
#include "theme.hpp"

using namespace theme;
static const double PI = 3.14159265358979323846;

// ------------------------------------------------------------------ helpers

void strokePolylines(ImDrawList* dl, const std::vector<std::vector<ImVec2>>& segs, ImU32 c, float th,
                     float progress, bool back) {
    size_t total = 0;
    for (auto& s : segs) total += s.size();
    size_t budget = (size_t)(std::clamp(progress, 0.f, 1.f) * (float)total);
    float a = ((c >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;
    for (int pass = back ? 0 : 1; pass < 2; ++pass) {
        size_t b = budget;
        for (auto& s : segs) {
            if (b < 2) break;
            int n = (int)std::min(s.size(), b);
            if (pass == 0) dl->AddPolyline(s.data(), n, col(BG, 0.85f * a), th + 4.5f, 0);
            else dl->AddPolyline(s.data(), n, c, th, 0);
            b -= n;
        }
    }
}

void dashedLine(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 c, float th, float dash, float gap) {
    float dx = b.x - a.x, dy = b.y - a.y, len = std::sqrt(dx * dx + dy * dy);
    if (len < 1) return;
    dx /= len; dy /= len;
    for (float t = 0; t < len; t += dash + gap) {
        float e = std::min(len, t + dash);
        dl->AddLine(ImVec2(a.x + dx * t, a.y + dy * t), ImVec2(a.x + dx * e, a.y + dy * e), c, th);
    }
}

ImU32 domainColor(cplx w, float alpha) {
    if (!finiteC(w)) return col(WHITE, alpha);
    double mag = std::abs(w);
    float h = (float)(std::arg(w) / (2 * PI));
    if (h < 0) h += 1;
    double lm = std::log2(mag + 1e-300);
    float ring = 0.5f + 0.5f * (float)std::cos(2 * PI * lm);
    float v = 0.66f + 0.34f * ring;
    float s = 0.72f;
    float zeroFade = (float)(mag / (mag + 0.08));
    v *= 0.3f + 0.7f * zeroFade;
    if (mag > 50) s *= (float)(50.0 / mag) * 0.5f + 0.5f;
    return hsv(h, s, v, alpha);
}

static ts::Box imagLabel(double val, float size) {
    if (val == 1) return ts::ident("i", size);
    if (val == -1) return ts::hcat({ts::text("\xE2\x88\x92", size), ts::ident("i", size)});
    return ts::hcat({ts::text(ts::fmtNum(val, 6), size), ts::ident("i", size)});
}

// ------------------------------------------------------------------ plane

void App::drawPlane2D(ImDrawList* dl, float la, bool labels) {
    const View2D& v = view_;
    double x0, y0, x1, y1;
    v.s2w(v.cmin, x0, y1);
    v.s2w(v.cmax, x1, y0);
    double step = niceStep(95.0 / v.scale);
    double mant = step / std::pow(10.0, std::floor(std::log10(step)));
    int sub = std::fabs(mant - 5) < 0.01 ? 5 : 2;
    double minor = step / sub;
    ImU32 cMinor = col(BLUE_D, 0.16f * la), cMajor = col(BLUE_D, 0.58f * la), cAxis = col(WHITE, 0.92f);

    for (long i = (long)std::ceil(x0 / minor); i <= (long)std::floor(x1 / minor); ++i) {
        if (i == 0) continue;
        bool major = i % sub == 0;
        float sx = v.w2s(i * minor, 0).x;
        dl->AddLine(ImVec2(sx, v.cmin.y), ImVec2(sx, v.cmax.y), major ? cMajor : cMinor, major ? 1.5f : 1.0f);
    }
    const double stepY = niceStep(95.0 / v.yscale());
    const double mantY = stepY / std::pow(10.0, std::floor(std::log10(stepY)));
    const int subY = std::fabs(mantY - 5) < 0.01 ? 5 : 2;
    const double minorY = stepY / subY;
    for (long i = (long)std::ceil(y0 / minorY); i <= (long)std::floor(y1 / minorY); ++i) {
        if (i == 0) continue;
        bool major = i % subY == 0;
        float sy = v.w2s(0, i * minorY).y;
        dl->AddLine(ImVec2(v.cmin.x, sy), ImVec2(v.cmax.x, sy), major ? cMajor : cMinor, major ? 1.5f : 1.0f);
    }
    ImVec2 o = v.w2s(0, 0);
    if (o.y >= v.cmin.y && o.y <= v.cmax.y) dl->AddLine(ImVec2(v.cmin.x, o.y), ImVec2(v.cmax.x, o.y), cAxis, 2.2f);
    if (o.x >= v.cmin.x && o.x <= v.cmax.x) dl->AddLine(ImVec2(o.x, v.cmin.y), ImVec2(o.x, v.cmax.y), cAxis, 2.2f);
    if (!labels) return;

    const float ls = 17.f;
    ImU32 lc = col(WHITE, 0.85f);
    float axisY = std::clamp(o.y, v.cmin.y + 4, v.cmax.y - 28);
    float axisX = std::clamp(o.x, v.cmin.x + 48, v.cmax.x - 6);
    for (long j = (long)std::ceil(x0 / step); j <= (long)std::floor(x1 / step); ++j) {
        if (j == 0) continue;
        double val = j * step;
        ts::Box b = ts::text(ts::fmtNum(val, 6), ls);
        float bx = v.w2s(val, 0).x - b.w * 0.5f;
        if (bx < v.cmin.x + 2 || bx + b.w > v.cmax.x - 2) continue;
        ts::draw(dl, b, ImVec2(bx, axisY + 22), lc);
    }
    for (long j = (long)std::ceil(y0 / stepY); j <= (long)std::floor(y1 / stepY); ++j) {
        if (j == 0) continue;
        double val = j * stepY;
        ts::Box b = complex_ ? imagLabel(val, ls) : ts::text(ts::fmtNum(val, 6), ls);
        float by = v.w2s(0, val).y + 6;
        if (by - 14 < v.cmin.y || by > v.cmax.y - 4) continue;
        ts::draw(dl, b, ImVec2(axisX - 9 - b.w, by), lc);
    }
    if (!complex_) {
        ts::Box bx = ts::ident("x", 24), by = ts::ident("y", 24);
        ts::draw(dl, bx, ImVec2(v.cmax.x - 24 - bx.w, axisY - 10), col(WHITE));
        ts::draw(dl, by, ImVec2(axisX + 12, v.cmin.y + 32), col(WHITE));
    }
}

// ------------------------------------------------------------------ real graphs

// Columns: x_i = x0 + i dx, one per pixel (plus one on each side).
static void columnGrid(const View2D& v, int& N, double& x0, double& dx) {
    N = (int)(v.cmax.x - v.cmin.x) + 2;
    dx = 1.0 / v.scale;
    double yy;
    v.s2w(v.cmin, x0, yy);
    x0 -= dx;
}

void App::sampleYs(const std::function<double(double)>& F, std::vector<double>& ys) const {
    int N;
    double x0, dx;
    columnGrid(view_, N, x0, dx);
    ys.assign((size_t)N + 1, NAN);
    // evaluate every column in parallel (series can be expensive)
    const int chunk = 48, nChunks = (N + chunk) / chunk;
    parallelFor(nChunks, [&](int ci) {
        for (int i = ci * chunk; i <= std::min(N, ci * chunk + chunk - 1); ++i) ys[i] = F(x0 + i * dx);
    });
}

void App::segsFromYs(const std::vector<double>& ys, const std::function<double(double)>& F,
                     std::vector<std::vector<ImVec2>>& segs) const {
    const View2D& v = view_;
    const float H = v.cmax.y - v.cmin.y, lim = 3 * H;
    int N;
    double x0, dx;
    columnGrid(v, N, x0, dx);
    N = std::min(N, (int)ys.size() - 1);
    std::vector<ImVec2> cur;
    double prevY = NAN;
    auto flush = [&] {
        if (cur.size() >= 2) segs.push_back(cur);
        cur.clear();
    };
    for (int i = 0; i <= N; ++i) {
        double x = x0 + i * dx, y = ys[i];
        if (!std::isfinite(y)) { flush(); prevY = NAN; continue; }
        if (std::isfinite(prevY) && std::fabs((y - prevY) * v.yscale()) > H * 0.5f) {  // jump or asymptote?
            double ym = F(x - dx * 0.5);
            bool between = std::isfinite(ym) && ym >= std::min(prevY, y) - 1e-9 && ym <= std::max(prevY, y) + 1e-9;
            if (!between) flush();
        }
        ImVec2 s = v.w2s(x, y);
        s.y = std::clamp(s.y, v.cmin.y - lim, v.cmax.y + lim);
        cur.push_back(s);
        prevY = y;
    }
    flush();
}

void App::sampleCurve(const std::function<double(double)>& F, std::vector<std::vector<ImVec2>>& segs) const {
    std::vector<double> ys;
    sampleYs(F, ys);
    segsFromYs(ys, F, segs);
}

void App::drawRealCurves(ImDrawList* dl, float alpha) {
    const View2D& v = view_;
    EvalEnv env = makeEnv();
    const bool morphing = morph_ < 1 && prev_.ok();
    const double ms = smooth(morph_);
    auto F = [&](double x) -> double {
        double y = realPart(evalF(f_, x, 0, env));
        if (morphing) {
            double y0 = realPart(evalF(prev_, x, 0, env));
            y = y0 + (y - y0) * ms;
        }
        return y;
    };

    const FuncDef* act = activeDef();
    const int activeColor = act ? act->color : YELLOW_C;
    const bool activeVisible = !act || rowVisible(*act);

    // the other rows of the project, each in its own colour
    const EvalEnv envFull = makeEnv(INFINITY);
    for (size_t i = 0; i < funcs_.size(); ++i) {
        FuncDef& d = funcs_[i];
        if ((int)i == active_ || !rowVisible(d) || !d.f.ok()) continue;
        RowCache& rc = rowCache_[d.id];
        Hasher hh;
        hh.u((uint64_t)(uintptr_t)d.plot.get());
        for (double p : pvals_) hh.d(p);
        hh.u((uint64_t)maxTerms_);
        hh.d(v.cx); hh.d(v.cy); hh.d(v.scale); hh.d(v.aspect);
        hh.d(v.cmin.x); hh.d(v.cmin.y); hh.d(v.cmax.x); hh.d(v.cmax.y);
        if (hh.h != rc.hash) {
            rc.hash = hh.h;
            rc.segs.clear();
            const Func& fn = d.f;
            sampleCurve([&](double x) { return realPart(evalF(fn, x, 0, envFull)); }, rc.segs);
        }
        strokePolylines(dl, rc.segs, col(d.color, alpha), 3.0f);
    }

    // Partial sums only change at whole term counts and the player blends linearly
    // between them, so cache the active curve per term count (k = -1: the full sum).
    {
        Hasher hh;
        hh.u((uint64_t)(uintptr_t)expr_.get());
        for (double p : pvals_) hh.d(p);
        hh.u((uint64_t)maxTerms_); hh.u((uint64_t)animTerms_);
        hh.d(v.cx); hh.d(v.cy); hh.d(v.scale); hh.d(v.aspect);
        hh.d(v.cmin.x); hh.d(v.cmin.y); hh.d(v.cmax.x); hh.d(v.cmax.y);
        if (hh.h != sumYsHash_) { sumYsHash_ = hh.h; sumYs_.clear(); }
    }
    auto partialF = [this](int k) {
        EvalEnv e = makeEnv(k < 0 ? INFINITY : (double)k);
        return std::function<double(double)>([this, e](double x) { return realPart(evalF(f_, x, 0, e)); });
    };
    auto ysAt = [&](int k) -> const std::vector<double>& {
        auto it = sumYs_.find(k);
        if (it != sumYs_.end()) return it->second;
        if (sumYs_.size() > 12) sumYs_.erase(sumYs_.begin());
        std::vector<double>& ys = sumYs_[k];
        sampleYs(partialF(k), ys);
        return ys;
    };

    // fading trail of previous partial sums
    if (hasSum_ && sumTrail_ && activeVisible) {
        double lim = std::min<double>(sumLimit(), sumCount_);
        int n = (int)std::floor(lim + 1e-9);
        for (int j = 1; j <= 3; ++j) {
            int k = n - j;
            if (k < 1) break;
            std::vector<std::vector<ImVec2>> segs;
            segsFromYs(ysAt(k), partialF(k), segs);
            strokePolylines(dl, segs, lerpCol(col(activeColor, 0.42f * alpha / j), col(BLUE_C, 0.42f * alpha / j), 0.25f * j), 2.0f, 1.f, false);
        }
    }

    {
        Hasher hh;
        hh.u(evalHash());
        hh.d(v.cx); hh.d(v.cy); hh.d(v.scale); hh.d(v.aspect);
        hh.d(v.cmin.x); hh.d(v.cmin.y); hh.d(v.cmax.x); hh.d(v.cmax.y);
        if (hh.h != curveHash_) {
            curveHash_ = hh.h;
            curveSegs_.clear();
            const double lim = sumLimit();
            if (hasSum_ && !morphing) {
                if (std::isinf(lim)) {
                    segsFromYs(ysAt(-1), F, curveSegs_);
                } else {
                    int n = (int)std::floor(lim);
                    double fr = lim - n;
                    const std::vector<double>& a = ysAt(n);
                    if (fr < 1e-9) segsFromYs(a, F, curveSegs_);
                    else {
                        const std::vector<double>& b = ysAt(n + 1);
                        std::vector<double> ys(std::min(a.size(), b.size()));
                        for (size_t i = 0; i < ys.size(); ++i) ys[i] = a[i] + (b[i] - a[i]) * fr;
                        segsFromYs(ys, F, curveSegs_);
                    }
                }
            } else {
                sampleCurve(F, curveSegs_);
            }
        }
    }
    if (activeVisible) strokePolylines(dl, curveSegs_, col(activeColor, alpha), 3.4f, smooth(create_));

    probeVisible_ = false;
    std::vector<std::vector<ImVec2>> dsegs;
    auto hover = [&] {
        if (!canHover()) return;
        ImVec2 m = ImGui::GetIO().MousePos;
        struct Cand { const std::vector<std::vector<ImVec2>>* segs; std::function<double(double)> f; int color; };
        std::vector<Cand> cands;
        if (activeVisible) cands.push_back({&curveSegs_, F, activeColor});
        cands.push_back({&dsegs, [&](double x) { return realPart(evalF(fx_, x, 0, env)); }, GREEN_C});
        for (size_t i = 0; i < funcs_.size(); ++i) {
            const FuncDef& d = funcs_[i];
            if ((int)i == active_ || !rowVisible(d) || !d.f.ok()) continue;
            const Func* fn = &d.f;
            cands.push_back({&rowCache_[d.id].segs, [fn, &envFull](double x) { return realPart(evalF(*fn, x, 0, envFull)); }, d.color});
        }
        float best = 14.f * 14.f;
        int bestC = -1;
        ImVec2 bestP;
        for (int c = 0; c < (int)cands.size(); ++c)
            for (const auto& s : *cands[c].segs)
                for (const ImVec2& p : s) {
                    float d = (p.x - m.x) * (p.x - m.x) + (p.y - m.y) * (p.y - m.y);
                    if (d < best) { best = d; bestC = c; bestP = p; }
                }
        if (bestC < 0) return;
        double wx, wy;
        view_.s2w(bestP, wx, wy);
        double fy = cands[bestC].f(wx);
        if (!std::isfinite(fy)) return;
        ImVec2 P = view_.w2s(wx, fy);
        ImU32 c = col(cands[bestC].color);
        dashedLine(dl, P, ImVec2(P.x, view_.w2s(0, 0).y), col(cands[bestC].color, 0.45f), 1.2f);
        dl->AddCircleFilled(P, 6.f, c);
        dl->AddCircle(P, 6.f, col(BG), 0, 2.f);
        const float s = 19.f;
        hoverLabel(dl, P, ts::parens(ts::hcat({ts::text(ts::fmtNum(wx, 5), s), ts::text(",", s), ts::space(6),
                                               ts::text(ts::fmtNum(fy, 5), s)}), s), c);
    };
    if (!deriv_ || !fx_.ok()) { hover(); return; }
    const float da = alpha * smooth(derivCreate_);
    auto G = [&](double x) { return realPart(evalF(fx_, x, 0, env)); };
    sampleCurve(G, dsegs);
    strokePolylines(dl, dsegs, col(GREEN_C, alpha), 2.8f, smooth(derivCreate_));

    double x0 = probeX_, y0 = F(x0), m = G(x0);
    if (!std::isfinite(y0) || !std::isfinite(m)) return;
    ImVec2 P = v.w2s(x0, y0), Q = v.w2s(x0, m);
    double L = 180.0 / (v.scale * std::sqrt(1 + m * m * v.aspect * v.aspect));
    ImVec2 A = v.w2s(x0 - L, y0 - m * L), B = v.w2s(x0 + L, y0 + m * L);
    dashedLine(dl, P, Q, col(GREEN_C, 0.6f * da), 1.6f);
    dl->AddLine(A, B, col(BG, 0.8f * da), 7.f);
    dl->AddLine(A, B, col(RED_C, da), 2.8f);
    dl->AddCircleFilled(Q, 5.5f, col(GREEN_C, da));
    dl->AddCircleFilled(P, 7.5f, col(WHITE, da));
    dl->AddCircle(P, 7.5f, col(RED_C, da), 0, 2.5f);

    float s = 21.f;
    ts::Box lab = ts::hcat({ts::text("slope", s), ts::binop("=", s), ts::text(ts::fmtNum(m, 3), s)});
    ImVec2 lp(B.x + 10, B.y - 8);
    if (lp.x + lab.w > v.cmax.x - 10) lp.x = A.x - lab.w - 10;
    lp.y = std::clamp(lp.y, v.cmin.y + 30, v.cmax.y - 12);
    dl->AddRectFilled(ImVec2(lp.x - 6, lp.y - lab.asc - 5), ImVec2(lp.x + lab.w + 6, lp.y + lab.desc + 5), col(BG, 0.7f * da), 6);
    ts::draw(dl, lab, lp, col(RED_C, da));
    probeScreen_ = P;
    probeVisible_ = P.x > v.cmin.x && P.x < v.cmax.x && P.y > v.cmin.y && P.y < v.cmax.y;
    float pdx = ImGui::GetIO().MousePos.x - P.x, pdy = ImGui::GetIO().MousePos.y - P.y;
    if (pdx * pdx + pdy * pdy > 16 * 16) hover();
}

// ------------------------------------------------------------------ complex grid

template <class WF>
static void mapLine(const WF& W, cplx a, cplx b, const View2D& v, std::vector<std::vector<cplx>>& out) {
    const float Wd = v.cmax.x - v.cmin.x, H = v.cmax.y - v.cmin.y;
    const float L = v.cmin.x - 60, R = v.cmax.x + 60, T = v.cmin.y - 60, B = v.cmax.y + 60;
    const float maxD = 6.f, jumpD = std::max(Wd, H) * 0.9f;
    const int maxDepth = 14;  // off-screen pieces are never subdivided, so depth is cheap
    float lenPx = (float)(std::abs(b - a) * v.scale);
    int baseN = std::clamp((int)(lenPx / 10), 24, 400);
    std::vector<cplx> cur;
    auto flush = [&] {
        if (cur.size() >= 2) out.push_back(cur);
        cur.clear();
    };
    auto ok = [](cplx w) { return finiteC(w) && std::fabs(w.real()) < 1e8 && std::fabs(w.imag()) < 1e8; };
    auto S = [&](cplx w) { return v.w2s(w.real(), w.imag()); };
    auto sameSideOff = [&](ImVec2 p, ImVec2 q) {
        return (p.x < L && q.x < L) || (p.x > R && q.x > R) || (p.y < T && q.y < T) || (p.y > B && q.y > B);
    };
    std::function<void(cplx, cplx, cplx, cplx, int)> seg = [&](cplx z0, cplx w0, cplx z1, cplx w1, int d) {
        bool f0 = ok(w0), f1 = ok(w1);
        if (f0 && f1) {
            ImVec2 s0 = S(w0), s1 = S(w1);
            float dx = s1.x - s0.x, dy = s1.y - s0.y, dist = std::sqrt(dx * dx + dy * dy);
            bool off = sameSideOff(s0, s1);
            if (dist > maxD && d < maxDepth && !off) {
                cplx zm = 0.5 * (z0 + z1), wm = W(zm);
                seg(z0, w0, zm, wm, d + 1);
                seg(zm, wm, z1, w1, d + 1);
                return;
            }
            if (dist > jumpD && !off) { flush(); cur.push_back(w1); return; }
            if (cur.empty()) cur.push_back(w0);
            cur.push_back(w1);
            return;
        }
        if ((f0 || f1) && d < maxDepth - 2) {
            cplx zm = 0.5 * (z0 + z1), wm = W(zm);
            seg(z0, w0, zm, wm, d + 1);
            seg(zm, wm, z1, w1, d + 1);
            return;
        }
        flush();
        if (f1) cur.push_back(w1);
    };
    cplx zp = a, wp = W(a);
    if (ok(wp)) cur.push_back(wp);
    for (int i = 1; i <= baseN; ++i) {
        cplx z = a + (b - a) * ((double)i / baseN), w = W(z);
        seg(zp, wp, z, w, 0);
        zp = z;
        wp = w;
    }
    flush();
}

// The input grid is chosen once (from the view when the transform starts) and then
// kept fixed, so zooming or panning magnifies the same transformed lines instead of
// swapping in a grid whose image may lie off-screen.
void App::fitGridToView() {
    double x0, y0, x1, y1;
    view_.s2w(view_.cmin, x0, y1);
    view_.s2w(view_.cmax, x1, y0);
    gridStep_ = niceStep(95.0 / view_.scale) / 2;
    const double m = 2 * gridStep_;
    gridX0_ = x0 - m; gridX1_ = x1 + m;
    gridY0_ = y0 - m; gridY1_ = y1 + m;
    gridHash_ = 0;
}

void App::warpedGrid() {
    const View2D& v = view_;
    if (!(gridStep_ > 0)) fitGridToView();
    Hasher hh;
    hh.u(evalHash());
    hh.d(gridX0_); hh.d(gridX1_); hh.d(gridY0_); hh.d(gridY1_); hh.d(gridStep_);
    hh.d(gridT_); hh.d(v.cx); hh.d(v.cy); hh.d(v.scale); hh.d(v.aspect);
    hh.d(v.cmin.x); hh.d(v.cmin.y); hh.d(v.cmax.x); hh.d(v.cmax.y);
    if (hh.h == gridHash_ && !gridLines_.empty()) return;
    gridHash_ = hh.h;
    auto& out = gridLines_;
    auto& kinds = gridKinds_;
    const double x0 = gridX0_, x1 = gridX1_, y0 = gridY0_, y1 = gridY1_;
    double sp = gridStep_;
    while ((x1 - x0) / sp > 240 || (y1 - y0) / sp > 240) sp *= 2;  // keep the line count sane
    struct Ln { cplx a, b; int kind; };
    std::vector<Ln> ls;
    for (long i = (long)std::ceil(x0 / sp); i <= (long)std::floor(x1 / sp); ++i)
        ls.push_back({cplx(i * sp, y0), cplx(i * sp, y1), i == 0 ? 3 : (i % 2 == 0 ? 1 : 0)});
    for (long j = (long)std::ceil(y0 / sp); j <= (long)std::floor(y1 / sp); ++j)
        ls.push_back({cplx(x0, j * sp), cplx(x1, j * sp), j == 0 ? 2 : (j % 2 == 0 ? 1 : 0)});

    EvalEnv env = makeEnv();
    const Func& cur = displayFunc();
    const bool morphing = morph_ < 1 && prev_.ok();
    const double ms = smooth(morph_), t = gridT_;
    auto W = [&](cplx z) {
        cplx w = evalF(cur, z, 0, env);
        if (morphing) {
            cplx w0 = evalF(prev_, z, 0, env);
            w = w0 + (w - w0) * ms;
        }
        return (1.0 - t) * z + t * w;
    };
    out.assign(ls.size(), {});
    kinds.assign(ls.size(), 0);
    parallelFor((int)ls.size(), [&](int i) {
        kinds[i] = ls[i].kind;
        mapLine(W, ls[i].a, ls[i].b, v, out[i]);
    });
}

void App::drawComplexGrid(ImDrawList* dl, float alpha) {
    warpedGrid();
    const auto& lines = gridLines_;
    const auto& kinds = gridKinds_;
    std::vector<ImVec2> pts;
    auto drawKind = [&](int kind, ImU32 c, float th) {
        for (size_t i = 0; i < lines.size(); ++i) {
            if (kinds[i] != kind) continue;
            for (const auto& pl : lines[i]) {
                pts.resize(pl.size());
                for (size_t k = 0; k < pl.size(); ++k) {
                    ImVec2 s = view_.w2s(pl[k].real(), pl[k].imag());
                    pts[k] = ImVec2(std::clamp(s.x, -1e5f, 1e5f), std::clamp(s.y, -1e5f, 1e5f));
                }
                dl->AddPolyline(pts.data(), (int)pts.size(), c, th, 0);
            }
        }
    };
    drawKind(0, col(BLUE_C, 0.42f * alpha), 1.1f);
    drawKind(1, col(BLUE_C, 0.95f * alpha), 1.9f);
    drawKind(3, col(RED_C, alpha), 3.0f);
    drawKind(2, col(YELLOW_C, alpha), 3.0f);

    if (!canHover()) return;
    ImVec2 m = ImGui::GetIO().MousePos;
    double zx, zy;
    view_.s2w(m, zx, zy);
    const cplx z(zx, zy);
    EvalEnv env = makeEnv();
    const Func& cur = displayFunc();
    cplx fz = evalF(cur, z, 0, env);
    if (!finiteC(fz)) return;
    cplx w = (1.0 - (double)gridT_) * z + (double)gridT_ * fz;
    ImVec2 W = view_.w2s(w.real(), w.imag());
    if (std::fabs(W.x) < 1e5f && std::fabs(W.y) < 1e5f) {
        dashedLine(dl, m, W, col(WHITE, 0.5f), 1.3f);
        dl->AddCircleFilled(W, 6.f, col(YELLOW_C));
        dl->AddCircle(W, 6.f, col(BG), 0, 2.f);
    }
    dl->AddCircleFilled(m, 4.5f, col(WHITE));
    const float s = 19.f;
    ts::Box fl = (complex_ && deriv_) ? ts::hcat({ts::ident("f", s), ts::text("\xE2\x80\xB2", s)}) : ts::ident("f", s);
    ts::Box l1 = ts::hcat({ts::ident("z", s), ts::binop("=", s), ts::expr(eq::num(z), s)});
    ts::Box l2 = ts::hcat({fl, ts::parens(ts::ident("z", s), s), ts::binop("=", s), ts::expr(eq::num(fz), s)});
    ts::Box b = l1;
    b.w = std::max(l1.w, l2.w);
    float gap = l1.desc + 8 + l2.asc;
    for (auto p : l2.prims) {  // stack the second line below the first
        p.a.y += gap; p.b.y += gap;
        for (auto& q : p.pts) q.y += gap;
        b.prims.push_back(p);
    }
    b.desc = gap + l2.desc;
    hoverLabel(dl, m, b, col(YELLOW_C));
}

void App::drawDomainColoring(ImDrawList* dl, float alpha) {
    const int down = hasSum_ ? 4 : 3;
    const int w = std::max(8, (int)((canvasMax_.x - canvasMin_.x) / down));
    const int h = std::max(8, (int)((canvasMax_.y - canvasMin_.y) / down));
    Hasher hh;
    hh.u(evalHash());
    hh.d(view_.cx); hh.d(view_.cy); hh.d(view_.scale); hh.d(view_.aspect); hh.u((uint64_t)w); hh.u((uint64_t)h);
    const uint64_t hsh = hh.h;
    if (hsh != dcHash_ || !dcTex_) {
        dcHash_ = hsh;
        dcW_ = w;
        dcH_ = h;
        dcPixels_.resize((size_t)w * h);
        EvalEnv env = makeEnv();
        const Func& cur = displayFunc();
        const bool morphing = morph_ < 1 && prev_.ok();
        const double ms = smooth(morph_);
        const float cw = canvasMax_.x - canvasMin_.x, ch = canvasMax_.y - canvasMin_.y;
        parallelFor(h, [&](int row) {
            for (int c = 0; c < w; ++c) {
                ImVec2 s(canvasMin_.x + (c + 0.5f) * cw / w, canvasMin_.y + (row + 0.5f) * ch / h);
                double x, y;
                view_.s2w(s, x, y);
                cplx wv = evalF(cur, cplx(x, y), 0, env);
                if (morphing) {
                    cplx w0 = evalF(prev_, cplx(x, y), 0, env);
                    wv = w0 + (wv - w0) * ms;
                }
                dcPixels_[(size_t)row * w + c] = domainColor(wv, 1.f);
            }
        });
        if (!dcTex_) {
            GLuint t;
            glGenTextures(1, &t);
            dcTex_ = t;
            glBindTexture(GL_TEXTURE_2D, t);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_2D, dcTex_);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dcPixels_.data());
    }
    dl->AddImage((ImTextureID)(uintptr_t)dcTex_, canvasMin_, canvasMax_, ImVec2(0, 0), ImVec2(1, 1),
                 col(WHITE, alpha * dcOpacity_));
}

// ------------------------------------------------------------------ scene

void App::drawScene2D(ImDrawList* dl) {
    const float a = fade_;
    if (complex_ && domainColor_) drawDomainColoring(dl, a);
    drawPlane2D(dl, complex_ ? 0.45f : 1.f, true);
    if (complex_) {
        probeVisible_ = false;
        drawComplexGrid(dl, a);
    } else {
        drawRealCurves(dl, a);
    }
}
