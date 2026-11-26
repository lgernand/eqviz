// Lessons on the seven Millennium Prize Problems, each ending with a test.
// They build on the demo projects' formulas and explain the ideas step by step.
#include "app.hpp"
#include "theme.hpp"

using namespace theme;

namespace {

constexpr const char* kSubject = "Millennium Prize Problems";
using Set = std::function<void(StepCtx&)>;

void setup(App& a, const char* name, std::vector<DemoRow> rows, std::vector<DemoParam> params, int graphs, bool stacked,
           float noteFrac = 0.37f) {
    std::vector<DemoPane> panes(graphs + 1);
    for (auto& p : panes) p.show = {-1};
    a.loadDemo(name, "", rows, params, panes, 1, 1);
    a.lessonLayout(graphs, stacked, noteFrac);
}

TestQuestion typed(std::string text, std::string answer, int review, double tol = 0.01, Set set = {}) {
    TestQuestion q;
    q.text = std::move(text);
    q.answer = std::move(answer);
    q.review = review;
    q.tol = tol;
    q.set = std::move(set);
    return q;
}

TestQuestion choice(std::string text, std::vector<std::string> choices, int correct, int review, Set set = {}) {
    TestQuestion q;
    q.text = std::move(text);
    q.choices = std::move(choices);
    q.correct = correct;
    q.review = review;
    q.set = std::move(set);
    return q;
}

// a 2D graph pane showing these rows, `active` first
void graph(StepCtx& s, int pane, std::vector<int> rows, double x0, double x1, double y0, double y1) {
    KeyPane& p = s.pane(pane);
    p.complex = p.threeD = false;
    s.show(pane, rows);
    s.active(pane, rows.empty() ? 0 : rows[0]);
    s.fit(pane, x0, x1, y0, y1);
}

// a 3D surface pane
void surface(StepCtx& s, int pane, int row, double r, float zScale, bool complex = false) {
    KeyPane& p = s.pane(pane);
    p.threeD = true;
    p.complex = complex;
    s.show(pane, {row});
    s.active(pane, row);
    s.fit(pane, -r, r, -r, r);
    p.zScale = zScale;
}

// ==================================================================== Riemann Hypothesis

LessonDef riemann() {
    LessonDef L;
    L.id = "mp-riemann";
    L.subject = kSubject;
    L.title = "The Riemann Hypothesis";
    L.blurb = "How the zeros of the zeta function control the primes.";
    L.setup = [](App& a) {
        setup(a, "Lesson: the Riemann Hypothesis",
              {{"pi_0(x) = sum(n, 1, x, isprime(n))", BLUE_C, "\xCF\x80(x): primes up to x"},       // 0
               {"a(x) = x/ln(x)", GREY, "x / ln x"},                                                 // 1
               {"li(x)", GREEN_C, "li(x)"},                                                         // 2
               {"zeta(z)", WHITE, ""},                                                              // 3
               {"Z(x) = abs(zeta(1/2 + i x))", RED_C, "|\xCE\xB6(1/2 + ix)|"},                       // 4
               {"L(u) = sum(n, 1, 12, mobius(n)/n Ei(u/n))", GREY, ""},                             // 5
               {"R(x) = L(ln x)", GREEN_C, "Riemann's R(x)"},                                       // 6
               {"pi_N(x) = R(x) - sum(k, 1, N, 2 re(L((1/2 + i zetazero(k)) ln x)))", YELLOW_C, "from the first N zeros"}},  // 7
              {{"N", 1, 0, 100}}, 2, false, 0.34f);
    };
    L.steps = {
        {"Counting primes",
         "The primes $2, 3, 5, 7, 11, 13, ...$ have no obvious pattern. Count them: $pi(x)$ is the number of primes up to $x$.\n\n"
         "- middle: up close, $pi(x)$ is a jagged staircase, one step per prime\n"
         "- right: from far away it looks like a smooth curve",
         [](StepCtx& s) {
             graph(s, 1, {0}, -1, 61, -1.5, 19);
             graph(s, 2, {0}, -20, 2100, -15, 330);
         }},
        {"The prime number theorem",
         "Gauss guessed, and Hadamard and de la Vallée Poussin proved in 1896, that\n\n"
         "$$pi(x) approx x/ln(x)$$\n\n"
         "(grey). Gauss's better guess is the **logarithmic integral** $li(x)$ (green), which is very close. "
         "Near $x$, about one number in $ln(x)$ is prime.\n\n"
         "The question is how far $pi(x)$ can stray from $li(x)$.",
         [](StepCtx& s) {
             graph(s, 2, {0, 1, 2}, -20, 2100, -15, 330);
             s.ask("How many primes are there up to 100?", "25", "Hover the staircase at x = 100 in the right pane.");
         }},
        {"The zeta function",
         "Riemann studied\n\n"
         "$$zeta(s) = sum(n, 1, inf, 1/n^s)$$\n\n"
         "Euler had shown it is a product over the primes, so it knows about all of them. Riemann extended it to every complex $s$ "
         "except $s = 1$.\n\n"
         "Right: its domain colouring. A zero is a point where every colour meets. There are **trivial zeros** at "
         "$-2, -4, -6, ...$, and **nontrivial zeros** further up, all of them found so far on the line $Re(s) = 1/2$.",
         [](StepCtx& s) {
             KeyPane& p = s.pane(2);
             p.complex = true;
             p.threeD = false;
             p.domainColor = true;
             s.show(2, {3});
             s.active(2, 3);
             s.fit(2, -4, 5, -2, 33);
         }},
        {"The critical line",
         "Walk up the line $Re(s) = 1/2$ and plot $|zeta(1/2 + i x)|$ (red). It touches zero again and again: "
         "at $x = 14.13$, $21.02$, $25.01$, $30.42$, ...\n\n"
         "**The Riemann Hypothesis:** every nontrivial zero of $zeta$ has real part exactly $1/2$.\n\n"
         "More than ten trillion zeros have been checked. None is off the line, but no proof is known.",
         [](StepCtx& s) {
             graph(s, 2, {4}, -1, 51, -0.4, 3.6);
             s.pane(2).domainColor = false;
             s.ask("At what height x is the first zero on the critical line (two decimals)?", "14.13", "The first place the red curve touches 0.");
         }},
        {"Primes from zeros",
         "Riemann found an exact formula. Start from a smooth estimate $R(x)$ (green) and add one wave for each zero:\n\n"
         "$$pi(x) = R(x) - sum(k, 1, inf, 2 Re(R(x^(rho_k))))$$\n\n"
         "where $rho_k = 1/2 + i t_k$ are the zeros. Watch the yellow curve as more zeros are added: it climbs the staircase.",
         [](StepCtx& s) {
             graph(s, 1, {7, 0, 6}, 1.5, 62, -1.5, 19);
             s.param("N", 1);
         }},
        {"Hearing the primes",
         "With 60 zeros the waves rebuild every step of the staircase. The zeros are the frequencies the primes are made of.\n\n"
         "If every zero sits on the critical line, the waves stay as small as possible, and $pi(x)$ never strays from $li(x)$ by more "
         "than about $sqrt(x) ln(x)$. One zero off the line would mean the primes are less evenly spread than that.",
         [](StepCtx& s) {
             s.param("N", 60);
             s.dur = 6.f;
         }},
    };
    L.test = {
        typed("How many primes are there up to 30?", "10", 0),
        choice("The Riemann Hypothesis says the nontrivial zeros of $zeta$ all have real part", {"0", "$1/2$", "1", "$-2$"}, 1, 3),
        typed("Estimate $pi(1000)$ with $x/ln(x)$, to the nearest whole number.", "1000/ln(1000)", 1),
        choice("The trivial zeros of $zeta$ are at", {"the negative even integers", "the positive integers", "the line $Re(s) = 1/2$", "nowhere"}, 0, 2),
        choice("Adding more zeros to Riemann's formula makes the yellow curve", {"match the staircase more closely", "drift further away", "flat", "undefined"}, 0, 5),
    };
    return L;
}

// ==================================================================== P vs NP

LessonDef pvsnp() {
    LessonDef L;
    L.id = "mp-pvsnp";
    L.subject = kSubject;
    L.title = "P vs NP";
    L.blurb = "If an answer is easy to check, is it easy to find? Polynomial against exponential time.";
    L.setup = [](App& a) {
        setup(a, "Lesson: P vs NP",
              {{"poly(n) = n^k", GREEN_C, "polynomial n^k"},            // 0
               {"brute(n) = 2^n", RED_C, "brute force 2^n"},             // 1
               {"perm(n) = gamma(n + 1)", GOLD_C, "every ordering n!"},  // 2
               {"lpoly(n) = k ln(n)", GREEN_C, "ln of n^k"},             // 3
               {"lbrute(n) = n ln(2)", RED_C, "ln of 2^n"},              // 4
               {"lperm(n) = ln(gamma(n + 1))", GOLD_C, "ln of n!"}},     // 5
              {{"k", 2, 1, 8}}, 1, false);
    };
    L.steps = {
        {"Easy to check, hard to find",
         "Some puzzles are hard to solve but easy to check:\n\n"
         "- given a filled-in Sudoku, checking it takes moments; filling it in can take hours\n"
         "- given two primes, multiplying them is quick; given only their product, finding them is very hard\n\n"
         "**P vs NP** asks whether this gap is real: if a solution can be **checked** quickly, can it always be **found** quickly?",
         [](StepCtx& s) {
             graph(s, 1, {0, 1}, -0.5, 16, -300, 5000);
             s.param("k", 2);
         }},
        {"Polynomial time",
         "\"Quickly\" means **polynomial time**: for an input of size $n$ the number of steps is at most about $n^k$ for a fixed $k$. "
         "These problems form the class **P**: sorting, shortest paths, multiplication.\n\n"
         "Brute force, trying all $2^n$ possibilities (red), is **exponential**. Watch $k$ grow: even $n^8$ is caught by $2^n$.",
         [](StepCtx& s) {
             s.param("k", 8);
             s.dur = 4.f;
             graph(s, 1, {0, 1}, -0.5, 50, -1e12, 1.2e14);
         }},
        {"Exponential beats every polynomial",
         "Take logarithms (this pane plots ln of the step counts). A polynomial becomes $k ln(n)$, which flattens out; "
         "$2^n$ becomes the straight line $n ln(2)$; $n!$ (gold) is steeper still.\n\n"
         "For any $k$, the line wins in the end.",
         [](StepCtx& s) {
             graph(s, 1, {3, 4, 5}, -1, 60, -3, 60);
             s.param("k", 3);
             s.ask("How many possibilities does brute force try for n = 10 yes/no choices?", "2^10");
         }},
        {"NP and NP-complete",
         "**NP** is the class of problems whose solutions can be **checked** in polynomial time. Every problem in P is in NP.\n\n"
         "Some NP problems are **NP-complete**: Boolean satisfiability, the travelling salesman (decision version), "
         "graph colouring, generalised Sudoku. Cook and Levin showed in 1971 that a fast algorithm for any one of them "
         "would give a fast algorithm for **every** problem in NP.",
         [](StepCtx& s) {
             s.param("k", 6);
             s.dur = 2.f;
             s.ask("From which n on is 2^n bigger than n\xC2\xB2? (smallest n > 4)", "5", "Try n = 5: 32 against 25.");
         }},
        {"What a proof would mean",
         "**P = NP** would make countless hard problems easy, and break most of today's public-key cryptography.\n\n"
         "**P ≠ NP**, which most researchers expect, would prove that some problems are hard no matter how clever we are.\n\n"
         "Proving either way needs new ideas: several known proof techniques are shown not to work.",
         [](StepCtx& s) { s.param("k", 3); }},
    };
    L.test = {
        choice("A problem is in NP if", {"its solutions can be checked in polynomial time", "it cannot be solved", "it always takes exponential time", "it is about graphs"}, 0, 3),
        typed("How many subsets does a set of 20 items have?", "2^20", 2),
        choice("Which grows fastest for large n?", {"$n^100$", "$2^n$", "$n!$", "$1000 n$"}, 2, 2),
        choice("If P = NP were proved, what would be most at risk?", {"much of public-key cryptography", "the Pythagorean theorem", "arithmetic", "nothing"}, 0, 4),
        typed("$ln(2^n) = n ln(2)$. What is $ln(2^50)$, to two decimals?", "50 ln(2)", 2),
    };
    return L;
}

// ==================================================================== Navier-Stokes

LessonDef navierStokes() {
    LessonDef L;
    L.id = "mp-navier-stokes";
    L.subject = kSubject;
    L.title = "Navier\xE2\x80\x93Stokes existence and smoothness";
    L.blurb = "The equations of fluid flow, exact solutions, and the open question of blow-up.";
    L.setup = [](App& a) {
        setup(a, "Lesson: Navier-Stokes",
              {{"omega(x, y) = 2 sin(x) sin(y) e^(-2 nu t)", BLUE_C, "Taylor\xE2\x80\x93Green vorticity"},                          // 0
               {"Omega(x, y) = Gamma / (4 pi nu (t + 1)) e^(-(x^2 + y^2) / (4 nu (t + 1)))", PURPLE_C, "Lamb\xE2\x80\x93Oseen vortex"},  // 1
               {"u(y) = erfc(y / (2 sqrt(nu (t + 0.05))))", YELLOW_C, "boundary layer"},                                       // 2
               {"P(y) = G (1 - y^2) / 2", GREEN_C, "channel flow (Poiseuille)"},                                               // 3
               {"C(y) = U (1 + y) / 2", RED_C, "shear flow (Couette)"}},                                                       // 4
              {{"t", 0, 0, 4}, {"nu", 0.15f, 0.01f, 1.f}, {"Gamma", 2, 0, 6}, {"G", 1, 0, 4}, {"U", 1, -2, 2}}, 2, true);
    };
    L.steps = {
        {"The equations of fluids",
         "Water, air and blood all obey the **Navier–Stokes equations**: for the velocity $u$ and pressure $p$,\n\n"
         "**∂u/∂t + (u·∇)u = −∇p + ν ∇²u,   ∇·u = 0**\n\n"
         "- left: how the velocity changes as the fluid moves\n"
         "- $-nabla p$: pressure pushes fluid from high to low\n"
         "- $nu nabla^2 u$: **viscosity** $nu$ smooths the flow out\n"
         "- $nabla dot u = 0$: the fluid is incompressible\n\n"
         "Top: two exact solutions. Fluid between plates, pushed by pressure, flows in a parabola (green); dragged by a moving plate, in a straight line (red).",
         [](StepCtx& s) {
             graph(s, 1, {3, 4}, -1.3, 1.3, -0.25, 1.25);
             graph(s, 2, {2}, -0.15, 3.5, -0.15, 1.25);
             s.param("t", 0).param("nu", 0.15).param("G", 1).param("U", 1);
         }},
        {"Viscosity spreads motion",
         "Bottom: fluid at rest above a plate that suddenly starts moving. Viscosity drags the fluid along, and the moving layer "
         "thickens like $sqrt(nu t)$. Watch time run.",
         [](StepCtx& s) {
             s.param("t", 4);
             s.dur = 5.f;
             s.ask("In the channel flow P(y) = (1 \xE2\x88\x92 y\xC2\xB2)/2, what is the speed at the centre, y = 0?", "0.5");
         }},
        {"Vortices decay",
         "Top: the **vorticity** (local spin) of the Taylor–Green flow, a grid of counter-rotating cells. It is an exact solution:\n\n"
         "$$omega = 2 sin(x) sin(y) e^(-2 nu t)$$\n\n"
         "The pattern keeps its shape while viscosity drains its energy.",
         [](StepCtx& s) {
             surface(s, 1, 0, 6, 1.f);
             s.param("t", 0);
         }},
        {"Watch it fade",
         "As $t$ increases, every cell weakens by the same factor $e^(-2 nu t)$.",
         [](StepCtx& s) {
             s.param("t", 3);
             s.dur = 5.f;
         }},
        {"A spreading vortex",
         "Bottom: a single whirlpool, the Lamb–Oseen vortex. Viscosity spreads its spin outwards: the peak falls and the core widens, "
         "but the total spin $Gamma$ stays the same.",
         [](StepCtx& s) {
             surface(s, 2, 1, 3, 1.6f);
             s.param("t", 0);
         }},
        {"The million-dollar question",
         "These solutions stay smooth forever. The question is whether **every** flow does.\n\n"
         "In three dimensions, start with any smooth, finite-energy velocity field. Does a smooth solution exist for all time, "
         "or can the velocity become infinite at some point in finite time, a **blow-up**?\n\n"
         "In 2D the answer is known: solutions stay smooth. In 3D, vortex tubes can stretch and intensify, and nobody has proved they can't blow up.",
         [](StepCtx& s) {
             s.param("t", 4);
             s.dur = 5.f;
         }},
    };
    L.test = {
        choice("The Navier–Stokes problem asks whether 3D solutions", {"stay smooth for all time", "exist for one second", "are linear", "are periodic"}, 0, 5),
        typed("Couette flow is C(y) = U(1 + y)/2. With U = 1, what is the speed at the top plate, y = 1?", "1", 0),
        choice("Increasing the viscosity $nu$ makes the Taylor–Green vortices", {"fade faster", "spin up", "stay the same", "blow up"}, 0, 3),
        typed("The vorticity decays like $e^(-2 nu t)$. With $nu = 0.5$, what fraction is left at $t = 1$?", "exp(-1)", 3),
        choice("$nabla dot u = 0$ says the fluid is", {"incompressible", "at rest", "hot", "turbulent"}, 0, 0),
    };
    return L;
}

// ==================================================================== Yang-Mills

LessonDef yangMills() {
    LessonDef L;
    L.id = "mp-yang-mills";
    L.subject = kSubject;
    L.title = "Yang\xE2\x80\x93Mills and the mass gap";
    L.blurb = "Why the strong force is short-ranged: forces, mass, energy gaps and confinement.";
    L.setup = [](App& a) {
        setup(a, "Lesson: Yang-Mills and the mass gap",
              {{"C(r) = -1/r", GREY, "massless carrier: -1/r"},                                  // 0
               {"Y(r) = -e^(-m r)/r", BLUE_C, "massive carrier: -e^(-mr)/r"},                     // 1
               {"Em(p) = sqrt(p^2 + m^2)", YELLOW_C, "massive: E = \xE2\x88\x9A(p\xC2\xB2 + m\xC2\xB2)"},  // 2
               {"E0(p) = abs(p)", GREY, "massless: E = |p|"},                                     // 3
               {"V(r) = -4 alpha/(3 r) + sigma r", RED_C, "quark\xE2\x80\x93" "antiquark potential"},  // 4
               {"I(x, y) = 6 rho^4 / (x^2 + y^2 + rho^2)^4", PURPLE_C, "instanton"}},             // 5
              {{"m", 1, 0, 3}, {"alpha", 0.3f, 0, 1}, {"sigma", 0.2f, 0, 1}, {"rho", 1, 0.5f, 1.6f}}, 2, true);
    };
    L.steps = {
        {"Forces from fields",
         "In modern physics every force is carried by a field and its particles: light (photons) for electromagnetism, "
         "gluons for the strong force.\n\n"
         "If the carrier is **massless**, the force reaches far: a $-1/r$ potential (grey). If it has **mass** $m$, the force dies off like "
         "$e^(-m r)/r$ (blue).",
         [](StepCtx& s) {
             graph(s, 1, {1, 0}, -0.2, 5, -3, 0.6);
             graph(s, 2, {2, 3}, -3, 3, -0.5, 3.5);
             s.param("m", 0.4);
         }},
        {"Mass sets the range",
         "Increase the mass: the blue curve hugs zero sooner. The force's range is about $1/m$.",
         [](StepCtx& s) {
             s.param("m", 3);
             s.dur = 4.f;
             s.ask("A carrier with mass m = 2 gives a force with range about 1/m. What is that range?", "0.5");
         }},
        {"The energy gap",
         "Bottom: energy against momentum. A massless particle has $E = |p|$: arbitrarily little energy makes one. "
         "A massive one has $E = sqrt(p^2 + m^2)$, so the least energy anything can carry is $m$: there is a **gap**.\n\n"
         "The **mass gap** $Delta$ is this least energy for the whole theory.",
         [](StepCtx& s) { s.param("m", 1); }},
        {"Yang–Mills and the strong force",
         "Yang–Mills theory (1954) describes the strong force between quarks. Its classical waves move at light speed, like massless particles. "
         "Yet the strong force is short-ranged: the particles we see (protons, pions, glueballs) all have mass.\n\n"
         "Top: the energy between a quark and an antiquark. Beyond short distances it rises in a straight line, $sigma r$: pulling them apart "
         "costs ever more energy. That is **confinement**: quarks are never seen alone.",
         [](StepCtx& s) { graph(s, 1, {4}, -0.2, 6, -2, 2); }},
        {"Instantons",
         "The quantum theory is full of structure that has no classical counterpart. One example: **instantons**, lumps of field "
         "concentrated in spacetime. Bottom: the action density of one, $6 rho^4/(r^2 + rho^2)^4$.",
         [](StepCtx& s) {
             surface(s, 2, 5, 3, 0.35f);
             s.param("rho", 1);
         }},
        {"The problem",
         "Physicists compute with Yang–Mills theory every day, and computer simulations show a mass gap. But nobody has proved the "
         "quantum theory even **exists** as rigorous mathematics.\n\n"
         "**The problem:** for any compact simple gauge group, prove that quantum Yang–Mills theory exists in four-dimensional spacetime "
         "and has a mass gap $Delta > 0$.",
         [](StepCtx& s) { s.param("rho", 0.8); s.dur = 3.f; }},
    };
    L.test = {
        typed("With $E = sqrt(p^2 + m^2)$ and $m = 2$, what is the smallest possible energy?", "2", 2),
        choice("A massless force carrier gives a force that", {"reaches far, like $1/r$", "dies off exponentially", "is zero", "grows with distance"}, 0, 0),
        typed("What is $-e^(-m r)/r$ for $m = 1$ at $r = 1$, to two decimals?", "-exp(-1)", 0),
        choice("Quarks are never seen alone because the energy between them", {"grows with distance", "falls like $1/r$", "is zero", "oscillates"}, 0, 3),
    };
    return L;
}

// ==================================================================== Hodge

LessonDef hodge() {
    LessonDef L;
    L.id = "mp-hodge";
    L.subject = kSubject;
    L.title = "The Hodge conjecture";
    L.blurb = "Shapes cut out by polynomial equations, their holes, and which holes come from algebra.";
    L.setup = [](App& a) {
        setup(a, "Lesson: the Hodge conjecture",
              {{"cu(x) = sqrt(1 - x^2)", BLUE_C, "x\xC2\xB2 + y\xC2\xB2 = 1"},                       // 0
               {"cd(x) = -cu(x)", BLUE_C, ""},                                                     // 1
               {"up(x) = sqrt(x^3 - x)", YELLOW_C, "y\xC2\xB2 = x\xC2\xB3 \xE2\x88\x92 x"},          // 2
               {"dn(x) = -up(x)", YELLOW_C, ""},                                                   // 3
               {"F(x, y) = (1 - x^3 - y^3)^(1/3)", BLUE_C, "x\xC2\xB3 + y\xC2\xB3 + z\xC2\xB3 = 1"},  // 4
               {"w(z) = sqrt(z^3 - z)", WHITE, ""}},                                               // 5
              {}, 2, true);
    };
    L.steps = {
        {"Shapes from equations",
         "Polynomial equations cut out shapes, called **algebraic varieties**:\n\n"
         "- $x^2 + y^2 = 1$ is a circle (blue)\n"
         "- $y^2 = x^3 - x$ is an **elliptic curve** (yellow), here in two pieces\n\n"
         "Bottom: one equation in three variables, $x^3 + y^3 + z^3 = 1$, gives a surface.",
         [](StepCtx& s) {
             graph(s, 1, {0, 1, 2, 3}, -1.8, 2.6, -2.1, 2.1);
             surface(s, 2, 4, 3, 1.f);
         }},
        {"Go complex",
         "Allow $x$ and $y$ to be **complex** numbers and the picture changes completely. The curve $y^2 = x^3 - x$ becomes a surface, and "
         "topologically it is a **torus**, the surface of a doughnut. The real picture shows only a slice of it.\n\n"
         "Bottom: $|y| = |sqrt(z^3 - z)|$ over the complex plane. Its three dips to zero (at $z = -1, 0, 1$) are where the two sheets of the torus meet.",
         [](StepCtx& s) { surface(s, 2, 5, 2.2, 0.5f, true); }},
        {"Counting holes",
         "**Topology** studies shapes up to stretching. A torus has two independent loops that can't be shrunk away: one around the hole, "
         "one through it. A sphere has none.\n\n"
         "**Cohomology** makes this precise: for each dimension it counts independent \"holes\" (for a torus: 1, 2, 1 in dimensions 0, 1, 2), "
         "and measures them with integrals.",
         [](StepCtx& s) { s.ask("How many independent loops does a torus have?", "2", "One around the hole, one through it."); }},
        {"Hodge classes",
         "On a complex variety, Hodge showed that each cohomology class splits into pieces of different types $(p, q)$. "
         "The pieces of type $(p, p)$ that are also rational are called **Hodge classes**.\n\n"
         "Any algebraic subvariety, a smaller shape inside cut out by more polynomial equations, gives a Hodge class.",
         [](StepCtx& s) { graph(s, 1, {2, 3}, -1.7, 2.6, -2.2, 2.2); }},
        {"The conjecture",
         "**The Hodge conjecture:** on a smooth complex projective variety, every Hodge class is a rational combination of classes of "
         "algebraic subvarieties.\n\n"
         "In words: the holes that calculus singles out all come from geometry defined by polynomials. It is known for type $(1, 1)$ "
         "(the Lefschetz theorem, 1924) and open in general.",
         [](StepCtx& s) { surface(s, 2, 4, 3, 1.f); }},
    };
    L.test = {
        choice("An algebraic variety is", {"the solutions of polynomial equations", "any smooth surface", "a group of symmetries", "a matrix"}, 0, 0),
        typed("How many independent loops does a torus have?", "2", 2),
        choice("Over the complex numbers, $y^2 = x^3 - x$ is shaped like", {"a sphere", "a torus", "a line", "a cube"}, 1, 1),
        choice("The Hodge conjecture says Hodge classes come from", {"algebraic subvarieties", "prime numbers", "fluid flows", "random noise"}, 0, 4),
        choice("The conjecture is already known for", {"classes of type $(1, 1)$", "every case", "no case at all", "real curves only"}, 0, 4),
    };
    return L;
}

// ==================================================================== Poincare

LessonDef poincare() {
    LessonDef L;
    L.id = "mp-poincare";
    L.subject = kSubject;
    L.title = "The Poincar\xC3\xA9 conjecture (solved)";
    L.blurb = "Shrinking loops, the 3-sphere, and how Ricci flow with surgery settled the conjecture.";
    L.setup = [](App& a) {
        setup(a, "Lesson: the Poincare conjecture",
              {{"P(x) = 0.9025 - (abs(x) - 1)^2 + 0.22 e^(-6x^2)", GREY, ""},           // 0
               {"r(x) = sqrt(P(x) - 4t)", YELLOW_C, "profile at time t"},               // 1
               {"q(x) = -r(x)", YELLOW_C, ""},                                          // 2
               {"b(x) = sqrt(P(x))", GREY, "at t = 0"},                                 // 3
               {"c(x) = -b(x)", GREY, ""},                                              // 4
               {"M(x, y) = sqrt(r(x)^2 - y^2)", BLUE_C, "dumbbell"},                    // 5
               {"rho(s) = sqrt(1 - 4s)", BLUE_C, "radius"},                             // 6
               {"R(s) = 6 / rho(s)^2", RED_C, "curvature 6/r\xC2\xB2"},                 // 7
               {"V(s) = 2 pi^2 rho(s)^3", GREEN_C, "volume"},                          // 8
               {"S(x, y) = sqrt(rho(t)^2 - x^2 - y^2)", PURPLE_C, "round sphere"}},    // 9
              {{"t", 0, 0, 0.24f}}, 2, true);
    };
    L.steps = {
        {"Loops that shrink",
         "Put a rubber band on a sphere: you can always slide it off to a point. On a torus a band around the hole can't be removed.\n\n"
         "A shape where every loop shrinks to a point is **simply connected**. Among closed surfaces, only the sphere is.",
         [](StepCtx& s) {
             surface(s, 1, 9, 1.6f, 1.f);
             graph(s, 2, {1, 2}, -2.2, 2.2, -1.25, 1.25);
             s.param("t", 0);
         }},
        {"The conjecture",
         "Poincaré asked in 1904 whether the same holds one dimension up.\n\n"
         "**The Poincaré conjecture:** every closed, simply connected 3-dimensional space is the **3-sphere**, the set of points at "
         "distance 1 from the origin in four dimensions.\n\n"
         "It resisted proof for a century, while the analogues in dimensions 5 and up (Smale, 1961) and 4 (Freedman, 1982) fell first.",
         nullptr},
        {"Ricci flow",
         "Hamilton (1982) proposed letting the shape evolve by **Ricci flow**: the geometry changes so that curved regions smooth out, "
         "like heat spreading.\n\n"
         "A round 3-sphere just shrinks: its radius is $rho(s) = sqrt(1 - 4 s)$ (blue), its curvature $6/rho^2$ (red) grows and its "
         "volume (green) falls, until at $s = 1/4$ it vanishes in a point.",
         [](StepCtx& s) {
             graph(s, 2, {6, 7, 8}, -0.015, 0.265, -1, 24);
             s.param("t", 0.23);
             s.dur = 5.f;
             s.ask("The radius is \xE2\x88\x9A(1 \xE2\x88\x92 4s). At what time s does it reach 0?", "1/4");
         }},
        {"Necks pinch",
         "A lumpier shape, like a dumbbell, does not shrink evenly: the thin neck pinches off first, a **singularity**.\n\n"
         "Perelman (2002–03) showed how to continue past them: cut along the pinching necks, cap the holes with round pieces (**surgery**), "
         "and keep flowing. He proved the pieces end up as recognisable shapes.",
         [](StepCtx& s) {
             surface(s, 1, 5, 3, 1.f);
             graph(s, 2, {1, 2, 3, 4}, -2.2, 2.2, -1.25, 1.25);
             s.param("t", 0);
         }},
        {"Watch it pinch",
         "Run the flow: the neck narrows to nothing while the ends are still large.",
         [](StepCtx& s) {
             s.param("t", 0.2);
             s.dur = 5.f;
         }},
        {"Solved",
         "For a simply connected space, everything that is left after the flow and surgery is a 3-sphere: the conjecture is true.\n\n"
         "Perelman's proof was checked by several teams. He declined both the Fields Medal (2006) and the Millennium Prize (2010). "
         "It is the only one of the seven problems solved so far.",
         [](StepCtx& s) { s.param("t", 0.1); s.dur = 2.f; }},
    };
    L.test = {
        choice("Which Millennium Prize Problem has been solved?", {"the Poincaré conjecture", "the Riemann Hypothesis", "P vs NP", "the Hodge conjecture"}, 0, 5),
        typed("Under Ricci flow, a round 3-sphere has radius $sqrt(1 - 4 s)$. At what time $s$ does it shrink to a point?", "1/4", 2),
        choice("Simply connected means", {"every loop can shrink to a point", "it is in one piece", "it is flat", "it has no corners"}, 0, 0),
        typed("The curvature of a round 3-sphere of radius $r$ is $6/r^2$. What is it for $r = 2$?", "1.5", 2),
        choice("Who completed the proof?", {"Grigori Perelman", "Henri Poincaré", "Richard Hamilton", "Andrew Wiles"}, 0, 5),
    };
    return L;
}

// ==================================================================== Birch and Swinnerton-Dyer

LessonDef bsd() {
    LessonDef L;
    L.id = "mp-bsd";
    L.subject = kSubject;
    L.title = "Birch and Swinnerton-Dyer";
    L.blurb = "Rational points on elliptic curves, their rank, and counting points modulo primes.";
    L.setup = [](App& a) {
        setup(a, "Lesson: Birch and Swinnerton-Dyer",
              {{"E(x) = sqrt(x^3 + a x + b)", YELLOW_C, "y\xC2\xB2 = x\xC2\xB3 + ax + b"},                                  // 0
               {"Eb(x) = -E(x)", YELLOW_C, ""},                                                                          // 1
               {"Np(p, A, B) = p + sum(x, 0, p - 1, legendre(4(x^3 + A x + B), p))", GREY, ""},                            // 2
               {"r0(u) = sum(p, 2, e^(e^u), isprime(p) ln(Np(p, -1, 0)/p))", BLUE_C, "rank 0"},                           // 3
               {"r1(u) = sum(p, 2, e^(e^u), isprime(p) ln(Np(p, -1, 0.25)/p))", GREEN_C, "rank 1"},                       // 4
               {"r2(u) = sum(p, 2, e^(e^u), isprime(p) ln(Np(p, 0, 17)/p))", GOLD_C, "rank 2"},                           // 5
               {"r3(u) = sum(p, 2, e^(e^u), isprime(p) ln(Np(p, -7, 6.25)/p))", RED_C, "rank 3"},                         // 6
               {"g1(u) = u", GREY, "slope 1"},                                                                           // 7
               {"g2(u) = 2u", GREY, "slope 2"},                                                                          // 8
               {"g3(u) = 3u", GREY, "slope 3"},                                                                          // 9
               {"ku(x) = sqrt(x^3 - 2)", GOLD_C, "y\xC2\xB2 = x\xC2\xB3 \xE2\x88\x92 2"},                               // 10
               {"kd(x) = -ku(x)", GOLD_C, ""}},                                                                          // 11
              {{"a", -2, -10, 10}, {"b", 1, -10, 20}}, 2, true);
    };
    L.steps = {
        {"Elliptic curves",
         "An **elliptic curve** is the set of solutions of\n\n"
         "$$y^2 = x^3 + a x + b$$\n\n"
         "Drag $a$ and $b$ in the panel to see the family. Number theorists ask about its **rational points**, solutions where $x$ and $y$ "
         "are fractions. Bottom: $(3, 5)$ is a rational point on $y^2 = x^3 - 2$, since $3^3 - 2 = 25 = 5^2$.",
         [](StepCtx& s) {
             graph(s, 1, {0, 1}, -3.5, 4.5, -6, 6);
             graph(s, 2, {10, 11}, -1, 4.5, -7, 7);
             s.pane(2).trace = "3";
             s.param("a", -2).param("b", 1);
         }},
        {"Adding points",
         "Draw a line through two rational points on the curve: it meets the curve in exactly one more point, and that point is rational too. "
         "This **chord-and-tangent** rule turns the rational points into a group: you can add them.\n\n"
         "Mordell proved (1922) that finitely many points generate all the others.",
         [](StepCtx& s) {
             s.param("a", -7).param("b", 6.25);
             s.dur = 3.f;
         }},
        {"Rank",
         "The **rank** is the number of independent generators of infinite order: how many \"genuinely different\" rational points the "
         "curve has. Rank 0 means only finitely many rational points; rank 1 or more, infinitely many.\n\n"
         "The rank is notoriously hard to compute. This curve, $y^2 = x^3 - 7 x + 25/4$, has rank 3.",
         [](StepCtx& s) { s.ask("Check that (3, 5) is on y\xC2\xB2 = x\xC2\xB3 \xE2\x88\x92 2: what is x\xC2\xB3 \xE2\x88\x92 2 at x = 3?", "25"); }},
        {"Counting points mod p",
         "Birch and Swinnerton-Dyer counted points instead, modulo primes: $N_p$ solutions mod $p$. On average $N_p$ is about $p$, but "
         "curves with many rational points tend to have more.\n\n"
         "They plotted ln of the product of $N_p/p$ over primes up to $x$, against $ln(ln(x))$. The slope came out equal to the rank. "
         "Bottom: curves of rank 0, 1, 2 and 3 with guide lines of slope 1, 2 and 3.",
         [](StepCtx& s) {
             graph(s, 2, {3, 4, 5, 6, 7, 8, 9}, -0.45, 1.85, -2, 6);
             s.pane(2).trace = "";
         }},
        {"The conjecture",
         "The counts combine into the curve's **L-function** $L(E, s)$.\n\n"
         "**The Birch and Swinnerton-Dyer conjecture:** the rank equals the order of vanishing of $L(E, s)$ at $s = 1$. A stronger form also "
         "predicts the leading coefficient.\n\n"
         "It is proved when that order is 0 or 1 (Gross–Zagier and Kolyvagin, 1980s), and open beyond.",
         nullptr},
    };
    L.test = {
        choice("The rank of an elliptic curve counts", {"independent rational points of infinite order", "points modulo p", "the degree of the equation", "zeros of zeta"}, 0, 2),
        typed("Is $(2, 3)$ on $y^2 = x^3 + 1$? Compute $x^3 + 1$ at $x = 2$.", "9", 0),
        choice("The conjecture says the rank equals", {"the order of vanishing of $L(E, s)$ at $s = 1$", "the number of primes up to 100", "3, always", "the number of real pieces of the curve"}, 0, 4),
        typed("In the bottom plot, what slope does a curve of rank 2 approach?", "2", 3),
    };
    return L;
}

}  // namespace

std::vector<LessonDef> millenniumLessons() { return {riemann(), pvsnp(), navierStokes(), yangMills(), hodge(), poincare(), bsd()}; }
