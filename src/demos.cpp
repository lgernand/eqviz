// Demo projects for the Millennium Prize Problems that are built from the generic
// demo builder (Riemann, Poincare and Navier-Stokes live in project.cpp).
#include <algorithm>
#include <cstring>

#include "app.hpp"
#include "theme.hpp"

using namespace theme;

void App::loadDemo(const char* name, const char* note, const std::vector<DemoRow>& rows,
                   const std::vector<DemoParam>& params, const std::vector<DemoPane>& panes, int layoutKind, int focus) {
    if (threeD_ && !dimAnimating_) startDim(false);
    setPanes((int)panes.size(), layoutKind);
    resetForProject();
    extendWorkspace_ = false;
    for (const DemoRow& r : rows) {
        FuncDef d;
        std::strncpy(d.input, r.eq, sizeof d.input - 1);
        std::strncpy(d.label, r.label, sizeof d.label - 1);
        d.color = r.color;
        d.id = nextRowId_++;
        funcs_.push_back(d);
    }
    std::strncpy(projName_, name, sizeof projName_ - 1);
    projNote_ = note;
    params_.clear();
    for (const DemoParam& p : params) {
        Param q;
        q.name = p.name; q.value = p.value; q.min = p.lo; q.max = p.hi; q.playing = p.play;
        params_.push_back(q);
    }
    for (int i = 0; i < (int)panes.size(); ++i) {
        const DemoPane& p = panes[i];
        enterPane(i);
        active_ = p.active;
        if (threeD_) startDim(false);
        complex_ = p.complex;
        fade_ = 1;
        fadeStage_ = 0;
        want3D_ = p.threeD;
        fitPending_ = true;
        fitRange_ = p.fit;
        colorMode_ = p.colorMode;
        surfRes_ = p.res;
        if (p.zScale > 0) { lockZScale_ = true; zScale_ = p.zScale; }
        fitZEveryFrame_ = p.fitZ;
        if (p.fitZ) lockZScale_ = true;
        hiddenRows_.clear();
        if (!p.show.empty())
            for (int r = 0; r < (int)funcs_.size(); ++r)
                if (std::find(p.show.begin(), p.show.end(), r) == p.show.end()) hiddenRows_.insert(funcs_[r].id);
        if (complex_ && !p.threeD) { gridT_ = 0; gridAnimP_ = 0; gridAnim_ = true; }
    }
    focus_ = std::clamp(focus, 0, (int)panes.size() - 1);
    enterPane(focus_);
    rebuildProject(true);
}

// ------------------------------------------------------------------ P vs NP

void App::loadDemoPvsNP() {
    loadDemo("P vs NP",
             "P vs NP: if a solution can be checked quickly, can it also be found quickly? \"Quickly\" means "
             "in polynomial time n^k. The best known algorithms for NP-complete problems (satisfiability, the "
             "travelling salesman, ...) take exponential time, like trying all 2^n possibilities. Drag k: however "
             "large it is, n^k is eventually overtaken by 2^n. On the log scale (right) polynomials grow like "
             "k ln n while brute force grows in a straight line.",
             {
                 {"poly(n) = n^k", GREEN_C, "Polynomial time n^k (in P)"},                 // 0
                 {"brute(n) = 2^n", RED_C, "Brute-force search 2^n"},                      // 1
                 {"perm(n) = gamma(n + 1)", GOLD_C, "Trying every ordering n!"},           // 2
                 {"lpoly(n) = k ln(n)", GREEN_C, "log of n^k"},                            // 3
                 {"lbrute(n) = n ln(2)", RED_C, "log of 2^n"},                             // 4
                 {"lperm(n) = ln(gamma(n + 1))", GOLD_C, "log of n!"},                     // 5
             },
             {{"k", 3, 1, 8}},
             {
                 {0, false, false, ImVec4(-0.5f, 20.f, -600.f, 10000.f), {0, 1, 2}},
                 {4, false, false, ImVec4(-1.f, 60.f, -3.f, 60.f), {3, 4, 5}},
             },
             1, 0);
    addContextNote(demo_notes::kPvsNP, 0.72f);
}

