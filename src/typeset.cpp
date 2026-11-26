#include "typeset.hpp"

#include "imgui_internal.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>

namespace ts {

using eq::Node;
using eq::NodeP;
using eq::Op;

static ImFont* gFont = nullptr;
static ImFont* gItalic = nullptr;
static bool gMathAlnum = false;

void init(ImFont* f, bool mathAlnum, ImFont* italic) {
    gFont = f;
    gMathAlnum = mathAlnum;
    gItalic = mathAlnum ? nullptr : italic;
}

static ImFont* font() { return gFont ? gFont : ImGui::GetFont(); }
static float qs(float s) { return std::max(6.0f, std::round(s * 2.f) * 0.5f); }
static const char* MINUS = "\xE2\x88\x92";  // U+2212

static void append(Box& dst, const Box& src, float dx, float dy) {
    for (Prim p : src.prims) {
        p.a.x += dx; p.a.y += dy;
        p.b.x += dx; p.b.y += dy;
        for (auto& q : p.pts) { q.x += dx; q.y += dy; }
        dst.prims.push_back(std::move(p));
    }
}

static Box textF(const std::string& s, float size, ImFont* f) {
    size = qs(size);
    Box b;
    b.w = (f ? f : font())->CalcTextSizeA(size, FLT_MAX, 0.f, s.c_str()).x;
    b.asc = 0.72f * size;
    b.desc = 0.22f * size;
    Prim p;
    p.kind = Prim::Text;
    p.size = size;
    p.s = s;
    p.font = f;
    b.prims.push_back(std::move(p));
    return b;
}

Box text(const std::string& s, float size) {
    size = qs(size);
    Box b;
    b.w = font()->CalcTextSizeA(size, FLT_MAX, 0.f, s.c_str()).x;
    b.asc = 0.72f * size;
    b.desc = 0.22f * size;
    Prim p;
    p.kind = Prim::Text;
    p.size = size;
    p.s = s;
    b.prims.push_back(std::move(p));
    return b;
}

Box space(float w) {
    Box b;
    b.w = w;
    return b;
}

Box hcat(const std::vector<Box>& bs) {
    Box r;
    float x = 0;
    for (auto& b : bs) {
        append(r, b, x, 0);
        x += b.w;
        r.asc = std::max(r.asc, b.asc);
        r.desc = std::max(r.desc, b.desc);
    }
    r.w = x;
    return r;
}

static uint32_t italicCp(uint32_t c) {
    if (!gMathAlnum) return c;
    if (c >= 'a' && c <= 'z') return c == 'h' ? 0x210E : 0x1D44E + (c - 'a');
    if (c >= 'A' && c <= 'Z') return 0x1D434 + (c - 'A');
    if (c >= 0x3B1 && c <= 0x3C9) return 0x1D6FC + (c - 0x3B1);
    return c;
}

static std::string italicize(const std::string& name) {
    if (uint32_t g = eq::greekCodepoint(name)) return eq::utf8(italicCp(g));
    std::string out;
    for (unsigned char c : name) out += eq::utf8(italicCp(c));
    return out;
}

Box ident(const std::string& full, float size) {
    std::string base = full, sub;
    size_t us = full.find('_');
    if (us != std::string::npos) {
        base = full.substr(0, us);
        sub = full.substr(us + 1);
    }
    Box b = gItalic ? textF(italicize(base), size, gItalic) : text(italicize(base), size);
    b.w += 0.04f * size;  // italic correction
    if (!sub.empty()) {
        bool digits = std::all_of(sub.begin(), sub.end(), [](unsigned char c) { return std::isdigit(c); });
        Box s = digits ? text(sub, size * 0.7f) : (gItalic ? textF(italicize(sub), size * 0.7f, gItalic) : text(italicize(sub), size * 0.7f));
        float d = 0.2f * size;
        Box r = b;
        append(r, s, b.w - 0.02f * size, d);
        r.w = b.w + s.w;
        r.desc = std::max(b.desc, s.desc + d);
        return r;
    }
    return b;
}

Box binop(const char* op, float size) {
    return hcat({space(0.24f * size), text(op, size), space(0.24f * size)});
}

Box frac(const Box& n, const Box& d, float size) {
    const float axis = 0.25f * size, gap = 0.16f * size, t = std::max(1.0f, 0.055f * size), pad = 0.1f * size;
    const float w = std::max(n.w, d.w) + 2 * pad;
    Box r;
    float nb = -(axis + t * 0.5f + gap + n.desc);
    float db = -axis + t * 0.5f + gap + d.asc;
    append(r, n, (w - n.w) * 0.5f, nb);
    append(r, d, (w - d.w) * 0.5f, db);
    Prim bar;
    bar.kind = Prim::Line;
    bar.a = ImVec2(0.0f, -axis);
    bar.b = ImVec2(w, -axis);
    bar.thick = t;
    r.prims.push_back(bar);
    r.w = w;
    r.asc = -nb + n.asc;
    r.desc = db + d.desc;
    return hcat({space(0.08f * size), r, space(0.08f * size)});
}

Box sup(const Box& base, const Box& s, float size) {
    float u = std::max(0.42f * size, base.asc - 0.3f * size);
    Box r = base;
    append(r, s, base.w + 0.03f * size, -u);
    r.w = base.w + 0.03f * size + s.w;
    r.asc = std::max(base.asc, u + s.asc);
    r.desc = std::max(base.desc, s.desc - u);
    return r;
}

static Box radical(const Box& c, float size) {
    float th = std::max(1.0f, 0.055f * size);
    float top = -(c.asc + 0.14f * size), bot = c.desc + 0.04f * size;
    float rw = 0.48f * size + 0.04f * (bot - top);
    Box r;
    Prim p;
    p.kind = Prim::Poly;
    p.thick = th;
    p.pts = {{0.0f, -0.26f * size}, {0.11f * size, -0.33f * size}, {0.30f * size, bot},
             {rw, top}, {rw + c.w + 0.1f * size, top}};
    r.prims.push_back(p);
    append(r, c, rw + 0.05f * size, 0);
    r.w = rw + c.w + 0.14f * size;
    r.asc = -top + th;
    r.desc = bot;
    return r;
}

// Crescent-shaped scalable parenthesis, emitted as a triangle strip of
// (outer, inner) point pairs: thin at the tips, thicker in the middle.
static Prim bigParen(float top, float bot, float w, float th, bool left) {
    Prim p;
    p.kind = Prim::Strip;
    const int N = 32;
    const float mid = (top + bot) * 0.5f;
    const ImVec2 a(w, top), c(-w * 0.45f, mid), b(w, bot);
    for (int i = 0; i <= N; ++i) {
        float t = i / (float)N, u = 1 - t;
        ImVec2 q(u * u * a.x + 2 * u * t * c.x + t * t * b.x, u * u * a.y + 2 * u * t * c.y + t * t * b.y);
        ImVec2 d(2 * u * (c.x - a.x) + 2 * t * (b.x - c.x), 2 * u * (c.y - a.y) + 2 * t * (b.y - c.y));
        float l = std::sqrt(d.x * d.x + d.y * d.y);
        ImVec2 n = l > 0 ? ImVec2(d.y / l, -d.x / l) : ImVec2(1, 0);
        if (n.x < 0) n = ImVec2(-n.x, -n.y);
        float k = th * (0.3f + 1.25f * std::sin(3.14159265f * t));
        p.pts.push_back(q);
        p.pts.push_back(ImVec2(q.x + n.x * k, q.y + n.y * k));
    }
    if (!left)
        for (auto& q : p.pts) q.x = w - q.x;
    return p;
}

Box parens(const Box& inner, float size, char open, char close) {
    float h = inner.asc + inner.desc;
    if (open == '|') {
        float top = -(inner.asc + 0.06f * size), bot = inner.desc + 0.06f * size;
        float th = std::max(1.0f, 0.055f * size), g = 0.12f * size;
        Box r;
        Prim l; l.kind = Prim::Line; l.thick = th; l.a = ImVec2(g, top); l.b = ImVec2(g, bot);
        r.prims.push_back(l);
        append(r, inner, 2 * g, 0);
        Prim rr = l; rr.a.x = rr.b.x = 3 * g + inner.w;
        r.prims.push_back(rr);
        r.w = 4 * g + inner.w;
        r.asc = -top;
        r.desc = bot;
        return r;
    }
    if (h <= 1.15f * size) {
        Box r = hcat({text(std::string(1, open), size), inner, text(std::string(1, close), size)});
        r.asc = std::max(r.asc, 0.78f * size);
        r.desc = std::max(r.desc, 0.26f * size);
        return r;
    }
    float top = -(inner.asc + 0.1f * size), bot = inner.desc + 0.1f * size;
    float H = bot - top;
    float pw = std::min(0.5f * size, 0.2f * size + 0.05f * H);
    float th = std::max(1.0f, 0.045f * size + 0.01f * H);
    Box r;
    r.prims.push_back(bigParen(top, bot, pw, th, true));
    append(r, inner, pw + 0.06f * size, 0);
    Prim right = bigParen(top, bot, pw, th, false);
    float off = pw + inner.w + 0.1f * size;
    for (auto& q : right.pts) q.x += off;
    r.prims.push_back(right);
    r.w = off + pw + 0.04f * size;
    r.asc = -top;
    r.desc = bot;
    return r;
}

static Box bigop(uint32_t cp, const Box& lower, const Box& upper, float size) {
    float bs = qs(size * 1.45f);
    std::string g = eq::utf8(cp);
    ImFontBaked* baked = font()->GetFontBaked(bs);
    float inkTop = -0.75f * bs, inkBot = 0.25f * bs, gw = font()->CalcTextSizeA(bs, FLT_MAX, 0.f, g.c_str()).x;
    if (baked) {
        if (ImFontGlyph* gl = baked->FindGlyph((ImWchar)cp)) {
            float asc = std::round(baked->Ascent);
            inkTop = gl->Y0 - asc;
            inkBot = gl->Y1 - asc;
        }
    }
    float axis = 0.25f * size;
    float dy = -axis - (inkTop + inkBot) * 0.5f;  // centre the glyph on the math axis
    inkTop += dy;
    inkBot += dy;
    float w = std::max({gw, lower.w, upper.w});
    Box r;
    Prim p;
    p.kind = Prim::Text;
    p.size = bs;
    p.s = g;
    p.a = ImVec2((w - gw) * 0.5f, dy);
    r.prims.push_back(p);
    float gap = 0.12f * size;
    float lb = inkBot + gap + lower.asc;
    float ub = inkTop - gap - upper.desc;
    append(r, lower, (w - lower.w) * 0.5f, lb);
    append(r, upper, (w - upper.w) * 0.5f, ub);
    r.w = w;
    r.asc = -ub + upper.asc;
    r.desc = lb + lower.desc;
    return hcat({space(0.08f * size), r, space(0.12f * size)});
}

std::string fmtNum(double v, int sig) {
    if (std::isnan(v)) return "NaN";
    if (std::isinf(v)) return v > 0 ? "\xE2\x88\x9E" : std::string(MINUS) + "\xE2\x88\x9E";
    char b[64];
    double av = std::fabs(v);
    if (av >= 1e6 || (av > 0 && av < 1e-4)) {  // a x 10^n with superscript digits
        int e = (int)std::floor(std::log10(av));
        double m = av / std::pow(10.0, e);
        if (m >= 9.9995) { m /= 10; ++e; }
        snprintf(b, sizeof b, "%.*g", std::max(1, sig - 1), m);
        std::string s = std::string(b) == "1" ? "" : std::string(b) + "\xC3\x97";
        s += "10";
        static const char* sup[] = {"\xE2\x81\xB0", "\xC2\xB9", "\xC2\xB2", "\xC2\xB3", "\xE2\x81\xB4",
                                    "\xE2\x81\xB5", "\xE2\x81\xB6", "\xE2\x81\xB7", "\xE2\x81\xB8", "\xE2\x81\xB9"};
        if (e < 0) s += "\xE2\x81\xBB";  // superscript minus
        for (char c : std::to_string(std::abs(e))) s += sup[c - '0'];
        return (v < 0 ? std::string(MINUS) : std::string()) + s;
    }
    double r = std::round(av);
    if (std::fabs(av - r) < 1e-9 * std::max(1.0, av) && av < 1e15) snprintf(b, sizeof b, "%.0f", r);
    else {
        snprintf(b, sizeof b, "%.*g", sig, av);
    }
    std::string s = b;
    if (s == "0") return s;
    return (v < 0 ? std::string(MINUS) : std::string()) + s;
}

std::string greekOrName(const std::string& name) {
    std::string base = name, sub;
    size_t us = name.find('_');
    if (us != std::string::npos) { base = name.substr(0, us); sub = name.substr(us + 1); }
    std::string out = base;
    if (uint32_t g = eq::greekCodepoint(base)) out = eq::utf8(g);
    if (!sub.empty()) {
        bool digits = std::all_of(sub.begin(), sub.end(), [](unsigned char c) { return std::isdigit(c); });
        if (digits) for (char c : sub) out += eq::utf8(0x2080 + (c - '0'));
        else out += "_" + sub;
    }
    return out;
}

std::string italicName(const std::string& name) {
    std::string base = name, sub;
    size_t us = name.find('_');
    if (us != std::string::npos) { base = name.substr(0, us); sub = name.substr(us + 1); }
    std::string out = italicize(base);
    if (!sub.empty()) {
        bool digits = std::all_of(sub.begin(), sub.end(), [](unsigned char c) { return std::isdigit(c); });
        if (digits) for (char c : sub) out += eq::utf8(0x2080 + (c - '0'));
        else out += "_" + sub;
    }
    return out;
}

// ------------------------------------------------------------- expression layout

namespace {

struct Ctx {
    const Rename* ren;
    float minSize;
};

int prec(const NodeP& n) {
    switch (n->op) {
        case Op::Num:
            if (n->val.imag() == 0) return n->val.real() < 0 ? 30 : 60;
            if (n->val.real() == 0) return n->val.imag() < 0 ? 30 : 20;
            return 10;
        case Op::Const: case Op::Var: case Op::Func: case Op::NumDeriv: case Op::Call: return 60;
        case Op::Add: case Op::Sub: return 10;
        case Op::Sum: case Op::Prod: return 15;
        case Op::Mul: return 20;
        case Op::Div: return 25;
        case Op::Neg: return 30;
        case Op::Pow: return 40;
        case Op::Fact: return 50;
    }
    return 60;
}

bool startsWithDigit(const NodeP& n) {
    switch (n->op) {
        case Op::Num: return n->val.imag() == 0 ? n->val.real() >= 0 : n->val.real() > 0;
        case Op::Pow: case Op::Mul: case Op::Fact: return startsWithDigit(n->kids[0]);
        default: return false;
    }
}

Box lay(const NodeP& n, float size, const Ctx& c);

Box layP(const NodeP& n, float size, const Ctx& c, bool paren) {
    Box b = lay(n, size, c);
    return paren ? parens(b, size) : b;
}

Box layNum(eq::cplx v, float size) {
    double re = v.real(), im = v.imag();
    if (im == 0) return text(fmtNum(re), size);
    auto imPart = [&](double x, bool withSign) {
        std::vector<Box> parts;
        if (withSign && x < 0) parts.push_back(text(MINUS, size));
        double ax = std::fabs(x);
        if (ax != 1) parts.push_back(text(fmtNum(ax), size));
        parts.push_back(ident("i", size));
        return hcat(parts);
    };
    if (re == 0) return imPart(im, true);
    return hcat({text(fmtNum(re), size), binop(im < 0 ? MINUS : "+", size), imPart(std::fabs(im), false)});
}

Box layFunc(const NodeP& n, float size, const Ctx& c) {
    const NodeP& a = n->kids[0];
    if (n->kids.size() == 2) {
        const NodeP& b = n->kids[1];
        if (n->fn == eq::Fn::Legendre) {  // Legendre symbol (a / p)
            float s = std::max(c.minSize, size * 0.9f);
            Box f = frac(lay(a, s, c), lay(b, s, c), size);
            f.prims.erase(std::remove_if(f.prims.begin(), f.prims.end(), [](const Prim& p) { return p.kind == Prim::Line; }),
                          f.prims.end());  // no fraction bar
            return parens(f, size);
        }
        return hcat({layP(a, size, c, prec(a) < 60), binop("mod", size), layP(b, size, c, prec(b) < 60)});
    }
    switch (n->fn) {
        case eq::Fn::Sqrt: return radical(lay(a, size, c), size);
        case eq::Fn::Abs: return parens(lay(a, size, c), size, '|', '|');
        case eq::Fn::Exp: return sup(ident("e", size), lay(a, std::max(c.minSize, size * 0.7f), c), size);
        case eq::Fn::Conj: {
            Box in = layP(a, size, c, prec(a) < 60);
            Prim bar;
            bar.kind = Prim::Line;
            bar.thick = std::max(1.0f, 0.05f * size);
            bar.a = ImVec2(0.05f * size, -(in.asc + 0.06f * size));
            bar.b = ImVec2(in.w - 0.02f * size, bar.a.y);
            in.prims.push_back(bar);
            in.asc += 0.12f * size;
            return in;
        }
        case eq::Fn::Floor:
            return hcat({text("\xE2\x8C\x8A", size), lay(a, size, c), text("\xE2\x8C\x8B", size)});
        case eq::Fn::Ceil:
            return hcat({text("\xE2\x8C\x88", size), lay(a, size, c), text("\xE2\x8C\x89", size)});
        case eq::Fn::Gamma: return hcat({text("\xCE\x93", size), parens(lay(a, size, c), size)});
        case eq::Fn::Zeta: return hcat({ident("zeta", size), parens(lay(a, size, c), size)});
        case eq::Fn::Mobius: return hcat({ident("mu", size), parens(lay(a, size, c), size)});
        case eq::Fn::PrimePi: return hcat({ident("pi", size), parens(lay(a, size, c), size)});
        case eq::Fn::ZetaZero: {  // gamma_k: imaginary part of the k-th zero
            Box g = ident("gamma", size);
            Box s = lay(a, std::max(c.minSize, size * 0.7f), c);
            float d = 0.22f * size;
            Box r = g;
            for (Prim p : s.prims) {
                p.a.x += g.w; p.a.y += d; p.b.x += g.w; p.b.y += d;
                for (auto& q : p.pts) { q.x += g.w; q.y += d; }
                r.prims.push_back(p);
            }
            r.w = g.w + s.w;
            r.desc = std::max(g.desc, s.desc + d);
            return r;
        }
        default:
            return hcat({text(eq::fnDisplayName(n->fn), size), space(0.06f * size), parens(lay(a, size, c), size)});
    }
}

Box lay(const NodeP& n, float size, const Ctx& c) {
    const float scr = std::max(c.minSize, size * 0.7f);
    switch (n->op) {
        case Op::Num: return layNum(n->val, size);
        case Op::Const:
            if (n->name == "inf") return text("\xE2\x88\x9E", size);
            return ident(n->name, size);
        case Op::Var: {
            auto it = c.ren->find(n->name);
            return ident(it != c.ren->end() ? it->second : n->name, size);
        }
        case Op::Add: {
            const NodeP &a = n->kids[0], &b = n->kids[1];
            Box L = lay(a, size, c);
            if (b->op == Op::Neg)
                return hcat({L, binop(MINUS, size), layP(b->kids[0], size, c, prec(b->kids[0]) <= 10)});
            if (b->op == Op::Num && b->val.imag() == 0 && b->val.real() < 0)
                return hcat({L, binop(MINUS, size), text(fmtNum(-b->val.real()), size)});
            return hcat({L, binop("+", size), layP(b, size, c, prec(b) < 10)});
        }
        case Op::Sub: {
            const NodeP &a = n->kids[0], &b = n->kids[1];
            return hcat({lay(a, size, c), binop(MINUS, size), layP(b, size, c, prec(b) <= 10 || b->op == Op::Neg || prec(b) == 30)});
        }
        case Op::Mul: {
            const NodeP &a = n->kids[0], &b = n->kids[1];
            int pa = prec(a), pb = prec(b);
            Box L = layP(a, size, c, pa < 20);
            bool rp = pb < 15 || pb == 30;
            Box R = layP(b, size, c, rp);
            bool dot = !rp && (startsWithDigit(b) || (a->op == Op::Num && b->op == Op::Num));
            if (dot) return hcat({L, space(0.1f * size), text("\xE2\x8B\x85", size), space(0.1f * size), R});
            float g = 0.05f * size;
            if (a->op == Op::Num) g = 0.02f * size;
            if (b->op == Op::Func || a->op == Op::Func || b->op == Op::Sum || b->op == Op::Prod) g = 0.12f * size;
            return hcat({L, space(g), R});
        }
        case Op::Div: {
            float s = std::max(c.minSize, size * 0.92f);
            return frac(lay(n->kids[0], s, c), lay(n->kids[1], s, c), size);
        }
        case Op::Pow: {
            const NodeP &a = n->kids[0], &b = n->kids[1];
            Box base = layP(a, size, c, prec(a) < 60 || (a->op == Op::Num && a->val.imag() != 0));
            return sup(base, lay(b, scr, c), size);
        }
        case Op::Neg: {
            const NodeP& a = n->kids[0];
            return hcat({text(MINUS, size), layP(a, size, c, prec(a) <= 10 || a->op == Op::Neg)});
        }
        case Op::Func: return layFunc(n, size, c);
        case Op::Sum: case Op::Prod: {
            Ctx cs = c;
            Box lower = hcat({ident(n->name, scr), text("=", scr), lay(n->kids[0], scr, cs)});
            Box upper = lay(n->kids[1], scr, cs);
            Box op = bigop(n->op == Op::Sum ? 0x2211 : 0x220F, lower, upper, size);
            const NodeP& body = n->kids[2];
            return hcat({op, layP(body, size, c, prec(body) < 20)});
        }
        case Op::Fact: {
            const NodeP& a = n->kids[0];
            return hcat({layP(a, size, c, prec(a) < 60), text("!", size)});
        }
        case Op::Call: {
            std::vector<Box> parts;
            for (size_t i = 0; i < n->kids.size(); ++i) {
                if (i) { parts.push_back(text(",", size)); parts.push_back(space(0.2f * size)); }
                parts.push_back(lay(n->kids[i], size, c));
            }
            return hcat({ident(n->name, size), parens(hcat(parts), size)});
        }
        case Op::NumDeriv: {
            auto it = c.ren->find(n->name);
            const std::string& v = it != c.ren->end() ? it->second : n->name;
            Box f = frac(text("d", size * 0.9f), hcat({text("d", size * 0.9f), ident(v, size * 0.9f)}), size);
            return hcat({f, parens(lay(n->kids[0], size, c), size)});
        }
    }
    return text("?", size);
}

}  // namespace

Box expr(const NodeP& n, float size, const Rename& rename) {
    if (!n) return Box{};
    Ctx c{&rename, std::max(9.0f, size * 0.5f)};
    return lay(n, size, c);
}

void draw(ImDrawList* dl, const Box& b, ImVec2 o, ImU32 col) {
    ImFont* f = font();
    for (const Prim& p : b.prims) {
        switch (p.kind) {
            case Prim::Text: {
                ImFont* pf = p.font ? p.font : f;
                ImFontBaked* baked = pf->GetFontBaked(p.size);
                float asc = baked ? std::round(baked->Ascent) : p.size * 0.8f;
                dl->AddText(pf, p.size, ImVec2(o.x + p.a.x, o.y + p.a.y - asc), col, p.s.c_str());
                break;
            }
            case Prim::Line:
                dl->AddLine(ImVec2(o.x + p.a.x, o.y + p.a.y), ImVec2(o.x + p.b.x, o.y + p.b.y), col, p.thick);
                break;
            case Prim::Strip: {
                const int n = (int)p.pts.size() / 2;
                if (n < 2) break;
                const ImVec2 uv = dl->_Data->TexUvWhitePixel;
                dl->PrimReserve((n - 1) * 6, n * 2);
                ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;
                for (int i = 0; i < n - 1; ++i) {
                    ImDrawIdx o0 = (ImDrawIdx)(base + 2 * i), i0 = (ImDrawIdx)(o0 + 1), o1 = (ImDrawIdx)(o0 + 2), i1 = (ImDrawIdx)(o0 + 3);
                    dl->PrimWriteIdx(o0); dl->PrimWriteIdx(i0); dl->PrimWriteIdx(o1);
                    dl->PrimWriteIdx(i0); dl->PrimWriteIdx(i1); dl->PrimWriteIdx(o1);
                }
                for (const ImVec2& q : p.pts) dl->PrimWriteVtx(ImVec2(o.x + q.x, o.y + q.y), uv, col);
                break;
            }
            case Prim::Poly: case Prim::Fill: {
                std::vector<ImVec2> pts(p.pts.size());
                for (size_t i = 0; i < pts.size(); ++i) pts[i] = ImVec2(o.x + p.pts[i].x, o.y + p.pts[i].y);
                if (p.kind == Prim::Poly) dl->AddPolyline(pts.data(), (int)pts.size(), col, p.thick, 0);
                else dl->AddConcavePolyFilled(pts.data(), (int)pts.size(), col);
                break;
            }
        }
    }
}

}  // namespace ts
