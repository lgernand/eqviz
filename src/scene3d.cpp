// 3D rendering: manim-style ThreeDAxes + Surface drawn with a painter's
// algorithm, including the animated 2D <-> 3D camera transition.
#include <algorithm>

#include "app.hpp"
#include "imgui_internal.h"
#include "theme.hpp"

using namespace theme;

namespace {

struct Builder {
    const Camera& cam;
    std::vector<Prim3>& out;

    void line(const Vec3& a, const Vec3& b, ImU32 c, float th, int segs = 1, float bias = 0) {
        for (int i = 0; i < segs; ++i) {
            Vec3 p = a + (b - a) * ((double)i / segs), q = a + (b - a) * ((double)(i + 1) / segs);
            Prim3 pr;
            float d0, d1;
            if (!cam.project(p, pr.p[0], d0) || !cam.project(q, pr.p[1], d1)) continue;
            pr.kind = Prim3::Line;
            pr.depth = 0.5f * (d0 + d1) + bias;
            pr.fill = c;
            pr.thick = th;
            out.push_back(pr);
        }
    }
    void quad(const Vec3 v[4], ImU32 fill, ImU32 stroke, float th, float bias = 0) {
        Prim3 pr;
        float d = 0, di;
        for (int k = 0; k < 4; ++k) {
            if (!cam.project(v[k], pr.p[k], di)) return;
            d += di;
        }
        pr.kind = Prim3::Quad;
        pr.depth = d * 0.25f + bias;
        pr.fill = fill;
        pr.stroke = stroke;
        pr.thick = th;
        out.push_back(pr);
    }
    void quadG(const Vec3 v[4], const ImU32 vc[4], ImU32 stroke, float th) {
        Prim3 pr;
        float d = 0, di;
        for (int k = 0; k < 4; ++k) {
            if (!cam.project(v[k], pr.p[k], di)) return;
            d += di;
            pr.vc[k] = vc[k];
        }
        pr.kind = Prim3::QuadG;
        pr.depth = d * 0.25f;
        pr.stroke = stroke;
        pr.thick = th;
        out.push_back(pr);
    }
    void dot(const Vec3& p, float r, ImU32 c, float bias = -0.01f) {
        Prim3 pr;
        float d;
        if (!cam.project(p, pr.p[0], d)) return;
        pr.kind = Prim3::Dot;
        pr.depth = d + bias;
        pr.fill = c;
        pr.thick = r;
        out.push_back(pr);
    }
    void arrowTip(const Vec3& from, const Vec3& tip, ImU32 c, float size) {
        ImVec2 a, b;
        float da, db;
        if (!cam.project(from, a, da) || !cam.project(tip, b, db)) return;
        float dx = b.x - a.x, dy = b.y - a.y, l = std::sqrt(dx * dx + dy * dy);
        if (l < 1e-3f) return;
        dx /= l; dy /= l;
        Prim3 pr;
        pr.kind = Prim3::Tri;
        pr.depth = db;
        pr.fill = c;
        pr.p[0] = ImVec2(b.x + dx * size, b.y + dy * size);
        pr.p[1] = ImVec2(b.x - dy * size * 0.5f, b.y + dx * size * 0.5f);
        pr.p[2] = ImVec2(b.x + dy * size * 0.5f, b.y - dx * size * 0.5f);
        out.push_back(pr);
    }
};

ImU32 heightColor(float t, float a) {
    const int stops[] = {BLUE_E, BLUE_C, GREEN_C, YELLOW_C, RED_C};
    t = std::clamp(t, 0.f, 1.f) * 4.f;
    int i = std::min(3, (int)t);
    return lerpCol(col(stops[i], a), col(stops[i + 1], a), t - i);
}

}  // namespace

// Evaluate the displayed function on the (N+1)^2 surface grid into surfVals_.
void App::fillSurface(int N) {
    const double R = domainR_, cx = domainCx_, cy = domainCy_;
    EvalEnv env = makeEnv();
    const Func& cur = displayFunc();
    const bool morphing = morph_ < 1 && prev_.ok();
    const double ms = smooth(morph_);
    parallelFor(N + 1, [&](int i) {
        double x = cx - R + 2 * R * i / N;
        for (int j = 0; j <= N; ++j) {
            double y = cy - R + 2 * R * j / N;
            cplx w = complex_ ? evalF(cur, cplx(x, y), 0, env) : evalF(cur, x, y, env);
            if (morphing) {
                cplx w0 = complex_ ? evalF(prev_, cplx(x, y), 0, env) : evalF(prev_, x, y, env);
                w = w0 + (w - w0) * ms;
            }
            surfVals_[(size_t)i * (N + 1) + j] = w;
        }
    });
}