// ------------------------------------------------------------------ Hodge

void App::loadDemoHodge() {
    loadDemo("Hodge conjecture",
             "The Hodge conjecture: on a smooth complex projective variety, every Hodge class (a special kind of "
             "cohomology class) is a rational combination of classes of algebraic subvarieties. Loosely, the "
             "shapes that calculus detects should all come from shapes cut out by polynomial equations. Shown: "
             "the real points of the Fermat cubic surface x\xC2\xB3 + y\xC2\xB3 + z\xC2\xB3 = 1, and the curve "
             "w\xC2\xB2 = z\xC2\xB3 \xE2\x88\x92 z, both as one sheet of its complex Riemann surface (a torus) "
             "and through its real points. These are illustrations: the conjecture lives in the topology of "
             "complex varieties, which can't be drawn directly.",
             {
                 {"F(x, y) = (1 - x^3 - y^3)^(1/3)", BLUE_C, "Fermat cubic surface x\xC2\xB3 + y\xC2\xB3 + z\xC2\xB3 = 1"},
                 {"w(z) = sqrt(z^3 - z)", WHITE, "|w| on w\xC2\xB2 = z\xC2\xB3 \xE2\x88\x92 z"},
                 {"up(x) = sqrt(x^3 - x)", YELLOW_C, "Real points of y\xC2\xB2 = x\xC2\xB3 \xE2\x88\x92 x"},
                 {"dn(x) = -up(x)", YELLOW_C, "(lower half)"},
             },
             {},
             {
                 {0, true, false, ImVec4(-3.f, 3.f, -3.f, 3.f), {}, 1, 1.f, 70},
                 {1, true, true, ImVec4(-3.f, 3.f, -3.f, 3.f), {}, 0, 0, 70},
                 {2, false, false, ImVec4(-1.7f, 2.6f, -2.2f, 2.2f), {2, 3}},
             },
             1, 0);
    addContextNote(demo_notes::kHodge, 0.72f);
}

// ------------------------------------------------------------------ Yang-Mills

void App::loadDemoYangMills() {
    loadDemo("Yang-Mills mass gap",
             "Yang\xE2\x80\x93Mills existence and mass gap: prove that quantum Yang\xE2\x80\x93Mills theory (the "
             "framework of the strong and weak nuclear forces) exists rigorously in four dimensions and has a mass "
             "gap \xCE\x94 > 0: the lightest particle is massive even though the classical field has massless waves. "
             "A mass gap is why the strong force is short-ranged and quarks are confined. Shown: a long-range 1/r "
             "force against a short-range Yukawa force, the energy gap in E = \xE2\x88\x9A(p\xC2\xB2 + m\xC2\xB2), "
             "the linearly rising potential that confines quarks, and the action density of an instanton "
             "(its peak 6/\xCF\x81\xE2\x81\xB4 grows 256-fold as the size \xCF\x81 shrinks, so that pane rescales "
             "its height continuously; hover for the true values).",
             {
                 {"C(r) = -1/r", GREY, "Massless force (Coulomb): reaches far"},                              // 0
                 {"Y(r) = -e^(-m r)/r", BLUE_C, "With a mass gap m (Yukawa): dies off like e^(-mr)"},          // 1
                 {"Em(p) = sqrt(p^2 + m^2)", YELLOW_C, "Massive: E = \xE2\x88\x9A(p\xC2\xB2 + m\xC2\xB2), gap m at p = 0"},  // 2
                 {"E0(p) = abs(p)", GREY, "Massless: E = |p|"},                                               // 3
                 {"V(r) = -4 alpha/(3 r) + sigma r", RED_C, "Quark\xE2\x80\x93" "antiquark potential: confinement"},  // 4
                 {"I(x, y) = 6 rho^4 / (x^2 + y^2 + rho^2)^4", PURPLE_C, "Instanton action density (BPST)"},   // 5
             },
             {{"m", 1, 0, 3}, {"alpha", 0.3f, 0, 1}, {"sigma", 0.2f, 0, 1}, {"rho", 1, 0.4f, 1.6f, true}},
             {
                 {1, false, false, ImVec4(-0.2f, 5.f, -3.f, 0.6f), {0, 1}},
                 {2, false, false, ImVec4(-3.f, 3.f, -0.5f, 3.5f), {2, 3}},
                 {4, false, false, ImVec4(-0.2f, 6.f, -2.f, 2.f), {4}},
                 {5, true, false, ImVec4(-3.f, 3.f, -3.f, 3.f), {}, 1, 0.f, 140, true},
             },
             1, 0);
    addContextNote(demo_notes::kYangMills, 0.72f);
}

