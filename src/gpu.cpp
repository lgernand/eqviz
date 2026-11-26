#include "gpu.hpp"

#include <cmath>
#include <cstdio>
#include <map>
#include <memory>

#include "gl.hpp"

namespace gpu {

#if defined(__APPLE__)
static const bool kHaveGL3 = true;
#else
static const bool kHaveGL3 = false;  // needs a GL function loader on other platforms
#endif

bool available() { return kHaveGL3; }

// ------------------------------------------------------------------ code generation

namespace {

using eq::Fn;
using eq::NodeP;
using eq::Op;

const char* kHelpers = R"GLSL(
const float PI = 3.14159265358979;
vec2 cmul(vec2 a, vec2 b) { return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x); }
vec2 cdiv(vec2 a, vec2 b) { float d = dot(b, b); return vec2(a.x * b.x + a.y * b.y, a.y * b.x - a.x * b.y) / d; }
vec2 cexp(vec2 a) { return exp(a.x) * vec2(cos(a.y), sin(a.y)); }
vec2 clog(vec2 a) { return vec2(log(length(a)), atan(a.y, a.x)); }
vec2 csqrt(vec2 a) {
    float r = length(a);
    float s = a.y < 0.0 ? -1.0 : 1.0;
    return vec2(sqrt(max(0.0, 0.5 * (r + a.x))), s * sqrt(max(0.0, 0.5 * (r - a.x))));
}
vec2 cpow(vec2 a, vec2 b) {
    if (b.y == 0.0 && b.x == floor(b.x) && abs(b.x) <= 64.0) {
        int n = int(abs(b.x));
        vec2 r = vec2(1.0, 0.0), base = a;
        for (int i = 0; i < 7; i++) {
            if ((n & 1) != 0) r = cmul(r, base);
            n >>= 1;
            if (n == 0) break;
            base = cmul(base, base);
        }
        return b.x < 0.0 ? cdiv(vec2(1.0, 0.0), r) : r;
    }
    if (a.x == 0.0 && a.y == 0.0) return b.x > 0.0 ? vec2(0.0) : vec2(1e38 * 10.0);
    if (a.y == 0.0 && a.x > 0.0) { float l = log(a.x); return exp(b.x * l) * vec2(cos(b.y * l), sin(b.y * l)); }
    return cexp(cmul(b, clog(a)));
}
vec2 csin(vec2 a) { return vec2(sin(a.x) * cosh(a.y), cos(a.x) * sinh(a.y)); }
vec2 ccos(vec2 a) { return vec2(cos(a.x) * cosh(a.y), -sin(a.x) * sinh(a.y)); }
vec2 csinh(vec2 a) { return vec2(sinh(a.x) * cos(a.y), cosh(a.x) * sin(a.y)); }
vec2 ccosh(vec2 a) { return vec2(cosh(a.x) * cos(a.y), sinh(a.x) * sin(a.y)); }
vec2 casin(vec2 a) {  // -i log(iz + sqrt(1 - z^2))
    vec2 w = clog(vec2(-a.y, a.x) + csqrt(vec2(1.0, 0.0) - cmul(a, a)));
    return vec2(w.y, -w.x);
}
vec2 catan(vec2 a) {  // (i/2) (log(1 - iz) - log(1 + iz))
    vec2 w = clog(vec2(1.0 + a.y, -a.x)) - clog(vec2(1.0 - a.y, a.x));
    return 0.5 * vec2(-w.y, w.x);
}
vec2 gamma_pos(vec2 z) {  // Lanczos, Re z >= 0.5
    z -= vec2(1.0, 0.0);
    vec2 x = vec2(0.99999999999980993, 0.0);
    x += cdiv(vec2(676.5203681218851, 0.0), z + vec2(1.0, 0.0));
    x += cdiv(vec2(-1259.1392167224028, 0.0), z + vec2(2.0, 0.0));
    x += cdiv(vec2(771.32342877765313, 0.0), z + vec2(3.0, 0.0));
    x += cdiv(vec2(-176.61502916214059, 0.0), z + vec2(4.0, 0.0));
    x += cdiv(vec2(12.507343278686905, 0.0), z + vec2(5.0, 0.0));
    x += cdiv(vec2(-0.13857109526572012, 0.0), z + vec2(6.0, 0.0));
    x += cdiv(vec2(9.9843695780195716e-6, 0.0), z + vec2(7.0, 0.0));
    x += cdiv(vec2(1.5056327351493116e-7, 0.0), z + vec2(8.0, 0.0));
    vec2 t = z + vec2(7.5, 0.0);
    return 2.5066282746310002 * cmul(cmul(cpow(t, z + vec2(0.5, 0.0)), cexp(-t)), x);
}
vec2 cgamma(vec2 z) {
    if (z.x >= 0.5) return gamma_pos(z);
    return cdiv(vec2(PI, 0.0), cmul(csin(PI * z), gamma_pos(vec2(1.0, 0.0) - z)));
}
vec2 cfact(vec2 a) {  // exact for small non-negative integers, Gamma otherwise
    if (a.y == 0.0 && a.x >= 0.0 && a.x <= 34.0 && a.x == floor(a.x)) {
        float r = 1.0;
        for (int k = 2; k <= 34; k++) { if (float(k) > a.x) break; r *= float(k); }
        return vec2(r, 0.0);
    }
    return cgamma(a + vec2(1.0, 0.0));
}
uniform float u_zw[129];
uniform int u_zn;
vec2 zeta_pos(vec2 s) {  // Borwein, weights w_k = (d_k - d_n) / d_n precomputed on the CPU
    vec2 sum = vec2(0.0);
    for (int k = 0; k < 128; k++) {
        if (k >= u_zn) break;
        float l = log(float(k + 1));
        vec2 t = u_zw[k] * exp(-s.x * l) * vec2(cos(-s.y * l), sin(-s.y * l));
        sum += (k % 2 == 1) ? -t : t;
    }
    vec2 den = vec2(1.0, 0.0) - cpow(vec2(2.0, 0.0), vec2(1.0, 0.0) - s);
    return cdiv(-sum, den);
}
vec2 czeta(vec2 s) {
    if (s.x >= 0.0) return zeta_pos(s);
    vec2 r = vec2(1.0, 0.0) - s;
    return cmul(cmul(cmul(cpow(vec2(2.0, 0.0), s), cpow(vec2(PI, 0.0), s - vec2(1.0, 0.0))),
                     csin(0.5 * PI * s)), cmul(gamma_pos(r), zeta_pos(r)));
}
vec3 hsv2rgb(float h, float s, float v) {
    vec3 k = clamp(abs(mod(h * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return v * mix(vec3(1.0), k, s);
}
vec4 domainColor(vec2 w) {  // same palette as the CPU path
    if (any(isnan(w)) || any(isinf(w))) return vec4(1.0);
    float mag = length(w);
    if (isinf(mag)) return vec4(1.0);
    float h = atan(w.y, w.x) / (2.0 * PI);
    if (h < 0.0) h += 1.0;
    float lm = log2(mag + 1e-30);
    float v = 0.66 + 0.34 * (0.5 + 0.5 * cos(2.0 * PI * lm));
    float s = 0.72;
    v *= 0.3 + 0.7 * (mag / (mag + 0.08));
    if (mag > 50.0) s *= (50.0 / mag) * 0.5 + 0.5;
    return vec4(hsv2rgb(h, s, v), 1.0);
}
)GLSL";

struct Gen {
    const std::vector<std::string>& params;
    std::vector<std::string> bound;  // sum indices in scope (innermost last)
    std::string funcs;               // generated helper functions (sums)
    std::string why;
    bool usesZeta = false;
    int nextId = 0;
    bool cVar = false;  // fractals: `c` is the complex iteration constant

    static std::string ident(const std::string& n) {
        std::string s = "v_";
        for (char c : n) s += std::isalnum((unsigned char)c) ? c : '_';
        return s;
    }
    static std::string num(double re, double im) {
        char b[96];
        snprintf(b, sizeof b, "vec2(%.9g, %.9g)", re, im);
        return b;
    }
    std::string scopeArgs(bool decl) const {
        std::string s = decl ? "vec2 z" : "z";
        if (cVar) s += decl ? ", vec2 c" : ", c";
        for (auto& b : bound) s += decl ? ", float " + ident(b) : ", " + ident(b);
        return s;
    }

    std::string gen(const NodeP& n) {
        if (!why.empty()) return "vec2(0.0)";
        auto k = [&](int i) { return gen(n->kids[i]); };
        switch (n->op) {
            case Op::Num:
                if (!std::isfinite(n->val.real()) || !std::isfinite(n->val.imag())) { why = "infinite constant"; return "vec2(0.0)"; }
                return num(n->val.real(), n->val.imag());
            case Op::Const:
                if (n->name == "inf") { why = "infinity"; return "vec2(0.0)"; }
                return num(n->val.real(), n->val.imag());
            case Op::Var: {
                for (auto it = bound.rbegin(); it != bound.rend(); ++it)
                    if (*it == n->name) return "vec2(" + ident(n->name) + ", 0.0)";
                if (n->name == "x") return "z";
                if (cVar && n->name == "c") return "c";
                for (size_t i = 0; i < params.size(); ++i)
                    if (params[i] == n->name) return "vec2(u_p[" + std::to_string(i) + "], 0.0)";
                return "vec2(0.0)";
            }
            case Op::Add: return "(" + k(0) + " + " + k(1) + ")";
            case Op::Sub: return "(" + k(0) + " - " + k(1) + ")";
            case Op::Neg: return "(-" + k(0) + ")";
            case Op::Mul: return "cmul(" + k(0) + ", " + k(1) + ")";
            case Op::Div: return "cdiv(" + k(0) + ", " + k(1) + ")";
            case Op::Pow: return "cpow(" + k(0) + ", " + k(1) + ")";
            case Op::Fact: return "cfact(" + k(0) + ")";
            case Op::Func: return func(n);
            case Op::Sum: case Op::Prod: return sum(n);
            default: why = "numeric derivative"; return "vec2(0.0)";
        }
    }

    std::string func(const NodeP& n) {
        if (n->kids.size() != 1) { why = std::string(eq::fnDisplayName(n->fn)) + " (two arguments)"; return "vec2(0.0)"; }
        const std::string a = gen(n->kids[0]);
        switch (n->fn) {
            case Fn::Sin: return "csin(" + a + ")";
            case Fn::Cos: return "ccos(" + a + ")";
            case Fn::Tan: return "cdiv(csin(" + a + "), ccos(" + a + "))";
            case Fn::Sec: return "cdiv(vec2(1.0, 0.0), ccos(" + a + "))";
            case Fn::Csc: return "cdiv(vec2(1.0, 0.0), csin(" + a + "))";
            case Fn::Cot: return "cdiv(ccos(" + a + "), csin(" + a + "))";
            case Fn::Asin: return "casin(" + a + ")";
            case Fn::Acos: return "(vec2(0.5 * PI, 0.0) - casin(" + a + "))";
            case Fn::Atan: return "catan(" + a + ")";
            case Fn::Sinh: return "csinh(" + a + ")";
            case Fn::Cosh: return "ccosh(" + a + ")";
            case Fn::Tanh: return "cdiv(csinh(" + a + "), ccosh(" + a + "))";
            case Fn::Exp: return "cexp(" + a + ")";
            case Fn::Ln: return "clog(" + a + ")";
            case Fn::Log: return "(clog(" + a + ") / 2.302585093)";
            case Fn::Sqrt: return "csqrt(" + a + ")";
            case Fn::Abs: return "vec2(length(" + a + "), 0.0)";
            case Fn::Re: return "vec2((" + a + ").x, 0.0)";
            case Fn::Im: return "vec2((" + a + ").y, 0.0)";
            case Fn::Conj: return "(vec2(1.0, -1.0) * " + a + ")";
            case Fn::Arg: { std::string t = a; return "vec2(atan((" + t + ").y, (" + t + ").x), 0.0)"; }
            case Fn::Floor: return "floor(" + a + ")";
            case Fn::Ceil: return "ceil(" + a + ")";
            case Fn::Round: return "floor(" + a + " + vec2(0.5))";
            case Fn::Sign: return "normalize(" + a + ")";
            case Fn::Gamma: return "cgamma(" + a + ")";
            case Fn::Zeta: usesZeta = true; return "czeta(" + a + ")";
            default: why = std::string(eq::fnDisplayName(n->fn)); return "vec2(0.0)";
        }
    }

    std::string sum(const NodeP& n) {
        const NodeP &lo = n->kids[0], &hi = n->kids[1], &body = n->kids[2];
        if ((lo->op == Op::Const && lo->name == "inf") || (hi->op == Op::Const && hi->name == "inf") ||
            (lo->op == Op::Neg && lo->kids[0]->op == Op::Const && lo->kids[0]->name == "inf")) {
            why = "infinite series";
            return "vec2(0.0)";
        }
        const bool isSum = n->op == Op::Sum;
        const std::string name = "series" + std::to_string(nextId++);
        const std::string args = scopeArgs(true), call = scopeArgs(false);
        const std::string glo = gen(lo), ghi = gen(hi);
        bound.push_back(n->name);
        const std::string gb = gen(body);
        bound.pop_back();
        const std::string idx = ident(n->name);
        std::string f;
        f += "vec2 " + name + "(" + args + ") {\n";
        f += "    float lo = ceil((" + glo + ").x - 1e-4), hi = floor((" + ghi + ").x + 1e-4);\n";
        f += "    float cnt = hi - lo + 1.0;\n";
        f += std::string("    vec2 acc = ") + (isSum ? "vec2(0.0)" : "vec2(1.0, 0.0)") + ";\n";
        f += "    if (cnt <= 0.0) return acc;\n";
        f += std::string("    float lim = ") + (n->animated ? "u_sumLimit" : "1e30") + ";\n";
        f += "    float nfull = min(cnt, floor(lim));\n";
        f += "    float fr = lim < cnt ? lim - floor(lim) : 0.0;\n";
        f += "    for (int j = 0; j < 20000; j++) {\n";
        f += "        if (float(j) >= nfull) break;\n";
        f += "        float " + idx + " = lo + float(j);\n";
        f += std::string("        ") + (isSum ? "acc += " + gb : "acc = cmul(acc, " + gb + ")") + ";\n";
        f += "    }\n";
        f += "    if (fr > 1e-6 && nfull < cnt) {\n";
        f += "        float " + idx + " = lo + nfull;\n";
        f += "        vec2 t = " + gb + ";\n";
        f += std::string("        ") + (isSum ? "acc += fr * t;" : "acc = cmul(acc, vec2(1.0, 0.0) + fr * (t - vec2(1.0, 0.0)));") + "\n";
        f += "    }\n    return acc;\n}\n";
        funcs += f;
        return name + "(" + call + ")";
    }
};

const char* kVertex = R"GLSL(#version 150
uniform vec4 u_rect;  // x0, y0, x1, y1 in framebuffer pixels (y down)
uniform vec2 u_fb;
void main() {
    vec2 c = vec2((gl_VertexID & 1) == 0 ? u_rect.x : u_rect.z, (gl_VertexID & 2) == 0 ? u_rect.y : u_rect.w);
    gl_Position = vec4(c.x / u_fb.x * 2.0 - 1.0, 1.0 - c.y / u_fb.y * 2.0, 0.0, 1.0);
}
)GLSL";

}  // namespace

std::string fragmentSource(const NodeP& f, const std::vector<std::string>& params, std::string& why) {
    Gen g{params, {}, {}, {}};
    const std::string body = g.gen(f);
    if (!g.why.empty()) { why = g.why; return ""; }
    std::string s = "#version 150\nuniform float u_p[32];\nuniform float u_sumLimit;\nuniform float u_alpha;\n";
    s += "uniform vec2 u_fb;\nuniform float u_fbScale;\nuniform vec4 u_view;  // cx, cy, scale, aspect\n";
    s += "uniform vec2 u_cc;\nout vec4 fragColor;\n";
    s += kHelpers;
    s += g.funcs;
    s += "vec2 F(vec2 z) { return " + body + "; }\n";
    s += "void main() {\n";
    s += "    vec2 px = vec2(gl_FragCoord.x, u_fb.y - gl_FragCoord.y) / u_fbScale;\n";
    s += "    vec2 z = vec2(u_view.x + (px.x - u_cc.x) / u_view.z, u_view.y - (px.y - u_cc.y) / (u_view.z * u_view.w));\n";
    s += "    vec4 c = domainColor(F(z));\n";
    s += "    fragColor = vec4(c.rgb, c.a * u_alpha);\n}\n";
    return s;
}

// Escape-time fractal of the map z <- f(z, c). Colours match fractalColor() on the CPU.
std::string fractalSource(const NodeP& f, const std::vector<std::string>& params, std::string& why) {
    Gen g{params, {}, {}, {}};
    g.cVar = true;
    const std::string body = g.gen(f);
    if (!g.why.empty()) { why = g.why; return ""; }
    std::string s = "#version 150\nuniform float u_p[32];\nuniform float u_sumLimit;\nuniform float u_alpha;\n";
    s += "uniform vec2 u_fb;\nuniform float u_fbScale;\nuniform vec4 u_view;\nuniform vec2 u_cc;\n";
    s += "uniform vec2 u_c;\nuniform int u_iter;\nuniform int u_julia;\nuniform float u_density;\n";
    s += "uniform vec3 u_pal[5];\nuniform vec3 u_bg;\nuniform int u_settle;\nout vec4 fragColor;\n";
    s += kHelpers;
    s += g.funcs;
    s += "vec2 F(vec2 z, vec2 c) { return " + body + "; }\n";
    s += R"GLSL(
vec3 palette(float t) {
    t = fract(t) * 5.0;
    int i = int(floor(t));
    float u = t - float(i);
    vec3 a = u_pal[0], b = u_pal[1];
    for (int k = 0; k < 5; k++) if (k == i) { a = u_pal[k]; b = u_pal[(k + 1) % 5]; }
    return mix(a, b, u);
}
void main() {
    vec2 px = vec2(gl_FragCoord.x, u_fb.y - gl_FragCoord.y) / u_fbScale;
    vec2 p = vec2(u_view.x + (px.x - u_cc.x) / u_view.z, u_view.y - (px.y - u_cc.y) / (u_view.z * u_view.w));
    vec2 z = u_julia != 0 ? p : vec2(0.0);
    vec2 c = u_julia != 0 ? u_c : p;
    vec3 col = u_bg;
    for (int i = 0; i < 20000; i++) {
        if (i >= u_iter) break;
        vec2 prev = z;
        z = F(z, c);
        float m = dot(z, z);
        if (isnan(m) || isinf(m) || m > 65536.0) {
            float nu = float(i);
            if (!isnan(m) && !isinf(m)) nu = float(i) + 1.0 - log2(max(1e-9, 0.5 * log2(m) / 8.0));
            nu = max(nu, 0.0);
            col = mix(u_bg, palette(sqrt(nu) * 0.18 * u_density), clamp(nu / 8.0, 0.0, 1.0));
            break;
        }
        vec2 d = z - prev;
        if (dot(d, d) < 1e-10 * max(1.0, m)) {  // settled on a fixed point: colour by where it went
            if (u_settle == 0) break;
            float h = atan(z.y, z.x) / (2.0 * PI);
            float v = 0.22 + 0.6 * exp(-float(i) * 0.1 / u_density);
            col = hsv2rgb(h - floor(h), 0.5, v);
            break;
        }
    }
    fragColor = vec4(col, u_alpha);
}
)GLSL";
    return s;
}

