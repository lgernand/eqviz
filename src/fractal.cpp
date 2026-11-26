// Fractals: the focused row is used as a map and iterated, z <- f(z, c).
//   Mandelbrot-style panes: c is the point, z starts at 0.
//   Julia-style panes: z starts at the point, c is the shared Julia constant.
// Points are coloured by how fast they escape (smooth iteration count), or, for maps that
// settle on a fixed point (Newton's method), by where they end up. The GPU renders in single
// precision; past that (deep zooms) the CPU takes over in double precision.
#include <algorithm>
#include <atomic>
#include <memory>
#include <thread>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "app.hpp"
#include "gl.hpp"
#include "gpu.hpp"
#include "theme.hpp"

using namespace theme;

namespace {

constexpr double kBail2 = 65536.0;  // escape radius 256, squared
constexpr double PI = 3.14159265358979323846;

const char* kPaletteNames[] = {"Theme", "Fire", "Ice", "Rainbow", "Greyscale"};
constexpr int kNumPalettes = 5;

void paletteStops(int idx, float out[15]) {
    int hex[5];
    switch (idx) {
        case 1: { const int h[5] = {0x1E0508, 0x8E1B2A, 0xE0482A, 0xF7A234, 0xFFEFB5}; std::copy(h, h + 5, hex); break; }
        case 2: { const int h[5] = {0x061A2C, 0x14607A, 0x3FA9C9, 0xA8E2F2, 0xF4FBFF}; std::copy(h, h + 5, hex); break; }
        case 3: { const int h[5] = {0x9A72AC, 0x58C4DD, 0x83C167, 0xFFD54A, 0xFC6255}; std::copy(h, h + 5, hex); break; }
        case 4: { const int h[5] = {0x141414, 0x4A4A4A, 0x8C8C8C, 0xC8C8C8, 0xF5F5F5}; std::copy(h, h + 5, hex); break; }
        default: {  // follows the colour theme: dark at the outside, bright towards the set
            const int dark = (int)ImGui::ColorConvertFloat4ToU32(mixv(BG, BLUE_E, 0.45f));
            const int h[5] = {((dark & 0xFF) << 16) | (dark & 0xFF00) | ((dark >> 16) & 0xFF), BLUE_E, BLUE_C, BLUE_A, GOLD_C};
            std::copy(h, h + 5, hex);
        }
    }
    for (int k = 0; k < 5; ++k) {
        out[3 * k + 0] = ((hex[k] >> 16) & 255) / 255.f;
        out[3 * k + 1] = ((hex[k] >> 8) & 255) / 255.f;
        out[3 * k + 2] = (hex[k] & 255) / 255.f;
    }
}

struct RGB { float r, g, b; };
RGB mixc(RGB a, RGB b, float t) { return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t}; }
RGB hsv2rgb(float h, float s, float v) {  // same formula as the shader
    auto ch = [&](float off) {
        float k = std::fmod(h * 6.f + off, 6.f);
        if (k < 0) k += 6.f;
        float x = std::clamp(std::fabs(k - 3.f) - 1.f, 0.f, 1.f);
        return v * (1.f + (x - 1.f) * s);
    };
    return {ch(0.f), ch(4.f), ch(2.f)};
}

struct Result {
    enum Kind { Bounded, Escaped, Settled } kind = Bounded;
    int n = 0;      // iterations done
    double nu = 0;  // smooth escape count
    cplx z;         // where it ended
};

// One compiled map, evaluated in double precision.
struct Map {
    eq::Program prog;
    std::vector<int> map;  // per slot: -1 z, -4 c, >= 0 parameter, -3 zero
    const std::vector<double>* params = nullptr;
    bool quadratic = false;  // exactly z^2 + c: inlined
    EvalEnv env;

    cplx step(cplx z, cplx c) const {
        if (quadratic) return z * z + c;
        cplx slots[64];
        const int n = std::min(prog.numSlots(), 64);
        for (int i = 0; i < n; ++i) {
            const int m = map[i];
            slots[i] = m == -1 ? z : m == -4 ? c : m >= 0 ? cplx((*params)[m], 0) : cplx(0, 0);
        }
        return prog.eval(slots, env.sumLimit, false, env.maxTerms, env.animTerms);
    }

