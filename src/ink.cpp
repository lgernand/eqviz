// Handwriting panes: write an equation with the mouse, trackpad or a pen, and see it typeset in
// the chosen equation font. Recognition runs on this machine:
//   1. templates: each symbol rendered from several handwriting-like fonts, thinned to a skeleton
//      and sampled to a point cloud (plus templates learned from the user's own corrections);
//   2. symbols: strokes are grouped (dynamic programming over the writing order) and each group is
//      matched against the templates with the $P point-cloud recognizer; lines, dots, "=" and
//      radicals are recognized by their geometry;
//   3. layout: fraction bars, square roots, exponents and function names turn the symbols into an
//      expression, which the engine parses and the typesetter draws.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "app.hpp"
#include "ink_vision.hpp"
#include "theme.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#include "imstb_truetype.h"
#pragma clang diagnostic pop

using namespace theme;
namespace fs = std::filesystem;

namespace {

constexpr int kN = 48;  // points per cloud

using Cloud = std::vector<ImVec2>;

struct Tmpl {
    std::string label;
    Cloud cloud;
    bool user = false;
};

std::vector<Tmpl> g_tmpl;
bool g_persist = true;  // write learned templates to disk (off for scripted test runs)

// Vision's reading of a symbol, as one of our labels ("" if it isn't one)
std::string visionLabel(const std::string& s) {
    if (s == "\xCF\x80") return "pi";
    if (s == "\xCE\xB8") return "theta";
    if (s == "\xE2\x88\x9A") return "sqrt";
    if (s == "\xE2\x88\x92" || s == "\xE2\x80\x94" || s == "\xE2\x80\x93") return "-";
    if (s == "\xC3\x97") return "x";
    if (s.size() != 1) return "";
    const char c = s[0];
    if (std::isdigit((unsigned char)c)) return s;
    if (std::isalpha((unsigned char)c)) return std::string(1, (char)std::tolower((unsigned char)c));
    if (std::strchr("+=-()/![]<>", c)) return s;
    return "";
}
bool g_built = false;
std::map<uint64_t, std::vector<std::pair<std::string, float>>> g_cache;  // classification by stroke-group content

// ------------------------------------------------------------------ clouds

Cloud normalize(Cloud pts) {
    if (pts.empty()) return pts;
    float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f;
    for (auto& p : pts) { x0 = std::min(x0, p.x); y0 = std::min(y0, p.y); x1 = std::max(x1, p.x); y1 = std::max(y1, p.y); }
    const float s = std::max(1e-6f, std::max(x1 - x0, y1 - y0));  // keeps the aspect ratio ("-" is not "|")
    float cx = 0, cy = 0;
    for (auto& p : pts) { p.x = (p.x - x0) / s; p.y = (p.y - y0) / s; cx += p.x; cy += p.y; }
    cx /= pts.size();
    cy /= pts.size();
    for (auto& p : pts) { p.x -= cx; p.y -= cy; }
    return pts;
}

// Resample strokes to kN points spread by arc length.
Cloud resample(const std::vector<const std::vector<ImVec2>*>& strokes) {
    double total = 0;
    for (auto* s : strokes)
        for (size_t i = 1; i < s->size(); ++i) total += std::hypot((*s)[i].x - (*s)[i - 1].x, (*s)[i].y - (*s)[i - 1].y);
    Cloud out;
    if (total < 1e-6) {
        for (auto* s : strokes)
            for (auto& p : *s) out.push_back(p);
        if (out.empty()) return out;
        while ((int)out.size() < kN) out.push_back(out[out.size() % std::max<size_t>(1, out.size())]);
        out.resize(kN);
        return out;
    }
    const double step = total / (kN - 1);
    double carry = 0;
    for (auto* s : strokes) {
        if (s->empty()) continue;
        out.push_back((*s)[0]);
        carry = 0;
        for (size_t i = 1; i < s->size(); ++i) {
            ImVec2 a = (*s)[i - 1], b = (*s)[i];
            double d = std::hypot(b.x - a.x, b.y - a.y);
            while (carry + d >= step && d > 0) {
                const double t = (step - carry) / d;
                ImVec2 q(a.x + (float)t * (b.x - a.x), a.y + (float)t * (b.y - a.y));
                out.push_back(q);
                a = q;
                d = std::hypot(b.x - a.x, b.y - a.y);
                carry = 0;
            }
            carry += d;
        }
    }
    while ((int)out.size() < kN) out.push_back(out.back());
    if ((int)out.size() > kN) {  // stroke starts added a few extra points: thin evenly
        Cloud t;
        for (int i = 0; i < kN; ++i) t.push_back(out[(size_t)((double)i * (out.size() - 1) / (kN - 1))]);
        out = t;
    }
    return out;
}

float cloudDist(const Cloud& a, const Cloud& b, int start) {
    const int n = (int)a.size();
    bool used[kN] = {false};
    float sum = 0, wsum = 0;
    int i = start;
    do {
        float best = 1e30f;
        int bi = 0;
        for (int j = 0; j < n; ++j) {
            if (used[j]) continue;
            const float dx = a[i].x - b[j].x, dy = a[i].y - b[j].y, d = dx * dx + dy * dy;
            if (d < best) { best = d; bi = j; }
        }
        used[bi] = true;
        const float w = 1.f - (float)((i - start + n) % n) / n;
        sum += w * std::sqrt(best);
        wsum += w;
        i = (i + 1) % n;
    } while (i != start);
    return sum / std::max(1e-6f, wsum);
}

float match(const Cloud& a, const Cloud& b) {  // $P greedy cloud match, both directions
    if ((int)a.size() != kN || (int)b.size() != kN) return 1e9f;
    const int step = (int)std::floor(std::sqrt((double)kN));
    float best = 1e30f;
    for (int i = 0; i < kN; i += step) best = std::min({best, cloudDist(a, b, i), cloudDist(b, a, i)});
    return best;
}

// ------------------------------------------------------------------ templates from fonts

// Zhang-Suen thinning of a binary image (1 = ink), in place.
void thin(std::vector<unsigned char>& im, int w, int h) {
    auto at = [&](int x, int y) -> int { return x < 0 || y < 0 || x >= w || y >= h ? 0 : im[(size_t)y * w + x]; };
    bool changed = true;
    std::vector<size_t> kill;
    while (changed) {
        changed = false;
        for (int pass = 0; pass < 2; ++pass) {
            kill.clear();
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x) {
                    if (!at(x, y)) continue;
                    const int p2 = at(x, y - 1), p3 = at(x + 1, y - 1), p4 = at(x + 1, y), p5 = at(x + 1, y + 1), p6 = at(x, y + 1),
                              p7 = at(x - 1, y + 1), p8 = at(x - 1, y), p9 = at(x - 1, y - 1);
                    const int B = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
                    if (B < 2 || B > 6) continue;
                    const int seq[9] = {p2, p3, p4, p5, p6, p7, p8, p9, p2};
                    int A = 0;
                    for (int k = 0; k < 8; ++k) A += seq[k] == 0 && seq[k + 1] == 1;
                    if (A != 1) continue;
                    if (pass == 0 ? (p2 * p4 * p6 == 0 && p4 * p6 * p8 == 0) : (p2 * p4 * p8 == 0 && p2 * p6 * p8 == 0)) kill.push_back((size_t)y * w + x);
                }
            for (size_t k : kill) im[k] = 0;
            changed |= !kill.empty();
        }
    }
}

