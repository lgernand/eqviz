// Symbol glossary for lessons: each step's text ends with definitions of the symbols that first
// appear in that step (symbols already met in an earlier step of the same lesson are not repeated).
#include <cctype>
#include <set>
#include <string>
#include <vector>

#include "app.hpp"

namespace {

struct Sym {
    const char* key;                  // stable name, used by the per-lesson meanings below
    std::vector<const char*> glyphs;  // found anywhere in the text
    std::vector<const char*> words;   // found as whole names inside $…$ maths
    const char* show;                 // how the glossary writes it
    const char* def;                  // the usual meaning
};

const std::vector<Sym>& symbols() {
    static const std::vector<Sym> s = {
        // Greek letters
        {"epsilon", {"ε"}, {"epsilon"}, "ε (epsilon)", "a small positive number, usually a tolerance: how close an output must be."},
        {"delta", {"δ"}, {"delta"}, "δ (delta)", "a small positive number, usually how close an input must be."},
        {"theta", {"θ"}, {"theta"}, "θ (theta)", "an angle, measured in radians unless marked with °."},
        {"pi", {"π"}, {"pi"}, "π (pi)", "the ratio of a circle's circumference to its diameter, about 3.14159. π radians is a half turn (180°)."},
        {"lambda", {"λ"}, {"lambda"}, "λ (lambda)", "an eigenvalue: the factor by which a special direction (an eigenvector) is stretched."},
        {"alpha", {"α"}, {"alpha"}, "α (alpha)", "a constant or angle named for this example."},
        {"rho", {"ρ"}, {"rho"}, "ρ (rho)", "a radius or a density, named for this example."},
        {"phi", {"φ"}, {"phi"}, "φ (phi)", "an angle, named for this example."},
        {"omega", {"ω"}, {"omega"}, "ω (omega)", "an angular frequency: radians per unit time."},
        {"Omega", {"Ω"}, {"Omega"}, "Ω (capital omega)", "a quantity named for this example."},
        {"sigma", {"σ"}, {"sigma"}, "σ (sigma)", "a constant named for this example."},
        {"kappa", {"κ"}, {"kappa"}, "κ (kappa)", "curvature: how fast a curve turns per unit length; 1/R for a circle of radius R."},
        {"nu", {"ν"}, {"nu"}, "ν (nu)", "viscosity: how strongly a fluid resists shearing."},
        {"zeta", {"ζ"}, {"zeta"}, "ζ (zeta)", "the Riemann zeta function, ζ(s) = 1 + 1/2ˢ + 1/3ˢ + ⋯, continued to the whole complex plane."},
        {"gamma", {"γ"}, {"gamma"}, "γ (gamma)", "a constant named for this example."},
        {"Gamma", {"Γ"}, {"Gamma"}, "Γ (capital gamma)", "a constant named for this example."},
        {"Delta", {"Δ"}, {"Delta"}, "Δ (capital delta)", "\"change in\": Δx is a step in x."},
        {"Sigma", {"Σ"}, {"sum"}, "Σ (capital sigma)", "a sum: Σ aₖ adds up the terms aₖ over the stated range of k."},
        // calculus
        {"integral", {"∫"}, {}, "∫", "an integral: the total of f(x) dx, the signed area under f, or an antiderivative when no limits are given."},
        {"partial", {"∂"}, {"partial"}, "∂", "a partial derivative: ∂f/∂x is the rate of change of f in x with the other variables held fixed."},
        {"nabla", {"∇"}, {"nabla"}, "∇ (nabla, del)", "the gradient ∇f = (∂f/∂x, ∂f/∂y, …), pointing uphill. ∇·F is divergence, ∇×F is curl, ∇² is the Laplacian."},
        {"infinity", {"∞"}, {"inf"}, "∞", "infinity: not a number, but \"without bound\". x → ∞ means x grows past every number."},
        {"prime", {"′", "″", "‴"}, {}, "′  ″  ‴ (primes)", "derivatives: f′ is the first derivative, f″ the second, f‴ the third."},
        {"arrow", {"→"}, {}, "→", "\"approaches\" (x → a: x gets arbitrarily close to a), or \"maps to\"."},
        {"abs", {}, {"|"}, "|x|", "absolute value: the distance from x to 0. |x − a| is the distance between x and a."},
        {"sqrt", {"√"}, {"sqrt"}, "√", "square root: the non-negative number whose square is the input."},
        {"cbrt", {"∛"}, {}, "∛", "cube root: the number whose cube is the input."},
        {"inverse", {"⁻¹"}, {}, "⁻¹", "an inverse: f⁻¹ undoes f (it is not 1/f); M⁻¹ undoes the matrix M."},
        {"compose", {"∘"}, {}, "∘", "composition: (f ∘ g)(x) = f(g(x)), apply g first."},
        {"factorial", {}, {"!"}, "n!", "factorial: 1 · 2 · 3 · ⋯ · n."},
        // relations
        {"le", {"≤", "≥"}, {}, "≤  ≥", "less than or equal to, greater than or equal to."},
        {"ne", {"≠"}, {}, "≠", "not equal to."},
        {"approx", {"≈"}, {"approx"}, "≈", "approximately equal to."},
        {"pm", {"±"}, {}, "±", "plus or minus: a ± b means both a + b and a − b (or the band between them)."},
        {"degree", {"°"}, {}, "°", "degrees: a full turn is 360° = 2π radians."},
        // vectors and matrices
        {"cross", {"×"}, {"times"}, "×", "the cross product of vectors (a vector perpendicular to both), or plain multiplication between numbers."},
        {"dot", {"·"}, {}, "·", "multiplication; between vectors, the dot product a·b = a₁b₁ + a₂b₂ + ⋯."},
        {"angle", {"⟨"}, {}, "⟨a, b⟩", "a vector written by its components."},
        {"hat", {"î", "ĵ"}, {}, "î, ĵ", "the unit vectors along the x and y axes."},
        {"det", {}, {"det"}, "det", "the determinant of a matrix: the factor by which it scales areas (negative if it flips orientation)."},
        // functions
        {"sin", {}, {"sin", "cos"}, "sin, cos", "sine and cosine: for angle θ, the point on the unit circle is (cos θ, sin θ)."},
        {"tan", {}, {"tan"}, "tan", "tangent: tan θ = sin θ / cos θ, the slope of the line at angle θ."},
        {"sec", {}, {"sec", "csc", "cot"}, "sec, csc, cot", "1/cos, 1/sin and 1/tan."},
        {"atan", {}, {"atan"}, "atan", "arctangent: the angle (between −π/2 and π/2) whose tangent is the input."},
        {"sinh", {}, {"sinh", "cosh"}, "sinh, cosh", "hyperbolic sine and cosine: (eˣ − e⁻ˣ)/2 and (eˣ + e⁻ˣ)/2."},
        {"e", {}, {"e", "exp"}, "e", "Euler's number, about 2.71828: the base whose exponential eˣ is its own derivative."},
        {"ln", {}, {"ln"}, "ln", "the natural logarithm, log base e: ln x is the power you raise e to to get x."},
        {"log", {}, {"log"}, "log", "a logarithm: the inverse of an exponential."},
        {"floor", {}, {"floor"}, "floor", "rounds down to the nearest integer: floor(2.7) = 2."},
        {"Re", {}, {"Re"}, "Re", "the real part of a complex number: Re(a + bi) = a."},
        {"li", {}, {"li"}, "li", "the logarithmic integral, li(x) = ∫ dt/ln t from 2 to x: about the number of primes up to x."},
    };
    return s;
}

// What a symbol means in a particular lesson, when that differs from the usual meaning.
struct Meaning {
    const char* lesson;
    const char* key;
    const char* def;
};
const std::vector<Meaning>& meanings() {
    static const std::vector<Meaning> m = {
        {"mp-riemann", "pi", "here π(x) is the prime-counting function: how many primes are at most x. (Elsewhere π is 3.14159…)"},
        {"mp-riemann", "rho", "a nontrivial zero of ζ: a complex number ρ with ζ(ρ) = 0, conjecturally always with real part 1/2."},
        {"cc-u0", "epsilon", "the output tolerance: the challenger's demand that f(x) land within ε of the limit L, i.e. |f(x) − L| < ε. It can be any positive number, however small."},
        {"cc-u0", "delta", "the input tolerance: your answer to ε, a distance such that every x within δ of a (but not a itself) meets the challenge, i.e. 0 < |x − a| < δ. Smaller ε usually forces a smaller δ."},
        {"cc-u15", "delta", "the Dirac delta: δ(t − c) is an instantaneous unit impulse at t = c, zero elsewhere with total area 1."},
        {"cc-u9", "alpha", "the launch angle above horizontal."},
        {"cc-u10", "lambda", "a Lagrange multiplier: at a constrained extreme, ∇f is λ times ∇g."},
        {"cc-u11", "rho", "in spherical coordinates, the distance from the origin."},
        {"cc-u11", "phi", "in spherical coordinates, the angle down from the positive z axis (0 to π)."},
        {"cc-u11", "theta", "the angle around the z axis, measured from the positive x axis."},
        {"cc-u12", "omega", "a differential form: the thing you integrate (a 1-form over curves, a 2-form over surfaces)."},
        {"cc-u12", "partial", "a partial derivative; in ∫_∂M, ∂M means the boundary of the region M."},
        {"cc-u14", "omega", "the forcing frequency, in radians per unit time; resonance happens when it matches the natural frequency."},
        {"cc-u17", "sigma", "a parameter of the Lorenz system (the Prandtl number), usually 10."},
        {"mp-navier-stokes", "omega", "vorticity: how fast the fluid spins at each point."},
        {"mp-navier-stokes", "Omega", "vorticity of the Lamb–Oseen vortex: how fast the fluid spins at each point."},
        {"mp-navier-stokes", "Gamma", "circulation: the total spin of the vortex, which viscosity spreads out but does not change."},
        {"mp-yang-mills", "alpha", "the strength of the short-range attraction between quarks."},
        {"mp-yang-mills", "sigma", "the string tension: the energy per unit length of the flux tube between quarks."},
        {"mp-yang-mills", "rho", "the size of the instanton."},
        {"mp-yang-mills", "Delta", "the mass gap: the least energy any particle of the theory can have."},
        {"mp-poincare", "rho", "the radius of the shrinking sphere."},
    };
    return m;
}

// The whole names inside $…$ maths (letters runs, plus | and !), and the text outside it.
void scan(const std::string& t, std::set<std::string>& words, std::string& plain) {
    bool math = false;
    std::string cur;
    auto flush = [&] {
        if (!cur.empty()) words.insert(cur);
        cur.clear();
    };
    for (size_t i = 0; i < t.size(); ++i) {
        const char c = t[i];
        if (c == '$') {
            flush();
            math = !math;
            if (i + 1 < t.size() && t[i + 1] == '$') ++i;
            continue;
        }
        if (!math) { plain += c; continue; }
        plain += c;  // glyphs inside maths count too
        if (std::isalpha((unsigned char)c)) { cur += c; continue; }
        if (c == '\\') { flush(); continue; }  // \nabla, \det: the name follows
        flush();
        if (c == '|' || c == '!') words.insert(std::string(1, c));
    }
    flush();
}

bool present(const Sym& s, const std::set<std::string>& words, const std::string& plain) {
    for (const char* g : s.glyphs)
        if (plain.find(g) != std::string::npos) return true;
    for (const char* w : s.words)
        if (words.count(w)) return true;
    return false;
}

}  // namespace

std::string lessonSymbols(const LessonDef& L, int step) {
    if (step < 0 || step >= (int)L.steps.size()) return "";
    std::set<std::string> seen;  // keys met in earlier steps
    std::vector<const Sym*> fresh;
    for (int i = 0; i <= step; ++i) {
        std::set<std::string> words;
        std::string plain;
        scan(L.steps[i].text, words, plain);
        for (const Sym& s : symbols()) {
            if (!present(s, words, plain) || seen.count(s.key)) continue;
            seen.insert(s.key);
            if (i == step) fresh.push_back(&s);
        }
    }
    if (fresh.empty()) return "";
    std::string out = "## Symbols\n\n";
    std::set<std::string> shown;  // entries shared by two keys (sin, cos) appear once
    for (const Sym* s : fresh) {
        const char* def = s->def;
        for (const Meaning& m : meanings())
            if (L.id == m.lesson && std::string(s->key) == m.key) def = m.def;
        if (!shown.insert(std::string(s->show) + def).second) continue;
        out += "- **" + std::string(s->show) + "**: " + def + "\n";
    }
    return out;
}