    Result run(cplx z, cplx c, int maxIter, std::vector<cplx>* orbit = nullptr) const {
        Result r;
        if (orbit) orbit->push_back(z);
        cplx saved = z;  // Brent-style cycle check: compare with a snapshot taken at doubling intervals
        int nextSave = 8;
        for (int i = 0; i < maxIter; ++i) {
            const cplx prev = z;
            z = step(z, c);
            if (orbit && orbit->size() < 400) orbit->push_back(z);
            const double m = std::norm(z);
            if (!std::isfinite(m) || m > kBail2) {
                r.kind = Result::Escaped;
                r.n = i + 1;
                r.nu = std::isfinite(m) ? i + 1.0 - std::log2(std::max(1e-9, 0.5 * std::log2(m) / 8.0)) : i;
                r.nu = std::max(r.nu, 0.0);
                r.z = z;
                return r;
            }
            if (std::norm(z - prev) < 1e-10 * std::max(1.0, m)) {
                r.kind = Result::Settled;
                r.n = i;
                r.z = z;
                return r;
            }
            if (!orbit) {  // stuck in a cycle: it will never escape
                if (std::norm(z - saved) < 1e-26 * std::max(1.0, m)) break;
                if (i == nextSave) { saved = z; nextSave *= 2; }
            }
        }
        r.n = maxIter;
        r.z = z;
        return r;
    }
};

struct FracSettings {
    View2D view;
    ImVec2 cmin;
    float cw = 0, ch = 0;
    int iters = 0;
    bool julia = false, settle = false;
    cplx jc;
    float density = 1;
    float pal[15] = {};
    RGB bg{0, 0, 0};
};

std::vector<uint32_t> renderCPU(const Map& mp, const FracSettings& s, int w, int h, const std::atomic<bool>* cancel) {
    std::vector<uint32_t> px((size_t)w * h, 0xFF000000u);
    parallelFor(h, [&](int row) {
        if (cancel && cancel->load(std::memory_order_relaxed)) return;
        for (int col = 0; col < w; ++col) {
            double x, y;
            s.view.s2w(ImVec2(s.cmin.x + (col + 0.5f) * s.cw / w, s.cmin.y + (row + 0.5f) * s.ch / h), x, y);
            const cplx p(x, y);
            const Result r = mp.run(s.julia ? p : cplx(0, 0), s.julia ? s.jc : p, s.iters);
            RGB c = s.bg;
            if (r.kind == Result::Escaped) {
                float t = (float)(std::sqrt(r.nu) * 0.18 * s.density);
                t = (t - std::floor(t)) * 5.f;
                const int i = std::min(4, (int)t), j = (i + 1) % 5;
                const float u = t - i;
                RGB a{s.pal[3 * i], s.pal[3 * i + 1], s.pal[3 * i + 2]}, b{s.pal[3 * j], s.pal[3 * j + 1], s.pal[3 * j + 2]};
                c = mixc(s.bg, mixc(a, b, u), std::clamp((float)r.nu / 8.f, 0.f, 1.f));
            } else if (r.kind == Result::Settled && s.settle) {
                float hue = (float)(std::atan2(r.z.imag(), r.z.real()) / (2 * PI));
                hue -= std::floor(hue);
                c = hsv2rgb(hue, 0.5f, 0.22f + 0.6f * std::exp(-r.n * 0.1f / s.density));
            }
            auto b8 = [](float v) { return (uint32_t)std::clamp((int)(v * 255.f + 0.5f), 0, 255); };
            px[(size_t)row * w + col] = b8(c.r) | (b8(c.g) << 8) | (b8(c.b) << 16) | (255u << 24);
        }
    });
    return px;
}

}  // namespace

// A full-resolution render running on a background thread.
struct FracJob {
    uint64_t hash = 0;
    int w = 0, h = 0;
    std::vector<uint32_t> px;
    std::vector<double> params;
    Map mp;
    FracSettings s;
    std::atomic<bool> done{false}, cancel{false};
    std::thread th;
    ~FracJob() {
        cancel = true;
        if (th.joinable()) th.join();
    }
};