// Skeleton pixels of a glyph (y down, glyph pixels), or empty if the font lacks it.
std::vector<ImVec2> glyphSkeleton(const stbtt_fontinfo& f, int cp, float px, float* baselineOut = nullptr, float* advanceOut = nullptr) {
    const int g = stbtt_FindGlyphIndex(&f, cp);
    if (!g) return {};
    const float scale = stbtt_ScaleForPixelHeight(&f, px);
    int w = 0, h = 0, xo = 0, yo = 0;
    unsigned char* bm = stbtt_GetGlyphBitmap(&f, scale, scale, g, &w, &h, &xo, &yo);
    int adv = 0, lsb = 0;
    stbtt_GetGlyphHMetrics(&f, g, &adv, &lsb);
    if (advanceOut) *advanceOut = adv * scale;
    if (!bm || w < 1 || h < 1) { if (bm) stbtt_FreeBitmap(bm, nullptr); return {}; }
    const int W = w + 2, H = h + 2;
    std::vector<unsigned char> im((size_t)W * H, 0);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) im[(size_t)(y + 1) * W + x + 1] = bm[(size_t)y * w + x] > 110;
    stbtt_FreeBitmap(bm, nullptr);
    thin(im, W, H);
    std::vector<ImVec2> pts;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            if (im[(size_t)y * W + x]) pts.push_back(ImVec2((float)(x - 1 + xo), (float)(y - 1 + yo)));
    if (baselineOut) *baselineOut = 0;  // yo is already relative to the baseline
    return pts;
}

// kN well-spread points of a pixel set (farthest-point sampling).
Cloud spread(const std::vector<ImVec2>& pix) {
    Cloud out;
    if (pix.empty()) return out;
    std::vector<float> d(pix.size(), 1e30f);
    size_t cur = 0;
    for (size_t i = 1; i < pix.size(); ++i)  // start at the top-left-most point for determinism
        if (pix[i].y + pix[i].x < pix[cur].y + pix[cur].x) cur = i;
    for (int k = 0; k < kN; ++k) {
        out.push_back(pix[cur]);
        size_t far = cur;
        float fd = -1;
        for (size_t i = 0; i < pix.size(); ++i) {
            const float dx = pix[i].x - pix[cur].x, dy = pix[i].y - pix[cur].y;
            d[i] = std::min(d[i], dx * dx + dy * dy);
            if (d[i] > fd) { fd = d[i]; far = i; }
        }
        cur = far;
    }
    return out;
}

struct SymDef { const char* label; uint32_t cp; uint32_t mathCp; };
const std::vector<SymDef>& symbolSet() {
    static std::vector<SymDef> v = [] {
        std::vector<SymDef> s;
        static const char* digits[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
        for (int d = 0; d < 10; ++d) s.push_back({digits[d], (uint32_t)('0' + d), (uint32_t)('0' + d)});
        static std::string letters[26];
        for (int c = 0; c < 26; ++c) {
            letters[c] = std::string(1, (char)('a' + c));
            const uint32_t mi = c == 'h' - 'a' ? 0x210E : 0x1D44E + c;  // math italic (STIX)
            s.push_back({letters[c].c_str(), (uint32_t)('a' + c), mi});
        }
        const SymDef more[] = {{"+", '+', '+'}, {"(", '(', '('}, {")", ')', ')'}, {"/", '/', '/'}, {"!", '!', '!'},
                               {"<", '<', '<'}, {">", '>', '>'}, {"[", '[', '['}, {"]", ']', ']'},
                               {"pi", 0x3C0, 0x1D70B}, {"theta", 0x3B8, 0x1D703}};  // other Greek letters: teach them by correcting
        for (const SymDef& m : more) s.push_back(m);
        return s;
    }();
    return v;
}

struct FontFile { const char* path; int index; bool math; };
const FontFile kTemplateFonts[] = {
    {"/System/Library/Fonts/Supplemental/Chalkboard.ttc", 0, false},
    {"/System/Library/Fonts/Supplemental/Chalkboard.ttc", 1, false},
    {"/System/Library/Fonts/Supplemental/ChalkboardSE.ttc", 0, false},
    {"/System/Library/Fonts/Supplemental/ChalkboardSE.ttc", 2, false},
    {"/System/Library/Fonts/Noteworthy.ttc", 0, false},
    {"/System/Library/Fonts/Noteworthy.ttc", 1, false},
    {"/System/Library/Fonts/MarkerFelt.ttc", 0, false},
    {"/System/Library/Fonts/MarkerFelt.ttc", 1, false},
    {"/System/Library/Fonts/Supplemental/Comic Sans MS.ttf", 0, false},
    {"/System/Library/Fonts/Supplemental/Comic Sans MS Bold.ttf", 0, false},
    {"/System/Library/Fonts/Supplemental/Trattatello.ttf", 0, false},
    {"/System/Library/Fonts/Supplemental/Chalkduster.ttf", 0, false},
    {"/System/Library/Fonts/Supplemental/STIXTwoMath.otf", 0, true},
    {"/System/Library/Fonts/Supplemental/STIXTwoText.ttf", 0, false},
    {"/System/Library/Fonts/Helvetica.ttc", 0, false},
};
// Not used for templates, so it stands in for "unseen" handwriting in tests.
const FontFile kTestFont = {"/System/Library/Fonts/Supplemental/Bradley Hand Bold.ttf", 0, false};

struct LoadedFont {
    std::vector<unsigned char> data;
    stbtt_fontinfo info{};
    bool ok = false;
};
bool loadFont(const FontFile& ff, LoadedFont& lf) {
    std::ifstream in(ff.path, std::ios::binary);
    if (!in) return false;
    lf.data.assign(std::istreambuf_iterator<char>(in), {});
    const int off = stbtt_GetFontOffsetForIndex(lf.data.data(), ff.index);
    lf.ok = off >= 0 && stbtt_InitFont(&lf.info, lf.data.data(), off);
    return lf.ok;
}

std::string userTemplatePath() {
    const char* home = std::getenv("HOME");
    fs::path dir = fs::path(home ? home : ".") / "Library" / "Application Support" / "eqviz";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return (dir / "ink_templates.txt").string();
}

void buildTemplates() {
    if (g_built) return;
    g_built = true;
    for (const FontFile& ff : kTemplateFonts) {
        LoadedFont lf;
        if (!loadFont(ff, lf)) continue;
        for (const SymDef& sd : symbolSet()) {
            const uint32_t cp = ff.math ? sd.mathCp : sd.cp;
            auto pix = glyphSkeleton(lf.info, (int)cp, 72.f);
            if (pix.size() < 6) continue;
            g_tmpl.push_back({sd.label, normalize(spread(pix)), false});
        }
    }
    {   // a radical without its overbar, drawn by hand
        Cloud r;
        const ImVec2 pts[] = {{0, 0.55f}, {0.12f, 0.5f}, {0.32f, 1.f}, {0.62f, 0}};
        std::vector<ImVec2> line(pts, pts + 4);
        std::vector<const std::vector<ImVec2>*> one = {&line};
        g_tmpl.push_back({"sqrt", normalize(resample(one)), false});
    }
    std::ifstream in(userTemplatePath());  // learned from the user's corrections
    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string label, pts;
        if (!std::getline(ss, label, '\t') || !std::getline(ss, pts)) continue;
        Cloud c;
        std::stringstream ps(pts);
        float x, y;
        char comma;
        while (ps >> x >> comma >> y) c.push_back(ImVec2(x, y));
        if ((int)c.size() == kN) g_tmpl.push_back({label, c, true});
    }
}