// ------------------------------------------------------------------ Birch and Swinnerton-Dyer

void App::loadDemoBSD() {
    loadDemo("Birch and Swinnerton-Dyer",
             "Birch and Swinnerton-Dyer: the rank of an elliptic curve (how many independent rational points "
             "generate all of them) equals the order of vanishing of its L-function at s = 1. It began with their "
             "computer experiments: the product of N_p/p over primes p \xE2\x89\xA4 x, where N_p "
             "counts the curve's points mod p, grows like C (log x)^rank. Right: log \xE2\x88\x8F N_p/p "
             "against u = ln ln x for curves of rank 0, 1, 2 and 3 (grey guides have slopes 1, 2, 3). Left: drag "
             "a and b to explore the curve; it starts as the rank-3 curve.",
             {
                 {"E(x) = sqrt(x^3 + a x + b)", YELLOW_C, "Elliptic curve y\xC2\xB2 = x\xC2\xB3 + ax + b"},              // 0
                 {"Eb(x) = -E(x)", YELLOW_C, "(lower half)"},                                                          // 1
                 {"Np(p, A, B) = p + sum(x, 0, p - 1, legendre(4(x^3 + A x + B), p))", GREY, "points mod p"},          // 2
                 {"r0(u) = sum(p, 2, e^(e^u), isprime(p) ln(Np(p, -1, 0)/p))", BLUE_C, "y\xC2\xB2 = x\xC2\xB3 \xE2\x88\x92 x  (rank 0)"},
                 {"r1(u) = sum(p, 2, e^(e^u), isprime(p) ln(Np(p, -1, 0.25)/p))", GREEN_C, "y\xC2\xB2 = x\xC2\xB3 \xE2\x88\x92 x + 1/4  (rank 1)"},
                 {"r2(u) = sum(p, 2, e^(e^u), isprime(p) ln(Np(p, 0, 17)/p))", GOLD_C, "y\xC2\xB2 = x\xC2\xB3 + 17  (rank 2)"},
                 {"r3(u) = sum(p, 2, e^(e^u), isprime(p) ln(Np(p, -7, 6.25)/p))", RED_C, "y\xC2\xB2 = x\xC2\xB3 \xE2\x88\x92 7x + 25/4  (rank 3)"},
                 {"g1(u) = u", GREY, "slope 1"},                                                                       // 7
                 {"g2(u) = 2u", GREY, "slope 2"},                                                                      // 8
                 {"g3(u) = 3u", GREY, "slope 3"},                                                                      // 9
             },
             {{"a", -7, -10, 10}, {"b", 6.25f, -10, 20}},
             {
                 {0, false, false, ImVec4(-3.8f, 4.5f, -7.f, 7.f), {0, 1}},
                 {6, false, false, ImVec4(-0.45f, 1.85f, -2.f, 6.f), {3, 4, 5, 6, 7, 8, 9}},
             },
             1, 1);
    addContextNote(demo_notes::kBSD, 0.72f);
}
