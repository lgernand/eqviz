// Calculus course (from the "Calculus: From Limits to Differential Equations" plan):
// the placement test and Part I, Units 0-3.
#include "lesson_kit.hpp"

using namespace theme;
using namespace kit;

namespace {

// ==================================================================== placement

LessonDef placement() {
    LessonDef L = unit("cc-placement", kStart, "Course guide and placement test",
                       "How the course works, the books, the course map, and the placement test that decides where you start.");
    L.setup = [](App& a) {
        setup(a, "Calculus course: placement",
              {{"f(x) = x^2", YELLOW_C, ""}, {"s(x) = f(a) + (f(a + h) - f(a))/h (x - a)", BLUE_C, "secant line"}},
              {{"a", 1, -2, 2.5f}, {"h", 1.2f, 0.001f, 2}}, 1, false);
    };
    L.steps = {
        {"How this course works",
         "You will work through **18 units in five parts**, about 68 weeks at 8\xE2\x80\x93" "10 hours a week, from the derivative to differential equations. "
         "Each unit ends with a closed-book test that you send to Claude for grading; **85% moves you on**.\n\n"
         "**Each unit, in order**\n\n"
         "- Read the story first: whose problem you are about to solve, and why it was hard.\n"
         "- Read the math with a pencil, reworking each example before its solution.\n"
         "- Work 30\xE2\x80\x93" "60 problems from the spine text and OpenStax.\n"
         "- Prove the milestone theorems closed-book until you can reproduce them cold.\n"
         "- Take the written unit test in one timed sitting, and send it in.\n\n"
         "**Working with Claude.** Ask anything, any time. Say \"hint 1\" (a nudge), \"hint 2\" (the key idea) or \"hint 3\" (the first step worked). "
         "Each problem scores 0\xE2\x80\x93" "4: 4 complete and correct, 3 a minor slip, 2 right idea with a real error, 1 some progress, 0 nothing usable. "
         "75\xE2\x80\x93" "84% means a short retest; below 75%, targeted re-study and a fresh test.\n\n"
         "In this app every unit also has live graphs and a quick check with fresh problems, so the written tests stay sealed.",
         [](StepCtx& s) {
             graph(s, 1, {0, 1}, -2.2, 3, -1, 7);
             s.pane(1).trace = "a";
         }},
        text("The library",
             "Simmons is the spine because he writes the history into the math. Each unit names its exact readings.\n\n"
             "**Core texts**\n\n"
             "- **Spine, Parts I\xE2\x80\x93IV:** George F. Simmons, Calculus with Analytic Geometry, 2nd ed. (1996)\n"
             "- **Rigor, Parts I\xE2\x80\x93III:** Michael Spivak, Calculus, 4th ed. (2008): the milestone proofs and the hardest problems\n"
             "- **Spine, Part V:** Simmons, Differential Equations with Applications and Historical Notes, 3rd ed. (2016)\n"
             "- **Drill, Part V:** Boyce, DiPrima & Meade, Elementary Differential Equations and Boundary Value Problems, 11th ed. (2017)\n"
             "- **Units 13, 16\xE2\x80\x93" "17:** Steven Strogatz, Nonlinear Dynamics and Chaos, 2nd ed. (2015)\n"
             "- **Unit 12:** H. M. Schey, Div, Grad, Curl, and All That, 4th ed. (2005)\n"
             "- **Unit 16:** Gilbert Strang, Introduction to Linear Algebra, 6th ed. (2023)\n"
             "- **Placement gaps only:** Simmons, Precalculus Mathematics in a Nutshell (1981)\n\n"
             "**Free:** OpenStax Calculus Volumes 1\xE2\x80\x93" "3; MIT OpenCourseWare 18.01, 18.02, 18.03; 3Blue1Brown's Essence of Calculus and "
             "Differential Equations; Paul's Online Math Notes; Keisler's Elementary Calculus: An Infinitesimal Approach; the Euler Archive and "
             "Ian Bruce's translations at 17centurymaths.com.\n\n"
             "**History shelf:** Edwards, The Historical Development of the Calculus (closest to this course); Dunham, The Calculus Gallery; "
             "Boyer, The History of the Calculus; Strogatz, Infinite Powers; Dunham, Journey Through Genius; Alexander, Infinitesimal; "
             "Bardi, The Calculus Wars; Westfall, Never at Rest (Newton); Antognazza, Leibniz; Maor, e: The Story of a Number; "
             "Dunham, Euler: The Master of Us All; Kline, Mathematical Thought from Ancient to Modern Times; Struik, A Source Book in Mathematics, 1200\xE2\x80\x93" "1800.\n\n"
             "Editions and years are from the plan's author's memory: check the current edition before buying."),
        text("Course map",
             "The course follows history closely: derivatives from the 1600s, series from the 1700s, vector calculus from the 1800s, chaos from 1963.\n\n"
             "- **Placement** (1 week)\n"
             "- **Unit 0** Bridge: rigorous limits, only if needed (0\xE2\x80\x93" "2 weeks): Zeno, Eudoxus, Bolzano, Cauchy, Weierstrass\n"
             "- **Part I** Units 1\xE2\x80\x93" "3: the derivative; chain rule and transcendental functions; applications (10 weeks)\n"
             "- **Part II** Units 4\xE2\x80\x93" "6: the integral and the Fundamental Theorem; applications; techniques and improper integrals; then the **midterm** (11 weeks)\n"
             "- **Part III** Units 7\xE2\x80\x93" "8: sequences, series and Taylor's theorem; parametric curves, polar coordinates and Euler's formula (7 weeks)\n"
             "- **Part IV** Units 9\xE2\x80\x93" "12: vectors and motion; partial derivatives; multiple integrals; vector calculus (14 weeks)\n"
             "- **Part V** Units 13\xE2\x80\x93" "18: first-order equations; second-order equations; the Laplace transform; systems and the phase plane; "
             "nonlinear dynamics and chaos; series solutions and Fourier series; then the **final** (25 weeks)\n\n"
             "Pace: 68 weeks at 8\xE2\x80\x93" "10 hours a week, or about 10 months at 15 hours. Mastery sets the pace, not the calendar."),
        text("The placement test",
             "Take this before anything else: **26 problems, about 2\xC2\xBD hours, closed book, no calculator.** "
             "Sections A\xE2\x80\x93" "D decide where you start; Section E checks whether you have seen any calculus; Section F tells Claude how to teach you. "
             "Show all work and send your answers in your Calculus mastery chat. In Section E, skip anything you have not seen: a blank there is information, not a failure.\n\n"
             "**Where you start** (Sections A\xE2\x80\x93" "D: 22 problems scored 0\xE2\x80\x93" "4, 88 points)\n\n"
             "- A or B under 80%: one to two weeks of review from Precalculus Mathematics in a Nutshell, then Unit 0 or Unit 1\n"
             "- A and B at 80%+, D under 75%: Unit 0, the bridge on rigorous limits, then Unit 1\n"
             "- A\xE2\x80\x93" "D all at 80%+: Unit 1\n"
             "- A\xE2\x80\x93" "D at 80%+ and Section E solid: Unit 1 as a fast review, one week, then its test as the gate"),
        text("Placement: algebra and trigonometry (written)",
             "**A. Algebra and functions**\n\n"
             "**1.** Simplify (x\xE2\x81\xBB\xC2\xB2 \xE2\x88\x92 y\xE2\x81\xBB\xC2\xB2) / (x\xE2\x81\xBB\xC2\xB9 \xE2\x88\x92 y\xE2\x81\xBB\xC2\xB9).\n\n"
             "**2.** Solve each inequality and give the answer in interval notation: (a) |2x \xE2\x88\x92 3| < 5; (b) x\xC2\xB2 \xE2\x88\x92 x \xE2\x88\x92 6 \xE2\x89\xA5 0.\n\n"
             "**3.** Rationalize the numerator and simplify: (\xE2\x88\x9A(x + h) \xE2\x88\x92 \xE2\x88\x9Ax) / h.\n\n"
             "**4.** Let f(x) = \xE2\x88\x9A(x \xE2\x88\x92 1) and g(x) = x\xC2\xB2 + 1. Find f\xE2\x88\x98g and g\xE2\x88\x98" "f, simplified, with their domains.\n\n"
             "**5.** Find the inverse of f(x) = (2x + 1)/(x \xE2\x88\x92 3), and state the domain and range of the inverse.\n\n"
             "**6.** Solve exactly: (a) 2^(x+1) = 5^x; (b) log\xE2\x82\x82(x) + log\xE2\x82\x82(x \xE2\x88\x92 2) = 3.\n\n"
             "**B. Trigonometry**\n\n"
             "**7.** Give exact values: sin(5\xCF\x80/6), cos(\xE2\x88\x92" "2\xCF\x80/3), tan(7\xCF\x80/4), sec(\xCF\x80/3).\n\n"
             "**8.** Solve 2 sin\xC2\xB2x \xE2\x88\x92 sin x \xE2\x88\x92 1 = 0 on [0, 2\xCF\x80).\n\n"
             "**9.** Prove the identity (1 \xE2\x88\x92 cos 2\xCE\xB8) / sin 2\xCE\xB8 = tan \xCE\xB8.\n\n"
             "**10.** Derive cos(a \xE2\x88\x92 b) = cos a cos b + sin a sin b by any method, then use it to find cos 15\xC2\xB0 exactly.\n\n"
             "**11.** If sin \xCE\xB8 = 3/5 and \xCE\xB8 is in quadrant II, find cos \xCE\xB8, tan \xCE\xB8 and sin 2\xCE\xB8.\n\n"
             "**12.** Simplify sin(arccos x) for \xE2\x88\x92" "1 \xE2\x89\xA4 x \xE2\x89\xA4 1, and explain the sign you chose."),
        text("Placement: limits and rigor (written)",
             "**C. Limits**\n\n"
             "**13.** Evaluate: (a) lim x\xE2\x86\x92" "2 of (x\xC2\xB3 \xE2\x88\x92 8)/(x \xE2\x88\x92 2); (b) lim x\xE2\x86\x92" "0 of (\xE2\x88\x9A(1 + x) \xE2\x88\x92 1)/x; "
             "(c) lim x\xE2\x86\x92\xE2\x88\x9E of (3x\xC2\xB2 \xE2\x88\x92 x)/(5x\xC2\xB2 + 2); (d) lim x\xE2\x86\x92\xE2\x88\x9E of (\xE2\x88\x9A(x\xC2\xB2 + x) \xE2\x88\x92 x).\n\n"
             "**14.** Evaluate lim x\xE2\x86\x92" "0 of sin(5x)/sin(3x), and lim x\xE2\x86\x92" "0 of (1 \xE2\x88\x92 cos x)/x\xC2\xB2.\n\n"
             "**15.** Find both one-sided limits at x = 1 of f(x) = |x \xE2\x88\x92 1|/(x \xE2\x88\x92 1) and of g(x) = 1/(x \xE2\x88\x92 1).\n\n"
             "**16.** Use the squeeze theorem to prove lim x\xE2\x86\x92" "0 of x\xC2\xB2 cos(1/x) = 0.\n\n"
             "**17.** Prove that lim \xCE\xB8\xE2\x86\x92" "0 of (sin \xCE\xB8)/\xCE\xB8 = 1. Give the whole argument: the geometric picture, the inequality, the squeeze, and why it also holds as \xCE\xB8\xE2\x86\x92" "0 from the left.\n\n"
             "**18.** Find all vertical and horizontal asymptotes and any holes of f(x) = (2x\xC2\xB2 \xE2\x88\x92 2)/(x\xC2\xB2 \xE2\x88\x92 3x + 2).\n\n"
             "**D. Continuity and rigor**\n\n"
             "**19.** State the \xCE\xB5\xE2\x80\x93\xCE\xB4 definition of lim x\xE2\x86\x92" "a of f(x) = L. Then prove from the definition that lim x\xE2\x86\x92" "4 of (3x \xE2\x88\x92 5) = 7.\n\n"
             "**20.** Prove from the definition that lim x\xE2\x86\x92" "2 of x\xC2\xB2 = 4.\n\n"
             "**21.** Let f(x) = (x\xC2\xB2 \xE2\x88\x92 9)/(x \xE2\x88\x92 3) for x \xE2\x89\xA0 3 and f(3) = c. Which c makes f continuous at 3? Then state the Intermediate Value Theorem and use it to show cos x = x has a solution in [0, \xCF\x80/2].\n\n"
             "**22.** Find a function that is continuous at x = 0 and discontinuous at every other point. Justify both claims."),
        text("Placement: look-ahead and about you",
             "**E. Look-ahead (skip what you have not seen)**\n\n"
             "**23.** Using the limit definition of the derivative, find the slope of the tangent line to y = x\xC2\xB2 at x = 3.\n\n"
             "**24.** Differentiate x\xC2\xB3 sin x and e^(2x).\n\n"
             "**25.** Evaluate the integral of x\xC2\xB2 from 0 to 1.\n\n"
             "**26.** Find the sum 1 + 1/2 + 1/4 + 1/8 + \xE2\x8B\xAF\n\n"
             "**F. About you (not graded)**\n\n"
             "- Why do you want calculus: physics, engineering, computing, pure mathematics, or curiosity?\n"
             "- How many hours a week can you honestly give it?\n"
             "- Which of the core texts do you already own, or can get?\n"
             "- Have you seen derivatives or integrals before, even informally?\n"
             "- How will you submit work: typed, LaTeX, or photos?\n"
             "- In the history, what pulls you most: the people's lives, how the ideas evolved, or reading the original texts?\n\n"
             "The warm-up check that follows uses different problems from the placement test, so you can take it first."),
    };
    L.test = {
        typed("Give the exact value of $sin((7 pi)/6)$.", "-1/2", 4),
        typed("Solve $|x - 1| < 3$. What is the right end of the interval?", "4", 4),
        typed("Solve $log_3(x) + log_3(x - 8) = 2$.", "9", 4),
        typed("If $cos(theta) = 5/13$ and $theta$ is in quadrant IV, what is $sin(theta)$?", "-12/13", 4),
        typed("Evaluate the limit of $(x^2 - 9)/(x - 3)$ as $x$ approaches 3.", "6", 5),
        typed("Evaluate the limit of $sin(4 x)/x$ as $x$ approaches 0.", "4", 5),
        typed("Evaluate the limit of $(2 x^2 + 1)/(x^2 - 5)$ as $x$ grows without bound.", "2", 5),
        choice("To show $x sin(1/x)$ approaches 0 at 0 with the squeeze theorem, bound it between", {"$-|x|$ and $|x|$", "$-1$ and $1$", "$0$ and $x^2$", "$sin(x)$ and $x$"}, 0, 5),
    };
    L.passPct = 80;
    return L;
}

// ==================================================================== Unit 0

LessonDef unit0() {
    LessonDef L = unit("cc-u0", kPart1, "Unit 0. Bridge: rigorous limits", "The \xCE\xB5\xE2\x80\x93\xCE\xB4 definition, continuity, and the Intermediate and Extreme Value Theorems.");
    L.setup = [](App& a) {
        setup(a, "Unit 0: rigorous limits",
              {{"f(x) = 3x - 5", YELLOW_C, ""},                    // 0
               {"7 + epsilon", GREY, "L + \xCE\xB5"},                  // 1
               {"7 - epsilon", GREY, "L \xE2\x88\x92 \xCE\xB5"},       // 2
               {"g(x) = x^2", YELLOW_C, ""},                       // 3
               {"4 + epsilon", GREY, "L + \xCE\xB5"},                  // 4
               {"4 - epsilon", GREY, "L \xE2\x88\x92 \xCE\xB5"},       // 5
               {"w(x) = x sin(1/x)", GREEN_C, ""},                 // 6
               {"abs(x)", GREY, "|x|"},                            // 7
               {"-abs(x)", GREY, "\xE2\x88\x92|x|"},               // 8
               {"c(x) = cos(x) - x", BLUE_C, ""}},                 // 9
              {{"epsilon", 1, 0.01f, 2}, {"delta", 0.333f, 0.001f, 1}, {"a", 0.739f, 0, 1.5f}}, 1, false);
    };
    UnitText u;
    u.bigIdea = "A limit is a precise promise: for any tolerance \xCE\xB5 you name, I can find a \xCE\xB4 that guarantees it.";
    u.topics = "The \xCE\xB5\xE2\x80\x93\xCE\xB4 definition; proofs of the limit laws; one-sided and infinite limits; limits at infinity; continuity; "
               "completeness of the real numbers; the Intermediate and Extreme Value Theorems.";
    u.weeks = "0\xE2\x80\x93" "2 weeks, only if placement calls for it";
    u.people = "Zeno, Eudoxus, Berkeley, Bolzano, Cauchy, Weierstrass";
    u.story = "Zeno of Elea (c. 450 BC) argued that motion is impossible because it means completing infinitely many steps. Eudoxus of Cnidus "
              "(c. 370 BC) avoided the infinite with his **method of exhaustion**, the ancestor of \xCE\xB5\xE2\x80\x93\xCE\xB4.\n\n"
              "In The Analyst (1734), Bishop George Berkeley mocked Newton's vanishing increments as \"ghosts of departed quantities.\" "
              "The answer took a century: Bolzano (1817), Cauchy's Cours d'analyse (1821), and Weierstrass's \xCE\xB5\xE2\x80\x93\xCE\xB4 lectures in Berlin in the 1850s and 60s.";
    u.read = "- **Math:** Spivak ch. 5\xE2\x80\x93" "8 (Limits; Continuous Functions; Three Hard Theorems; Least Upper Bounds).\n"
             "- **History:** Judith Grabiner, The Origins of Cauchy's Rigorous Calculus (1981).\n"
             "- **Primary source:** Berkeley, The Analyst (1734), free online.";
    u.prove = "- **(a)** The sum and product laws for limits.\n"
              "- **(b)** A continuous function on [a, b], negative at a and positive at b, has a zero; use least upper bounds.\n"
              "- **(c)** A function continuous on [a, b] is bounded there.";
    u.testMinutes = 90;
    u.testName = "Unit 0 test";
    u.test = "**1.** Prove with \xCE\xB5\xE2\x80\x93\xCE\xB4 that lim x\xE2\x86\x92" "3 of (2x\xC2\xB2 \xE2\x88\x92 x) = 15.\n\n"
             "**2.** Prove with \xCE\xB5\xE2\x80\x93\xCE\xB4 that lim x\xE2\x86\x92" "0 of x sin(1/x) = 0.\n\n"
             "**3.** Prove: if f(x) \xE2\x86\x92 L and g(x) \xE2\x86\x92 M as x\xE2\x86\x92" "a, then f(x) + g(x) \xE2\x86\x92 L + M.\n\n"
             "**4.** Show that x\xE2\x81\xB5 \xE2\x88\x92 3x + 1 = 0 has at least three real roots.\n\n"
             "**5.** Give a function continuous on (0, 1] that has no maximum there. Which hypothesis of the Extreme Value Theorem fails?\n\n"
             "**6.** History: What exactly did Berkeley object to in Newton's fluxions, and how does the \xCE\xB5\xE2\x80\x93\xCE\xB4 definition answer him?";
    auto band = [](StepCtx& s) {
        graph(s, 1, {0, 1, 2}, 2.2, 5.8, 2.5, 11.5);
        shade(s, 1, 0, "4 - delta", "4 + delta", true);
        s.param("epsilon", 1).param("delta", 0.333);
    };
    L.steps = {
        overview(u, band),
        story(u),
        {"The \xCE\xB5\xE2\x80\x93\xCE\xB4 game",
         "To prove $3 x - 5$ approaches 7 as $x$ approaches 4, play a game. A challenger names a tolerance $epsilon$: the grey lines 7 \xC2\xB1 $epsilon$. "
         "You must answer with a $delta$ so that every $x$ within $delta$ of 4 (the shaded strip) lands between the grey lines.\n\n"
         "Here $|f(x) - 7| = 3 |x - 4|$, so $delta = epsilon/3$ always wins. Watch the challenger tighten $epsilon$ and the strip follow.",
         [](StepCtx& s) {
             s.param("epsilon", 0.3).param("delta", 0.1);
             s.dur = 3.5f;
             s.ask("For \xCE\xB5 = 0.6, what is the largest \xCE\xB4 that works for 3x \xE2\x88\x92 5 at x = 4?", "0.2", "|f(x) \xE2\x88\x92 7| = 3|x \xE2\x88\x92 4|");
         }},
        {"A curved example",
         "For $x^2$ at 2: $|x^2 - 4| = |x - 2| |x + 2|$. Keep $|x - 2| < 1$ so that $|x + 2| < 5$; then $|x^2 - 4| < 5 |x - 2|$. "
         "So $delta$ = the smaller of 1 and $epsilon/5$ works.\n\n"
         "This \"first restrict, then bound\" trick is the pattern for almost every \xCE\xB5\xE2\x80\x93\xCE\xB4 proof.",
         [](StepCtx& s) {
             graph(s, 1, {3, 4, 5}, 1.2, 2.8, 2.3, 5.9);
             shade(s, 1, 3, "2 - delta", "2 + delta", true);
             s.param("epsilon", 0.5).param("delta", 0.1);
         }},
        {"Squeezing",
         "$x sin(1/x)$ wiggles infinitely often near 0, but it is trapped between $-|x|$ and $|x|$, both of which go to 0. "
         "So it goes to 0 too: given $epsilon$, take $delta = epsilon$.\n\n"
         "Zoom in on the origin: the wiggles never escape the funnel.",
         [](StepCtx& s) { graph(s, 1, {6, 7, 8}, -0.45, 0.45, -0.4, 0.4); }},
        {"Intermediate values",
         "A continuous function cannot jump. $c(x) = cos(x) - x$ is positive at 0 and negative at $pi/2$, so somewhere between it is exactly 0: "
         "the **Intermediate Value Theorem**. The dashed line marks that root, about 0.739.\n\n"
         "The proof needs the **completeness** of the real numbers: every bounded set has a least upper bound. Over the rationals the theorem fails.",
         [](StepCtx& s) {
             graph(s, 1, {9}, -0.4, 1.9, -2, 1.4);
             s.pane(1).trace = "a";
             s.param("a", 0.739);
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("For $f(x) = 5 x + 1$ at $x = 2$ (limit 11), what is the largest $delta$ that guarantees $|f(x) - 11| < 0.5$?", "0.1", 2),
        typed("If $|x - 3| < 1$, what is the largest value $|x + 3|$ can take?", "7", 3),
        choice("$x^3 + x - 1 = 0$ has a root in which interval?", {"[0, 1]", "[1, 2]", "[2, 3]", "[\xE2\x88\x92" "2, \xE2\x88\x92" "1]"}, 0, 5),
        choice("$1/x$ on (0, 1] has no maximum. Which hypothesis of the Extreme Value Theorem fails?",
               {"the interval is not closed", "1/x is not continuous on (0, 1]", "the interval is too short", "1/x is not differentiable"}, 0, 5),
        typed("What is the limit of $x^2 sin(1/x)$ as $x$ approaches 0?", "0", 4),
    };
    return L;
}

// ==================================================================== Unit 1

LessonDef unit1() {
    LessonDef L = unit("cc-u1", kPart1, "Unit 1. The derivative", "Tangents and velocity as one limit; the rules; derivatives of sine and cosine.");
    L.setup = [](App& a) {
        setup(a, "Unit 1: the derivative",
              {{"f(x) = x^3 - 3x", YELLOW_C, ""},                                   // 0
               {"s(x) = f(a) + (f(a + h) - f(a))/h (x - a)", BLUE_C, "secant line"},  // 1
               {"k(x) = abs(x - 2)", GREEN_C, ""},                                  // 2
               {"sin(x)", RED_C, ""},                                               // 3
               {"w(x) = x^2 sin(1/x)", PURPLE_C, ""}},                              // 4
              {{"a", 1.5f, -2.5f, 2.5f}, {"h", 1, 0.001f, 1.5f}}, 1, false);
    };
    UnitText u;
    u.bigIdea = "The derivative is a limit of difference quotients, f\xE2\x80\xB2(a) = lim (f(a + h) \xE2\x88\x92 f(a))/h as h \xE2\x86\x92 0. One idea answers both the tangent problem and the velocity problem.";
    u.topics = "Tangent lines and instantaneous velocity; the definition; the derivative as a function; differentiability implies continuity, but not conversely; "
               "power, sum, product and quotient rules; derivatives of sin and cos from lim sin h / h = 1; higher derivatives; the notations dy/dx, \xE1\xBA\x8F and f\xE2\x80\xB2.";
    u.weeks = "3 weeks";
    u.people = "Fermat, Descartes, Barrow, Newton, Leibniz";
    u.story = "Pierre de Fermat found maxima and tangents around 1636 by setting f(x + e) \"adequal\" to f(x), dividing by e, then discarding e. "
              "Descartes's La G\xC3\xA9om\xC3\xA9trie (1637) married algebra to curves. Isaac Barrow found tangents geometrically and in 1669 gave up the "
              "Lucasian chair at Cambridge to his student, Newton.\n\n"
              "Newton built his **method of fluxions** in 1665\xE2\x80\x93" "66 at Woolsthorpe, while plague closed Cambridge. Leibniz invented his own calculus "
              "in Paris in 1675 and published first, in Acta Eruditorum in 1684. His dy/dx notation won because it computes for you.";
    u.read = "- **Math:** Simmons, chapters on the derivative and its computation; Spivak ch. 9\xE2\x80\x93" "10.\n"
             "- **History:** Edwards, chapters on Fermat, Barrow, Newton and Leibniz; Dunham, The Calculus Gallery, ch. 1\xE2\x80\x93" "2.\n"
             "- **Primary source:** Leibniz, \"Nova methodus pro maximis et minimis\" (1684), in Struik's Source Book.";
    u.prove = "- **(a)** Differentiability implies continuity.\n- **(b)** The product rule.\n- **(c)** d/dx sin x = cos x, from the definition.\n"
              "- **(d)** The power rule for positive integers, via the binomial theorem.";
    u.testName = "Unit 1 test";
    u.test = "**1.** Use the definition to find f\xE2\x80\xB2(x) for f(x) = \xE2\x88\x9A(2x + 1).\n\n"
             "**2.** Use the definition to find f\xE2\x80\xB2(x) for f(x) = 1/x\xC2\xB2.\n\n"
             "**3.** Prove that f(x) = |x \xE2\x88\x92 2| is continuous but not differentiable at x = 2.\n\n"
             "**4.** Find a and b so that f(x) = x\xC2\xB2 for x \xE2\x89\xA4 1 and f(x) = ax + b for x > 1 is differentiable everywhere.\n\n"
             "**5.** Derive the quotient rule from the product rule, assuming g is differentiable and nonzero.\n\n"
             "**6.** Starting from lim h\xE2\x86\x92" "0 of (sin h)/h = 1, prove that d/dx cos x = \xE2\x88\x92sin x.\n\n"
             "**7.** Differentiate: (a) (x\xC2\xB3 \xE2\x88\x92 2x)/(x\xC2\xB2 + 1); (b) x\xC2\xB2 sin x tan x; (c) sec x, using the quotient rule.\n\n"
             "**8.** Find the tangent line to y = x\xC2\xB3 \xE2\x88\x92 3x at x = 2, and every point where the tangent is horizontal.\n\n"
             "**9.** Let f(x) = x\xC2\xB2 sin(1/x) for x \xE2\x89\xA0 0 and f(0) = 0. Show that f is differentiable everywhere but f\xE2\x80\xB2 is not continuous at 0.\n\n"
             "**10.** History: Explain in modern terms what Fermat computed with \"adequality,\" and why discarding e troubled later critics.";
    L.steps = {
        overview(u, [](StepCtx& s) {
            graph(s, 1, {0, 1}, -2.6, 2.9, -3.2, 4.5);
            s.pane(1).trace = "a";
            s.param("a", 1.5).param("h", 1);
        }),
        story(u),
        {"From secant to tangent",
         "The blue secant through $x = a$ and $x = a + h$ has slope $(f(a + h) - f(a))/h$. Fermat's trick, made honest: let $h$ shrink. "
         "The secant settles onto the tangent line, and its slope onto the **derivative**.",
         [](StepCtx& s) {
             s.param("h", 0.001);
             s.dur = 5.f;
             s.ask("What is the slope of x\xC2\xB3 \xE2\x88\x92 3x at x = 1.5?", "3.75", "The derivative is 3x\xC2\xB2 \xE2\x88\x92 3.");
         }},
        {"The derivative as a function",
         "Do it at every point and the slopes form a new function, $f'(x) = 3 x^2 - 3$ (green). Drag the probe along the curve: the tangent's slope is the green height.\n\n"
         "Where $f'$ crosses zero the tangent is flat: x = \xC2\xB1" "1.",
         [](StepCtx& s) {
             graph(s, 1, {0}, -2.6, 2.9, -3.2, 4.5);
             s.pane(1).deriv = true;
             s.pane(1).probeX = 1.5;
         }},
        {"Continuous but not differentiable",
         "$|x - 2|$ has a corner at 2. Slopes on the left are $-1$, on the right $+1$, and the difference quotient has no single limit. "
         "Differentiable implies continuous, but not the other way round.",
         [](StepCtx& s) {
             graph(s, 1, {2}, -0.2, 4.2, -1.3, 2.2);
             s.pane(1).deriv = true;
             s.pane(1).probeX = 2.6;
         }},
        {"Sine's derivative is cosine",
         "Expand $sin(x + h) = sin(x) cos(h) + cos(x) sin(h)$, divide by $h$, and use the limits you proved with the squeeze theorem: "
         "$sin(h)/h$ goes to 1 and $(cos(h) - 1)/h$ goes to 0. What is left is $cos(x)$ (green).",
         [](StepCtx& s) {
             graph(s, 1, {3}, -7, 7, -1.7, 1.7);
             s.pane(1).deriv = true;
             s.pane(1).probeX = 1;
         }},
        {"A derivative that jumps around",
         "$x^2 sin(1/x)$ (with value 0 at 0) is differentiable everywhere, and its derivative at 0 is 0. But $f'(x) = 2 x sin(1/x) - cos(1/x)$ "
         "keeps swinging between $-1$ and $1$ near 0: a derivative need not be continuous. You will prove this on the unit test.",
         [](StepCtx& s) {
             graph(s, 1, {4}, -0.3, 0.3, -0.09, 0.09);
             s.pane(1).deriv = true;
             s.pane(1).probeX = 0.12;
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is the derivative of $x^4 - 2 x$ at $x = 1$?", "2", 3),
        typed("Using the definition, $f(x) = 1/x$. What is $f'(2)$?", "-1/4", 2),
        typed("What is the derivative of $x sin(x)$ at $x = pi/2$?", "1", 5),
        typed("The tangent to $y = x^2$ at $x = 3$ crosses the y-axis at what height?", "-9", 2),
        choice("At $x = 0$, $f(x) = |x|$ is", {"continuous and differentiable", "continuous but not differentiable", "differentiable but not continuous", "neither"}, 1, 4),
        typed("What is the derivative of $tan(x)$ at $x = 0$?", "1", 5),
    };
    return L;
}

// ==================================================================== Unit 2

LessonDef unit2() {
    LessonDef L = unit("cc-u2", kPart1, "Unit 2. Chain rule and transcendental functions", "Compositions, implicit and inverse functions, e and the logarithm.");
    L.setup = [](App& a) {
        setup(a, "Unit 2: chain rule and transcendental functions",
              {{"c(x) = sin(x^2)", YELLOW_C, ""},              // 0
               {"q(x) = (1 + 1/x)^x", BLUE_C, "(1 + 1/n)^n"},  // 1
               {"e", GREY, "e = 2.71828\xE2\x80\xA6"},         // 2
               {"exp(x)", YELLOW_C, ""},                        // 3
               {"ln(x)", GREEN_C, ""},                          // 4
               {"x", GREY, "y = x"},                            // 5
               {"g(x) = x exp(-x^2)", PURPLE_C, ""}},           // 6
              {}, 1, false);
    };
    UnitText u;
    u.bigIdea = "The chain rule differentiates compositions. With it, inverse functions, exponentials and logarithms all fall into place.";
    u.topics = "The chain rule, with a correct proof; implicit differentiation; derivatives of inverse functions; e as the limit of (1 + 1/n)\xE2\x81\xBF; "
               "derivatives of e\xCB\xA3 and ln x; general exponentials and logarithms; logarithmic differentiation; inverse trigonometric functions; hyperbolic functions.";
    u.weeks = "3 weeks";
    u.people = "Napier, Briggs, Saint-Vincent, Jacob Bernoulli, Euler";
    u.story = "John Napier published logarithms in 1614, before anyone wrote exponents the way we do; Henry Briggs then built base-10 tables. "
              "Jacob Bernoulli met e in 1683 while studying compound interest.\n\n"
              "Gr\xC3\xA9goire de Saint-Vincent and Alphonse Antonio de Sarasa (1647\xE2\x80\x93" "49) noticed that areas under the hyperbola y = 1/x behave like logarithms. "
              "Euler adopted the letter e by 1731. His Introductio in analysin infinitorum (1748) made the exponential function, not the logarithm, the basic object.";
    u.read = "- **Math:** Simmons, chapters on exponential, logarithmic and inverse trigonometric functions; Spivak ch. 10 (the chain rule proof) and ch. 12 (Inverse Functions).\n"
             "- **History:** Eli Maor, e: The Story of a Number; Dunham, Euler: The Master of Us All.\n"
             "- **Primary source:** Euler, Introduction to Analysis of the Infinite, Book I, ch. 6\xE2\x80\x93" "7 (trans. John Blanton, 1988).";
    u.prove = "- **(a)** The chain rule, including the case where the inner function's increment is zero.\n- **(b)** The derivative of an inverse function.\n"
              "- **(c)** d/dx e\xCB\xA3 = e\xCB\xA3, given lim h\xE2\x86\x92" "0 of (e\xCA\xB0 \xE2\x88\x92 1)/h = 1. (Unit 4 redoes logarithms rigorously, with ln x defined as an integral.)";
    u.testName = "Unit 2 test";
    u.test = "**1.** Differentiate: (a) sin\xC2\xB3(4x\xC2\xB2 + 1); (b) e^(x sin x); (c) ln|sec x + tan x|; (d) arctan(\xE2\x88\x9Ax).\n\n"
             "**2.** Find dy/dx if x\xC2\xB3 + y\xC2\xB3 = 6xy, and find the tangent line at (3, 3).\n\n"
             "**3.** Use logarithmic differentiation to differentiate x\xCB\xA3 and (x\xC2\xB2 + 1)^(sin x).\n\n"
             "**4.** Prove that d/dx arcsin x = 1/\xE2\x88\x9A(1 \xE2\x88\x92 x\xC2\xB2), and state where this holds.\n\n"
             "**5.** Explain why the \"proof\" that writes \xCE\x94y/\xCE\x94x = (\xCE\x94y/\xCE\x94u)(\xCE\x94u/\xCE\x94x) is incomplete. Then give a correct proof of the chain rule.\n\n"
             "**6.** Show that (1 + x/n)\xE2\x81\xBF \xE2\x86\x92 e\xCB\xA3 as n\xE2\x86\x92\xE2\x88\x9E. Hint: take logarithms and use the derivative of ln at 1.\n\n"
             "**7.** If f(2) = 3, f\xE2\x80\xB2(2) = 5 and g is the inverse of f, find g\xE2\x80\xB2(3).\n\n"
             "**8.** Find every x where the tangent to y = x e^(\xE2\x88\x92x\xC2\xB2) is horizontal.\n\n"
             "**9.** History: How did Napier compute logarithms without exponents, and what problems in astronomy and navigation made them worth the effort?";
    L.steps = {
        overview(u, [](StepCtx& s) {
            graph(s, 1, {0}, -3.2, 3.2, -1.6, 1.6);
        }),
        story(u, [](StepCtx& s) { graph(s, 1, {1, 2}, 0.5, 60, 1.9, 2.85); }),
        {"The chain rule",
         "$sin(x^2)$ is sine applied to $x^2$. Nudge $x$: the inner $x^2$ changes at rate $2 x$, and sine multiplies that by $cos(x^2)$. So\n\n"
         "$$d/(d x) sin(x^2) = 2 x cos(x^2)$$\n\n"
         "Rates multiply along a chain. The green curve is the derivative; drag the probe to compare.",
         [](StepCtx& s) {
             graph(s, 1, {0}, -3.2, 3.2, -6, 6);
             s.pane(1).deriv = true;
             s.pane(1).probeX = 1;
             s.ask("What is the derivative of sin(x\xC2\xB2) at x = 1?", "2 cos(1)", "2x cos(x\xC2\xB2) at x = 1");
         }},
        {"e as a limit",
         "Compound 100% interest $n$ times a year and a dollar grows to $(1 + 1/n)^n$. As $n$ grows this climbs towards a limit, "
         "Bernoulli's number e = 2.71828\xE2\x80\xA6 (grey). It is also the base whose exponential is its own derivative.",
         [](StepCtx& s) { graph(s, 1, {1, 2}, 0.5, 60, 1.9, 2.85); }},
        {"Inverse functions",
         "$ln(x)$ is the mirror image of $e^x$ in the line $y = x$. Mirroring swaps rise and run, so slopes invert: since the slope of $e^x$ is $e^x$, "
         "the slope of ln at $x$ is $1/x$.",
         [](StepCtx& s) {
             graph(s, 1, {3, 4, 5}, -3.2, 4.2, -3, 4);
             s.ask("What is the derivative of ln(x) at x = 4?", "1/4");
         }},
        {"Implicit curves",
         "Descartes's folium $x^3 + y^3 = 6 x y$ is not the graph of a function, yet it has tangents. Differentiate both sides, treating $y$ as a function of $x$:\n\n"
         "$$3 x^2 + 3 y^2 y' = 6 y + 6 x y'$$\n\n"
         "and solve for $y'$. At $(3, 3)$ the tangent has slope $-1$. The yellow dot travels round the curve.",
         [](StepCtx& s) {
             clean(s, 1);
             s.show(1, {});
             s.fit(1, -3.4, 4.4, -3.4, 4.4);
             curve(s, 1, "6 t/(1 + t^3)", "6 t^2/(1 + t^3)", "-0.55", "25", "1");
         }},
        {"Flat tangents",
         "For $g(x) = x e^(-x^2)$ the product and chain rules give $g'(x) = e^(-x^2)(1 - 2 x^2)$: flat where x = \xC2\xB1$1/sqrt(2)$.",
         [](StepCtx& s) {
             graph(s, 1, {6}, -3, 3, -0.7, 0.7);
             s.pane(1).deriv = true;
             s.pane(1).probeX = 0.7071;
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is the derivative of $sin(3 x)$ at $x = 0$?", "3", 2),
        typed("What is the derivative of $e^(2 x)$ at $x = 0$?", "2", 2),
        typed("What is the derivative of $ln(x^2 + 1)$ at $x = 1$?", "1", 2),
        typed("$f(1) = 2$, $f'(1) = 4$, and $g$ is the inverse of $f$. What is $g'(2)$?", "1/4", 4),
        typed("What is the derivative of $atan(x)$ at $x = 1$?", "1/2", 4),
        typed("What is the limit of $(1 + 3/n)^n$ as $n$ grows?", "e^3", 3),
    };
    return L;
}

// ==================================================================== Unit 3

LessonDef unit3() {
    LessonDef L = unit("cc-u3", kPart1, "Unit 3. Applications of the derivative", "The Mean Value Theorem, optimization, L'H\xC3\xB4pital's rule and Newton's method.");
    L.setup = [](App& a) {
        setup(a, "Unit 3: applications of the derivative",
              {{"f(x) = x^3 - 3x", YELLOW_C, ""},                                          // 0
               {"m(x) = f(a) + (f(b) - f(a))/(b - a) (x - a)", BLUE_C, "secant from a to b"}, // 1
               {"A(x) = 2 pi x^2 + 2000/x", GREEN_C, "surface area of the can, radius x"},    // 2
               {"N(x) = x^3 - 2x - 5", RED_C, ""},                                          // 3
               {"q(x) = (exp(x) - 1 - x)/x^2", PURPLE_C, ""},                               // 4
               {"y(x) = x exp(-x)", YELLOW_C, ""}},                                         // 5
              {{"a", -1.5f, -2.5f, 0}, {"b", 2, 0.5f, 2.5f}, {"r", 5.42f, 1, 14}, {"p", -0.4f, -2, 2}}, 1, false);
    };
    UnitText u;
    u.bigIdea = "The Mean Value Theorem turns facts about f\xE2\x80\xB2 into facts about f: f\xE2\x80\xB2(c) = (f(b) \xE2\x88\x92 f(a))/(b \xE2\x88\x92 a) for some c in (a, b). "
                "Curve sketching, optimization and error bounds all follow from it.";
    u.topics = "Related rates; linear approximation and differentials; extreme values and the closed-interval method; Rolle's theorem and the Mean Value Theorem; "
               "tests for increase, decrease and concavity; curve sketching; optimization; L'H\xC3\xB4pital's rule via Cauchy's Mean Value Theorem; Newton's method.";
    u.weeks = "4 weeks";
    u.people = "Rolle, L'H\xC3\xB4pital, Johann Bernoulli, Newton, Raphson, Lagrange, Cauchy";
    u.story = "Michel Rolle proved his theorem for polynomials in 1691, and around 1700 was one of the calculus's loudest critics in the Paris Academy. "
              "In 1696 the Marquis de l'H\xC3\xB4pital published Analyse des infiniment petits, the first calculus textbook. Much of it, including \"his\" rule, "
              "came from lessons he paid Johann Bernoulli for under a 1694 agreement.\n\n"
              "Newton's method appears in his De analysi (1669), applied to y\xC2\xB3 \xE2\x88\x92 2y \xE2\x88\x92 5 = 0; Joseph Raphson published a cleaner version in 1690. "
              "Lagrange (1797) and Cauchy (1823) made the Mean Value Theorem the backbone of rigorous calculus.";
    u.read = "- **Math:** Spivak ch. 11 (Significance of the Derivative); Simmons, chapters on applications of the derivative.\n"
             "- **History:** Dunham, The Calculus Gallery, ch. 3 (the Bernoullis); Paul Nahin, When Least Is Best (2004).\n"
             "- **Primary source:** L'H\xC3\xB4pital's rule as he stated it, in Struik's Source Book.";
    u.prove = "- **(a)** Fermat's theorem: at an interior extremum, f\xE2\x80\xB2 = 0.\n- **(b)** Rolle's theorem, then the Mean Value Theorem from it.\n"
              "- **(c)** f\xE2\x80\xB2 > 0 on an interval implies f is increasing there.\n- **(d)** Cauchy's Mean Value Theorem, then L'H\xC3\xB4pital's rule for 0/0.";
    u.testMinutes = 150;
    u.testName = "Unit 3 test";
    u.test = "**1.** A 5 m ladder leans against a wall, and its foot slides away at 0.5 m/s. How fast is the top sliding down when the foot is 3 m from the wall?\n\n"
             "**2.** Use a linear approximation to estimate \xE2\x88\x9B" "27.5. Is your estimate too high or too low? Justify with concavity.\n\n"
             "**3.** Find the absolute maximum and minimum of f(x) = x\xC2\xB3 \xE2\x88\x92 6x\xC2\xB2 + 9x + 1 on [\xE2\x88\x92" "1, 4].\n\n"
             "**4.** Prove that if f\xE2\x80\xB2(x) = 0 on an interval, f is constant there. Then prove |sin a \xE2\x88\x92 sin b| \xE2\x89\xA4 |a \xE2\x88\x92 b| for all a and b.\n\n"
             "**5.** Sketch y = x e^(\xE2\x88\x92x), showing intercepts, intervals of increase, extrema, concavity, inflection points and asymptotes.\n\n"
             "**6.** A closed cylindrical can must hold 1000 cm\xC2\xB3. Find the dimensions that minimize its surface area, and prove your answer is a minimum.\n\n"
             "**7.** Evaluate: (a) lim x\xE2\x86\x92" "0 of (e\xCB\xA3 \xE2\x88\x92 1 \xE2\x88\x92 x)/x\xC2\xB2; (b) lim x\xE2\x86\x92" "0\xE2\x81\xBA of x ln x; (c) lim x\xE2\x86\x92\xE2\x88\x9E of (1 + 2/x)\xCB\xA3.\n\n"
             "**8.** Apply two steps of Newton's method to x\xC2\xB3 \xE2\x88\x92 2x \xE2\x88\x92 5 = 0 from x\xE2\x82\x80 = 2, the equation Newton himself used.\n\n"
             "**9.** Prove that x\xE2\x81\xB5 + 10x + 3 = 0 has exactly one real root.\n\n"
             "**10.** History: Rolle publicly attacked the calculus, yet his theorem now sits at its center. Explain how the Mean Value Theorem replaces the "
             "\"infinitely small\" reasoning that critics like Rolle and Berkeley objected to.";
    L.steps = {
        overview(u, [](StepCtx& s) {
            graph(s, 1, {0, 1}, -2.6, 2.8, -3.2, 3.2);
            s.pane(1).deriv = true;
            s.pane(1).probeX = 1.0408;
            s.param("a", -1.5).param("b", 2);
        }),
        story(u),
        {"The Mean Value Theorem",
         "Join $(a, f(a))$ and $(b, f(b))$ with the blue secant. Somewhere between, a tangent runs parallel to it: $f'(c)$ equals the average slope. "
         "Here the secant has slope 0.25, and the red tangent at $c$ = 1.04 matches it.\n\n"
         "If you drove 100 miles in 2 hours, at some instant you were doing exactly 50 mph.",
         [](StepCtx& s) { s.ask("For f(x) = x\xC2\xB2 on [1, 3], at what c does the Mean Value Theorem's tangent occur?", "2", "f\xE2\x80\xB2(c) = 2c must equal (9 \xE2\x88\x92 1)/2."); }},
        {"Optimization",
         "A can holding 1000 cm\xC2\xB3 with radius $x$ has height $1000/(pi x^2)$ and surface area $A(x) = 2 pi x^2 + 2000/x$. "
         "The minimum is where $A'(x) = 4 pi x - 2000/x^2 = 0$: the dashed line, about 5.42 cm. (The written test asks you to prove it is a minimum.)",
         [](StepCtx& s) {
             graph(s, 1, {2}, 0, 14, 0, 2400);
             s.pane(1).trace = "r";
             s.param("r", 5.42);
             s.ask("What radius minimizes A(x) = 2\xCF\x80x\xC2\xB2 + 2000/x? (an expression is fine)", "(500/pi)^(1/3)", "Solve 4\xCF\x80x = 2000/x\xC2\xB2.");
         }},
        {"Newton's method",
         "To solve $N(x) = 0$, follow the tangent at a guess down to the axis and use that as the next guess:\n\n"
         "**new guess = old guess \xE2\x88\x92 N(old)/N\xE2\x80\xB2(old)**\n\n"
         "From $x_0 = 2$ for Newton's own $x^3 - 2 x - 5$, the tangent (red) crosses the axis at 2.1. Each step roughly doubles the correct digits.",
         [](StepCtx& s) {
             graph(s, 1, {3}, 1.6, 2.6, -3, 5);
             s.pane(1).deriv = true;
             s.pane(1).probeX = 2;
             s.ask("One step of Newton's method on x\xC2\xB3 \xE2\x88\x92 2x \xE2\x88\x92 5 from x\xE2\x82\x80 = 2 gives x\xE2\x82\x81 = ?", "2.1", "N(2) = \xE2\x88\x92" "1 and N\xE2\x80\xB2(2) = 10.");
         }},
        {"L'H\xC3\xB4pital's rule",
         "$(e^x - 1 - x)/x^2$ is $0/0$ at 0. L'H\xC3\xB4pital: differentiate top and bottom, twice here, to get $e^x/2$, so the limit is $1/2$. "
         "Move $p$ towards 0 and watch the dot approach 0.5.",
         [](StepCtx& s) {
             graph(s, 1, {4}, -2.2, 2.2, 0, 1.4);
             s.pane(1).trace = "p";
             s.param("p", -0.02);
             s.dur = 3.f;
         }},
        {"Curve sketching",
         "For $y = x e^(-x)$: $y' = (1 - x) e^(-x)$, so a maximum at $x = 1$; $y'' = (x - 2) e^(-x)$, so an inflection point at $x = 2$; "
         "and $y$ goes to 0 as $x$ grows. Drag the probe through both points.",
         [](StepCtx& s) {
             graph(s, 1, {5}, -0.8, 6, -0.6, 0.6);
             s.pane(1).deriv = true;
             s.pane(1).probeX = 1;
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is the absolute maximum of $x^3 - 3 x$ on [0, 2]?", "2", 0),
        typed("Mean Value Theorem for $f(x) = x^3$ on [0, 3]: what is $c$?", "sqrt(3)", 2),
        typed("Evaluate the limit of $(sin(x) - x)/x^3$ as $x$ approaches 0.", "-1/6", 5),
        typed("One step of Newton's method for $x^2 - 2 = 0$ from $x_0 = 1$ gives $x_1$ = ?", "1.5", 4),
        typed("Use the linear approximation of $sqrt(x)$ at 4 to estimate $sqrt(4.1)$.", "2.025", 0, 0.0005),
        typed("A rectangle has perimeter 20. What is the largest possible area?", "25", 3),
    };
    return L;
}

}  // namespace

std::vector<LessonDef> calculusCourse1() { return {placement(), unit0(), unit1(), unit2(), unit3()}; }