// ------------------------------------------------------------------ classifying a stroke group

struct Box { float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f; };
Box boxOf(const InkStrokes& s, const std::vector<int>& idx) {
    Box b;
    for (int i : idx)
        for (auto& p : s[i]) { b.x0 = std::min(b.x0, p.x); b.y0 = std::min(b.y0, p.y); b.x1 = std::max(b.x1, p.x); b.y1 = std::max(b.y1, p.y); }
    return b;
}

bool isFlat(const Box& b, float unit) { return (b.y1 - b.y0) < 0.24f * std::max(1.f, b.x1 - b.x0) && (b.x1 - b.x0) > 0.3f * unit; }

// A radical: the low point is on the left and a long flat run crosses the top to the right.
bool isRadical(const std::vector<ImVec2>& s, const Box& b) {
    const float w = b.x1 - b.x0, h = b.y1 - b.y0;
    if (s.size() < 6 || w < 0.85f * h || h < 4) return false;
    // it starts at the left, below the top (the small tick), not at the top-left corner like a 5 or 7
    if (s.front().x > b.x0 + 0.3f * w || s.front().y < b.y0 + 0.3f * h) return false;
    float lowX = 0, lowY = -1e30f;
    for (auto& p : s) if (p.y > lowY) { lowY = p.y; lowX = p.x; }
    if (lowX > b.x0 + 0.5f * w) return false;
    float topMin = 1e30f, topMax = -1e30f;
    for (auto& p : s)
        if (p.y < b.y0 + 0.18f * h) { topMin = std::min(topMin, p.x); topMax = std::max(topMax, p.x); }
    return topMax > b.x0 + 0.85f * w && topMin < b.x0 + 0.55f * w && topMax - topMin > 0.4f * w;
}

uint64_t groupHash(const InkStrokes& s, const std::vector<int>& idx) {
    Hasher h;
    for (int i : idx) {
        h.u(0x9E37u + s[i].size());
        for (auto& p : s[i]) { h.d(p.x); h.d(p.y); }
    }
    return h.h;
}

std::vector<std::pair<std::string, float>> classify(const InkStrokes& s, const std::vector<int>& idx, float unit) {
    const Box b = boxOf(s, idx);
    const float w = b.x1 - b.x0, h = b.y1 - b.y0;
    if (idx.size() == 1) {
        if (std::max(w, h) < 0.2f * unit) return {{".", 0.03f}};
        if (isFlat(b, unit)) return {{"-", 0.035f}};
        if (isRadical(s[idx[0]], b)) return {{"sqrt", 0.035f}};
    }
    if (idx.size() == 2) {
        const Box a = boxOf(s, {idx[0]}), c = boxOf(s, {idx[1]});
        const float wa = a.x1 - a.x0, wc = c.x1 - c.x0;
        const float ov = std::min(a.x1, c.x1) - std::max(a.x0, c.x0);
        if (isFlat(a, unit * 0.5f) && isFlat(c, unit * 0.5f) && ov > 0.4f * std::min(wa, wc) && std::max(wa, wc) < 3.f * std::min(wa, wc) &&
            std::fabs((a.y0 + a.y1) - (c.y0 + c.y1)) * 0.5f < 1.1f * unit)
            return {{"=", 0.01f}};
    }
    const uint64_t key = groupHash(s, idx);
    auto it = g_cache.find(key);
    if (it != g_cache.end()) return it->second;
    std::vector<const std::vector<ImVec2>*> strokes;
    for (int i : idx) strokes.push_back(&s[i]);
    const Cloud c = normalize(resample(strokes));
    std::map<std::string, float> best;
    for (const Tmpl& t : g_tmpl) {
        float d = match(c, t.cloud);
        if (t.user) d *= 0.93f;  // the user's own writing counts a little more
        auto bi = best.find(t.label);
        if (bi == best.end() || d < bi->second) best[t.label] = d;
    }
    std::vector<std::pair<std::string, float>> v(best.begin(), best.end());
    std::sort(v.begin(), v.end(), [](auto& x, auto& y) { return x.second < y.second; });
    if (v.size() > 8) v.resize(8);
    if (g_cache.size() > 4000) g_cache.clear();
    g_cache[key] = v;
    return v;
}

// ------------------------------------------------------------------ layout -> expression

bool isDigit(const std::string& l) { return l.size() == 1 && std::isdigit((unsigned char)l[0]); }
bool isLetter(const std::string& l) { return (l.size() == 1 && std::isalpha((unsigned char)l[0])) || l == "pi" || l == "theta" || l == "alpha" || l == "lambda"; }
bool isOperand(const std::string& l) { return isDigit(l) || isLetter(l) || l == ")" || l == "]" || l == "!"; }

struct Sym {  // working copy for the parser
    std::string label;
    float x0, y0, x1, y1;
    const InkSymbol* src;
    float cx() const { return 0.5f * (x0 + x1); }
    float cy() const { return 0.5f * (y0 + y1); }
    float w() const { return x1 - x0; }
    float h() const { return y1 - y0; }
};

std::string parse(std::vector<Sym> syms);

std::string joinLinear(const std::vector<Sym>& seq) {
    // A symbol is an exponent when it sits clearly above the line: judged against the line's median
    // baseline and symbol height (short letters like x would mislead a symbol-by-symbol test).
    std::vector<float> bottoms, heights;
    for (auto& s : seq)
        if (s.label != "-" && s.label != "." && s.label != "=") { bottoms.push_back(s.y1); heights.push_back(s.h()); }
    std::sort(bottoms.begin(), bottoms.end());
    std::sort(heights.begin(), heights.end());
    const float baseline = bottoms.empty() ? 0 : bottoms[bottoms.size() / 2];
    const float H = heights.empty() ? 1 : std::max(1.f, heights[heights.size() / 2]);
    auto isOp = [](const std::string& l) { return l == "-" || l == "+" || l == "=" || l == "." || l == "<" || l == ">"; };
    auto raised = [&](const Sym& s) { return !isOp(s.label) && s.y1 < baseline - 0.38f * H && s.h() < 0.9f * H; };
    std::string out;
    std::string prev;  // previous token's label, for spacing
    const Sym* base = nullptr;
    for (size_t i = 0; i < seq.size(); ++i) {
        const Sym& s = seq[i];
        if (base && isOperand(base->label) && raised(s)) {
            std::vector<Sym> ex;
            while (i < seq.size()) {  // an operator joins the exponent only between raised symbols, high up
                if (raised(seq[i])) ex.push_back(seq[i++]);
                else if (isOp(seq[i].label) && i + 1 < seq.size() && raised(seq[i + 1]) && seq[i].cy() < baseline - 0.6f * H) ex.push_back(seq[i++]);
                else break;
            }
            --i;
            out += "^(" + parse(ex) + ")";
            prev = ")";
            continue;
        }
        std::string t = s.label;
        if (t == "-" || t == "+" || t == "=" || t == "<" || t == ">") { out += " " + t + " "; prev = t; base = nullptr; continue; }
        if (t == ".") {
            const bool decimal = !prev.empty() && isDigit(prev) && i + 1 < seq.size() && isDigit(seq[i + 1].label);
            out += decimal ? "." : " * ";
            prev = ".";
            base = nullptr;
            continue;
        }
        // spacing: letters next to letters or digits multiply (and "x2" would read as x subscript 2)
        if (!prev.empty() && ((isLetter(t) && (isLetter(prev) || isDigit(prev) || prev == ")")) || (isDigit(t) && isLetter(prev)) ||
                              (t == "(" && (isOperand(prev))) || (prev == "fn" && t != "(")))
            out += " ";
        out += t;
        prev = (t.size() > 1 && !isLetter(t)) ? "fn" : t;
        base = &seq[i];
    }
    return out;
}

