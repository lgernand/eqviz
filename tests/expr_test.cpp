// Small self-check for the expression engine: parsing, evaluation, derivatives.
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <string>

#include "../src/expr.hpp"

using eq::cplx;

static int failures = 0;

static cplx evalAt(const eq::NodeP& n, std::map<std::string, cplx> vars, bool real = true, double lim = INFINITY) {
    eq::Program p(n);
    std::vector<cplx> slots(p.numSlots());
    for (int i = 0; i < p.numSlots(); ++i) slots[i] = vars.count(p.slotNames()[i]) ? vars[p.slotNames()[i]] : cplx(0);
    return p.eval(slots.data(), lim, real, 4096, 20);
}

static void check(const char* src, std::map<std::string, cplx> vars, cplx want, bool deriv = false, bool real = true,
                  double lim = INFINITY, double tol = 1e-6) {
    auto r = eq::parse(src);
    if (!r.root) {
        std::printf("FAIL  %-40s parse error: %s\n", src, r.error.c_str());
        ++failures;
        return;
    }
    eq::NodeP n = eq::markAnimatedSums(r.root);
    if (deriv) n = eq::derivative(n, "x");
    cplx got = evalAt(n, vars, real, lim);
    bool ok = std::isnan(want.real()) ? std::isnan(got.real()) || std::isnan(got.imag())
                                      : std::abs(got - want) < tol * std::max(1.0, std::abs(want));
    if (!ok) ++failures;
    std::printf("%s  %s%-38s = (%.6g, %.6g)   want (%.6g, %.6g)   %s\n", ok ? "ok  " : "FAIL", deriv ? "d/dx " : "",
                src, got.real(), got.imag(), want.real(), want.imag(), deriv ? eq::toString(n).c_str() : "");
}

static void checkError(const char* src) {
    auto r = eq::parse(src);
    bool ok = !r.root && !r.error.empty();
    if (!ok) ++failures;
    std::printf("%s  error for %-30s -> %s\n", ok ? "ok  " : "FAIL", src, r.error.c_str());
}

// Build a table from "name(params) = body" lines and evaluate `fname` at x.
static cplx evalProject(const std::vector<std::string>& lines, const std::string& fname,
                        std::map<std::string, cplx> vars, std::string* errOut = nullptr) {
    std::set<std::string> names;
    std::vector<eq::DefHeader> hs;
    for (auto& l : lines) { hs.push_back(eq::parseHeader(l)); names.insert(hs.back().name); }
    eq::FnTable t;
    for (auto& h : hs) {
        auto r = eq::parse(h.body, &names);
        if (!r.root) { if (errOut) *errOut = r.error; return NAN; }
        t.defs[h.name] = {h.params, r.root};
    }
    std::string err;
    std::map<std::string, eq::NodeP> rep;
    rep[t.defs[fname].params[0]] = eq::var("x");
    eq::NodeP n = eq::expandCalls(eq::substitute(t.defs[fname].body, rep), t, err);
    if (!n) { if (errOut) *errOut = err; return NAN; }
    return evalAt(n, vars, false);
}

static void checkV(const char* label, cplx got, cplx want, double tol = 1e-6) {
    bool ok = std::isnan(want.real()) ? std::isnan(got.real()) : std::abs(got - want) < tol * std::max(1.0, std::abs(want));
    failures += !ok;
    std::printf("%s  %-44s = (%.8g, %.3g)   want (%.8g, %.3g)\n", ok ? "ok  " : "FAIL", label, got.real(), got.imag(),
                want.real(), want.imag());
}