namespace {

bool isVar(const eq::NodeP& n, const char* name) { return n && n->op == eq::Op::Var && n->name == name; }
bool isZSquared(const eq::NodeP& n) {
    if (!n) return false;
    if (n->op == eq::Op::Pow && isVar(n->kids[0], "x") && n->kids[1]->op == eq::Op::Num && n->kids[1]->val == cplx(2, 0))
        return true;
    return n->op == eq::Op::Mul && isVar(n->kids[0], "x") && isVar(n->kids[1], "x");
}
bool isQuadratic(const eq::NodeP& f) {
    return f && f->op == eq::Op::Add &&
           ((isZSquared(f->kids[0]) && isVar(f->kids[1], "c")) || (isZSquared(f->kids[1]) && isVar(f->kids[0], "c")));
}

ts::Box cplxBox(cplx w, float s, int sig = 5) {
    const double tiny = 1e-10 * std::max(1.0, std::abs(w));  // print round-off as 0
    const double re = std::fabs(w.real()) < tiny ? 0 : w.real(), im = std::fabs(w.imag()) < tiny ? 0 : w.imag();
    if (im == 0) return ts::text(ts::fmtNum(re, sig), s);
    std::vector<ts::Box> parts;
    parts.push_back(ts::text(ts::fmtNum(re, sig), s));
    parts.push_back(ts::binop(im < 0 ? "\xE2\x88\x92" : "+", s));
    parts.push_back(ts::text(ts::fmtNum(std::fabs(im), sig), s));
    parts.push_back(ts::ident("i", s));
    return ts::hcat(parts);
}

}  // namespace

// ------------------------------------------------------------------ state

bool App::anyFractalPane() const {
    if (fractal_ && !isNote_) return true;
    for (int i = 0; i < (int)panes_.size(); ++i)
        if (i != cur_ && panes_[i].fractal_ && !panes_[i].isNote_) return true;
    return false;
}

bool App::anyJuliaPane(int row) const {
    auto match = [&](bool fr, bool ju, bool note, int act) { return fr && ju && !note && (row < 0 || act == row); };
    if (match(fractal_, fracJulia_, isNote_, active_)) return true;
    for (int i = 0; i < (int)panes_.size(); ++i)
        if (i != cur_ && match(panes_[i].fractal_, panes_[i].fracJulia_, panes_[i].isNote_, panes_[i].active_)) return true;
    return false;
}

void App::setFractal(bool on, bool julia) {
    fractal_ = on;
    fracJulia_ = julia;
    dcHash_ = 0;
    if (on) {
        domainColor_ = false;
        gridAnim_ = false;
        gridT_ = 1;
    }
    rebuildParams();  // `c` stops (or starts) being a slider
}

int App::fracIterations() const {
    if (!fracAutoIter_) return fracIter_;
    const double zoom = view_.scale / std::max(1e-12, homeScale_);
    return std::clamp((int)std::lround(fracIter_ * (1.0 + 0.28 * std::log2(std::max(1.0, zoom)))), fracIter_, 12000);
}

// ------------------------------------------------------------------ drawing