std::string parse(std::vector<Sym> syms) {
    if (syms.empty()) return "";
    std::sort(syms.begin(), syms.end(), [](const Sym& a, const Sym& b) { return a.cx() < b.cx(); });
    // 1. the widest fraction bar with something above and below it
    int bar = -1;
    float barW = 0;
    for (int i = 0; i < (int)syms.size(); ++i) {
        const Sym& b = syms[i];
        if (b.label != "-") continue;
        bool above = false, below = false;
        for (int j = 0; j < (int)syms.size(); ++j) {
            if (j == i || syms[j].cx() < b.x0 || syms[j].cx() > b.x1) continue;
            if (syms[j].cy() < b.cy()) above = true;
            else below = true;
        }
        if (above && below && b.w() > barW) { bar = i; barW = b.w(); }
    }
    if (bar >= 0) {
        const Sym b = syms[bar];
        std::vector<Sym> left, num, den, right;
        for (int j = 0; j < (int)syms.size(); ++j) {
            if (j == bar) continue;
            const Sym& s = syms[j];
            if (s.cx() < b.x0) left.push_back(s);
            else if (s.cx() > b.x1) right.push_back(s);
            else if (s.cy() < b.cy()) num.push_back(s);
            else den.push_back(s);
        }
        std::string l = parse(left), r = parse(right);
        std::string f = "(" + parse(num) + ")/(" + parse(den) + ")";
        return l + (l.empty() ? "" : " ") + f + (r.empty() ? "" : " " + r);
    }
    // 2. square roots cover what follows them
    for (int i = 0; i < (int)syms.size(); ++i) {
        const Sym rt = syms[i];
        if (rt.label != "sqrt") continue;
        std::vector<Sym> before, inside, after;
        for (int j = 0; j < (int)syms.size(); ++j) {
            if (j == i) continue;
            const Sym& s = syms[j];
            if (s.cx() > rt.x0 + 0.2f * rt.w() && s.cx() < rt.x1 + 0.05f * rt.w() && s.cy() > rt.y0 && s.cy() < rt.y1 + 0.1f * rt.h()) inside.push_back(s);
            else if (s.cx() < rt.cx()) before.push_back(s);
            else after.push_back(s);
        }
        std::string l = parse(before), r = parse(after);
        return l + (l.empty() ? "" : " ") + "sqrt(" + parse(inside) + ")" + (r.empty() ? "" : " " + r);
    }
    // 3. a line of symbols, with function names merged
    static const char* names[] = {"sqrt", "sinh", "cosh", "tanh", "asin", "acos", "atan", "sin", "cos", "tan", "sec", "csc", "cot", "exp", "log", "abs", "ln"};
    std::vector<Sym> seq;
    for (size_t i = 0; i < syms.size();) {
        bool merged = false;
        for (const char* nm : names) {
            const size_t L = std::strlen(nm);
            if (i + L > syms.size()) continue;
            bool ok = true;
            for (size_t k = 0; k < L && ok; ++k) {
                const Sym& s = syms[i + k];
                if (k > 0 && std::fabs(s.cy() - syms[i].cy()) > 0.6f * std::max(s.h(), syms[i].h())) { ok = false; break; }  // same line
                const std::string want(1, nm[k]);
                bool has = s.label == want;
                if (!has && s.src) has = std::find(s.src->shape.begin(), s.src->shape.end(), want) != s.src->shape.end();
                if (!has && s.src && !s.src->alts.empty()) {
                    const float best = s.src->alts[0].second;
                    for (size_t a = 0; a < s.src->alts.size() && a < 5; ++a)
                        if (s.src->alts[a].first == want && s.src->alts[a].second <= best * 1.3f + 0.005f) has = true;
                }
                ok = has;
            }
            if (!ok) continue;
            Sym m = syms[i];
            m.label = nm;
            for (size_t k = 1; k < L; ++k) {
                m.x1 = std::max(m.x1, syms[i + k].x1);
                m.y0 = std::min(m.y0, syms[i + k].y0);
                m.y1 = std::max(m.y1, syms[i + k].y1);
            }
            seq.push_back(m);
            i += L;
            merged = true;
            break;
        }
        if (!merged) seq.push_back(syms[i++]);
    }
    return joinLinear(seq);
}

// Prefer digits among digits and letters among letters when a symbol is ambiguous.
void contextFix(std::vector<InkSymbol>& syms) {
    std::vector<int> order(syms.size());
    for (size_t i = 0; i < syms.size(); ++i) order[i] = (int)i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return syms[a].box.x < syms[b].box.x; });
    std::vector<std::string> orig;  // decide from the first reads, so one change can't cascade
    for (int i : order) orig.push_back(syms[i].label);
    for (size_t k = 0; k < order.size(); ++k) {
        InkSymbol& s = syms[order[k]];
        if (s.alts.size() < 2 || s.label.size() != 1) continue;
        const bool left = k > 0 && isDigit(orig[k - 1]), right = k + 1 < order.size() && isDigit(orig[k + 1]);
        const bool leftLetter = k > 0 && isLetter(orig[k - 1]), rightLetter = k + 1 < order.size() && isLetter(orig[k + 1]);
        const bool nbDigit = (left || right) && !leftLetter && !rightLetter;
        if (!nbDigit || isDigit(s.label)) continue;
        const float best = s.alts[0].second;
        for (auto& a : s.alts)
            if (isDigit(a.first) && a.second <= best * 1.15f + 0.003f) { s.label = a.first; break; }
    }
}

}  // namespace

// ------------------------------------------------------------------ recognition

void inkSetPersist(bool on) { g_persist = on; }