int main() {
    const double PI = 3.14159265358979323846, E = std::exp(1.0);
    check("2x^2 + 3x - 1", {{"x", 2.0}}, 13);
    check("sin x", {{"x", PI / 2}}, 1);
    check("sin^2 x + cos^2 x", {{"x", 0.7}}, 1);
    check("sin 2x", {{"x", 0.3}}, std::sin(0.6));
    check("2pi", {}, 2 * PI);
    check("ax + b", {{"a", 3.0}, {"b", 1.0}, {"x", 2.0}}, 7);
    check("a_1 x", {{"a_1", 3.0}, {"x", 2.0}}, 6);
    check("y = 2x", {{"x", 4.0}}, 8);
    check("f(x) = x^2 - 1", {{"x", 3.0}}, 8);
    check("-x^2", {{"x", 3.0}}, -9);
    check("2^-1", {}, 0.5);
    check("2^3^2", {}, 512);
    check("(x+1)(x-1)", {{"x", 3.0}}, 8);
    check("|x - 3|", {{"x", 1.0}}, 2);
    check("5!", {}, 120);
    check("sum(k, 1, 10, k)", {}, 55);
    check("sum(k=1, 4, 1/k)", {}, 1 + 0.5 + 1.0 / 3 + 0.25);
    check("prod(k, 1, 5, k)", {}, 120);
    check("sum(k, 1, 10, k)", {}, 1 + 2 + 3 + 0.5 * 4, false, true, 3.5);  // partial: 3 terms + half of the 4th
    check("sum(i, 1, 3, i^2)", {}, 14);                                    // i as index, not sqrt(-1)
    check("e^(i pi)", {}, -1, false, false);
    check("x^(1/3)", {{"x", -8.0}}, -2);
    check("sqrt(-4)", {}, cplx(0, 2), false, false);
    check("gamma(5)", {}, 24);
    check("gamma(0.5)", {}, std::sqrt(PI));
    check("ln(e^2)", {}, 2);
    check("log(1000)", {}, 3);
    check("z^2", {{"z", cplx(1, 1)}}, cplx(0, 2), false, false);
    check("sum(n, 1, N, 1/n^z)", {{"N", 50.0}, {"z", 2.0}}, 1.6251327336);
    check("x^3", {{"x", 2.0}}, 12, true);
    check("sin(2x)", {{"x", 0.4}}, 2 * std::cos(0.8), true);
    check("e^(x^2)", {{"x", 0.5}}, 2 * 0.5 * std::exp(0.25), true);
    check("x^x", {{"x", 2.0}}, 4 * (std::log(2.0) + 1), true);
    check("ln(x)/x", {{"x", 2.0}}, (1 - std::log(2.0)) / 4, true);
    check("sum(k, 0, 12, x^k / k!)", {{"x", 1.0}}, E, true);
    check("tan x", {{"x", 0.3}}, 1 / std::pow(std::cos(0.3), 2), true);
    check("sqrt(x^2 + 1)", {{"x", 1.0}}, 1 / std::sqrt(2.0), true);
    check("gamma(x)", {{"x", 3.0}}, 2 * (1.5 - 0.5772156649), true);  // numeric fallback: Γ'(3) = Γ(3)ψ(3)
    // infinite series
    const cplx NaN(NAN, 0);
    check("sum(k, 1, inf, 1/k^2)", {}, PI * PI / 6, false, true, INFINITY, 1e-6);
    check("sum(k, 0, inf, x^k / k!)", {{"x", 2.0}}, E * E);
    check("sum(k, 0, inf, x^k)", {{"x", 0.5}}, 2);
    check("sum(k, 0, inf, x^k)", {{"x", -0.9}}, 1 / 1.9, false, true, INFINITY, 1e-6);
    check("sum(k, 0, inf, x^k)", {{"x", 1.5}}, NaN);                 // diverges for |x| > 1
    check("sum(k, 1, inf, 1/k)", {}, NaN);                           // harmonic series diverges
    check("sum(k, 0, inf, (-1)^k)", {}, NaN);
    check("sum(k, 0, inf, (-1)^k / (2k+1))", {}, PI / 4, false, true, INFINITY, 1e-6);
    check("sum(k, 1, inf, (-1)^(k+1) x^k / k)", {{"x", 0.5}}, std::log(1.5));
    check("prod(k, 2, inf, 1 - 1/k^2)", {}, 0.5, false, true, INFINITY, 1e-6);
    check("sum(n, -inf, inf, 1/(z - n)^2)", {{"z", 0.3}}, PI * PI / std::pow(std::sin(0.3 * PI), 2), false, false, INFINITY, 1e-5);
    check("sum(n, 1, inf, 1/n^z)", {{"z", cplx(2, 3)}}, cplx(0.798021985146275, -0.113744308052939), false, false, INFINITY, 1e-5);
    check("sum(n, 1, inf, 1/n^z)", {{"z", cplx(0.5, 3)}}, NaN, false, false);  // diverges for Re z <= 1
    check("sum(k, 1, inf, k)", {}, 6, false, true, 3);                // player showing 3 terms
    check("sum(k, 0, inf, x^k / k!)", {{"x", 1.0}}, E, true);         // derivative of e^x series
    // Riemann zeta
    check("zeta(2)", {}, PI * PI / 6);
    check("zeta(3)", {}, 1.2020569031595942);
    check("zeta(0)", {}, -0.5, false, false);
    check("zeta(-1)", {}, -1.0 / 12, false, false);
    check("zeta(-2)", {}, 0, false, false);
    check("zeta(0.5)", {}, -1.4603545088095868, false, false);
    check("zeta(2 + 3i)", {}, cplx(0.798021985146275, -0.113744308052939), false, false);
    check("zeta(1/2 + 14.134725141734693i)", {}, 0, false, false, INFINITY, 1e-8);
    check("zeta(1/2 + 21.022039638771555i)", {}, 0, false, false, INFINITY, 1e-8);
    {  // continuity across Re(s) = 0, where the method switches to the functional equation
        cplx a = evalAt(eq::parse("zeta(-0.000001 + 5i)").root, {}, false), b = evalAt(eq::parse("zeta(0.000001 + 5i)").root, {}, false);
        bool ok = std::abs(a - b) < 1e-4;
        failures += !ok;
        std::printf("%s  zeta continuous across Re(s) = 0         |diff| = %.3g\n", ok ? "ok  " : "FAIL", std::abs(a - b));
    }
    check("abs(zeta(1/2 + i x))", {{"x", 14.134725141734693}}, 0, false, true, INFINITY, 1e-8);
    // number theory, Ei / li, zeta zeros
    check("isprime(97) + isprime(91) + isprime(2)", {}, 2);
    check("sum(n, 1, x, isprime(n))", {{"x", 100.5}}, 25);
    check("mobius(30) + mobius(12) + mobius(7)", {}, -2);
    check("li(100)", {}, 30.12614158407963, false, false);
    check("Ei(1)", {}, 1.8951178163559368, false, false);
    check("Ei(-2)", {}, -0.04890051070806112, false, false);
    {   // series and continued fraction agree where they meet (|z| = 16)
        auto a = evalAt(eq::parse("Ei(15.9 + 1.7i)").root, {}, false), b = evalAt(eq::parse("Ei(16.1 + 1.7i)").root, {}, false);
        checkV("Ei smooth across |z| = 16 (ratio ~ e^0.2)", std::abs(b / a), std::exp(0.2), 0.02);
    }
    checkV("li(x^rho) + li(x^conj rho) is real", evalAt(eq::parse("Ei((1/2 + 14.1347i) ln 50) + Ei((1/2 - 14.1347i) ln 50)").root, {}, false).imag(), 0, 1e-9);
    checkV("zetazero(1)", eq::zetaZero(1), 14.134725141734693, 1e-10);
    checkV("zetazero(2)", eq::zetaZero(2), 21.022039638771555, 1e-10);
    checkV("zetazero(10)", eq::zetaZero(10), 49.773832477672302, 1e-10);
    checkV("zetazero(100)", eq::zetaZero(100), 236.52422966581620, 1e-9);
    {   // no zeros missed: count matches Riemann-von Mangoldt N(T) ~ theta(T)/pi + 1
        double T = 700, th = T / 2 * std::log(T / (2 * PI)) - T / 2 - PI / 8;
        checkV("zero count below t = 700 vs N(T)", eq::zetaZeroCount(), std::round(th / PI + 1), 2.0 / 414);
    }
    // user functions
    checkV("g(x) = f(x)^2 + f(2x)", evalProject({"f(x) = x + 1", "g(x) = f(x)^2 + f(2x)"}, "g", {{"x", 3.0}}), 16 + 7);
    checkV("two arguments h(a, b)", evalProject({"h(a, b) = a - 2b", "k(t) = h(t^2, t)"}, "k", {{"x", 3.0}}), 3);
    checkV("no index capture", evalProject({"S(u) = sum(k, 1, 3, u)", "T(x) = sum(k, 1, 2, S(k))"}, "T", {{"x", 0.0}}), 9);
    {
        std::string err;
        evalProject({"f(x) = g(x)", "g(x) = f(x) + 1"}, "f", {{"x", 1.0}}, &err);
        bool ok = err.find("Circular") != std::string::npos;
        failures += !ok;
        std::printf("%s  circular definition -> %s\n", ok ? "ok  " : "FAIL", err.c_str());
        evalProject({"f(x, y) = x", "g(x) = f(x)"}, "g", {{"x", 1.0}}, &err);
        ok = err.find("argument") != std::string::npos;
        failures += !ok;
        std::printf("%s  wrong arity -> %s\n", ok ? "ok  " : "FAIL", err.c_str());
    }
    {   // Riemann's explicit formula with 100 zeros vs the prime staircase
        std::vector<std::string> rh = {
            "L(u) = sum(n, 1, 12, mobius(n)/n Ei(u/n))", "R(x) = L(ln x)",
            "pi_N(x) = R(x) - sum(k, 1, N, 2 re(L((1/2 + i zetazero(k)) ln x)))"};
        checkV("R(100) (Riemann's R, 12 terms)", evalProject(rh, "R", {{"x", 100.0}}), 25.6616332669242, 1e-4);
        for (double x : {20.5, 30.5, 50.5}) {
            double primes = 0;
            for (int n = 2; n <= (int)x; ++n) { bool p = true; for (int d = 2; d * d <= n; ++d) p &= n % d != 0; primes += p; }
            char lbl[64];
            snprintf(lbl, sizeof lbl, "pi_N(%.1f), 100 zeros  ~ pi(x)", x);
            checkV(lbl, evalProject(rh, "pi_N", {{"x", x}, {"N", 100.0}}).real(), primes, 0.05);
        }
    }
    check("erf(0.5) + erfc(0.5)", {}, 1);
    check("erf(1)", {}, 0.8427007929497149);
    check("erfc(x)", {{"x", 0.3}}, -2 / std::sqrt(PI) * std::exp(-0.09), true);
    // large primes and prime counting
    check("isprime(1000000007) + isprime(1000000009) + isprime(1000000011)", {}, 2);
    check("isprime(2147483647)", {}, 1);
    check("isprime(9007199254740881)", {}, 1);
    check("primepi(100000)", {}, 9592);
    check("primepi(1000000)", {}, 78498);
    check("primepi(10000000)", {}, 664579);
    check("primepi(100000000)", {}, 5761455);
    check("primepi(1000000000)", {}, 50847534);
    check("sum(n, 1, x, isprime(n))", {{"x", 2000000.5}}, 148933);        // fast path, past the old cap
    check("sum(n, 1000, x, isprime(n))", {{"x", 100000.0}}, 9592 - 168);
    check("sum(n, 1, x, 1)", {{"x", 250000.0}}, 250000);                  // ordinary sums: cap now 1e7
    // two-argument built-ins
    check("mod(17, 5) + mod(-3, 5)", {}, 2 + 2);
    check("legendre(2, 7) + legendre(3, 7) + legendre(14, 7)", {}, 1 - 1 + 0);
    checkV("points on y^2 = x^3 - x mod 5", evalProject({"Np(p, A, B) = p + sum(x, 0, p - 1, legendre(4(x^3 + A x + B), p))",
                                                       "t(x) = Np(5, -1, 0)"}, "t", {{"x", 0.0}}), 7);
    checkError("mod(5)");
    checkError("sin(");
    checkError("2 + * 3");
    checkError("sum(1, 2)");
    checkError("x = y = 2");
    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