void App::drawFractal(ImDrawList* dl, float alpha) {
    const eq::NodeP f = expr_;
    if (!f) return;
    const int iters = fracIterations();
    // maps without c (Newton's method) are about where points settle; for the others a
    // settled orbit is simply inside the set
    const bool settleColors = !eq::dependsOn(f, "c");
    float pal[15];
    paletteStops(fracPalette_, pal);
    const ImVec4 bgv = colv(BG);

    double x0, y0, x1, y1;
    view_.s2w(canvasMin_, x0, y1);
    view_.s2w(canvasMax_, x1, y0);
    const double extent = std::max({std::fabs(x0), std::fabs(x1), std::fabs(y0), std::fabs(y1), 2.0});
    std::vector<std::string> names;
    for (size_t i = 0; i < params_.size() && i < 32; ++i) names.push_back(params_[i].name);

    // single precision runs out when a pixel is a few dozen float steps wide
    std::string reason;
    if (!useGpu_) reason = "GPU off";
    else if (1.0 / view_.scale < 4e-6 * extent) reason = "zoomed past single precision";
    else if (params_.size() > 32) reason = "too many sliders";
    if (reason.empty()) {
        gpu::DomainColorDraw d{};
        d.cx = view_.cx; d.cy = view_.cy; d.scale = view_.scale; d.aspect = view_.aspect;
        ImVec2 c = view_.center();
        d.ccx = c.x; d.ccy = c.y;
        d.alpha = alpha;
        double lim = sumLimit();
        d.sumLimit = std::isinf(lim) ? 1e30f : (float)lim;
        for (size_t i = 0; i < names.size(); ++i) d.params[i] = (float)pvals_[i];
        d.nParams = (int)names.size();
        gpu::FractalDraw fd;
        fd.cRe = (float)fracC_.real();
        fd.cIm = (float)fracC_.imag();
        fd.iter = iters;
        fd.julia = fracJulia_;
        fd.density = fracDensity_;
        std::copy(pal, pal + 15, fd.pal);
        fd.bg[0] = bgv.x; fd.bg[1] = bgv.y; fd.bg[2] = bgv.z;
        fd.colorSettled = settleColors;
        std::string why;
        if (gpu::drawFractal(dl, f, names, d, fd, why)) {
            fracStatus_ = "GPU";
            return;
        }
        reason = "uses " + why;
    }
    fracStatus_ = "CPU, double precision (" + reason + ")";

    // ---- CPU, double precision: a coarse image while the view moves; once it settles the full
    // resolution image is computed on a background thread (synchronously while exporting video).
    const float cw = canvasMax_.x - canvasMin_.x, ch = canvasMax_.y - canvasMin_.y;
    Hasher sh;
    sh.u(evalHash());
    sh.d(view_.cx); sh.d(view_.cy); sh.d(view_.scale); sh.d(view_.aspect); sh.d(cw); sh.d(ch);
    sh.u((uint64_t)iters); sh.u(fracJulia_ ? 1 : 0); sh.u((uint64_t)fracPalette_); sh.d(fracDensity_);
    sh.u(settleColors ? 1 : 0); sh.d(fracC_.real()); sh.d(fracC_.imag()); sh.u((uint64_t)BG);
    for (float v : pal) sh.d(v);
    if (sh.h != fracViewHash_) { fracViewHash_ = sh.h; fracMoveT_ = time_; }
    const bool moving = time_ - fracMoveT_ < 0.2;
    const bool quad = isQuadratic(f);
    const int coarse = quad ? 5 : 8, fine = quad ? 1 : 2;
    auto key = [&](int down) { Hasher k; k.u(sh.h); k.u((uint64_t)down); return k.h; };

    FracSettings st;
    st.view = view_;
    st.cmin = canvasMin_;
    st.cw = cw;
    st.ch = ch;
    st.iters = iters;
    st.julia = fracJulia_;
    st.settle = settleColors;
    st.jc = fracC_;
    st.density = fracDensity_;
    std::copy(pal, pal + 15, st.pal);
    st.bg = {bgv.x, bgv.y, bgv.z};
    auto makeMap = [&](const std::vector<double>* params) {
        Map mp;
        mp.prog = eq::Program(f);
        mp.params = params;
        mp.quadratic = quad;
        mp.env = makeEnv();
        for (const auto& n : mp.prog.slotNames()) {
            int m = -3;
            if (n == "x") m = -1;
            else if (n == "c") m = -4;
            else
                for (size_t i = 0; i < params_.size(); ++i)
                    if (params_[i].name == n) m = (int)i;
            mp.map.push_back(m);
        }
        return mp;
    };
    auto dimsFor = [&](int down, int& w, int& h) {
        w = std::max(8, (int)(cw / down));
        h = std::max(8, (int)(ch / down));
    };
    auto upload = [&](const std::vector<uint32_t>& px, int w, int h, uint64_t k) {
        dcHash_ = k;
        dcW_ = w;
        dcH_ = h;
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    };
    auto renderNow = [&](int down) {
        int w, h;
        dimsFor(down, w, h);
        if (std::getenv("EQVIZ_DEBUG_FRACTAL")) std::printf("fractal cpu %dx%d iters %d frame %d\n", w, h, iters, frameNo_);
        const Map mp = makeMap(&pvals_);
        upload(renderCPU(mp, st, w, h, nullptr), w, h, key(down));
    };

    std::shared_ptr<FracJob>& job = fracJobs_[cur_];
    bool refining = false;
    if (exporting_) {
        job.reset();
        if (dcHash_ != key(fine) || !dcTex_) renderNow(fine);
    } else if (moving) {
        job.reset();  // cancels it
        if (dcHash_ != key(coarse) && dcHash_ != key(fine)) renderNow(coarse);
    } else if (dcHash_ != key(fine) || !dcTex_) {
        if (job && job->hash == key(fine)) {
            if (job->done) {
                upload(job->px, job->w, job->h, job->hash);
                job.reset();
            } else {
                refining = true;
            }
        } else {
            job.reset();
            if (dcHash_ != key(coarse) || !dcTex_) renderNow(coarse);  // something to show meanwhile
            auto j = std::make_shared<FracJob>();
            j->hash = key(fine);
            dimsFor(fine, j->w, j->h);
            j->params = pvals_;
            j->mp = makeMap(&j->params);
            j->s = st;
            FracJob* raw = j.get();
            j->th = std::thread([raw] {
                raw->px = renderCPU(raw->mp, raw->s, raw->w, raw->h, &raw->cancel);
                raw->done = true;
            });
            job = j;
            refining = true;
        }
    }
    if (refining) fracStatus_ += ", refining\xE2\x80\xA6";
    dl->AddImage((ImTextureID)(uintptr_t)dcTex_, canvasMin_, canvasMax_, ImVec2(0, 0), ImVec2(1, 1), col(WHITE, alpha));
}