void App::inkRecognize() {
    Hasher h;
    for (auto& s : inkStrokes_) {
        h.u(s.size());
        for (auto& p : s) { h.d(p.x); h.d(p.y); }
    }
    for (auto& [k, v] : inkFix_) { for (char c : k + v) h.u((uint64_t)(unsigned char)c); }
    if (h.h == inkHash_) return;
    inkHash_ = h.h;
    inkResult_ = InkResult{};
    if (inkStrokes_.empty()) return;
    buildTemplates();
    const InkStrokes& S = inkStrokes_;
    const int m = (int)S.size();
    // typical symbol size: median of the larger side of each non-flat stroke
    std::vector<float> sizes;
    for (int i = 0; i < m; ++i) {
        const Box b = boxOf(S, {i});
        if ((b.y1 - b.y0) > 0.3f * (b.x1 - b.x0)) sizes.push_back(std::max(b.x1 - b.x0, b.y1 - b.y0));
    }
    std::sort(sizes.begin(), sizes.end());
    const float unit = sizes.empty() ? 40.f : std::max(12.f, sizes[sizes.size() / 2]);

    // group consecutive strokes (in writing order) into symbols: dynamic programming over 1-3 stroke groups
    std::vector<float> best(m + 1, 1e30f);
    std::vector<int> from(m + 1, -1);
    best[0] = 0;
    auto key = [](const std::vector<int>& idx) {
        std::string k;
        for (int i : idx) k += std::to_string(i) + ",";
        return k;
    };
    for (int j = 1; j <= m; ++j)
        for (int k = 1; k <= 3 && k <= j; ++k) {
            std::vector<int> idx;
            for (int i = j - k; i < j; ++i) idx.push_back(i);
            if (k > 1) {  // the strokes of one symbol overlap or nearly touch, and never include a long bar
                Box u = boxOf(S, {idx[0]});
                bool ok = true;
                for (int q = 0; q < k && ok; ++q) {
                    const Box b = boxOf(S, {idx[q]});
                    if (isFlat(b, unit) && (b.x1 - b.x0) > 1.7f * unit) ok = false;  // fraction bars stand alone
                    if (isRadical(S[idx[q]], b)) ok = false;                          // so do radicals
                }
                for (int q = 1; q < k && ok; ++q) {
                    const Box b = boxOf(S, {idx[q]});
                    const float ov = std::min(u.x1, b.x1) - std::max(u.x0, b.x0);
                    const float gapY = std::max(0.f, std::max(b.y0 - u.y1, u.y0 - b.y1));
                    if (ov < -0.15f * unit || gapY > 0.9f * unit) ok = false;
                    u.x0 = std::min(u.x0, b.x0); u.x1 = std::max(u.x1, b.x1); u.y0 = std::min(u.y0, b.y0); u.y1 = std::max(u.y1, b.y1);
                }
                if (!ok) continue;
            }
            auto fix = inkFix_.find(key(idx));
            float c;
            if (fix != inkFix_.end()) c = 0.f;  // the user said what this is
            else {
                auto alts = classify(S, idx, unit);
                c = alts.empty() ? 1.f : alts[0].second;
            }
            c += 0.045f;  // per-symbol cost: prefer fewer, better-fitting symbols
            if (best[j - k] + c < best[j]) { best[j] = best[j - k] + c; from[j] = k; }
        }
    std::vector<std::vector<int>> groups;
    for (int j = m; j > 0; j -= from[j]) {
        std::vector<int> idx;
        for (int i = j - from[j]; i < j; ++i) idx.push_back(i);
        groups.push_back(idx);
        if (from[j] <= 0) break;
    }
    std::reverse(groups.begin(), groups.end());
    for (auto& g : groups) {
        InkSymbol sym;
        sym.strokes = g;
        sym.key = key(g);
        const Box b = boxOf(S, g);
        sym.box = ImVec4(b.x0, b.y0, b.x1, b.y1);
        sym.alts = classify(S, g, unit);
        auto fix = inkFix_.find(sym.key);
        sym.label = fix != inkFix_.end() ? fix->second : sym.alts.empty() ? "?" : sym.alts[0].first;
        for (size_t a = 0; a < sym.alts.size() && a < 3; ++a) sym.shape.push_back(sym.alts[a].first);
        inkResult_.symbols.push_back(sym);
    }
    if (inkVision_ && inkvision::available()) {
        // Apple Vision reads the whole ink as text; each character it finds votes for the symbol under it
        const std::vector<inkvision::Char> chars = inkvision::readChars(S, 3);
        if (std::getenv("EQVIZ_DEBUG_INK")) {
            std::string v;
            for (auto& c : chars) { char b[48]; std::snprintf(b, sizeof b, "%s(%.2f) ", c.text.c_str(), c.conf); v += b; }
            std::printf("vision: %s\n", v.c_str());
        }
        std::vector<std::map<std::string, float>> votes(inkResult_.symbols.size());
        // per text line and reading: align Vision's characters with the symbols inside the line, left to right
        std::map<std::pair<int, int>, std::vector<const inkvision::Char*>> lines;
        for (const auto& c : chars) lines[{c.line, c.rank}].push_back(&c);
        for (auto& kv : lines) {
            const auto& lk = kv.first;
            const auto& lc = kv.second;
            const inkvision::Char& L0 = *lc[0];
            const float pad = 0.35f * unit;
            std::vector<int> syms;
            for (size_t k = 0; k < inkResult_.symbols.size(); ++k) {
                const ImVec4& b = inkResult_.symbols[k].box;
                const float cx = 0.5f * (b.x + b.z), cy = 0.5f * (b.y + b.w);
                if (cx >= L0.x0 - pad && cx <= L0.x1 + pad && cy >= L0.y0 - pad && cy <= L0.y1 + pad) syms.push_back((int)k);
            }
            std::sort(syms.begin(), syms.end(), [&](int a, int b) { return inkResult_.symbols[a].box.x < inkResult_.symbols[b].box.x; });
            const int n = (int)syms.size(), mm = (int)lc.size();
            if (!n || !mm) continue;
            auto subCost = [&](int i, int j) {
                const std::string l = visionLabel(lc[j]->text);
                const InkSymbol& sy = inkResult_.symbols[syms[i]];
                if (l.empty()) return 0.9f;
                if (l == sy.label) return 0.f;
                for (size_t a = 0; a < sy.alts.size() && a < 6; ++a)
                    if (sy.alts[a].first == l) return 0.15f + 0.05f * a;
                return 0.7f;  // Vision disagrees with every guess: still a plausible pairing
            };
            std::vector<std::vector<float>> D(n + 1, std::vector<float>(mm + 1, 0));
            for (int i = 1; i <= n; ++i) D[i][0] = i * 0.5f;
            for (int j = 1; j <= mm; ++j) D[0][j] = j * 0.5f;
            for (int i = 1; i <= n; ++i)
                for (int j = 1; j <= mm; ++j)
                    D[i][j] = std::min({D[i - 1][j - 1] + subCost(i - 1, j - 1), D[i - 1][j] + 0.5f, D[i][j - 1] + 0.5f});
            const float w = lk.second == 0 ? 1.f : lk.second == 1 ? 0.5f : 0.3f;  // later readings count less
            for (int i = n, j = mm; i > 0 && j > 0;) {
                if (D[i][j] == D[i - 1][j - 1] + subCost(i - 1, j - 1)) {
                    const std::string l = visionLabel(lc[j - 1]->text);
                    if (!l.empty()) votes[syms[i - 1]][l] = std::max(votes[syms[i - 1]][l], w * lc[j - 1]->conf);
                    --i; --j;
                } else if (D[i][j] == D[i - 1][j] + 0.5f) --i;
                else --j;
            }
        }
        for (size_t k = 0; k < inkResult_.symbols.size(); ++k) {
            InkSymbol& sym = inkResult_.symbols[k];
            if (votes[k].empty() || sym.label == "-" || sym.label == "=" || sym.label == "." || sym.label == "sqrt" || inkFix_.count(sym.key)) continue;
            std::map<std::string, float> d;
            float worst = 0;
            for (auto& a : sym.alts) { d[a.first] = a.second; worst = std::max(worst, a.second); }
            for (auto& [l, conf] : votes[k]) {
                if (l == "-" || l == "=" || l == "sqrt") continue;  // geometry decides those
                if (!d.count(l)) d[l] = worst + 0.01f;
                d[l] *= 1.f - 0.65f * std::clamp(conf, 0.f, 1.f);
            }
            std::vector<std::pair<std::string, float>> v(d.begin(), d.end());
            std::sort(v.begin(), v.end(), [](auto& x, auto& y) { return x.second < y.second; });
            if (v.size() > 8) v.resize(8);
            sym.alts = v;
            sym.label = v[0].first;
        }
    }
    contextFix(inkResult_.symbols);
    for (auto& sym : inkResult_.symbols) {  // corrections win over context
        auto fix = inkFix_.find(sym.key);
        if (fix != inkFix_.end()) sym.label = fix->second;
    }
    if (std::getenv("EQVIZ_DEBUG_INK"))
        for (size_t k = 0; k < inkResult_.symbols.size(); ++k)
            std::printf("  symbol %zu %s box x %.0f-%.0f y %.0f-%.0f\n", k, inkResult_.symbols[k].label.c_str(), inkResult_.symbols[k].box.x,
                        inkResult_.symbols[k].box.z, inkResult_.symbols[k].box.y, inkResult_.symbols[k].box.w);
    std::vector<Sym> work;
    for (auto& s : inkResult_.symbols) work.push_back({s.label, s.box.x, s.box.y, s.box.z, s.box.w, &s});
    std::string e = parse(work);
    // tidy spaces
    std::string t;
    for (char c : e)
        if (!(c == ' ' && (t.empty() || t.back() == ' ' || t.back() == '('))) t += c;
    while (!t.empty() && t.back() == ' ') t.pop_back();
    inkResult_.expr = t;
    if (std::getenv("EQVIZ_DEBUG_INK")) {
        std::string labels;
        for (auto& sym : inkResult_.symbols) {
            char b[64];
            std::snprintf(b, sizeof b, "[%s %.3f|%zu", sym.label.c_str(), sym.alts.empty() ? 0.f : sym.alts[0].second, sym.strokes.size());
            labels += b;
            if (sym.alts.size() > 1) { std::snprintf(b, sizeof b, " %s %.3f", sym.alts[1].first.c_str(), sym.alts[1].second); labels += b; }
            labels += "]";
        }
        std::printf("ink: %s  ->  %s\n", labels.c_str(), t.c_str());
    }
    // check it parses (both sides of an equation)
    const size_t eqp = t.find('=');
    for (const std::string& part : eqp == std::string::npos ? std::vector<std::string>{t} : std::vector<std::string>{t.substr(0, eqp), t.substr(eqp + 1)}) {
        eq::ParseResult r = eq::parse(part);
        if (!r.root && inkResult_.error.empty()) inkResult_.error = r.error.empty() ? "can't read this yet" : r.error;
    }
}

