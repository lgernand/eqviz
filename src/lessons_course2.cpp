// Calculus course: Part II (Units 4-6 and the midterm) and Part III (Units 7-8).
#include "lesson_kit.hpp"

using namespace theme;
using namespace kit;

namespace {

// ==================================================================== Unit 4

LessonDef unit4() {
    LessonDef L = unit("cc-u4", kPart2, "Unit 4. The integral and the Fundamental Theorem", "Riemann sums, both halves of the Fundamental Theorem, substitution, and ln x as an area.");
    L.setup = [](App& a) {
        setup(a, "Unit 4: the integral and the Fundamental Theorem",
              {{"f(x) = x^3 + 1", YELLOW_C, ""},                                                         // 0
               {"R(x) = f(2 floor(n x / 2)/n + 2/n) (sign(x (2 - x)) + 1)/2", BLUE_C, "right-endpoint rectangles"},  // 1
               {"c(x) = cos(x)", YELLOW_C, ""},                                                          // 2
               {"A(x) = sum(k, 1, 240, c((k - 1/2) x/240) x/240)", BLUE_C, "area from 0 to x"},           // 3
               {"q(x) = 1 - x^2", GREEN_C, "between y = 1 and y = x²"},                                  // 4
               {"r(x) = 1/x", YELLOW_C, ""}},                                                            // 5
              {{"n", 4, 1, 64}, {"a", 2, 0.2f, 6.3f}}, 1, false);
    };
    UnitText u;
    u.bigIdea = "The definite integral is a limit of sums. The Fundamental Theorem says integration and differentiation undo each other: "
                "d/dx ∫ₐˣ f(t) dt = f(x), and ∫ₐᵇ F′(x) dx = F(b) − F(a).";
    u.topics = "Antiderivatives; sigma notation and Riemann sums; upper and lower sums and the definition of integrability; properties of the integral; "
               "both parts of the Fundamental Theorem; substitution; ln x defined as the integral of 1/t from 1 to x, which makes Unit 2 rigorous; "
               "average value and the Mean Value Theorem for integrals.";
    u.weeks = "3 weeks";
    u.people = "Archimedes, Cavalieri, Barrow, Newton, Leibniz, Cauchy, Riemann";
    u.story = "Around 250 BC Archimedes proved that a parabolic segment has 4/3 the area of its inscribed triangle, by exhaustion. His Method, which shows "
              "how he found such results, was lost until J. L. Heiberg identified it under a medieval prayer book in 1906. Bonaventura Cavalieri's "
              "Geometria indivisibilibus (1635) treated areas as stacks of lines, and the Jesuits fought to suppress the idea.\n\n"
              "Barrow proved a geometric form of the Fundamental Theorem in 1670; Newton and Leibniz turned it into an algorithm. Leibniz first wrote ∫, "
              "a long S for summa, in a manuscript dated 29 October 1675. Cauchy (1823) defined the integral as a limit of sums, and Riemann (1854) asked "
              "exactly which functions can be integrated.\n\n"
              "Right: the parabolic segment between $y = 1$ and $y = x^2$, flipped onto the axis. Its area is 4/3; the inscribed triangle with vertices "
              "$(-1, 1)$, $(1, 1)$, $(0, 0)$ has area 1.";
    u.read = "- **Math:** Spivak ch. 13–14 (Integrals; The Fundamental Theorem) and ch. 18 (logarithm and exponential defined through the integral); Simmons, chapters on the definite integral.\n"
             "- **History:** Reviel Netz & William Noel, The Archimedes Codex (2007); Amir Alexander, Infinitesimal; Dunham, Journey Through Genius, ch. 4 (Archimedes).\n"
             "- **Primary source:** Archimedes, Quadrature of the Parabola, in T. L. Heath, The Works of Archimedes (Dover).";
    u.prove = "- **(a)** The integral of x² from 0 to b equals b³/3, from Riemann sums.\n- **(b)** The Fundamental Theorem, part 1.\n"
              "- **(c)** Part 2 from part 1.\n- **(d)** ln(xy) = ln x + ln y from the integral definition.";
    u.testName = "Unit 4 test";
    u.test = "**1.** Compute the integral of x³ + 1 from 0 to 2 as a limit of right-endpoint Riemann sums. Check it with the Fundamental Theorem.\n\n"
             "**2.** Let f(x) = 1 for rational x and 0 for irrational x. Prove f is not integrable on [0, 1].\n\n"
             "**3.** Find the derivative of F(x) = ∫ e^(t²) dt from t = x² to t = sin x.\n\n"
             "**4.** Evaluate: (a) ∫ x√(x² + 4) dx; (b) ∫₀^(π/2) sin³x cos x dx; (c) ∫ (ln x)/x dx; (d) ∫₁^e dx / (x(1 + ln x)).\n\n"
             "**5.** Prove part 1 of the Fundamental Theorem: if f is continuous on [a, b] and F(x) = ∫ₐˣ f(t) dt, then F′(x) = f(x).\n\n"
             "**6.** Using only the definition ln x = ∫₁ˣ dt/t, prove ln(xy) = ln x + ln y for x, y > 0.\n\n"
             "**7.** Find the average value of sin x on [0, π], and a point c where sin c equals it.\n\n"
             "**8.** Verify Archimedes' 4/3 result with calculus for the segment of y = x² cut off by the line y = 1.\n\n"
             "**9.** History: Describe Cavalieri's method of indivisibles, and give one paradox that made indivisibles look dangerous.";
    L.steps = {
        overview(u, [](StepCtx& s) {
            graph(s, 1, {0, 1}, -0.3, 2.4, -0.6, 10);
            shade(s, 1, 1, "0", "2");
            s.param("n", 4);
        }),
        story(u, [](StepCtx& s) {
            graph(s, 1, {4}, -1.6, 1.6, -0.3, 1.3);
            shade(s, 1, 4, "-1", "1");
        }),
        {"Riemann sums",
         "Cut [0, 2] into $n$ strips and stack rectangles as tall as $f$ at each strip's right end. Their total area is a **Riemann sum**; "
         "for this rising curve it overshoots. As $n$ grows the sums close in on one number: the integral.",
         [](StepCtx& s) {
             graph(s, 1, {0, 1}, -0.3, 2.4, -0.6, 10);
             shade(s, 1, 1, "0", "2");
             s.param("n", 64);
             s.dur = 5.f;
             s.ask("What is the exact area under x³ + 1 from 0 to 2?", "6", "An antiderivative is x⁴/4 + x.");
         }},
        {"The Fundamental Theorem",
         "Let $A(x)$ be the area under cos from 0 to $x$ (blue). Its height at $a$ equals the shaded area. Nudge $a$ by $h$ and the area grows by a strip "
         "about $cos(a)$ tall, so the slope of $A$ is cos: **part 1**. Then any antiderivative gives the area: **part 2**.",
         [](StepCtx& s) {
             graph(s, 1, {2, 3}, -0.5, 7, -1.6, 1.6);
             shade(s, 1, 2, "0", "a");
             s.pane(1).trace = "a";
             s.param("a", 1.0472);
             s.ask("What is the derivative of ∫₀ˣ cos t dt at x = π/3?", "1/2", "Part 1: it is cos(x).");
         }},
        {"The logarithm as an area",
         "Define $ln(x)$ as the area under $1/t$ from 1 to $x$. Everything about logarithms follows from this, rigorously: stretching the hyperbola "
         "shows $ln(x y) = ln(x) + ln(y)$, and the slope of the area is $1/x$. The area reaches 1 exactly at $x = e$.",
         [](StepCtx& s) {
             graph(s, 1, {5}, -0.3, 6.5, -0.4, 3);
             shade(s, 1, 5, "1", "a");
             s.pane(1).trace = "a";
             s.param("a", 2.71828);
             s.dur = 2.5f;
             s.ask("What is ∫₁^e dt/t?", "1");
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is ∫₀³ x² dx?", "9", 2),
        typed("What is the derivative of ∫₀ˣ cos(t²) dt at $x = 0$?", "1", 3),
        typed("What is ∫₀^π sin x dx?", "2", 3),
        typed("What is the average value of $x^2$ on [0, 3]?", "3", 2),
        typed("Use substitution: what is ∫₀¹ 2x e^(x²) dx?", "e - 1", 3),
        choice("Right-endpoint Riemann sums of an increasing function", {"overestimate the integral", "underestimate it", "are exact", "could do either"}, 0, 2),
    };
    return L;
}

// ==================================================================== Unit 5

LessonDef unit5() {
    LessonDef L = unit("cc-u5", kPart2, "Unit 5. Applications of the integral", "Areas between curves, volumes, arc length, work and centers of mass.");
    L.setup = [](App& a) {
        setup(a, "Unit 5: applications of the integral",
              {{"f(x) = x^2", YELLOW_C, ""},                                          // 0
               {"g(x) = 4x - x^2", BLUE_C, ""},                                       // 1
               {"d(x) = g(x) - f(x)", GREEN_C, "g − f"},                              // 2
               {"S(x, y) = sqrt(4 - x^2 - y^2)", PURPLE_C, "hemisphere of radius 2"},  // 3
               {"w(x) = (2/3) x^(3/2)", YELLOW_C, ""},                                // 4
               {"sin(x)", RED_C, ""}},                                                // 5
              {{"s", 0, 0, 3}}, 1, false);
    };
    UnitText u;
    u.bigIdea = "Anything built by adding up thin slices is an integral: area, volume, length, work, mass, probability.";
    u.topics = "Area between curves; volumes by slicing, disks and washers, and cylindrical shells; arc length; surface area of revolution; work; hydrostatic force; "
               "moments and centers of mass; the theorem of Pappus; probability densities.";
    u.weeks = "3 weeks";
    u.people = "Kepler, Pappus, Guldin, Neile, van Heuraet, Huygens";
    u.story = "Johannes Kepler bought wine for his 1613 wedding and doubted the merchant's measuring rod. His Nova stereometria doliorum vinariorum (1615) "
              "computed barrel volumes by slicing. Pappus of Alexandria (c. 320 AD) found the volume of a solid of revolution through its centroid, "
              "a result Paul Guldin rediscovered in the 1630s.\n\n"
              "In 1657–59 William Neile and Hendrik van Heuraet found the exact length of the curve y² = x³, the first algebraic curve to be \"rectified.\" "
              "Many had believed no curved line could equal a straight one exactly.";
    u.read = "- **Math:** Simmons, chapters on applications of integration; OpenStax Volume 1, ch. 6.\n"
             "- **History:** Edwards, chapters on 17th-century quadratures and rectification; Strogatz, Infinite Powers, on Kepler and Galileo.\n"
             "- **Primary source:** Kepler's integration methods, excerpted in Struik's Source Book.";
    u.prove = "- **(a)** Volume = ∫ A(x) dx, from slicing.\n- **(b)** The arc-length formula, using the Mean Value Theorem.\n"
              "- **(c)** Pappus's volume theorem for a region revolved about an axis.";
    u.testName = "Unit 5 test";
    u.test = "**1.** Find the area enclosed by y = x² and y = 4x − x².\n\n"
             "**2.** Derive the volume of a sphere of radius r using disks, and the volume of a cone using slices.\n\n"
             "**3.** Revolve the region under y = √x, 0 ≤ x ≤ 4, about the y-axis. Find the volume with shells, then check it with washers.\n\n"
             "**4.** Find the length of y = (2/3)x^(3/2) from x = 0 to x = 3.\n\n"
             "**5.** An inverted conical tank, 10 m tall with a top radius of 4 m, is full of water (1000 kg/m³, g = 9.8 m/s²). Find the work needed to pump all the water over the top.\n\n"
             "**6.** Use Pappus's theorem to find the volume of the torus formed by revolving a disk of radius a about a line at distance R > a from its center.\n\n"
             "**7.** Find the centroid of the region under y = sin x on [0, π].\n\n"
             "**8.** History: Kepler computed barrel volumes in 1615, half a century before Newton and Leibniz. What did he lack that the Fundamental Theorem later supplied?";
    L.steps = {
        overview(u, [](StepCtx& s) {
            graph(s, 1, {0, 1}, -0.6, 4.6, -1, 5);
            shade(s, 1, 2, "0", "2");
        }),
        story(u, [](StepCtx& s) { surface(s, 1, 3, 2.2, 1.f); }),
        {"Area between curves",
         "Between $y = 4 x - x^2$ (blue) and $y = x^2$ (yellow), a thin vertical strip has height $g(x) - f(x)$. Add the strips from where the curves meet "
         "($x = 0$) to where they meet again ($x = 2$). The shaded green area is that integral.",
         [](StepCtx& s) {
             graph(s, 1, {0, 1, 2}, -0.6, 4.6, -1, 5);
             shade(s, 1, 2, "0", "2");
             s.ask("What is the area between y = x and y = x² on [0, 1]?", "1/6");
         }},
        {"Volumes by slicing",
         "Slice a ball of radius $r$ perpendicular to an axis: the slice at height $x$ is a disk of radius $sqrt(r^2 - x^2)$, area $pi (r^2 - x^2)$. "
         "Integrate from $-r$ to $r$ and you get $4/3 pi r^3$. Kepler did this for wine barrels.",
         [](StepCtx& s) {
             surface(s, 1, 3, 2.2, 1.f);
             s.ask("What is the volume of a ball of radius 3?", "36 pi");
         }},
        {"Arc length",
         "A tiny piece of curve is nearly straight, with length $sqrt(1 + (y')^2) d x$ by Pythagoras. For $y = 2/3 x^(3/2)$ that is $sqrt(1 + x)$, "
         "which integrates exactly: this curve is rectifiable, like Neile's. Watch the point travel along it.",
         [](StepCtx& s) {
             graph(s, 1, {}, -0.3, 3.6, -0.3, 3.8);
             curve(s, 1, "t", "(2/3) t^(3/2)", "0", "3", "s");
             s.param("s", 3);
             s.dur = 4.f;
             s.ask("What is the length of y = (2/3)x^(3/2) from x = 0 to x = 1?", "(2/3)(2^(3/2) - 1)", "∫₀¹ √(1 + x) dx");
         }},
        {"Centers of mass",
         "The centroid of a region is its balance point: average $x$ weighted by strip height. For the region under $sin(x)$ on [0, π], symmetry "
         "gives $x = pi/2$ (dashed), and the integral of $sin(x)^2/2$ gives the height $pi/8$.",
         [](StepCtx& s) {
             graph(s, 1, {5}, -0.3, 3.5, -0.3, 1.3);
             shade(s, 1, 5, "0", "pi");
             s.pane(1).trace = "pi/2";
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is the area between $y = 2 x$ and $y = x^2$?", "4/3", 2),
        typed("Revolve $y = sqrt(x)$, 0 ≤ x ≤ 4, about the x-axis. What is the volume (disks)?", "8 pi", 3),
        typed("What is the length of the line $y = 2 x$ from $x = 0$ to $x = 3$?", "3 sqrt(5)", 4),
        typed("How much work (in joules) stretches a spring with k = 10 N/m by 0.5 m from rest length?", "1.25", 1),
        typed("What is the x-coordinate of the centroid of the triangle under $y = x$ on [0, 2]?", "4/3", 5),
    };
    return L;
}

// ==================================================================== Unit 6

LessonDef unit6() {
    LessonDef L = unit("cc-u6", kPart2, "Unit 6. Techniques of integration and improper integrals", "Parts, trig substitution, partial fractions, numerical rules, and infinite intervals.");
    L.setup = [](App& a) {
        setup(a, "Unit 6: techniques and improper integrals",
              {{"r(x) = 1/x", YELLOW_C, ""},                                      // 0
               {"q(x) = 1/x^2", GREEN_C, ""},                                     // 1
               {"T(x, y) = sqrt(1/x^2 - y^2)", PURPLE_C, "Torricelli's trumpet"},  // 2
               {"g(x) = exp(-x^2)", BLUE_C, ""}},                                 // 3
              {{"b", 3, 1.2f, 60}}, 2, true);
    };
    UnitText u;
    u.bigIdea = "Integration has no algorithm as clean as differentiation. You build a toolkit, learn when each tool fits, and learn that some antiderivatives are not elementary at all.";
    u.topics = "Integration by parts; trigonometric integrals; trigonometric substitution; partial fractions; strategy; numerical integration by the trapezoid and "
               "Simpson's rules, with error bounds; improper integrals over infinite intervals and with unbounded integrands; comparison tests for improper integrals.";
    u.weeks = "4 weeks";
    u.people = "Torricelli, Leibniz, Johann Bernoulli, Euler, Simpson, Liouville";
    u.story = "In 1641 Evangelista Torricelli, Galileo's student, showed that an infinitely long solid could have finite volume. The result shocked philosophers, "
              "Thomas Hobbes among them. Leibniz and Johann Bernoulli developed partial fractions around 1702, and Euler's Institutiones calculi integralis "
              "(1768–70) organized the techniques you will learn.\n\n"
              "Thomas Simpson published his rule in 1743, though Cavalieri and James Gregory had it earlier, and Kepler's barrel rule of 1615 is a special case. "
              "In the 1830s Joseph Liouville proved that some integrals, such as ∫ e^(−x²) dx, have no elementary antiderivative.\n\n"
              "Bottom: Torricelli's trumpet, $y = 1/x$ for x ≥ 1 spun about the axis (one half shown).";
    u.read = "- **Math:** Spivak ch. 19 (Integration in Elementary Terms); Simmons, chapter on methods of integration; OpenStax Volume 2, ch. 3.\n"
             "- **History:** Paul Nahin, Inside Interesting Integrals (2015); Alexander, Infinitesimal, on Torricelli.\n"
             "- **Primary source:** Euler on integration, in Ian Bruce's translations at 17centurymaths.com.";
    u.prove = "- **(a)** Integration by parts, from the product rule.\n- **(b)** Simpson's rule, by integrating the parabola through three points.\n"
              "- **(c)** ∫₁^∞ dx/xᵖ converges exactly when p > 1.";
    u.testMinutes = 150;
    u.testName = "Unit 6 test";
    u.test = "**1.** ∫ x² eˣ dx\n\n**2.** ∫ eˣ sin x dx\n\n**3.** ∫ sin²x cos³x dx\n\n**4.** ∫ dx / (x²√(x² + 9))\n\n"
             "**5.** ∫ (3x + 5) / ((x − 1)(x² + 1)) dx\n\n"
             "**6.** Evaluate or show divergence: (a) ∫₀^∞ x e^(−x) dx; (b) ∫₀¹ ln x dx; (c) ∫₁^∞ (sin²x)/x² dx (decide convergence only).\n\n"
             "**7.** Use Simpson's rule with n = 4 to approximate ∫₀¹ e^(−x²) dx, and bound the error. A calculator is allowed for this problem.\n\n"
             "**8.** Revolve y = 1/x, x ≥ 1, about the x-axis to form Torricelli's trumpet. Show that its volume is finite and its surface area infinite.\n\n"
             "**9.** Prove that ∫₁^∞ dx/xᵖ converges if and only if p > 1.\n\n"
             "**10.** History: Name an integral Liouville showed has no elementary antiderivative. Explain why that does not mean the integral fails to exist.";
    auto both = [](StepCtx& s) {
        graph(s, 1, {0}, -0.5, 12, -0.2, 1.4);
        shade(s, 1, 0, "1", "b");
        graph(s, 2, {1}, -0.5, 12, -0.2, 1.4);
        shade(s, 2, 1, "1", "b");
    };
    L.steps = {
        overview(u, [both](StepCtx& s) { both(s); s.param("b", 3); }),
        story(u, [](StepCtx& s) { surface(s, 2, 2, 3.5, 1.f, false, 4.5, 0); }),
        {"Improper integrals",
         "An integral to infinity is a limit: integrate to $b$, then let $b$ grow. Top: the area under $1/x$ is $ln(b)$, which grows forever. "
         "Bottom: the area under $1/x^2$ is $1 - 1/b$, which settles at 1. The two curves look alike; one has finite area.\n\n"
         "Torricelli's trumpet uses exactly this: its volume involves $1/x^2$ (finite), its surface area $1/x$ (infinite).",
         [both](StepCtx& s) {
             both(s);
             s.param("b", 60);
             s.dur = 5.f;
             s.ask("What is ∫₁^∞ dx/x²?", "1");
         }},
        {"Integration by parts",
         "Reverse the product rule: **∫ u dv = uv − ∫ v du**. Choose $u$ to get simpler when differentiated. For ∫ x eˣ dx, "
         "take $u = x$: the answer is $x e^x - e^x$.\n\n"
         "Other tools: trig identities and substitutions turn $sqrt(a^2 - x^2)$ into $a cos(theta)$; partial fractions split rational functions.",
         [](StepCtx& s) { s.ask("What is ∫₀¹ x eˣ dx?", "1", "x eˣ − eˣ from 0 to 1"); }},
        {"No elementary antiderivative",
         "$e^(-x^2)$ has an antiderivative, but Liouville proved it cannot be written with elementary functions. The area still exists, and numerical "
         "rules find it: Simpson's rule fits parabolas through triples of points. Top: the area from 0 to 1.",
         [](StepCtx& s) {
             graph(s, 1, {3}, -2.5, 2.5, -0.2, 1.2);
             shade(s, 1, 3, "0", "1");
             s.ask("What is ∫₀¹ e^(−x²) dx, to three decimals? (read it off the graph)", "0.7468", "", 0.0015);
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is ∫₀^∞ e^(−2x) dx?", "1/2", 2),
        typed("What is ∫₁^∞ dx/x³?", "1/2", 2),
        choice("∫₁^∞ dx/√x", {"diverges", "equals 2", "equals 1", "equals 1/2"}, 0, 2),
        typed("What is ∫₀^(π/2) sin²x dx?", "pi/4", 3),
        typed("Partial fractions: what is ∫₂³ dx/(x(x − 1))?", "ln(4/3)", 3),
        typed("By parts: what is ∫₁^e ln x dx?", "1", 3),
    };
    return L;
}

// ==================================================================== midterm

LessonDef midterm() {
    LessonDef L = unit("cc-midterm", kPart2, "Midterm: single-variable calculus", "One week of review, then a 3-hour exam on Units 1–6.");
    L.setup = [](App& a) {
        setup(a, "Calculus course: midterm",
              {{"y(x) = sqrt(x)", YELLOW_C, ""}, {"h(x) = sqrt(1 + x^3)", BLUE_C, ""},
               {"F(x) = sum(k, 1, 200, h((k - 1/2) x/200) x/200)", GREEN_C, "F(x) = ∫₀ˣ √(1 + t³) dt"}},
              {{"a", 2, 0, 3}}, 1, false);
    };
    L.steps = {
        {"Review week",
         "The midterm covers **Units 1–6**: the derivative and its rules, the chain rule and transcendental functions, the Mean Value Theorem and its applications, "
         "the integral and the Fundamental Theorem, applications of the integral, and techniques with improper integrals.\n\n"
         "- Rework the hardest problem you missed on each unit test.\n"
         "- Write every milestone proof from Units 1–6 cold: the product rule, d/dx sin x, the chain rule, Rolle and the Mean Value Theorem, both parts "
         "of the Fundamental Theorem, integration by parts.\n"
         "- Reread the history: the essay question asks about the Newton–Leibniz priority dispute.\n\n"
         "Right: F(x) = ∫₀ˣ √(1 + t³) dt, and its integrand. Problem 10 asks about this function.",
         [](StepCtx& s) {
             graph(s, 1, {2, 1}, -0.3, 3.2, -0.3, 6);
             s.pane(1).trace = "a";
         }},
        text("Midterm (written)",
             "**Closed book, 3 hours, one sitting.** Send your work to Claude in your Calculus mastery chat. 85% moves you on to Part III.\n\n"
             "**1.** Prove from the definition that f(x) = x³ is differentiable, and find f′.\n\n"
             "**2.** State and prove the Mean Value Theorem, assuming Rolle's theorem.\n\n"
             "**3.** Find dy/dx for y = (sin x)^(ln x).\n\n"
             "**4.** A person 2 m tall walks at 1.5 m/s away from a 6 m street lamp. How fast is the tip of their shadow moving?\n\n"
             "**5.** Find the point on y = √x closest to (3, 0).\n\n"
             "**6.** Evaluate lim x→0 of (tan x − x)/x³.\n\n"
             "**7.** ∫ x arctan x dx\n\n"
             "**8.** ∫ dx / (x² − 4x + 13)\n\n"
             "**9.** Find the volume when the region between y = x and y = x² is revolved about the x-axis.\n\n"
             "**10.** Let F(x) = ∫₀ˣ √(1 + t³) dt. Find F′(2), and decide whether F is concave up at x = 1.\n\n"
             "**11.** Decide whether each converges: (a) ∫₀^∞ dx/(1 + x³); (b) ∫₂^∞ dx/(x ln x).\n\n"
             "**12.** Essay, one page: the Newton–Leibniz priority dispute. What did each man do, and when? How did the dispute affect English mathematics for the next century?"),
    };
    L.test = {
        typed("What is the derivative of $x^2 e^x$ at $x = 1$?", "3 e", 0),
        typed("Evaluate the limit of $(x - sin(x))/x^3$ as $x$ approaches 0.", "1/6", 0),
        typed("What is ∫₀¹ x/(1 + x²) dx?", "ln(2)/2", 0),
        typed("The point on $y = x^2$ closest to $(0, 2)$ has what y-coordinate?", "3/2", 0),
        typed("Revolve $y = x^2$, 0 ≤ x ≤ 1, about the x-axis. What is the volume?", "pi/5", 0),
        typed("$F(x) = $ ∫₀ˣ e^(t²) dt. What is $F'(1)$?", "e", 0),
        choice("∫₂^∞ dx/(x (ln x)²)", {"converges", "diverges"}, 0, 0),
        typed("What is ∫₋₁⁰ dx/(x² + 2x + 2)?", "pi/4", 0),
    };
    return L;
}

// ==================================================================== Unit 7

LessonDef unit7() {
    LessonDef L = unit("cc-u7", kPart3, "Unit 7. Sequences, series and Taylor's theorem", "Convergence tests, power series, and functions as polynomials of infinite degree.");
    L.setup = [](App& a) {
        setup(a, "Unit 7: sequences, series and Taylor's theorem",
              {{"H(x) = sum(k, 1, floor(x), 1/k)", BLUE_C, "harmonic partial sums"},          // 0
               {"ln(x) + 0.5772", GREY, "ln n + \xCE\xB3"},                                    // 1
               {"B(x) = sum(k, 1, floor(x), 1/k^2)", GREEN_C, "partial sums of 1/k²"},          // 2
               {"pi^2/6", GREY, "\xCF\x80\xC2\xB2/6"},                                          // 3
               {"exp(x)", RED_C, ""},                                                           // 4
               {"T(x) = sum(k, 0, n, x^k/k!)", YELLOW_C, "Taylor polynomial of e\xCB\xA3"},     // 5
               {"1/(1 - x)", RED_C, ""},                                                        // 6
               {"G(x) = sum(k, 0, n, x^k)", YELLOW_C, "geometric partial sum"},                 // 7
               {"M(x) = 4 sum(k, 0, floor(x), (-1)^k/(2k + 1))", PURPLE_C, "4 \xC3\x97 Madhava's partial sums"},  // 8
               {"pi", GREY, "\xCF\x80"}},                                                       // 9
              {{"n", 3, 0, 20}}, 1, false);
    };
    UnitText u;
    u.bigIdea = "An infinite sum is the limit of its partial sums. Power series turn hard calculus problems into algebra; Taylor's theorem says how good a polynomial approximation is.";
    u.topics = "Sequences and the Monotone Convergence Theorem; series, geometric and telescoping; the divergence test; integral, comparison, limit comparison, ratio, root "
               "and alternating series tests; absolute and conditional convergence; Riemann's rearrangement theorem; power series, radius and interval of convergence; "
               "term-by-term differentiation and integration; Taylor polynomials and Taylor's theorem with the Lagrange remainder; the binomial series.";
    u.weeks = "5 weeks";
    u.people = "Oresme, Madhava, Gregory, Newton, Taylor, Euler, Cauchy, Abel";
    u.story = "Nicole Oresme proved the harmonic series diverges around 1350. In Kerala, Madhava of Sangamagrama (c. 1340–1425) found series for arctan, sine and cosine, "
              "recorded in Jyeṣṭhadeva's Yuktibhāṣā around 1530. Europe rediscovered them through James Gregory (1671), Newton's binomial series (1665) and "
              "Brook Taylor's Methodus incrementorum (1715).\n\n"
              "In 1734–35 Euler showed that 1 + 1/4 + 1/9 + ⋯ = π²/6, solving the Basel problem with bold, unjustified manipulations. Cauchy (1821) put convergence "
              "on solid ground. Abel wrote in 1826 that divergent series were an invention of the devil.\n\n"
              "Right: Madhava's series 4(1 − 1/3 + 1/5 − 1/7 + ⋯) creeping towards π.";
    u.read = "- **Math:** Spivak ch. 20 and 22–24 (Taylor polynomials, sequences, series, power series); OpenStax Volume 2, ch. 5–6.\n"
             "- **History:** Dunham, Journey Through Genius, ch. 8–9; George Gheverghese Joseph, The Crest of the Peacock, on the Kerala school; Julian Havil, Gamma (2003).\n"
             "- **Primary source:** Euler, Introduction to Analysis of the Infinite, Book I, ch. 10, where he sums 1/n².";
    u.prove = "- **(a)** The Monotone Convergence Theorem, from completeness.\n- **(b)** The ratio test.\n- **(c)** The alternating series test, with its error bound.\n"
              "- **(d)** Taylor's theorem with the Lagrange remainder.\n- **(e)** eˣ equals its Maclaurin series for every x.";
    u.testMinutes = 180;
    u.testName = "Unit 7 test";
    u.test = "**1.** Decide whether each sequence converges, and find its limit: (a) n^(1/n); (b) n!/nⁿ; (c) a₁ = √2, aₙ₊₁ = √(2 + aₙ). For (c), prove convergence before finding the limit.\n\n"
             "**2.** Prove the harmonic series diverges in two ways: Oresme's grouping, and the integral test.\n\n"
             "**3.** Classify each as absolutely convergent, conditionally convergent or divergent: (a) Σ n²/2ⁿ; (b) Σ (−1)ⁿ/√n; (c) Σ 1/(n ln n), n ≥ 2; (d) Σ n!/nⁿ; (e) Σ sin(1/n²).\n\n"
             "**4.** Find the radius and interval of convergence of Σ (x − 3)ⁿ/(n·2ⁿ).\n\n"
             "**5.** Starting from 1/(1 + t²) = 1 − t² + t⁴ − ⋯, derive Madhava's series for π/4. On what interval does the arctan series converge?\n\n"
             "**6.** Find the Maclaurin series of sin x, and prove it converges to sin x for every x using the Lagrange remainder.\n\n"
             "**7.** Use a Taylor series to compute ∫₀¹ e^(−x²) dx to within 0.001. Compare with your Simpson's-rule result from Unit 6.\n\n"
             "**8.** Find each sum: (a) Σ n/3ⁿ; (b) Σ 1/(n(n + 1)); (c) Σ (−1)ⁿ⁺¹/n.\n\n"
             "**9.** Use series to evaluate lim x→0 of (sin x − x + x³/6)/x⁵.\n\n"
             "**10.** History: Euler \"proved\" Σ 1/n² = π²/6 by treating sin x / x as a polynomial with infinitely many roots and factoring it. "
             "Reconstruct his argument, and identify the step that was unjustified in 1735.";
    L.steps = {
        overview(u, [](StepCtx& s) { graph(s, 1, {0, 1}, -1, 60, 0, 5); }),
        story(u, [](StepCtx& s) { graph(s, 1, {8, 9}, -0.5, 40, 2.8, 4.2); }),
        {"Partial sums",
         "The harmonic series 1 + 1/2 + 1/3 + ⋯ diverges, but only like $ln(n)$ (grey): a million terms add up to about 14. Oresme's proof groups the terms "
         "into blocks each worth at least 1/2.\n\n"
         "The squares 1 + 1/4 + 1/9 + ⋯ converge. Euler found the limit: $pi^2/6$.",
         [](StepCtx& s) {
             graph(s, 1, {2, 3}, -1, 40, 0.9, 1.75);
             s.ask("What is 1 + 1/2 + 1/4 + 1/8 + ⋯ ?", "2");
         }},
        {"Taylor polynomials",
         "Match the value and derivatives of $e^x$ at 0 and you get $T(x) = 1 + x + x^2/(2!) + x^3/(3!)$ + ⋯ + $x^n/(n!)$. Watch $n$ grow: the polynomial hugs the curve over a wider "
         "and wider stretch. Taylor's theorem bounds the error by the next term's size.",
         [](StepCtx& s) {
             graph(s, 1, {4, 5}, -6, 4, -2, 20);
             s.param("n", 1);
             s.ask("What is the coefficient of x³ in the Maclaurin series of eˣ?", "1/6");
         }},
        {"More terms",
         "With 12 terms the polynomial is indistinguishable from $e^x$ across the view. For $e^x$ the error goes to 0 for every $x$: the series converges everywhere.",
         [](StepCtx& s) {
             s.param("n", 12);
             s.dur = 4.f;
         }},
        {"Radius of convergence",
         "Not every series converges everywhere. 1 + x + x² + ⋯ equals $1/(1 - x)$ only for $|x| < 1$: its **radius of convergence** is 1. "
         "Watch more terms improve the fit inside and blow up outside.",
         [](StepCtx& s) {
             graph(s, 1, {6, 7}, -2, 2, -3, 8);
             s.param("n", 2);
         }},
        {"Inside and outside",
         "With 20 terms the agreement inside $(-1, 1)$ is excellent, and outside the partial sums run off to infinity. The ratio test finds the radius.",
         [](StepCtx& s) {
             s.param("n", 20);
             s.dur = 4.f;
             s.ask("What is the radius of convergence of Σ xᵏ/3ᵏ?", "3");
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is Σ 1/(k(k + 1)) for k from 1 to ∞?", "1", 3),
        choice("Σ 1/kᵖ converges exactly when", {"p > 1", "p ≥ 1", "p > 0", "p > 2"}, 0, 3),
        typed("What is 1 − 1/2 + 1/3 − 1/4 + ⋯ ?", "ln(2)", 3),
        typed("What is the radius of convergence of Σ xᵏ/k?", "1", 7),
        typed("What is the coefficient of x⁴ in the Maclaurin series of cos x?", "1/24", 5),
        typed("What is the limit of n^(1/n) as n grows?", "1", 3),
    };
    return L;
}

// ==================================================================== Unit 8

LessonDef unit8() {
    LessonDef L = unit("cc-u8", kPart3, "Unit 8. Parametric curves, polar coordinates and Euler's formula", "The cycloid, the brachistochrone, polar areas, and e^(iθ).");
    L.setup = [](App& a) {
        setup(a, "Unit 8: parametric curves, polar coordinates and Euler's formula", {{"x", GREY, ""}},
              {{"s", 0, 0, 12.57f}, {"k", 0.6f, 0, 0.95f}}, 1, false);
    };
    UnitText u;
    u.bigIdea = "Curves need not be graphs of functions. Describing them by a parameter leads to the cycloid, the brachistochrone and e^(iθ) = cos θ + i sin θ.";
    u.topics = "Parametric curves, their tangents and arc length; the cycloid; polar coordinates, with area and arc length in polar form; conic sections in polar form, "
               "ready for Kepler in Unit 9; complex numbers and Euler's formula from power series; De Moivre's theorem and roots of complex numbers.";
    u.weeks = "2 weeks";
    u.people = "Pascal, Roberval, Wren, Huygens, Johann Bernoulli, Newton, Cotes, Euler";
    u.story = "The cycloid caused so many quarrels it was called \"the Helen of geometers.\" Roberval found its area, Christopher Wren its length (1658), and Huygens "
              "built a cycloidal pendulum that keeps perfect time (1673). In June 1696 Johann Bernoulli challenged Europe to find the curve of fastest descent.\n\n"
              "Newton received the problem in January 1697 and solved it overnight. He published anonymously, and Bernoulli recognized the author \"as the lion by its claw.\" "
              "The answer was again the cycloid. Roger Cotes found an equivalent of Euler's formula in 1714; Euler gave it its modern form in the Introductio (1748).";
    u.read = "- **Math:** Simmons, chapters on polar coordinates and parametric equations; OpenStax Volume 2, ch. 7; Spivak ch. 25–26 (complex numbers and functions), optional.\n"
             "- **History:** Paul Nahin, Dr. Euler's Fabulous Formula (2006) and An Imaginary Tale (1998); Nahin, When Least Is Best, on the brachistochrone.\n"
             "- **Primary source:** Johann Bernoulli's brachistochrone challenge, in Struik's Source Book.";
    u.prove = "- **(a)** The polar area formula A = ½∫ r² dθ.\n- **(b)** One arch of a cycloid of radius r has length 8r, as Wren found.\n"
              "- **(c)** Euler's formula, from the series for eˣ, sin x and cos x.";
    u.testName = "Unit 8 test";
    u.test = "**1.** For the cycloid x = r(t − sin t), y = r(1 − cos t), find dy/dx, the area under one arch, and the length of one arch.\n\n"
             "**2.** Sketch the cardioid r = 1 + cos θ and find the area it encloses.\n\n"
             "**3.** Find the area inside r = 3 sin θ and outside r = 1 + sin θ.\n\n"
             "**4.** Convert r = 6/(2 + cos θ) to Cartesian form, identify the conic, and give its eccentricity.\n\n"
             "**5.** Use e^(iθ) to express cos 3θ and sin 3θ in terms of cos θ and sin θ.\n\n"
             "**6.** Compute e^(iπ) and (1 + i)⁸ using polar form, and find all cube roots of 8i.\n\n"
             "**7.** History: State the brachistochrone problem, explain why the cycloid answer surprised people, and explain why the challenge launched the calculus of variations.";
    auto cycloid = [](StepCtx& s) {
        graph(s, 1, {}, -0.6, 13.2, -0.8, 3.2);
        curve(s, 1, "t - sin(t)", "1 - cos(t)", "0", "4 pi", "s");
    };
    L.steps = {
        overview(u, [cycloid](StepCtx& s) { cycloid(s); s.param("s", 2); }),
        story(u),
        {"The cycloid",
         "A point on the rim of a rolling wheel of radius 1 traces $x = t - sin(t)$, $y = 1 - cos(t)$. The green arrow is its velocity; at the bottom of "
         "each arch the point stops for an instant. Wren showed one arch has length exactly 8.",
         [cycloid](StepCtx& s) {
             cycloid(s);
             s.param("s", 12.566);
             s.dur = 6.f;
             s.ask("What is the area under one arch of the cycloid with r = 1? (Roberval)", "3 pi", "∫ y dx = ∫₀^2π (1 − cos t)² dt");
         }},
        {"Polar curves",
         "In polar form a curve gives the distance $r$ for each angle $theta$. The cardioid $r = 1 + cos(theta)$ is drawn here as $x = r cos(theta)$, "
         "$y = r sin(theta)$. Its area comes from thin wedges: $1/2 r^2 d theta$.",
         [](StepCtx& s) {
             graph(s, 1, {}, -1, 2.4, -1.4, 1.4);
             curve(s, 1, "(1 + cos(t)) cos(t)", "(1 + cos(t)) sin(t)", "0", "2 pi", "s");
             s.param("s", 1.2);
             s.ask("What area does the cardioid r = 1 + cos \xCE\xB8 enclose?", "3 pi/2");
         }},
        {"Conics in polar form",
         "With the focus at the origin, $r = 1/(1 + k cos(theta))$ is an ellipse for 0 ≤ k < 1, a parabola at $k = 1$ and a hyperbola beyond. $k$ is the "
         "**eccentricity**. Planets follow exactly these curves: Unit 9 proves it.",
         [](StepCtx& s) {
             graph(s, 1, {}, -5.6, 1.4, -2.3, 2.3);
             curve(s, 1, "cos(t)/(1 + k cos(t))", "sin(t)/(1 + k cos(t))", "0", "2 pi", "s");
             s.param("k", 0.8).param("s", 2);
             s.dur = 3.f;
         }},
        {"Euler's formula",
         "Put $i theta$ into the series for $e^x$, separate the even and odd powers, and the series for cosine and sine appear:\n\n"
         "$$e^(i theta) = cos(theta) + i sin(theta)$$\n\n"
         "So $e^(i theta)$ is the point at angle $theta$ on the unit circle. At $theta = pi$: $e^(i pi) = -1$. Multiplying complex numbers adds their angles: "
         "that is De Moivre's theorem.",
         [](StepCtx& s) {
             clean(s, 1);
             KeyPane& p = s.pane(1);
             p.unitCircle = true;
             p.circleOpts = kCircSinCos | kCircArc | kCircSnap;
             s.fit(1, -1.7, 1.7, -1.35, 1.35);
             s.param("theta", PI);
             s.dur = 3.f;
             s.ask("What is the real part of (1 + i)⁴?", "-4", "1 + i = \xE2\x88\x9A" "2 e^(i\xCF\x80/4)");
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What area does the circle $r = 2 cos(theta)$ enclose?", "pi", 4),
        typed("How long is one arch of a cycloid traced by a wheel of radius 2?", "16", 3),
        typed("What is the real part of $e^(i pi)$?", "-1", 6),
        typed("What is $|(1 + i)^8|$?", "16", 6),
        typed("$r = 4 sin(theta)$ is a circle. What is its radius?", "2", 4),
        choice("The curve of fastest descent (the brachistochrone) is", {"a cycloid", "a straight line", "a circular arc", "a parabola"}, 0, 1),
    };
    return L;
}

}  // namespace

std::vector<LessonDef> calculusCourse2() { return {unit4(), unit5(), unit6(), midterm(), unit7(), unit8()}; }