void App::cancelFractalJobs() { fracJobs_.clear(); }

void App::drawFractalOverlay(ImDrawList* dl) {
    const float s = 19.f * labelScale_;
    // caption: what the point stands for
    {
        ts::Box cap;
        if (fracJulia_)
            cap = ts::hcat({ts::text("Julia set", s * 0.8f), ts::space(10), ts::ident("c", s * 0.8f), ts::binop("=", s * 0.8f),
                            cplxBox(fracC_, s * 0.8f)});
        else
            cap = ts::hcat({ts::ident("c", s * 0.8f), ts::text(" = point,  ", s * 0.8f), ts::ident("z", s * 0.8f),
                            ts::text("\xE2\x82\x80 = 0", s * 0.8f)});
        ts::Box it = ts::text(std::to_string(fracIterations()) + " iterations", s * 0.62f);
        const float x = canvasMax_.x - std::max(cap.w, it.w) - 22;
        const float y = canvasMax_.y - (cap.asc + cap.desc + it.asc + it.desc + 14) - 46;  // above the axis labels
        dl->AddRectFilled(ImVec2(x - 12, y - 6), ImVec2(canvasMax_.x - 10, y + cap.asc + cap.desc + it.asc + it.desc + 14),
                          col(BG, 0.95f), 8);
        ts::draw(dl, cap, ImVec2(x, y + cap.asc), col(WHITE));
        ts::draw(dl, it, ImVec2(x, y + cap.asc + cap.desc + 6 + it.asc), col(WHITE, 0.55f));
    }

    // the Julia constant, draggable, in Mandelbrot-style panes when a Julia pane exists
    fracCVisible_ = false;
    if (!fracJulia_ && anyJuliaPane(active_)) {
        ImVec2 p = view_.w2s(fracC_.real(), fracC_.imag());
        if (p.x > canvasMin_.x - 20 && p.x < canvasMax_.x + 20 && p.y > canvasMin_.y - 20 && p.y < canvasMax_.y + 20) {
            fracCScreen_ = p;
            fracCVisible_ = true;
            const ImVec2 m = ImGui::GetIO().MousePos;
            const bool hot = draggingC_ || (std::hypot(m.x - p.x, m.y - p.y) < 14 && canHover());
            dl->AddCircleFilled(p, hot ? 9.f : 7.f, col(YELLOW_C), 24);
            dl->AddCircle(p, hot ? 13.f : 11.f, col(BG, 0.9f), 24, 2.5f);
            ts::Box lb = ts::ident("c", s);
            ts::draw(dl, lb, ImVec2(p.x + 13, p.y - 9), col(YELLOW_C));
        }
    }

    // the orbit of the point under the cursor
    if (!canHover() || draggingC_) return;
    const ImVec2 m = ImGui::GetIO().MousePos;
    if (fracCVisible_ && std::hypot(m.x - fracCScreen_.x, m.y - fracCScreen_.y) < 14) return;
    double wx, wy;
    view_.s2w(m, wx, wy);
    const cplx p(wx, wy);
    Map mp;
    mp.prog = eq::Program(expr_);
    if (!mp.prog.valid()) return;
    mp.params = &pvals_;
    mp.quadratic = isQuadratic(expr_);
    mp.env = makeEnv();
    for (const auto& n : mp.prog.slotNames()) {
        int k = -3;
        if (n == "x") k = -1;
        else if (n == "c") k = -4;
        else
            for (size_t i = 0; i < params_.size(); ++i)
                if (params_[i].name == n) k = (int)i;
        mp.map.push_back(k);
    }
    std::vector<cplx> orbit;
    const Result r = mp.run(fracJulia_ ? p : cplx(0, 0), fracJulia_ ? fracC_ : p, fracIterations(), &orbit);
    std::vector<ImVec2> pts;
    for (const cplx& z : orbit) {
        if (!finiteC(z)) break;
        ImVec2 q = view_.w2s(z.real(), z.imag());
        if (std::fabs(q.x) > 1e5f || std::fabs(q.y) > 1e5f) break;
        pts.push_back(q);
    }
    const int show = std::min((int)pts.size(), 120);
    for (int i = 0; i + 1 < show; ++i) {
        const float fade = 1.f - 0.75f * i / std::max(1, show);
        dl->AddLine(pts[i], pts[i + 1], col(WHITE, 0.55f * fade), 1.4f);
    }
    for (int i = 0; i < show; ++i) dl->AddCircleFilled(pts[i], i == 0 ? 4.5f : 2.6f, col(i == 0 ? YELLOW_C : WHITE, i == 0 ? 1.f : 0.8f));

    std::vector<ts::Box> parts = {ts::ident(fracJulia_ ? "z" : "c", s * 0.85f)};
    if (fracJulia_) parts.push_back(ts::text("\xE2\x82\x80", s * 0.85f));
    parts.push_back(ts::binop("=", s * 0.85f));
    // digits enough to resolve a pixel at this zoom
    const int sig = std::clamp((int)std::ceil(std::log10(std::max(1.0, std::abs(p)) * view_.scale)) + 1, 5, 15);
    parts.push_back(cplxBox(p, s * 0.85f, sig));
    parts.push_back(ts::space(12));
    std::string what;
    if (r.kind == Result::Escaped) what = "escapes after " + std::to_string(r.n) + (r.n == 1 ? " step" : " steps");
    else if (r.kind == Result::Settled) what = "settles after " + std::to_string(r.n) + " steps";
    else what = "stays bounded (" + std::to_string(r.n) + " steps)";
    parts.push_back(ts::text(what, s * 0.75f));
    if (r.kind == Result::Settled) {
        parts.push_back(ts::text(" at ", s * 0.75f));
        parts.push_back(cplxBox(r.z, s * 0.75f));
    }
    hoverLabel(dl, m, ts::hcat(parts), col(r.kind == Result::Bounded ? YELLOW_C : BLUE_C));
}

