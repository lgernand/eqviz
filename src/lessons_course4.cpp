// Calculus course: Part V, differential equations (Units 13-18), and the final exam.
#include <cstdio>

#include "lesson_kit.hpp"

using namespace theme;
using namespace kit;

namespace {

// ==================================================================== Unit 13

LessonDef unit13() {
    LessonDef L = unit("cc-u13", kPart5, "Unit 13. First-order equations and modeling", "Slope fields, separable, linear and exact equations, logistic growth, and Euler's method.");
    L.setup = [](App& a) { setup(a, "Unit 13: first-order equations", {{"x", GREY, ""}}, {}, 1, false); };
    UnitText u;
    u.bigIdea = "A differential equation states how a quantity changes; solving it predicts the quantity. Slope fields show the answer even when no formula exists.";
    u.topics = "Classification by order and linearity; direction fields; separable equations; linear equations and integrating factors; exact equations, using Unit 10; "
               "substitutions, including Bernoulli equations; existence and uniqueness (Picard–Lindelöf) and Picard iteration; autonomous equations, equilibria and stability; "
               "Euler's method; models of growth and decay, cooling, mixing, logistic populations, falling bodies with drag, and RC circuits.";
    u.weeks = "4 weeks";
    u.people = "Leibniz, Jacob and Johann Bernoulli, Newton, Euler, Malthus, Verhulst, Cauchy, Picard";
    u.story = "Leibniz found separation of variables in 1691. Jacob Bernoulli posed his equation in 1695, and Leibniz solved it a year later. Newton published his law of "
              "cooling anonymously in 1701, and Euler gave his numerical method in 1768.\n\n"
              "Thomas Malthus (1798) predicted exponential population growth; Pierre-François Verhulst (1838) corrected it with the logistic equation, forgotten until "
              "Pearl and Reed rediscovered it in 1920. Cauchy proved the first existence theorem in the 1820s, and Émile Picard's iteration (1890) gave the proof you will learn.\n\n"
              "Right: the logistic slope field. Every solution starting above zero levels off at the carrying capacity.";
    u.read = "- **Math:** Boyce & DiPrima ch. 1–2, including §2.7–2.8 (Euler's method; existence and uniqueness); Simmons, Differential Equations, first-order chapters "
             "and the chapter on Picard's theorem; Strogatz ch. 2 (flows on the line).\n"
             "- **History:** Simmons's historical notes; Kline, Mathematical Thought, the chapter on ordinary differential equations in the 18th century.\n"
             "- **Primary source:** Malthus, An Essay on the Principle of Population (1798), ch. 1–2, free online.";
    u.prove = "- **(a)** The integrating-factor method for y′ + p(x)y = q(x).\n- **(b)** The exactness test.\n"
              "- **(c)** Picard iteration on y′ = y, y(0) = 1 produces the Maclaurin series of eˣ.\n"
              "- **(d)** State Picard–Lindelöf, and show y′ = y^(1/3), y(0) = 0 has more than one solution.";
    u.testMinutes = 150;
    u.testName = "Unit 13 test";
    u.test = "**1.** Solve y′ = xy², y(0) = 1, and give the largest interval on which the solution exists.\n\n"
             "**2.** Solve y′ + (2/x)y = (cos x)/x² for x > 0.\n\n"
             "**3.** Check that (2xy + 3) dx + (x² − 1) dy = 0 is exact, and solve it.\n\n"
             "**4.** Solve the Bernoulli equation y′ + y = xy³.\n\n"
             "**5.** A 100-liter tank holds brine with 5 kg of salt. Brine at 0.2 kg/L flows in at 4 L/min, and the mixture flows out at 4 L/min. Find the salt in the tank at time t, and its long-run value.\n\n"
             "**6.** Solve P′ = 0.4P(1 − P/500), P(0) = 50. Find when P = 250, and sketch the phase line.\n\n"
             "**7.** Use Euler's method with h = 0.1 to estimate y(0.3) for y′ = x + y, y(0) = 1. Compare with the exact solution; a calculator is allowed.\n\n"
             "**8.** Show that y′ = 3y^(2/3), y(0) = 0 has infinitely many solutions, and name the hypothesis of the uniqueness theorem that fails.\n\n"
             "**9.** History: Why was Malthus's exponential model inadequate, and what does Verhulst's extra term represent?";
    auto slope = [](StepCtx& s, const char* Q, double x0, double x1, double y0, double y1, double sx, double sy, bool equal = true) {
        graph(s, 1, {}, x0, x1, y0, y1);
        field(s, 1, "1", Q, kFieldArrows | kFieldSolution, sx, sy, equal);
    };
    L.steps = {
        overview(u, [slope](StepCtx& s) { slope(s, "x - y", -3, 4, -3, 4, 0, 1); }),
        story(u, [slope](StepCtx& s) { slope(s, "y (1 - y)", -1, 9, -0.4, 1.6, 0, 0.08, false); }),
        {"Slope fields",
         "$y' = x - y$ assigns a slope to every point; the short segments show them. A solution is a curve that follows the segments everywhere. "
         "Drag the yellow point to choose an initial condition and watch its solution. All of them approach the line $y = x - 1$.",
         [slope](StepCtx& s) {
             slope(s, "x - y", -3, 4, -3, 4, 0, 1);
             s.ask("Solve y\xE2\x80\xB2 = 2y, y(0) = 3. What is y(1)?", "3 e^2");
         }},
        {"Separable equations",
         "$y' = x y^2$ separates: $d y / y^2 = x d x$, so $-1/y = x^2/2 + C$. With $y(0) = 1$: $y = 1/(1 - x^2/2)$, which blows up at $x = sqrt(2)$. "
         "The equation looks harmless, yet the solution exists only on $(-sqrt(2), sqrt(2))$.",
         [slope](StepCtx& s) {
             slope(s, "x y^2", -2.5, 2.5, -1, 4, 0, 1);
             s.ask("For y\xE2\x80\xB2 = y\xC2\xB2, y(0) = 1, at what x does the solution blow up?", "1", "y = 1/(1 \xE2\x88\x92 x)");
         }},
        {"Logistic growth",
         "$P' = r P (1 - P/K)$: growth is nearly exponential when $P$ is small and stops at the **carrying capacity** $K$. The equilibria $P = 0$ (unstable) and "
         "$P = K$ (stable) organize every solution. Here $r = 0.8$ and $K = 5$; drag the starting value.",
         [slope](StepCtx& s) {
             slope(s, "0.8 y (1 - y/5)", -1, 14, -0.6, 7, 0, 0.3, false);
             s.ask("What is the carrying capacity of P\xE2\x80\xB2 = 0.4P(1 \xE2\x88\x92 P/500)?", "500");
         }},
        {"When uniqueness fails",
         "$y' = 3 |y|^(2/3)$ with $y(0) = 0$ has the solution $y = 0$, and also $y = (x - c)^3$ for x ≥ c, glued to 0 before. Infinitely many solutions "
         "share the same start. Picard–Lindelöf guarantees uniqueness only when the right side is Lipschitz in $y$, and $|y|^(2/3)$ is not, at $y = 0$.",
         [slope](StepCtx& s) { slope(s, "3 abs(y)^(2/3)", -2, 3, -2, 3, 0.5, 0.01); }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("Solve $y' = -y$, $y(0) = 5$. What is $y(ln(5))$?", "1", 2),
        typed("What is the carrying capacity of $P' = P (1 - P/100)$?", "100", 4),
        typed("Euler's method for $y' = y$, $y(0) = 1$, with $h = 0.5$: what is the estimate of $y(1)$ after two steps?", "2.25", 2),
        typed("Newton cooling: $T' = -k (T - 20)$, $T(0) = 80$, $k = ln(2)$. What is $T(1)$?", "50", 2),
        choice("An integrating factor for $y' + 2 y = x$ is", {"$e^(2 x)$", "$e^(-2 x)$", "$e^(x^2)$", "$2 x$"}, 0, 2),
        typed("For $y' = y^2$, $y(0) = 2$, at what $x$ does the solution blow up?", "1/2", 3),
    };
    return L;
}

// ==================================================================== Unit 14

LessonDef unit14() {
    LessonDef L = unit("cc-u14", kPart5, "Unit 14. Second-order linear equations and oscillations", "The characteristic equation, damping, forcing and resonance.");
    L.setup = [](App& a) {
        setup(a, "Unit 14: second-order linear equations",
              {{"y(x) = exp(-c x/2) cos(sqrt(16 - c^2/4) x)", YELLOW_C, "y(t), mass 1, spring 16, damping c"},
               {"A(x) = 1/sqrt((16 - x^2)^2 + (c x)^2)", GREEN_C, "forced amplitude against frequency \xCF\x89"}},
              {{"c", 1, 0, 7.9f}}, 2, true);
    };
    UnitText u;
    u.bigIdea = "Newton's second law makes motion a second-order equation. Linear equations' solutions superpose, and constant coefficients reduce them to algebra: "
                "ay″ + by′ + cy = 0 has solutions e^(rt) with ar² + br + c = 0.";
    u.topics = "Linear homogeneous equations, superposition, and the Wronskian; constant coefficients with real, repeated and complex roots, where Euler's formula returns; "
               "reduction of order; undetermined coefficients and variation of parameters; free, damped and forced vibrations; beats and resonance; RLC circuits; "
               "higher-order linear equations; Cauchy–Euler equations.";
    u.weeks = "4 weeks";
    u.people = "Euler, Lagrange, Wroński, Billah, Scanlan";
    u.story = "Euler solved constant-coefficient equations with the characteristic equation around 1739–43. Euler and Lagrange developed variation of parameters to compute how "
              "planets perturb each other's orbits. Józef Hoene-Wroński introduced his determinant in 1812.\n\n"
              "Textbooks long blamed the 1940 collapse of the Tacoma Narrows Bridge on resonance. Billah and Scanlan showed in 1991 that the cause was aeroelastic flutter, "
              "a different mechanism.";
    u.read = "- **Math:** Boyce & DiPrima ch. 3–4; Simmons, Differential Equations, second-order chapters.\n"
             "- **History:** Simmons's notes on Euler and the Bernoullis; K. Yusuf Billah & Robert Scanlan, \"Resonance, Tacoma Narrows Bridge Failure, and Undergraduate Physics Textbooks,\" American Journal of Physics (1991).\n"
             "- **Primary source:** Euler's papers on linear equations, in the Euler Archive.";
    u.prove = "- **(a)** The solutions of a second-order linear homogeneous equation form a two-dimensional vector space.\n- **(b)** The three cases for ay″ + by′ + cy = 0.\n"
              "- **(c)** The variation-of-parameters formula.\n- **(d)** The amplitude of a forced, damped oscillator, and the frequency where it peaks.";
    u.testMinutes = 150;
    u.testName = "Unit 14 test";
    u.test = "**1.** Solve: (a) y″ − 5y′ + 6y = 0; (b) y″ + 4y′ + 4y = 0; (c) y″ + 2y′ + 5y = 0 with y(0) = 1, y′(0) = 0.\n\n"
             "**2.** Find a particular solution: (a) y″ + y = 3t²; (b) y″ − y = eᵗ; (c) y″ + 4y = sin 2t.\n\n"
             "**3.** Use variation of parameters to solve y″ + y = sec t.\n\n"
             "**4.** Compute the Wronskian of eᵗ and teᵗ, and say what it tells you.\n\n"
             "**5.** Given that y₁ = x solves x²y″ − xy′ + y = 0, find a second solution by reduction of order.\n\n"
             "**6.** A 1 kg mass hangs on a spring with k = 16 N/m and damping c N·s/m. For which c is the motion underdamped, critically damped, overdamped? "
             "With c = 0 and forcing F₀ cos ωt, at what ω does resonance occur, and what does the solution look like?\n\n"
             "**7.** Solve the Cauchy–Euler equation x²y″ + 3xy′ + y = 0.\n\n"
             "**8.** History: State the textbook resonance explanation of the Tacoma Narrows collapse, what Billah and Scanlan found instead, and the mathematical difference between the two.";
    auto both = [](StepCtx& s) {
        graph(s, 1, {0}, -0.3, 10, -1.2, 1.2);
        graph(s, 2, {}, -1.6, 1.6, -6, 6);
        field(s, 2, "y", "-16 x - c y", kFieldArrows | kFieldSolution, 1, 0, false);
    };
    L.steps = {
        overview(u, [both](StepCtx& s) { both(s); s.param("c", 1); }),
        story(u),
        {"Damped oscillations",
         "$y'' + c y' + 16 y = 0$ has characteristic equation $r^2 + c r + 16 = 0$. For small $c$ the roots are complex, −c/2 ± iω, and Euler's formula "
         "turns $e^(r t)$ into a decaying cosine (top). Bottom: the same motion in the phase plane, position across and velocity up: a spiral into rest.",
         [both](StepCtx& s) {
             both(s);
             s.ask("What is the larger root of the characteristic equation of y\xE2\x80\xB3 \xE2\x88\x92 5y\xE2\x80\xB2 + 6y = 0?", "3");
         }},
        {"More damping",
         "Raise $c$ and the oscillation dies faster. At $c = 8$ the roots meet ($r = -4$, repeated): **critical damping**, the fastest return without overshoot. "
         "Beyond it the motion is **overdamped**: two real roots, no oscillation. The phase-plane solution stops spiralling.",
         [](StepCtx& s) {
             s.param("c", 6);
             s.dur = 3.f;
         }},
        {"Resonance",
         "Force the spring with $cos(omega t)$. The steady response has amplitude $1/sqrt((16 - omega^2)^2 + (c omega)^2)$ (top). With little damping it peaks sharply "
         "near the natural frequency $omega = 4$; with none, the amplitude grows without bound.",
         [](StepCtx& s) {
             graph(s, 1, {1}, -0.2, 8, -0.1, 2.2);
             s.param("c", 0.3);
             s.dur = 3.f;
             s.ask("An undamped 1 kg mass on a spring with k = 16 N/m resonates at what \xCF\x89?", "4");
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is the angular frequency of $y'' + 9 y = 0$?", "3", 2),
        typed("For a 1 kg mass on a spring with k = 16 N/m, what damping c is critical?", "8", 3),
        typed("What is the Wronskian of $e^t$ and $e^(2 t)$ at $t = 0$?", "1", 2),
        typed("$y'' + y = 3$ has a constant particular solution. What is it?", "3", 2),
        choice("$y'' + 4 y' + 4 y = 0$ is", {"critically damped (a repeated root)", "underdamped", "overdamped", "undamped"}, 0, 3),
        typed("The roots of $y'' + 2 y' + 5 y = 0$ are −1 ± 2i. What is the angular frequency of its oscillation?", "2", 2),
    };
    return L;
}

// ==================================================================== Unit 15

LessonDef unit15() {
    LessonDef L = unit("cc-u15", kPart5, "Unit 15. The Laplace transform", "Turning differential equations into algebra; steps, impulses and convolution.");
    L.setup = [](App& a) {
        setup(a, "Unit 15: the Laplace transform",
              {{"F(x) = exp(-s x) sin(x)", YELLOW_C, "e^(\xE2\x88\x92st) sin t"}, {"u(x) = (sign(x - 2) + 1)/2", GREEN_C, "unit step at t = 2"},
               {"p(x) = exp(-((x - 2)/w)^2)/(w sqrt(pi))", PURPLE_C, "a narrow pulse of area 1"}},
              {{"s", 1, 0.2f, 4}, {"w", 0.5f, 0.02f, 1}}, 1, false);
    };
    UnitText u;
    u.bigIdea = "The Laplace transform, L{f}(s) = ∫₀^∞ e^(−st) f(t) dt, turns a differential equation into algebra, and handles switches, impulses and shocks cleanly.";
    u.topics = "The definition and when it exists; transforms of elementary functions and of derivatives; inverse transforms by partial fractions; solving initial-value problems; "
               "step functions and the shifting theorems; impulses and the Dirac delta; convolution and transfer functions.";
    u.weeks = "3 weeks";
    u.people = "Laplace, Heaviside, Bromwich, Doetsch, Dirac, Schwartz";
    u.story = "Laplace used the transform in probability theory in the 1780s. A century later Oliver Heaviside, a self-taught telegraph engineer, solved circuit equations by "
              "treating d/dt as an algebraic symbol. Mathematicians rejected his methods; he asked whether he should refuse his dinner because he did not understand digestion.\n\n"
              "Thomas Bromwich (1916) and Gustav Doetsch (1930s) justified Heaviside through the Laplace transform. Paul Dirac's delta function (1927) worked long before "
              "Laurent Schwartz's theory of distributions made it rigorous, earning him a Fields Medal in 1950.";
    u.read = "- **Math:** Boyce & DiPrima ch. 6; Simmons, Differential Equations, the Laplace transform chapter.\n"
             "- **History:** Paul Nahin, Oliver Heaviside: The Life, Work, and Times of an Electrical Genius of the Victorian Age (2002).\n"
             "- **Primary source:** Heaviside, Electromagnetic Theory, Vol. 2 (1899), free online.";
    u.prove = "- **(a)** The transform of f′ is sF(s) − f(0).\n- **(b)** Both shifting theorems.\n- **(c)** The convolution theorem.";
    u.testName = "Unit 15 test";
    u.test = "**1.** From the definition, find the transforms of e^(at), sin bt and t.\n\n"
             "**2.** Find the inverse transforms: (a) (3s + 2)/(s² + 4); (b) 1/(s(s + 1)(s + 2)); (c) e^(−2s)/(s² + 1).\n\n"
             "**3.** Solve y″ + 3y′ + 2y = e^(−t), y(0) = 0, y′(0) = 1 by Laplace transform.\n\n"
             "**4.** Solve y″ + y = g(t), y(0) = y′(0) = 0, where g(t) = 1 for 0 ≤ t < π and 0 afterward.\n\n"
             "**5.** Solve y″ + 4y = δ(t − π), y(0) = 1, y′(0) = 0, and describe the motion physically.\n\n"
             "**6.** Use the convolution theorem to find the inverse transform of 1/(s²(s² + 1)). Check by partial fractions.\n\n"
             "**7.** History: What did Heaviside do that lacked justification, and how was it eventually made rigorous?";
    L.steps = {
        overview(u, [](StepCtx& s) {
            graph(s, 1, {0}, -0.5, 12, -0.6, 1);
            shade(s, 1, 0, "0", "40");
            s.param("s", 1);
        }),
        story(u),
        {"The transform as an area",
         "For each $s$, weight $f(t) = sin(t)$ by $e^(-s t)$ and take the area to infinity. The result, a function of $s$, is $1/(s^2 + 1)$. "
         "Larger $s$ damps harder and shrinks the area. Derivatives become multiplication by $s$, which is why equations become algebra.",
         [](StepCtx& s) {
             s.param("s", 3);
             s.dur = 3.f;
             s.ask("What is the Laplace transform of sin t at s = 2?", "1/5", "1/(s\xC2\xB2 + 1)");
         }},
        {"Switches",
         "The unit step $u(t - 2)$ switches on at $t = 2$. Its transform is $e^(-2 s)/s$: a delay of 2 multiplies the transform by $e^(-2 s)$. "
         "Forcing that starts, stops or changes is a sum of steps.",
         [](StepCtx& s) { graph(s, 1, {1}, -0.8, 6, -0.3, 1.4); }},
        {"Impulses",
         "Squeeze a pulse of area 1 narrower and taller and it tends to the **Dirac delta** $delta(t - 2)$: zero everywhere except an infinitely sharp kick at $t = 2$ "
         "with total area 1. Its transform is $e^(-2 s)$. Watch the pulse sharpen.",
         [](StepCtx& s) {
             graph(s, 1, {2}, -0.5, 4.5, -0.5, 6);
             s.param("w", 0.08);
             s.dur = 3.5f;
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is the Laplace transform of 1 at $s = 2$?", "1/2", 2),
        typed("What is the Laplace transform of $e^(3 t)$ at $s = 5$?", "1/2", 2),
        typed("What is the Laplace transform of $t$ at $s = 2$?", "1/4", 2),
        typed("What is the Laplace transform of $sin(2 t)$ at $s = 2$?", "1/4", 2),
        choice("The inverse transform of $1/(s - 1)$ is", {"$e^t$", "$e^(-t)$", "$sin(t)$", "$t$"}, 0, 2),
        typed("What is the Laplace transform of $cos(t)$ at $s = 1$?", "1/2", 2),
    };
    return L;
}

// ==================================================================== Unit 16

LessonDef unit16() {
    LessonDef L = unit("cc-u16", kPart5, "Unit 16. Systems and the phase plane", "Eigenvalues, the matrix exponential, and phase portraits of linear systems.");
    L.setup = [](App& a) { setup(a, "Unit 16: systems and the phase plane", {{"x", GREY, ""}}, {}, 2, false, 0.34f); };
    UnitText u;
    u.bigIdea = "Linear systems x′ = Ax are solved by eigenvalues and eigenvectors, x(t) = e^(At) x(0), and the phase plane shows every solution at once.";
    u.topics = "A linear algebra primer: matrices, determinants, independence, eigenvalues and eigenvectors; turning higher-order equations into systems; x′ = Ax with real, "
               "complex and repeated eigenvalues; phase portraits of nodes, saddles, spirals and centers; the trace–determinant plane; fundamental matrices and the matrix "
               "exponential; nonhomogeneous systems; coupled oscillators and normal modes.";
    u.weeks = "5 weeks";
    u.people = "Lagrange, Laplace, Cauchy, Sylvester, Cayley, Poincaré";
    u.story = "Lagrange and Laplace (1770s–80s) used linear systems to argue that the solar system is stable. Cauchy proved in 1829 that symmetric matrices have real "
              "eigenvalues. Sylvester coined the word \"matrix\" in 1850, and Cayley built matrix algebra in 1858. Henri Poincaré (1881) named and classified the "
              "equilibrium types you will draw: node, saddle, focus and center.";
    u.read = "- **Math:** Boyce & DiPrima ch. 7; Strogatz ch. 5–6; Strang, Introduction to Linear Algebra, ch. 1–6 (or his Differential Equations and Linear Algebra, 2014).\n"
             "- **History:** Simmons's notes; Kline, Mathematical Thought, the chapter on ordinary differential equations in the 19th century; Thomas Hawkins, "
             "\"Cauchy and the Spectral Theory of Matrices,\" Historia Mathematica (1975).\n"
             "- **Primary source:** Poincaré, Science and Method (1908), the chapter on chance: a nontechnical preview of Unit 17.";
    u.prove = "- **(a)** Eigenvectors for distinct eigenvalues are independent.\n- **(b)** e^(At), defined by its power series, solves x′ = Ax.\n"
              "- **(c)** The trace–determinant classification.";
    u.testMinutes = 180;
    u.testName = "Unit 16 test";
    u.test = "**1.** Find the eigenvalues and eigenvectors of A = [[3, −2], [2, −2]].\n\n"
             "**2.** Solve x′ = Ax for that A with x(0) = (1, 0), and sketch the phase portrait.\n\n"
             "**3.** Solve x′ = [[−1, 2], [−2, −1]]x and classify the origin.\n\n"
             "**4.** Write y‴ − 2y″ − y′ + 2y = 0 as a first-order system, and find its eigenvalues.\n\n"
             "**5.** Compute e^(At) for A = [[0, 1], [0, 0]] and for A = [[2, 1], [0, 2]].\n\n"
             "**6.** Two 1 kg masses are joined by three springs, each with k = 1 N/m, in a line between two walls. Find the normal modes and their frequencies.\n\n"
             "**7.** Classify the origin for systems with (trace, determinant) = (−3, 2), (0, 4), (1, −1) and (2, 1).\n\n"
             "**8.** History: Lagrange and Laplace argued for the stability of the solar system from linearized equations. What did linearizing assume, and why did "
             "Poincaré show the question was far harder?";
    // phase portrait (pane 1) and the same matrix as a transformation with its eigenvectors (pane 2)
    auto system = [](double a, double b, double c, double d) {
        return [=](StepCtx& s) {
            char P[64], Q[64];
            std::snprintf(P, sizeof P, "%g x + %g y", a, b);
            std::snprintf(Q, sizeof Q, "%g x + %g y", c, d);
            graph(s, 1, {}, -3, 3, -3, 3);
            field(s, 1, P, Q, kFieldArrows | kFieldStream);
            clean(s, 2);
            s.show(2, {});
            KeyPane& m = s.pane(2);
            m.matrix = true;
            m.matOpts = kMatGrid | kMatBasis | kMatEigen;
            s.matrix(2, a, b, c, d);
            s.fit(2, -3.5, 3.5, -3.5, 3.5);
            s.dur = 2.f;
        };
    };
    L.steps = {
        overview(u, system(3, -2, 2, -2)),
        story(u),
        {"Saddles",
         "For $x' = A x$ try $x = e^(lambda t) v$: you need $A v = lambda v$. Here $A$ has rows $(3, -2)$ and $(2, -2)$, eigenvalues 2 and $-1$. Along one eigenvector "
         "(right, pink and teal lines) solutions grow, along the other they shrink: a **saddle**. Every other solution (left) is a mixture, swept out along the growing direction.",
         [](StepCtx& s) { s.ask("What is the larger eigenvalue of the matrix with rows (3, \xE2\x88\x92" "2) and (2, \xE2\x88\x92" "2)?", "2"); }},
        {"Spirals",
         "Rows $(-1, 2)$ and $(-2, -1)$: eigenvalues −1 ± 2i. Complex eigenvalues mean rotation (no real eigenvectors), and the real part $-1$ means decay: "
         "solutions spiral into the origin, a **stable spiral**.",
         system(-1, 2, -2, -1)},
        {"Centers",
         "Rows $(0, 1)$ and $(-4, 0)$ come from $y'' + 4 y = 0$. The eigenvalues ±2i are purely imaginary: solutions circle forever on closed ellipses, a **center**.",
         system(0, 1, -4, 0)},
        {"Nodes",
         "Rows $(-2, 0)$ and $(0, -1)$: two negative real eigenvalues. Every solution flows straight into the origin, a **stable node**, curving towards the slower eigen-direction.\n\n"
         "In general the trace $T$ and determinant $D$ decide: $D < 0$ saddle; $D > 0$ and $T^2 < 4 D$ spiral (center if $T = 0$); $T^2 > 4 D$ node.",
         system(-2, 0, 0, -1)},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is the larger eigenvalue of the matrix with rows (1, 1) and (4, 1)?", "3", 2),
        choice("(trace, determinant) = (−3, 2): the origin is", {"a stable node", "a saddle", "a center", "an unstable spiral"}, 0, 5),
        choice("(trace, determinant) = (0, 4): the origin is", {"a center", "a stable node", "a saddle", "a stable spiral"}, 0, 5),
        choice("(trace, determinant) = (1, −1): the origin is", {"a saddle", "an unstable node", "a center", "a stable spiral"}, 0, 5),
        typed("For A with rows (0, 1) and (0, 0), $e^(A t)$ has rows (1, t) and (0, 1). What is its top-right entry at $t = 2$?", "2", 6),
        typed("$x'' = -x$ oscillates with what angular frequency?", "1", 4),
    };
    return L;
}

// ==================================================================== Unit 17

LessonDef unit17() {
    LessonDef L = unit("cc-u17", kPart5, "Unit 17. Nonlinear systems, stability and chaos", "Linearization, predators and prey, the pendulum, Lyapunov functions, bifurcations and Lorenz.");
    L.setup = [](App& a) { setup(a, "Unit 17: nonlinear dynamics and chaos", {{"x", GREY, ""}}, {{"r", -1, -2, 2}}, 1, false); };
    UnitText u;
    u.bigIdea = "Most real systems are nonlinear. Linearizing near equilibria, conserved quantities and Lyapunov functions reveal their behavior, and in three dimensions chaos appears.";
    u.topics = "Equilibria and linearization with the Jacobian (Unit 10); the Hartman–Grobman theorem (statement); predator–prey and competing species; conservative systems "
               "and the nonlinear pendulum; Lyapunov functions; limit cycles and the Poincaré–Bendixson theorem (statement); bifurcations; the Lorenz equations and "
               "sensitive dependence on initial conditions.";
    u.weeks = "3 weeks";
    u.people = "Lotka, Volterra, D'Ancona, Poincaré, Phragmén, Lyapunov, Lorenz";
    u.story = "Alfred Lotka (1925) and Vito Volterra (1926) modeled predators and prey, Volterra prompted by Umberto D'Ancona's Adriatic fish catches. Poincaré won King "
              "Oscar II's 1889 prize for a memoir on the three-body problem. Edvard Phragmén's editorial query exposed a serious error; Poincaré recalled the printed "
              "copies and, in fixing it, discovered chaos.\n\n"
              "Aleksandr Lyapunov's 1892 thesis gave stability its modern form. In 1961 Edward Lorenz at MIT restarted a weather simulation from a value rounded to three "
              "decimals, 0.506 instead of 0.506127, and got a completely different forecast. His 1963 paper made chaos a science.";
    u.read = "- **Math:** Strogatz ch. 6–9; Boyce & DiPrima ch. 9.\n"
             "- **History:** June Barrow-Green, Poincaré and the Three Body Problem (1997); James Gleick, Chaos: Making a New Science (1987); Florin Diacu & Philip Holmes, Celestial Encounters (1996).\n"
             "- **Primary source:** Edward Lorenz, \"Deterministic Nonperiodic Flow,\" Journal of the Atmospheric Sciences (1963), readable with what you know by now.";
    u.prove = "- **(a)** Linearization at an equilibrium via the Jacobian.\n- **(b)** The Lotka–Volterra system has a conserved quantity, so its orbits are closed.\n"
              "- **(c)** Energy is a Lyapunov function for the damped pendulum.";
    u.testMinutes = 150;
    u.testName = "Unit 17 test";
    u.test = "**1.** For x′ = x(3 − x − 2y), y′ = y(2 − x − y), find all equilibria, classify each by linearization, and interpret the result for two competing species.\n\n"
             "**2.** For x′ = x(2 − y), y′ = y(x − 1), find the equilibria, linearize at each, and find a conserved quantity.\n\n"
             "**3.** Write the pendulum equation θ″ + (g/L) sin θ = 0 as a system. Find its energy, sketch the phase portrait, and identify the separatrix.\n\n"
             "**4.** Use V = x² + y² to show the origin is asymptotically stable for x′ = −x + y − x³, y′ = −x − y − y³.\n\n"
             "**5.** Find the bifurcation value of r for x′ = r + x², and sketch the bifurcation diagram.\n\n"
             "**6.** For the Lorenz system with σ = 10 and b = 8/3, show the origin is the only equilibrium when r < 1, and find the other equilibria when r > 1.\n\n"
             "**7.** History: Explain why Lorenz's rounding accident was a discovery rather than a bug, and how it connects to Poincaré's 1890 memoir.";
    auto portrait = [](const char* P, const char* Q, double x0, double x1, double y0, double y1, int extra = 0, double sx = 0, double sy = 1) {
        return [=](StepCtx& s) {
            graph(s, 1, {}, x0, x1, y0, y1);
            field(s, 1, P, Q, kFieldArrows | kFieldStream | extra, sx, sy);
        };
    };
    L.steps = {
        overview(u, portrait("x (2 - y)", "y (x - 1)", -0.3, 4.5, -0.3, 5, kFieldSolution, 1, 0.8)),
        story(u),
        {"Predators and prey",
         "$x$ = prey, $y$ = predators: prey grow alone and are eaten; predators starve alone and grow by eating. Around the equilibrium $(1, 2)$ every orbit is closed: "
         "populations cycle forever, predators lagging prey. Drag the yellow start to choose another cycle.",
         portrait("x (2 - y)", "y (x - 1)", -0.3, 4.5, -0.3, 5, kFieldSolution, 1, 0.8)},
        {"Competing species",
         "Two species eat the same food: $x' = x(3 - x - 2 y)$, $y' = y(2 - x - y)$. Equilibria at $(0, 0)$, $(3, 0)$, $(0, 2)$ and $(1, 1)$. Linearizing with the Jacobian "
         "shows $(1, 1)$ is a saddle: coexistence is unstable, and one species wins depending on where you start.",
         [portrait](StepCtx& s) {
             portrait("x (3 - x - 2 y)", "y (2 - x - y)", -0.2, 3.6, -0.2, 2.6)(s);
             s.ask("What is the x-coordinate of the coexistence equilibrium?", "1", "Solve 3 \xE2\x88\x92 x \xE2\x88\x92 2y = 0 and 2 \xE2\x88\x92 x \xE2\x88\x92 y = 0.");
         }},
        {"The pendulum",
         "$theta'' + sin(theta) = 0$ as a system: angle across, angular velocity up. Near $theta = 0$, closed orbits (swinging); near $theta = pi$, a saddle (balanced upside "
         "down). The **separatrix** through the saddles divides swinging from whirling over the top.",
         portrait("y", "-sin(x)", -7, 7, -3.2, 3.2)},
        {"Bifurcations",
         "$x' = r + x^2$ (slope field: time across, $x$ up). For $r < 0$ there are two equilibria, ±$sqrt(-r)$, one stable and one unstable. As $r$ rises to 0 "
         "they collide and vanish: a **saddle-node bifurcation**. Watch it happen.",
         [](StepCtx& s) {
             graph(s, 1, {}, -1, 6, -2.5, 2.5);
             field(s, 1, "1", "r + y^2", kFieldArrows | kFieldSolution, 0, 0.5);
             s.param("r", 0.4);
             s.dur = 5.f;
         }},
        {"Chaos",
         "In the plane, Poincaré–Bendixson says bounded orbits settle onto equilibria or cycles: no chaos. In three dimensions there is room for more. Lorenz's equations "
         "$x' = 10(y - x)$, $y' = x(28 - z) - y$, $z' = x y - 8/3 z$ trace a butterfly-shaped attractor on which nearby starts separate exponentially fast.\n\n"
         "Right: the damped pendulum, by contrast, settles down from every start.",
         portrait("y", "-sin(x) - 0.5 y", -7, 7, -3.2, 3.2)},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("For $x' = x (2 - y)$, $y' = y (x - 1)$, what is the y-coordinate of the nonzero equilibrium?", "2", 2),
        choice("The pendulum's equilibrium at $theta = pi$ (upside down) is", {"a saddle (unstable)", "a center", "a stable spiral", "a stable node"}, 0, 4),
        typed("At what value of $r$ does $x' = r + x^2$ have its bifurcation?", "0", 5),
        typed("For the Lorenz system (σ = 10, b = 8/3), the origin is the only equilibrium when r is below what value?", "1", 6),
        choice("If $V > 0$ away from the origin and $V' < 0$ along solutions, the origin is", {"asymptotically stable", "unstable", "a saddle", "a center"}, 0, 0),
        choice("For the damped pendulum $theta'' + 0.5 theta' + sin(theta) = 0$, the origin is", {"a stable spiral", "a center", "a saddle", "an unstable node"}, 0, 6),
    };
    return L;
}

// ==================================================================== Unit 18

LessonDef unit18() {
    LessonDef L = unit("cc-u18", kPart5, "Unit 18. Series solutions and Fourier series", "Frobenius, Bessel, Fourier series, and the heat and wave equations.");
    L.setup = [](App& a) {
        setup(a, "Unit 18: series solutions and Fourier series",
              {{"S(x) = sum(k, 1, n, 2 (-1)^(k + 1) sin(k x)/k)", YELLOW_C, "Fourier series of x on (\xE2\x88\x92\xCF\x80, \xCF\x80)"},
               {"x", GREY, "y = x"},
               {"Q(x) = sum(k, 1, n, 4/(pi (2k - 1)) sin((2k - 1) x))", BLUE_C, "square wave"},
               {"u(x, y) = sum(k, 1, 12, 4/(pi (2k - 1)) sin((2k - 1) x) exp(-(2k - 1)^2 y))", PURPLE_C, "temperature u(x, t), with t = y"}},
              {{"n", 3, 1, 40}}, 1, false);
    };
    UnitText u;
    u.bigIdea = "When elementary functions run out, power series solve equations like Bessel's. Fourier series solve the heat and wave equations, ∂u/∂t = k ∂²u/∂x², "
                "and the questions they raised launched modern analysis.";
    u.topics = "Power-series solutions near ordinary points; regular singular points and the method of Frobenius; Legendre's and Bessel's equations; two-point boundary-value "
               "problems and eigenvalues; Fourier series, even and odd extensions, and Dirichlet's convergence theorem (statement); separation of variables for the heat, "
               "wave and Laplace equations.";
    u.weeks = "4 weeks";
    u.people = "d'Alembert, Euler, Daniel Bernoulli, Fourier, Lagrange, Dirichlet, Bessel, Riemann, Cantor";
    u.story = "D'Alembert derived the wave equation in 1747, and he, Euler and Daniel Bernoulli then argued for decades over what a \"function\" could be. Joseph Fourier, who "
              "had served as a scientist on Napoleon's expedition to Egypt, submitted his memoir on heat in 1807. Lagrange objected to its claim that any function is a "
              "trigonometric series, and it appeared only in 1822.\n\n"
              "Dirichlet proved the first convergence theorem in 1829 and gave the modern definition of function. Bessel studied his functions in 1824 to handle planetary "
              "perturbations. Fourier's questions led directly to Riemann's integral (1854) and to Cantor's set theory in the 1870s.";
    u.read = "- **Math:** Boyce & DiPrima ch. 5 and 10; Simmons, Differential Equations, chapters on power-series solutions and Fourier series.\n"
             "- **History:** David Bressoud, A Radical Approach to Real Analysis (2nd ed., 2007), which rebuilds analysis from Fourier's crisis; Paul Nahin, Hot Molecules, Cold Electrons (2020).\n"
             "- **Primary source:** Fourier, The Analytical Theory of Heat (Freeman translation, 1878; Dover).";
    u.prove = "- **(a)** The recurrence for a series solution of Airy's equation y″ − xy = 0.\n- **(b)** The Fourier coefficient formulas, from orthogonality.\n"
              "- **(c)** The solution of the heat equation on a rod with ends held at zero, by separation of variables.";
    u.testMinutes = 180;
    u.testName = "Unit 18 test";
    u.test = "**1.** Find the first four nonzero terms of two independent power-series solutions of y″ + xy′ + y = 0 about x = 0. Identify one of them in closed form.\n\n"
             "**2.** For 2x²y″ + 3xy′ − (1 + x)y = 0, find the indicial roots and the first three terms of the Frobenius solution for the larger root.\n\n"
             "**3.** Find the Fourier series of f(x) = x on (−π, π). Evaluate it at x = π/2 to derive Madhava's series for π/4.\n\n"
             "**4.** Find the Fourier series of f(x) = x² on (−π, π), and use it to prove Σ 1/n² = π²/6, Euler's Basel result by a rigorous route.\n\n"
             "**5.** Solve ∂u/∂t = ∂²u/∂x² on 0 < x < π with u(0, t) = u(π, t) = 0 and u(x, 0) = 1. Describe what happens as t → ∞.\n\n"
             "**6.** Solve the wave equation ∂²u/∂t² = c² ∂²u/∂x² for a string of length L with fixed ends, initial shape f(x) and zero initial velocity, leaving the "
             "coefficients as integrals. Then show the solution has d'Alembert's form ½[F(x − ct) + F(x + ct)].\n\n"
             "**7.** Find the eigenvalues and eigenfunctions of y″ + λy = 0 with y(0) = 0 and y′(L) = 0.\n\n"
             "**8.** History essay: Why did Lagrange object to Fourier's claim, what did Dirichlet prove in 1829, and how did the question lead Riemann and Cantor to new mathematics?";
    L.steps = {
        overview(u, [](StepCtx& s) {
            graph(s, 1, {0, 1}, -7, 7, -4.2, 4.2);
            s.param("n", 3);
        }),
        story(u),
        {"Fourier series",
         "On (−π, π), $x$ is a sum of sines: x = 2(sin x − sin(2x)/2 + sin(3x)/3 − ⋯). The coefficients come from orthogonality: multiply by $sin(k x)$ and "
         "integrate. Watch more terms build the line, and repeat it as a sawtooth. Near the jumps a 9% overshoot never goes away: the Gibbs phenomenon.",
         [](StepCtx& s) {
             s.param("n", 30);
             s.dur = 5.f;
             s.ask("What is the first coefficient b\xE2\x82\x81 in the Fourier sine series of x on (\xE2\x88\x92\xCF\x80, \xCF\x80)?", "2");
         }},
        {"Square waves",
         "A discontinuous square wave is a sum of continuous sines with odd frequencies. Lagrange refused to believe it. Dirichlet (1829) proved such series converge, "
         "to the average of the two sides at a jump.",
         [](StepCtx& s) {
             graph(s, 1, {2}, -7, 7, -1.6, 1.6);
             s.param("n", 1);
         }},
        {"Adding odd harmonics",
         "Each new odd harmonic sharpens the corners.",
         [](StepCtx& s) {
             s.param("n", 25);
             s.dur = 5.f;
         }},
        {"The heat equation",
         "A rod from 0 to $pi$ starts at temperature 1 with its ends held at 0. Separation of variables gives modes $sin(k x) e^(-k^2 t)$: the surface shows the temperature "
         "across the rod ($x$) as time runs ($y$). High-frequency modes die fastest, so the profile smooths into a single sine bump and fades.",
         [](StepCtx& s) {
             surface(s, 1, 3, PI / 2, 0.8f, false, PI / 2, PI / 2);
             s.ask("For u\xE2\x82\x9C = u\xE2\x82\x93\xE2\x82\x93, the mode sin(3x) decays like e^(\xE2\x88\x92kt). What is k?", "9");
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is the coefficient $b_2$ in the Fourier sine series of $x$ on $(-pi, pi)$?", "-1", 2),
        typed("What is Σ 1/n² for n from 1 to ∞?", "pi^2/6", 2),
        typed("Evaluate 1 − 1/3 + 1/5 − 1/7 + ⋯ (set x = π/2 in the series for x).", "pi/4", 2),
        typed("What is the smallest eigenvalue of $y'' + lambda y = 0$ with $y(0) = y(pi) = 0$?", "1", 5),
        choice("Who proved the first convergence theorem for Fourier series (1829)?", {"Dirichlet", "Fourier", "Lagrange", "Cantor"}, 0, 1),
    };
    return L;
}

// ==================================================================== final exam

LessonDef finalExam() {
    LessonDef L = unit("cc-final", kPart5, "Final exam", "Two weeks of review, then a 4-hour exam on everything, weighted toward Part V.");
    L.setup = [](App& a) { setup(a, "Calculus course: final exam", {{"x", GREY, ""}}, {}, 1, false); };
    L.steps = {
        {"Review",
         "The final covers the whole course, weighted toward differential equations. Spend two weeks on review:\n\n"
         "- Rework the hardest problem from every unit test and from the midterm.\n"
         "- Write the milestone proofs cold, one part per day.\n"
         "- Choose your essay thread now and gather names and dates: the infinitely small (Eudoxus to Abraham Robinson), the stability of the solar system "
         "(Newton to Poincaré), or the meaning of \"function\" (Leibniz to Dirichlet).\n\n"
         "**After the final**, the natural next steps are real analysis (Stephen Abbott, Understanding Analysis), complex analysis (Tristan Needham, Visual Complex Analysis) "
         "and partial differential equations (Walter Strauss, Partial Differential Equations: An Introduction).\n\n"
         "Right: the damped pendulum of problem 10.",
         [](StepCtx& s) {
             graph(s, 1, {}, -7, 7, -3.2, 3.2);
             field(s, 1, "y", "-sin(x) - 0.5 y", kFieldArrows | kFieldStream);
         }},
        text("Final exam (written)",
             "**Closed book, 4 hours, one sitting.** Send your work to Claude in your Calculus mastery chat.\n\n"
             "**1.** Prove that if f″ > 0 on (a, b), the graph of f lies above each of its tangent lines there.\n\n"
             "**2.** Evaluate ∫₀^∞ x² e^(−x²) dx.\n\n"
             "**3.** Find the Maclaurin series of ln(1 + x) and the exact set where it equals ln(1 + x). Use it to sum Σ (−1)ⁿ⁺¹/(n·2ⁿ).\n\n"
             "**4.** Find the absolute maximum and minimum of f(x, y) = x² + 2y² on the disk x² + y² ≤ 1.\n\n"
             "**5.** Find the outward flux of F = ⟨x, y, z⟩ through the closed cylinder x² + y² ≤ 1, 0 ≤ z ≤ 2, both directly and by the divergence theorem.\n\n"
             "**6.** A fish population follows P′ = P(1 − P) − h. Find the equilibria and their stability for h = 0.16, and the largest harvest rate h the population can survive.\n\n"
             "**7.** Solve y″ − 2y′ + y = eᵗ/t for t > 0 by variation of parameters.\n\n"
             "**8.** Solve y″ + 2y′ + 2y = δ(t − π), y(0) = y′(0) = 0, by Laplace transform.\n\n"
             "**9.** Solve x′ = [[1, 1], [4, 1]]x with x(0) = (2, 0), and classify the origin.\n\n"
             "**10.** For the damped pendulum θ″ + 0.5θ′ + sin θ = 0, find and classify the equilibria and sketch the phase portrait.\n\n"
             "**11.** Solve Laplace's equation ∂²u/∂x² + ∂²u/∂y² = 0 on the square 0 < x, y < π, with u = 0 on three sides and u(x, π) = sin x.\n\n"
             "**12.** Essay: trace one thread across at least three centuries, with named people and dates. Choose (a) the infinitely small, from Eudoxus to Abraham Robinson; "
             "(b) the stability of the solar system, from Newton to Poincaré; or (c) the meaning of \"function,\" from Leibniz to Dirichlet."),
    };
    L.test = {
        typed("What is ∫₀^∞ x e^(−x²) dx?", "1/2", 0),
        typed("What is the coefficient of x³ in the Maclaurin series of ln(1 + x)?", "1/3", 0),
        typed("What is the maximum of $x^2 + 2 y^2$ on the unit disk?", "2", 0),
        typed("What is the outward flux of (x, y, z) through the closed cylinder of radius 1 and height 2?", "6 pi", 0),
        typed("For $P' = P (1 - P) - h$, what is the largest harvest rate $h$ that leaves an equilibrium?", "1/4", 0),
        typed("$y'' - 2 y' + y = 0$ has a repeated characteristic root. What is it?", "1", 0),
        typed("What is the Laplace transform of $delta(t - pi)$ at $s = 1$?", "exp(-pi)", 0),
        typed("What is the smaller eigenvalue of the matrix with rows (1, 1) and (4, 1)?", "-1", 0),
        choice("For the damped pendulum $theta'' + 0.5 theta' + sin(theta) = 0$, the equilibrium at $theta = pi$ is", {"a saddle", "a stable spiral", "a center", "a stable node"}, 0, 0),
        typed("$u = sin(x) sinh(y)/sinh(pi)$ solves the Laplace problem of problem 11. What is $u$ at $(pi/2, pi)$?", "1", 0),
    };
    return L;
}

}  // namespace

std::vector<LessonDef> calculusCourse4() { return {unit13(), unit14(), unit15(), unit16(), unit17(), unit18(), finalExam()}; }