// ------------------------------------------------------------------ GL programs

#if defined(__APPLE__)
namespace {

struct Program {
    GLuint id = 0;
    GLint rect, fb, p, sumLimit, alpha, fbScale, view, cc, zw, zn;
    GLint fc, iter, julia, density, pal, bg, settle;
};
std::map<std::string, std::unique_ptr<Program>> g_programs;  // by fragment source
std::map<std::string, bool> g_failed;
GLuint g_vao = 0;

GLuint compileShader(GLenum type, const std::string& src, std::string& log) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[2048];
        glGetShaderInfoLog(s, sizeof buf, nullptr, buf);
        log = buf;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

Program* programFor(const std::string& frag) {
    auto it = g_programs.find(frag);
    if (it != g_programs.end()) return it->second.get();
    if (g_failed.count(frag)) return nullptr;
    std::string log;
    GLuint vs = compileShader(GL_VERTEX_SHADER, kVertex, log);
    GLuint fs = vs ? compileShader(GL_FRAGMENT_SHADER, frag, log) : 0;
    if (!vs || !fs) {
        std::fprintf(stderr, "eqviz: GPU shader failed, using the CPU instead:\n%s\n", log.c_str());
        if (vs) glDeleteShader(vs);
        g_failed[frag] = true;
        return nullptr;
    }
    auto p = std::make_unique<Program>();
    p->id = glCreateProgram();
    glAttachShader(p->id, vs);
    glAttachShader(p->id, fs);
    glBindFragDataLocation(p->id, 0, "fragColor");
    glLinkProgram(p->id);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = 0;
    glGetProgramiv(p->id, GL_LINK_STATUS, &ok);
    if (!ok) {
        glDeleteProgram(p->id);
        g_failed[frag] = true;
        return nullptr;
    }
    p->rect = glGetUniformLocation(p->id, "u_rect");
    p->fb = glGetUniformLocation(p->id, "u_fb");
    p->p = glGetUniformLocation(p->id, "u_p");
    p->sumLimit = glGetUniformLocation(p->id, "u_sumLimit");
    p->alpha = glGetUniformLocation(p->id, "u_alpha");
    p->fbScale = glGetUniformLocation(p->id, "u_fbScale");
    p->view = glGetUniformLocation(p->id, "u_view");
    p->cc = glGetUniformLocation(p->id, "u_cc");
    p->zw = glGetUniformLocation(p->id, "u_zw");
    p->zn = glGetUniformLocation(p->id, "u_zn");
    p->fc = glGetUniformLocation(p->id, "u_c");
    p->iter = glGetUniformLocation(p->id, "u_iter");
    p->julia = glGetUniformLocation(p->id, "u_julia");
    p->density = glGetUniformLocation(p->id, "u_density");
    p->pal = glGetUniformLocation(p->id, "u_pal");
    p->bg = glGetUniformLocation(p->id, "u_bg");
    p->settle = glGetUniformLocation(p->id, "u_settle");
    if (g_programs.size() > 64) g_programs.clear();  // forget old expressions
    Program* raw = p.get();
    g_programs[frag] = std::move(p);
    return raw;
}

struct Payload {
    Program* prog;
    DomainColorDraw d;
    bool fractal;
    FractalDraw fd;
};

void callback(const ImDrawList*, const ImDrawCmd* cmd) {
    const Payload& pl = *(const Payload*)cmd->UserCallbackData;
    const Program* p = pl.prog;
    ImDrawData* dd = ImGui::GetDrawData();
    const float sc = dd->FramebufferScale.x;
    const float fbW = dd->DisplaySize.x * sc, fbH = dd->DisplaySize.y * dd->FramebufferScale.y;
    ImVec4 cr = cmd->ClipRect;
    cr.x = (cr.x - dd->DisplayPos.x) * sc; cr.z = (cr.z - dd->DisplayPos.x) * sc;
    cr.y = (cr.y - dd->DisplayPos.y) * sc; cr.w = (cr.w - dd->DisplayPos.y) * sc;
    if (cr.z <= cr.x || cr.w <= cr.y) return;
    if (!g_vao) glGenVertexArrays(1, &g_vao);
    glUseProgram(p->id);
    glBindVertexArray(g_vao);
    glEnable(GL_SCISSOR_TEST);
    glScissor((GLint)cr.x, (GLint)(fbH - cr.w), (GLsizei)(cr.z - cr.x), (GLsizei)(cr.w - cr.y));
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    const DomainColorDraw& d = pl.d;
    glUniform4f(p->rect, cr.x, cr.y, cr.z, cr.w);
    glUniform2f(p->fb, fbW, fbH);
    glUniform1f(p->fbScale, sc);
    glUniform4f(p->view, (float)d.cx, (float)d.cy, (float)d.scale, (float)d.aspect);
    glUniform2f(p->cc, d.ccx, d.ccy);
    glUniform1f(p->alpha, d.alpha);
    glUniform1f(p->sumLimit, d.sumLimit);
    if (p->p >= 0 && d.nParams > 0) glUniform1fv(p->p, d.nParams, d.params);
    if (p->zw >= 0 && d.zetaN > 0) {
        const std::vector<double>& w = eq::zetaBorweinWeights(d.zetaN);
        float wf[129] = {0};
        for (int k = 0; k <= d.zetaN && k < 129; ++k) wf[k] = (float)w[k];
        glUniform1fv(p->zw, 129, wf);
        glUniform1i(p->zn, d.zetaN);
    }
    if (pl.fractal) {
        const FractalDraw& f = pl.fd;
        glUniform2f(p->fc, f.cRe, f.cIm);
        glUniform1i(p->iter, f.iter);
        glUniform1i(p->julia, f.julia ? 1 : 0);
        glUniform1f(p->density, f.density);
        glUniform3fv(p->pal, 5, f.pal);
        glUniform3fv(p->bg, 1, f.bg);
        glUniform1i(p->settle, f.colorSettled ? 1 : 0);
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

}  // namespace
#endif

bool drawDomainColoring(ImDrawList* dl, const NodeP& f, const std::vector<std::string>& params,
                        const DomainColorDraw& d) {
#if defined(__APPLE__)
    if (!f) return false;
    std::string why;
    const std::string src = fragmentSource(f, params, why);
    if (src.empty()) return false;
    Program* p = programFor(src);
    if (!p) return false;
    Payload pl{p, d, false, {}};
    dl->AddCallback(callback, &pl, sizeof pl);
    dl->AddCallback(ImGui::GetPlatformIO().DrawCallback_ResetRenderState, nullptr);
    return true;
#else
    (void)dl; (void)f; (void)params; (void)d;
    return false;
#endif
}

bool drawFractal(ImDrawList* dl, const NodeP& f, const std::vector<std::string>& params, const DomainColorDraw& d,
                 const FractalDraw& fd, std::string& why) {
#if defined(__APPLE__)
    if (!f) { why = "no function"; return false; }
    const std::string src = fractalSource(f, params, why);
    if (src.empty()) return false;
    Program* p = programFor(src);
    if (!p) { why = "shader failed to compile"; return false; }
    Payload pl{p, d, true, fd};
    dl->AddCallback(callback, &pl, sizeof pl);
    dl->AddCallback(ImGui::GetPlatformIO().DrawCallback_ResetRenderState, nullptr);
    return true;
#else
    (void)dl; (void)f; (void)params; (void)d; (void)fd;
    why = "no GPU path on this platform";
    return false;
#endif
}

void shutdown() {
#if defined(__APPLE__)
    for (auto& kv : g_programs) glDeleteProgram(kv.second->id);
    g_programs.clear();
    if (g_vao) glDeleteVertexArrays(1, &g_vao);
    g_vao = 0;
#endif
}

}  // namespace gpu