// ------------------------------------------------------------------ panel

void App::drawFractalSection() {
    auto labelRow = [](const char* label, float w = 110.f) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(w);
        ImGui::SetNextItemWidth(-1);
    };
    {
        float half = (ImGui::GetContentRegionAvail().x - 6) * 0.5f;
        for (int k = 0; k < 2; ++k) {
            if (k) ImGui::SameLine(0, 6);
            const bool sel = fracJulia_ == (k == 1);
            if (sel) {
                ImGui::PushStyleColor(ImGuiCol_Button, mixv(BG, BLUE_C, 0.38f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, mixv(BG, BLUE_C, 0.50f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, mixv(BG, BLUE_C, 0.62f));
            }
            if (ImGui::Button(k ? "Julia:  z\xE2\x82\x80 = point" : "Mandelbrot:  c = point", ImVec2(half, 30))) setFractal(true, k == 1);
            if (sel) ImGui::PopStyleColor(3);
            ImGui::SetItemTooltip(k ? "Start the iteration at each point, with c fixed (drag c in a Mandelbrot pane)"
                                    : "Use each point as c and start the iteration at z = 0");
        }
    }
    if (fracJulia_) {
        labelRow("c");
        double v[2] = {fracC_.real(), fracC_.imag()};
        if (ImGui::DragScalarN("##fc", ImGuiDataType_Double, v, 2, 0.0005f, nullptr, nullptr, "%.4f")) fracC_ = cplx(v[0], v[1]);
        ImGui::SetItemTooltip("The Julia constant (real, imaginary). Drag the yellow c in a Mandelbrot pane to move it.");
    }
    labelRow("Iterations");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 70);
    ImGui::SliderInt("##fit", &fracIter_, 10, 3000, "%d", ImGuiSliderFlags_Logarithmic);
    ImGui::SameLine(0, 6);
    ImGui::Checkbox("Auto", &fracAutoIter_);
    ImGui::SetItemTooltip("Add iterations as you zoom in (now %d)", fracIterations());
    labelRow("Palette");
    ImGui::Combo("##fpal", &fracPalette_, kPaletteNames, kNumPalettes);
    labelRow("Colour bands");
    ImGui::SliderFloat("##fden", &fracDensity_, 0.2f, 5.f, "%.2f\xC3\x97", ImGuiSliderFlags_Logarithmic);
    ImGui::Checkbox("Render on the GPU", &useGpu_);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::PushFont(nullptr, 12.5f);
    ImGui::TextWrapped("On %s. Hover to see a point's orbit.%s", fracStatus_.empty() ? "GPU" : fracStatus_.c_str(),
                       fracJulia_ ? "" : " Double-click to set the Julia constant c.");
    ImGui::PopFont();
    ImGui::PopStyleColor();
}