// ------------------------------------------------------------------ the pane

// Learn a symbol from the user's own strokes (a correction).
bool inkTeach(const std::string& label, const InkStrokes& s, const std::vector<int>& idx) {
    if (label.empty() || label == "-" || label == "=" || label == "." || label == "sqrt") return false;  // those are geometric
    buildTemplates();
    std::vector<const std::vector<ImVec2>*> strokes;
    for (int i : idx) strokes.push_back(&s[i]);
    const Cloud c = normalize(resample(strokes));
    if ((int)c.size() != kN) return false;
    g_tmpl.push_back({label, c, true});
    g_cache.clear();
    if (!g_persist) return true;
    std::ofstream out(userTemplatePath(), std::ios::app);
    out << label << "\t";
    for (auto& p : c) out << p.x << "," << p.y << " ";
    out << "\n";
    return true;
}

namespace {
bool teach(const std::string& label, const InkStrokes& s, const std::vector<int>& idx) { return inkTeach(label, s, idx); }

ts::Box resultBox(const std::string& expr, float size) {
    const size_t eqp = expr.find('=');
    auto side = [&](const std::string& t) -> ts::Box {
        eq::ParseResult r = eq::parse(t);
        return r.root ? ts::expr(r.root, size) : ts::text(t, size);
    };
    if (eqp == std::string::npos) return side(expr);
    return ts::hcat({side(expr.substr(0, eqp)), ts::binop("=", size), side(expr.substr(eqp + 1))});
}
}  // namespace