void App::drawScene3D(ImDrawList* dl) {
    const float hs = heightScale();
    const float flatA = 1.f - hs;
    const double R = domainR_, cx = domainCx_, cy = domainCy_;
    std::vector<Prim3> prims;
    prims.reserve(8000);
    Builder B{cam_, prims};

    // ---- unsorted: the full 2D plane and flat 2D content, fading out as the surface rises
    auto projLine = [&](const Vec3& a, const Vec3& b, ImU32 c, float th) {
        ImVec2 p, q;
        float d;
        if (cam_.project(a, p, d) && cam_.project(b, q, d)) dl->AddLine(p, q, c, th);
    };
    if (flatA > 0.01f) {
        double step = niceStep(95.0 / view_.scale);
        double mant = step / std::pow(10.0, std::floor(std::log10(step)));
        int sub = std::fabs(mant - 5) < 0.01 ? 5 : 2;
        double minor = step / sub;
        for (long i = (long)std::ceil(planeX0_ / minor); i <= (long)std::floor(planeX1_ / minor); ++i) {
            if (i == 0) continue;
            bool major = i % sub == 0;
            projLine(Vec3(i * minor, planeY0_, 0), Vec3(i * minor, planeY1_, 0),
                     col(BLUE_D, (major ? 0.58f : 0.16f) * flatA * (complex_ ? 0.45f : 1.f)), major ? 1.5f : 1.f);
        }
        for (long i = (long)std::ceil(planeY0_ / minor); i <= (long)std::floor(planeY1_ / minor); ++i) {
            if (i == 0) continue;
            bool major = i % sub == 0;
            projLine(Vec3(planeX0_, i * minor, 0), Vec3(planeX1_, i * minor, 0),
                     col(BLUE_D, (major ? 0.58f : 0.16f) * flatA * (complex_ ? 0.45f : 1.f)), major ? 1.5f : 1.f);
        }
        projLine(Vec3(planeX0_, 0, 0), Vec3(planeX1_, 0, 0), col(WHITE, 0.92f * flatA), 2.2f);
        projLine(Vec3(0, planeY0_, 0), Vec3(0, planeY1_, 0), col(WHITE, 0.92f * flatA), 2.2f);

        const float ca = flatA * fade_;
        std::vector<ImVec2> pts;
        if (!complex_) {
            EvalEnv env = makeEnv();
            const int N = 600;
            std::vector<std::vector<ImVec2>> segs;
            std::vector<ImVec2> cur;
            for (int i = 0; i <= N; ++i) {
                double x = planeX0_ + (planeX1_ - planeX0_) * i / N;
                double y = realPart(evalF(f_, x, 0, env));
                ImVec2 s;
                float d;
                if (!std::isfinite(y) || y < planeY0_ - 5 * R || y > planeY1_ + 5 * R || !cam_.project(Vec3(x, y, 0), s, d)) {
                    if (cur.size() >= 2) segs.push_back(cur);
                    cur.clear();
                    continue;
                }
                cur.push_back(s);
            }
            if (cur.size() >= 2) segs.push_back(cur);
            strokePolylines(dl, segs, col(YELLOW_C, ca), 3.4f);
        } else {
            warpedGrid();
            const auto& lines = gridLines_;
            const auto& kinds = gridKinds_;
            for (size_t i = 0; i < lines.size(); ++i) {
                int k = kinds[i];
                ImU32 c = k == 0 ? col(BLUE_C, 0.42f * ca) : k == 1 ? col(BLUE_C, 0.95f * ca) : k == 2 ? col(YELLOW_C, ca) : col(RED_C, ca);
                float th = k == 0 ? 1.1f : k == 1 ? 1.9f : 3.f;
                for (const auto& pl : lines[i]) {
                    pts.clear();
                    for (const cplx& w : pl) {
                        ImVec2 s;
                        float d;
                        if (std::fabs(w.real()) > 1e4 || std::fabs(w.imag()) > 1e4 || !cam_.project(Vec3(w.real(), w.imag(), 0), s, d)) {
                            if (pts.size() >= 2) dl->AddPolyline(pts.data(), (int)pts.size(), c, th, 0);
                            pts.clear();
                            continue;
                        }
                        pts.push_back(s);
                    }
                    if (pts.size() >= 2) dl->AddPolyline(pts.data(), (int)pts.size(), c, th, 0);
                }
            }
        }
    }

    // ---- floor grid over the surface domain
    {
        double step = niceStep(R / 4);
        ImU32 gc = col(BLUE_D, 0.38f * hs), gm = col(BLUE_D, 0.14f * hs);
        for (long i = (long)std::ceil((cx - R) / (step / 2)); i <= (long)std::floor((cx + R) / (step / 2)); ++i) {
            double x = i * step / 2;
            if (std::fabs(x) < 1e-12) continue;
            B.line(Vec3(x, cy - R, 0), Vec3(x, cy + R, 0), i % 2 == 0 ? gc : gm, i % 2 == 0 ? 1.3f : 0.9f, 16);
        }
        for (long i = (long)std::ceil((cy - R) / (step / 2)); i <= (long)std::floor((cy + R) / (step / 2)); ++i) {
            double y = i * step / 2;
            if (std::fabs(y) < 1e-12) continue;
            B.line(Vec3(cx - R, y, 0), Vec3(cx + R, y, 0), i % 2 == 0 ? gc : gm, i % 2 == 0 ? 1.3f : 0.9f, 16);
        }
    }

    // ---- surface
    const int N = std::clamp(surfRes_, 8, 400);
    {
        Hasher hh;
        hh.u(evalHash());
        hh.u((uint64_t)N); hh.d(R); hh.d(cx); hh.d(cy);
        if (hh.h != surfHash_ || surfVals_.size() != (size_t)(N + 1) * (N + 1)) {
            surfHash_ = hh.h;
            surfVals_.assign((size_t)(N + 1) * (N + 1), cplx(0));
            fillSurface(N);
        }
    }
    const std::vector<cplx>& vals = surfVals_;
    if (fitZEveryFrame_ && !complex_) {  // keep the tallest point at a fixed height
        double m = 0;
        for (const cplx& w : vals) {
            double v = realPart(w);
            if (std::isfinite(v)) m = std::max(m, std::fabs(v));
        }
        if (m > 1e-12) zScale_ = (float)(1.1 * R / m);
    }
    const double zMax = 2.2 * R;
    std::vector<Vec3> P(vals.size());
    std::vector<char> okv(vals.size());
    double zmin = 1e300, zmax = -1e300;
    for (int i = 0; i <= N; ++i)
        for (int j = 0; j <= N; ++j) {
            size_t k = (size_t)i * (N + 1) + j;
            double x = cx - R + 2 * R * i / N, y = cy - R + 2 * R * j / N, h;
            if (complex_) {
                double m = std::abs(vals[k]);
                okv[k] = std::isfinite(m) || std::isinf(m);
                h = std::min(std::isfinite(m) ? m * zScale_ : zMax, zMax);
            } else {
                double v = realPart(vals[k]);
                okv[k] = std::isfinite(v);
                // overshooting values are flattened at the cut-off rather than dropped (which left holes)
                h = okv[k] ? std::clamp(v * zScale_, -4 * R, 4 * R) : 0;
            }
            if (okv[k]) { zmin = std::min(zmin, h); zmax = std::max(zmax, h); }
            P[k] = Vec3(x, y, h * hs);
        }
    if (!complex_) {
        // Where a real function stops being real (e.g. sqrt of a negative), drop the
        // boundary vertices onto the floor so the surface ends in a clean rim.
        std::vector<char> rim(okv.size(), 0);
        for (int i = 0; i <= N; ++i)
            for (int j = 0; j <= N; ++j) {
                size_t k = (size_t)i * (N + 1) + j;
                if (okv[k] || !finiteC(vals[k])) continue;
                bool nb = (i > 0 && okv[k - (N + 1)]) || (i < N && okv[k + (N + 1)]) || (j > 0 && okv[k - 1]) || (j < N && okv[k + 1]);
                if (nb) rim[k] = 1;
            }
        for (size_t k = 0; k < okv.size(); ++k)
            if (rim[k]) { okv[k] = 1; P[k].z = 0; }
    }
    const float sa = surfOpacity_ * std::min(1.f, hs * 2.f) * fade_;
    if (sa > 0.005f) {
        Vec3 L = normalize(cam_.up * 0.9 - cam_.fwd * 0.6 - cam_.right * 0.35);
        ImU32 strokeC = col(LIGHT_GREY, 0.22f * sa);
        float strokeW = N > 70 ? 0.f : 0.7f;
        const size_t NV = P.size();
        // per-vertex work done once (each vertex is shared by four quads)
        std::vector<ImVec2> sp(NV);
        std::vector<float> dep(NV);
        std::vector<char> vis(NV);
        std::vector<ImU32> base;
        const bool perVertex = complex_ || colorMode_ == 1;
        if (perVertex) base.resize(NV);
        parallelFor(N + 1, [&](int i) {
            for (int j = 0; j <= N; ++j) {
                size_t k = (size_t)i * (N + 1) + j;
                vis[k] = okv[k] && cam_.project(P[k], sp[k], dep[k]);
                if (perVertex && okv[k])
                    base[k] = complex_ ? domainColor(vals[k], sa)
                                       : heightColor((float)((P[k].z / std::max(1e-6f, hs) - zmin) / std::max(1e-9, zmax - zmin)), sa);
            }
        });
        // build the quads in parallel, one row of quads per task
        std::vector<std::vector<Prim3>> rows(N);
        parallelFor(N, [&](int i) {
            std::vector<Prim3>& out = rows[i];
            out.reserve(N);
            for (int j = 0; j < N; ++j) {
                size_t k00 = (size_t)i * (N + 1) + j, k10 = k00 + (N + 1), k11 = k10 + 1, k01 = k00 + 1;
                if (!vis[k00] || !vis[k10] || !vis[k11] || !vis[k01]) continue;
                Vec3 n = normalize(cross(P[k11] - P[k00], P[k01] - P[k10]));
                float sh = 0.42f + 0.58f * (float)std::fabs(dot(n, L));
                Prim3 pr;
                pr.p[0] = sp[k00]; pr.p[1] = sp[k10]; pr.p[2] = sp[k11]; pr.p[3] = sp[k01];
                pr.depth = 0.25f * (dep[k00] + dep[k10] + dep[k11] + dep[k01]);
                pr.thick = strokeW;
                if (perVertex) {
                    pr.kind = Prim3::QuadG;
                    pr.vc[0] = shade(base[k00], sh); pr.vc[1] = shade(base[k10], sh);
                    pr.vc[2] = shade(base[k11], sh); pr.vc[3] = shade(base[k01], sh);
                    pr.stroke = complex_ ? col(BG, 0.12f * sa) : strokeC;
                } else {
                    pr.kind = Prim3::Quad;
                    pr.fill = shade(col(((i + j) & 1) ? BLUE_D : BLUE_E, sa), sh);
                    pr.stroke = strokeC;
                }
                out.push_back(pr);
            }
        });
        size_t total = prims.size();
        for (auto& r : rows) total += r.size();
        prims.reserve(total);
        for (auto& r : rows) prims.insert(prims.end(), r.begin(), r.end());
    }

    // ---- axes (through the origin, or along the domain edge nearest to it)
    const double ax0 = std::clamp(0.0, cx - R, cx + R), ay0 = std::clamp(0.0, cy - R, cy + R);
    const double xa1 = std::max(cx + R, ax0) + 0.25 * R, ya1 = std::max(cy + R, ay0) + 0.25 * R, za1 = 1.1 * R;
    const float aa = hs;
    if (aa > 0.01f) {
        ImU32 ac = col(WHITE, 0.92f * aa);
        double xa0 = std::min(cx - R, ax0) - 0.15 * R, ya0 = std::min(cy - R, ay0) - 0.15 * R, za0 = -0.6 * R;
        B.line(Vec3(xa0, ay0, 0), Vec3(xa1, ay0, 0), ac, 2.f, 24);
        B.line(Vec3(ax0, ya0, 0), Vec3(ax0, ya1, 0), ac, 2.f, 24);
        B.line(Vec3(ax0, ay0, za0), Vec3(ax0, ay0, za1), ac, 2.f, 24);
        B.arrowTip(Vec3(xa1 - 0.1 * R, ay0, 0), Vec3(xa1, ay0, 0), ac, 12.f);
        B.arrowTip(Vec3(ax0, ya1 - 0.1 * R, 0), Vec3(ax0, ya1, 0), ac, 12.f);
        B.arrowTip(Vec3(ax0, ay0, za1 - 0.1 * R), Vec3(ax0, ay0, za1), ac, 12.f);
    }

    // ---- tangent plane at the probe
    if (deriv_ && !complex_ && fx_.ok() && fy_.ok()) {
        float ta = hs * smooth(derivCreate_) * fade_;
        EvalEnv env = makeEnv();
        double x0 = probeX_, y0 = probeY_;
        double f0 = realPart(evalF(f_, x0, y0, env));
        double fx = realPart(evalF(fx_, x0, y0, env));
        double fy = realPart(evalF(fy_, x0, y0, env));
        if (ta > 0.01f && std::isfinite(f0) && std::isfinite(fx) && std::isfinite(fy)) {
            const double k = zScale_ * hs;
            const double s = std::min(0.4 * R, 0.45 * R / std::max(1e-9, (std::fabs(fx) + std::fabs(fy)) * zScale_));
            auto pt = [&](double dx, double dy) { return Vec3(x0 + dx, y0 + dy, (f0 + fx * dx + fy * dy) * k); };
            const int M = 6;
            for (int i = 0; i < M; ++i)
                for (int j = 0; j < M; ++j) {
                    double ax = -s + 2 * s * i / M, bx = -s + 2 * s * (i + 1) / M;
                    double ay = -s + 2 * s * j / M, by = -s + 2 * s * (j + 1) / M;
                    Vec3 q[4] = {pt(ax, ay), pt(bx, ay), pt(bx, by), pt(ax, by)};
                    B.quad(q, col(GOLD_C, 0.62f * ta), 0, 0, (float)(-0.06 * R));
                }
            B.line(pt(-s, -s), pt(s, -s), col(GOLD_C, ta), 1.6f, 6, (float)(-0.08 * R));
            B.line(pt(s, -s), pt(s, s), col(GOLD_C, ta), 1.6f, 6, (float)(-0.08 * R));
            B.line(pt(s, s), pt(-s, s), col(GOLD_C, ta), 1.6f, 6, (float)(-0.08 * R));
            B.line(pt(-s, s), pt(-s, -s), col(GOLD_C, ta), 1.6f, 6, (float)(-0.08 * R));
            B.line(pt(-s * 1.2, 0), pt(s * 1.2, 0), col(RED_C, ta), 3.f, 8, (float)(-0.1 * R));
            B.line(pt(0, -s * 1.2), pt(0, s * 1.2), col(GREEN_C, ta), 3.f, 8, (float)(-0.1 * R));
            B.dot(pt(0, 0), 6.5f, col(WHITE, ta), (float)(-0.12 * R));
        }
    }

    // ---- painter's algorithm: sort compact (depth, index) keys rather than whole primitives
    std::vector<std::pair<float, uint32_t>> order(prims.size());
    for (size_t i = 0; i < prims.size(); ++i) order[i] = {-prims[i].depth, (uint32_t)i};
    std::sort(order.begin(), order.end());
    ImDrawListFlags saved = dl->Flags;
    dl->Flags &= ~ImDrawListFlags_AntiAliasedFill;  // MSAA smooths quad edges without seams
    for (const auto& o : order) {
        const Prim3& p = prims[o.second];
        switch (p.kind) {
            case Prim3::Quad:
                dl->AddConvexPolyFilled(p.p, 4, p.fill);
                if (p.thick > 0 && (p.stroke >> IM_COL32_A_SHIFT) > 0) dl->AddPolyline(p.p, 4, p.stroke, p.thick, ImDrawFlags_Closed);
                break;
            case Prim3::QuadG: {
                const ImVec2 uv = dl->_Data->TexUvWhitePixel;
                dl->PrimReserve(6, 4);
                ImDrawIdx b = (ImDrawIdx)dl->_VtxCurrentIdx;
                dl->PrimWriteIdx(b); dl->PrimWriteIdx((ImDrawIdx)(b + 1)); dl->PrimWriteIdx((ImDrawIdx)(b + 2));
                dl->PrimWriteIdx(b); dl->PrimWriteIdx((ImDrawIdx)(b + 2)); dl->PrimWriteIdx((ImDrawIdx)(b + 3));
                for (int k = 0; k < 4; ++k) dl->PrimWriteVtx(p.p[k], uv, p.vc[k]);
                if (p.thick > 0 && (p.stroke >> IM_COL32_A_SHIFT) > 0) dl->AddPolyline(p.p, 4, p.stroke, p.thick, ImDrawFlags_Closed);
                break;
            }
            case Prim3::Line: dl->AddLine(p.p[0], p.p[1], p.fill, p.thick); break;
            case Prim3::Tri: dl->AddTriangleFilled(p.p[0], p.p[1], p.p[2], p.fill); break;
            case Prim3::Dot:
                dl->AddCircleFilled(p.p[0], p.thick, p.fill);
                dl->AddCircle(p.p[0], p.thick, col(BG, 0.8f), 0, 1.5f);
                break;
        }
    }
    dl->Flags = saved;

    // ---- hover: front-most surface sample near the cursor
    if (canHover() && hs > 0.99f && sa > 0.005f) {
        ImVec2 m = ImGui::GetIO().MousePos;
        const float r2 = 14.f * 14.f;
        float bestD = 1e30f;
        size_t bestK = (size_t)-1;
        ImVec2 bestS;
        for (size_t k = 0; k < P.size(); ++k) {
            if (!okv[k]) continue;
            ImVec2 sp;
            float d;
            if (!cam_.project(P[k], sp, d)) continue;
            float dx = sp.x - m.x, dy = sp.y - m.y;
            if (dx * dx + dy * dy < r2 && d < bestD) { bestD = d; bestK = k; bestS = sp; }
        }
        if (bestK != (size_t)-1) {
            int i = (int)(bestK / (N + 1)), j = (int)(bestK % (N + 1));
            double x = cx - R + 2 * R * i / N, y = cy - R + 2 * R * j / N;
            dl->AddCircleFilled(bestS, 6.f, col(YELLOW_C));
            dl->AddCircle(bestS, 6.f, col(BG), 0, 2.f);
            const float s = 19.f;
            ts::Box b;
            if (complex_) {
                cplx w = vals[bestK];
                ts::Box fl = deriv_ ? ts::hcat({ts::ident("f", s), ts::text("\xE2\x80\xB2", s)}) : ts::ident("f", s);
                ts::Box l1 = ts::hcat({ts::ident("z", s), ts::binop("=", s), ts::expr(eq::num(cplx(x, y)), s)});
                ts::Box l2 = ts::hcat({fl, ts::parens(ts::ident("z", s), s), ts::binop("=", s), ts::expr(eq::num(w), s)});
                b = l1;
                b.w = std::max(l1.w, l2.w);
                float gap = l1.desc + 8 + l2.asc;
                for (auto p : l2.prims) {
                    p.a.y += gap; p.b.y += gap;
                    for (auto& q : p.pts) q.y += gap;
                    b.prims.push_back(p);
                }
                b.desc = gap + l2.desc;
            } else {
                double v = realPart(vals[bestK]);
                b = ts::parens(ts::hcat({ts::text(ts::fmtNum(x, 4), s), ts::text(",", s), ts::space(6),
                                         ts::text(ts::fmtNum(y, 4), s), ts::text(",", s), ts::space(6),
                                         ts::text(ts::fmtNum(v, 5), s)}), s);
            }
            hoverLabel(dl, bestS, b, col(YELLOW_C));
        }
    }

    // ---- axis labels
    if (aa > 0.05f) {
        const float s = 26.f;
        auto label = [&](const Vec3& p, const ts::Box& b) {
            ImVec2 sp;
            float d;
            if (!cam_.project(p, sp, d)) return;
            ts::draw(dl, b, ImVec2(sp.x - b.w * 0.5f, sp.y + b.asc * 0.5f), col(WHITE, aa));
        };
        if (complex_) {
            label(Vec3(xa1 + 0.13 * R, ay0, 0), ts::text("Re", s * 0.8f));
            label(Vec3(ax0, ya1 + 0.13 * R, 0), ts::text("Im", s * 0.8f));
            label(Vec3(ax0, ay0, za1 + 0.12 * R), ts::parens(ts::ident("f", s), s, '|', '|'));
        } else {
            label(Vec3(xa1 + 0.12 * R, ay0, 0), ts::ident("x", s));
            label(Vec3(ax0, ya1 + 0.12 * R, 0), ts::ident("y", s));
            label(Vec3(ax0, ay0, za1 + 0.12 * R), ts::ident("z", s));
        }
    }
}