void App::drawInkPane(ImDrawList* dl, int pane) {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 o = canvasMin_;
    const float W = canvasMax_.x - canvasMin_.x, H = canvasMax_.y - canvasMin_.y;
    const float stripH = std::min(120.f, H * 0.3f);
    const ImVec4 ink(o.x, o.y + 46, canvasMax_.x, canvasMax_.y - stripH);  // the writing area
    dl->AddRectFilled(canvasMin_, canvasMax_, ImGui::ColorConvertFloat4ToU32(surface(0.03f)));
    for (float y = ink.y + 70; y < ink.w - 10; y += 70)  // faint ruled lines
        dl->AddLine(ImVec2(ink.x + 20, y), ImVec2(ink.z - 20, y), col(WHITE, 0.05f), 1.f);

    if (!inkDrawing_) inkRecognize();

    // ---- input: left-drag writes, right-drag erases, click a symbol's tag to correct it
    const ImVec2 m = io.MousePos;
    const bool overInk = !io.WantCaptureMouse && !layoutBusy_ && inVisible(m) && m.x >= ink.x && m.x < ink.z && m.y >= ink.y && m.y < ink.w;
    std::vector<ImVec4> tags;
    const float tagS = 14.f;
    for (auto& s : inkResult_.symbols) {
        ts::Box lb = ts::text(s.label == "sqrt" ? "\xE2\x88\x9A" : s.label == "-" ? "\xE2\x88\x92" : s.label, tagS);
        const float tx = o.x + s.box.x, ty = o.y + s.box.y - 22;
        tags.push_back(ImVec4(tx - 2, ty, tx + lb.w + 8, ty + 18));
    }
    bool openFix = false;
    if (overInk && ImGui::IsMouseClicked(0)) {
        for (size_t k = 0; k < tags.size(); ++k)
            if (m.x >= tags[k].x && m.x < tags[k].z && m.y >= tags[k].y && m.y < tags[k].w) { inkPick_ = (int)k; openFix = true; }
        if (!openFix) {
            inkStrokes_.push_back({ImVec2(m.x - o.x, m.y - o.y)});
            inkDrawing_ = true;
        }
        focus_ = pane;
    }
    if (inkDrawing_) {
        if (ImGui::IsMouseDown(0)) {
            auto& st = inkStrokes_.back();
            const ImVec2 q(m.x - o.x, m.y - o.y);
            if (std::hypot(q.x - st.back().x, q.y - st.back().y) > 1.5f) st.push_back(q);
        } else inkDrawing_ = false;
    }
    if (overInk && ImGui::IsMouseClicked(1)) { inkErasing_ = true; focus_ = pane; }
    if (inkErasing_) {
        if (!ImGui::IsMouseDown(1)) inkErasing_ = false;
        else {
            const ImVec2 q(m.x - o.x, m.y - o.y);
            for (size_t i = 0; i < inkStrokes_.size(); ++i) {
                bool hit = false;
                for (auto& p : inkStrokes_[i]) hit |= std::hypot(p.x - q.x, p.y - q.y) < 12.f;
                if (hit) { inkStrokes_.erase(inkStrokes_.begin() + i); inkFix_.clear(); --i; }
            }
            dl->AddCircle(m, 12.f, col(WHITE, 0.5f), 24, 1.5f);
        }
    }

    // ---- the ink, and what each symbol was read as
    for (auto& st : inkStrokes_) {
        std::vector<ImVec2> pts;
        for (auto& p : st) pts.push_back(ImVec2(o.x + p.x, o.y + p.y));
        if (pts.size() == 1) dl->AddCircleFilled(pts[0], 2.2f, col(WHITE, 0.9f));
        else dl->AddPolyline(pts.data(), (int)pts.size(), col(WHITE, 0.92f), 3.f, 0);
    }
    if (!inkDrawing_)
        for (size_t k = 0; k < inkResult_.symbols.size(); ++k) {
            const InkSymbol& s = inkResult_.symbols[k];
            const bool fixed = inkFix_.count(s.key) > 0;
            dl->AddRect(ImVec2(o.x + s.box.x - 4, o.y + s.box.y - 4), ImVec2(o.x + s.box.z + 4, o.y + s.box.w + 4), col(BLUE_C, 0.22f), 4.f);
            const ImVec4 t = tags[k];
            const bool hot = m.x >= t.x && m.x < t.z && m.y >= t.y && m.y < t.w;
            dl->AddRectFilled(ImVec2(t.x, t.y), ImVec2(t.z, t.w), col(fixed ? GREEN_C : BLUE_C, hot ? 0.55f : 0.3f), 4.f);
            ts::draw(dl, ts::text(s.label == "sqrt" ? "\xE2\x88\x9A" : s.label == "-" ? "\xE2\x88\x92" : s.label, tagS), ImVec2(t.x + 4, t.w - 4), col(WHITE));
        }
    if (inkStrokes_.empty())
        dl->AddText(nullptr, 17.f, ImVec2(ink.x + 30, ink.y + 30), col(WHITE, 0.35f),
                    "Write an equation here with the mouse, trackpad or a pen.  Right-drag erases.");

    // ---- the result strip: the equation in the chosen font
    const ImVec2 r0(o.x, canvasMax_.y - stripH);
    dl->AddRectFilled(r0, canvasMax_, ImGui::ColorConvertFloat4ToU32(surface(0.07f)));
    dl->AddLine(r0, ImVec2(canvasMax_.x, r0.y), col(BLUE_C, 0.35f), 1.5f);
    if (!inkResult_.expr.empty()) {
        float size = 40.f * labelScale_;
        ts::Box b = resultBox(inkResult_.expr, size);
        const float maxW = W - 230, maxH = stripH - 34;
        const float fit = std::min({1.f, maxW > 60 ? maxW / std::max(1.f, b.w) : 1.f, maxH / std::max(1.f, b.asc + b.desc)});
        if (fit < 1) b = resultBox(inkResult_.expr, size * fit);
        const float baseY = r0.y + 6 + (stripH - 26 - (b.asc + b.desc)) * 0.5f + b.asc;  // centred above the text line
        ts::draw(dl, b, ImVec2(r0.x + 28, baseY), inkResult_.error.empty() ? col(YELLOW_C) : col(RED_C));
        dl->AddText(nullptr, 13.f, ImVec2(r0.x + 28, canvasMax_.y - 20), col(WHITE, 0.4f),
                    (inkResult_.error.empty() ? inkResult_.expr : inkResult_.expr + "    (" + inkResult_.error + ")").c_str());
    }

    // ---- toolbar (top) and actions (strip), as small ImGui windows over the pane
    const ImGuiWindowFlags fl = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoFocusOnAppearing;
    char id[32];
    std::snprintf(id, sizeof id, "##inktb%d", pane);
    ImGui::SetNextWindowPos(ImVec2(o.x + 10, o.y + 8));
    if (ImGui::Begin(id, nullptr, fl)) {
        ImGui::PushFont(fonts_.ui, 14.f);
        if (ImGui::Button("Undo") && !inkStrokes_.empty()) { inkStrokes_.pop_back(); inkFix_.clear(); }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) { inkStrokes_.clear(); inkFix_.clear(); }
        ImGui::SameLine(0, 14);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Equation font");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170);
        if (ImGui::BeginCombo("##inkfont", mathFontChoices()[mathFontIdx_].name)) {
            for (int i = 0; i < (int)mathFontChoices().size(); ++i)
                if (ImGui::Selectable(mathFontChoices()[i].name, i == mathFontIdx_)) pendingMathFont_ = i;
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("The font the recognized equation is typeset in (also set in Appearance)");
        if (openFix && inkPick_ >= 0) ImGui::OpenPopup("##inkfix");
        if (inkPick_ >= 0 && inkPick_ < (int)inkResult_.symbols.size()) {
            ImGui::SetNextWindowPos(ImVec2(tags[inkPick_].x, tags[inkPick_].w + 4), ImGuiCond_Appearing);
            if (ImGui::BeginPopup("##inkfix")) {
                InkSymbol& s = inkResult_.symbols[inkPick_];
                ImGui::TextDisabled("This symbol is:");
                std::string chosen;
                for (size_t a = 0; a < s.alts.size() && a < 6; ++a) {
                    const std::string& l = s.alts[a].first;
                    if (ImGui::Selectable((l + "##alt" + std::to_string(a)).c_str(), l == s.label)) chosen = l;
                }
                static char typed[24] = "";
                ImGui::SetNextItemWidth(120);
                if (ImGui::InputTextWithHint("##typed", "or type it", typed, sizeof typed, ImGuiInputTextFlags_EnterReturnsTrue) && typed[0]) {
                    chosen = typed;
                    typed[0] = 0;
                }
                ImGui::TextDisabled("Your correction teaches the recognizer.");
                if (!chosen.empty()) {
                    inkFix_[s.key] = chosen;
                    teach(chosen, inkStrokes_, s.strokes);
                    inkHash_ = 0;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            } else inkPick_ = -1;
        }
        ImGui::PopFont();
    }
    ImGui::End();

    std::snprintf(id, sizeof id, "##inkact%d", pane);
    ImGui::SetNextWindowPos(ImVec2(canvasMax_.x - 196, r0.y + stripH * 0.5f - 34));
    if (ImGui::Begin(id, nullptr, fl)) {
        ImGui::PushFont(fonts_.ui, 14.f);
        const bool ok = !inkResult_.expr.empty() && inkResult_.error.empty();
        ImGui::BeginDisabled(!ok);
        if (ImGui::Button("Add as function", ImVec2(170, 0))) {
            std::string e = inkResult_.expr;
            const size_t eqp = e.find('=');
            if (eqp != std::string::npos) {  // "y = ..." becomes the row's body; "f(x) = ..." stays a definition
                std::string lhs = e.substr(0, eqp);
                lhs.erase(std::remove(lhs.begin(), lhs.end(), ' '), lhs.end());
                if (lhs == "y") e = e.substr(eqp + 1);
            }
            FuncDef d;
            std::snprintf(d.input, sizeof d.input, "%s", e.c_str());
            d.color = nextColor();
            d.id = nextRowId_++;
            funcs_.push_back(d);
            rebuildProject(true);
        }
        ImGui::SetItemTooltip("Add this as a new row in Functions, so the graph panes can plot it");
        if (ImGui::Button("Copy text", ImVec2(170, 0))) ImGui::SetClipboardText(inkResult_.expr.c_str());
        ImGui::EndDisabled();
        ImGui::PopFont();
    }
    ImGui::End();
}

// ------------------------------------------------------------------ testing: synthetic handwriting

void App::inkSynthesize(const std::string& text) {
    // Lays out text in a handwriting font the recognizer has no templates from. Each glyph's
    // skeleton becomes strokes (split where the skeleton breaks). "^c" raises and shrinks c;
    // "[a|b]" writes a over b with a fraction bar; "#" draws a radical over the rest of the line.
    LoadedFont lf;
    if (!loadFont(kTestFont, lf)) return;
    float x = 70, base = 190;
    const float px = 64;
    auto glyph = [&](uint32_t cp, float gx, float gbase, float size) -> float {
        float adv = size * 0.5f;
        auto pix = glyphSkeleton(lf.info, (int)cp, size, nullptr, &adv);
        const size_t first = inkStrokes_.size();
        std::vector<ImVec2> rest = pix;
        while (!rest.empty()) {  // chain neighbouring pixels into strokes
            std::vector<ImVec2> st;
            size_t cur = 0;
            for (size_t i = 1; i < rest.size(); ++i) if (rest[i].y + rest[i].x * 0.3f < rest[cur].y + rest[cur].x * 0.3f) cur = i;
            ImVec2 p = rest[cur];
            rest.erase(rest.begin() + cur);
            st.push_back(ImVec2(gx + p.x, gbase + p.y));
            for (;;) {
                float bd = 1e30f;
                size_t bi = 0;
                for (size_t i = 0; i < rest.size(); ++i) {
                    const float d = (rest[i].x - p.x) * (rest[i].x - p.x) + (rest[i].y - p.y) * (rest[i].y - p.y);
                    if (d < bd) { bd = d; bi = i; }
                }
                if (rest.empty() || bd > 30.f) break;
                p = rest[bi];
                rest.erase(rest.begin() + bi);
                st.push_back(ImVec2(gx + p.x, gbase + p.y));
            }
            if (st.size() >= 3) inkStrokes_.push_back(st);
        }
        // join strokes of this glyph whose ends nearly touch (a pen would draw them in one go)
        for (bool joined = true; joined;) {
            joined = false;
            for (size_t a = first; a < inkStrokes_.size() && !joined; ++a)
                for (size_t b = a + 1; b < inkStrokes_.size() && !joined; ++b) {
                    auto& A = inkStrokes_[a];
                    auto& B = inkStrokes_[b];
                    auto d = [](ImVec2 p, ImVec2 q) { return std::hypot(p.x - q.x, p.y - q.y); };
                    const float lim = size * 0.12f;
                    if (d(A.back(), B.front()) < lim) { A.insert(A.end(), B.begin(), B.end()); }
                    else if (d(A.back(), B.back()) < lim) { A.insert(A.end(), B.rbegin(), B.rend()); }
                    else if (d(A.front(), B.back()) < lim) { B.insert(B.end(), A.begin(), A.end()); A = B; }
                    else if (d(A.front(), B.front()) < lim) { std::reverse(A.begin(), A.end()); A.insert(A.end(), B.begin(), B.end()); }
                    else continue;
                    inkStrokes_.erase(inkStrokes_.begin() + b);
                    joined = true;
                }
        }
        return adv;
    };
    auto line = [&](const std::string& t, float& cx, float cbase, float size) {
        for (size_t i = 0; i < t.size(); ++i) {
            const char c = t[i];
            if (c == ' ') { cx += size * 0.35f; continue; }
            if (c == '^' && i + 1 < t.size()) { cx += glyph((unsigned char)t[++i], cx, cbase - size * 0.45f, size * 0.62f) + 2; continue; }
            cx += glyph((unsigned char)c, cx, cbase, size) + 3;
        }
    };
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '[') {  // fraction
            const size_t bar = text.find('|', i), end = text.find(']', i);
            if (bar == std::string::npos || end == std::string::npos) break;
            const std::string a = text.substr(i + 1, bar - i - 1), b = text.substr(bar + 1, end - bar - 1);
            const float wa = a.size() * px * 0.5f, wb = b.size() * px * 0.5f;
            const float w = std::max(wa, wb) + 20;
            float ax = x + (w - wa) * 0.5f, bx = x + (w - wb) * 0.5f;
            line(a, ax, base - px * 0.62f, px * 0.85f);
            line(b, bx, base + px * 0.62f, px * 0.85f);
            std::vector<ImVec2> barStroke;
            for (int k = 0; k <= 20; ++k) barStroke.push_back(ImVec2(x + w * k / 20.f, base - px * 0.32f + (k % 3) * 0.6f));
            inkStrokes_.push_back(barStroke);
            x += w + 14;
            i = end;
            continue;
        }
        if (c == '#') {  // radical over the rest
            const float x0 = x;
            std::string rest = text.substr(i + 1);
            float rx = x + 34;
            const size_t before = inkStrokes_.size();
            line(rest, rx, base, px);
            std::vector<ImVec2> r = {{x0, base - px * 0.3f}, {x0 + 8, base - px * 0.36f}, {x0 + 18, base + 4}, {x0 + 30, base - px * 0.95f}, {rx + 6, base - px * 0.95f}};
            std::vector<ImVec2> dense;
            for (size_t k = 0; k + 1 < r.size(); ++k)
                for (int q = 0; q < 8; ++q) dense.push_back(ImVec2(r[k].x + (r[k + 1].x - r[k].x) * q / 8.f, r[k].y + (r[k + 1].y - r[k].y) * q / 8.f));
            dense.push_back(r.back());
            inkStrokes_.insert(inkStrokes_.begin() + before, dense);  // written first, as people do
            x = rx;
            break;
        }
        std::string one(1, c);
        if (c == '^' && i + 1 < text.size()) { one = text.substr(i, 2); ++i; }
        line(one, x, base, px);
    }
    inkHash_ = 0;
}
