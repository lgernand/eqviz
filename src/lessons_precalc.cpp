// Precalculus: the function skills the trigonometry and calculus lessons assume.
#include "lesson_kit.hpp"

using namespace kit;
using namespace theme;

namespace {

constexpr const char* kPrecalc = "Precalculus";

LessonDef pcFunctions() {
    LessonDef L;
    L.id = "pc-functions";
    L.subject = kPrecalc;
    L.title = "1. Functions and transformations";
    L.blurb = "Inputs and outputs, shifting and stretching graphs, composition, inverses, and |x| as distance.";
    L.setup = [](App& a) {
        setup(a, "Lesson: functions",
              {{"f(x) = x^2", YELLOW_C, ""},                          // 0
               {"g(x) = a (x - h)^2 + k", BLUE_C, ""},                // 1
               {"p(x) = 2x + 1", GREEN_C, ""},                        // 2
               {"q(x) = (x - 1)/2", RED_C, ""},                       // 3
               {"l(x) = x", GREY, "y = x"},                           // 4
               {"c(x) = p(f(x))", PURPLE_C, ""},                      // 5
               {"d(x) = abs(x - 4)", GOLD_C, "|x - 4|"}},             // 6
              {{"a", 1, -3, 3}, {"h", 0, -4, 4}, {"k", 0, -4, 4}}, 1, false);
    };
    L.steps = {
        {"What a function is",
         "A **function** is a rule that gives exactly one output for each input. We write $f(x)$ for the output of $f$ at the input $x$.\n\n"
         "$$f(x) = x^2$$\n\n"
         "The **graph** is every point $(x, f(x))$. Because each input has one output, a vertical line crosses the graph at most once.\n\n"
         "- The **domain** is the set of allowed inputs (here, every real number).\n"
         "- The **range** is the set of outputs (here, $y >= 0$).",
         [](StepCtx& s) {
             graph(s, 1, {0}, -4, 4, -1.5, 8);
             s.param("a", 1).param("h", 0).param("k", 0);
             s.ask("If f(x) = x², what is f(−3)?", "9");
         }},
        {"Shifting",
         "Replace $x$ by $x - h$ and add $k$:\n\n"
         "$$g(x) = (x - h)^2 + k$$\n\n"
         "The graph moves **right** by $h$ and **up** by $k$. The minus sign surprises people: at $x = h$ the inside is 0, "
         "so the lowest point has moved to $x = h$.\n\n"
         "Watch $h$ sweep, then try the sliders yourself.",
         [](StepCtx& s) {
             graph(s, 1, {0, 1}, -5, 5, -2, 8);
             s.active(1, 1);
             s.param("h", 2).param("k", 1);
             s.play = {"h"};
             s.ask("Where is the lowest point of (x − 3)² + 2? Give its x.", "3");
         }},
        {"Stretching and flipping",
         "Multiplying by $a$ stretches the graph vertically:\n\n"
         "$$g(x) = a (x - h)^2 + k$$\n\n"
         "- $a > 1$ stretches it taller, $0 < a < 1$ squashes it.\n"
         "- $a < 0$ flips it upside down, so the lowest point becomes the highest.\n\n"
         "Every transformation of a graph is one of these moves: shift, stretch, flip.",
         [](StepCtx& s) {
             s.param("a", -2).param("h", 1).param("k", 4);
             s.play = {"a"};
             s.ask("What is the highest value of y = −2(x − 1)² + 4?", "4");
         }},
        {"Composition",
         "Feeding one function's output into another is **composition**: $p(f(x))$ means apply $f$ first, then $p$.\n\n"
         "With $f(x) = x^2$ and $p(x) = 2x + 1$:\n\n"
         "$$p(f(x)) = 2x^2 + 1$$\n\n"
         "Order matters: $f(p(x)) = (2x + 1)^2$ is a different function. The chain rule in calculus is about exactly this.",
         [](StepCtx& s) {
             graph(s, 1, {0, 2, 5}, -3, 3, -3, 10);
             s.active(1, 5);
             s.ask("With f(x) = x² and p(x) = 2x + 1, what is p(f(3))?", "19");
         }},
        {"Inverse functions",
         "The **inverse** of $p$ undoes it: if $p(x) = 2x + 1$, solve $y = 2x + 1$ for $x$ to get\n\n"
         "$$q(x) = (x - 1)/2$$\n\n"
         "Then $q(p(x)) = x$ for every $x$. The graph of an inverse is the mirror image across the line $y = x$ (grey).\n\n"
         "A function has an inverse only if no two inputs share an output: $x^2$ doesn't (both 2 and −2 give 4) unless we keep only $x >= 0$.",
         [](StepCtx& s) {
             graph(s, 1, {2, 3, 4}, -4, 6, -4, 6);
             s.refit(1, true);
             s.ask("Which x has p(x) = 7?", "3", "Undo p: subtract 1, then halve.");
         }},
        {"Absolute value as distance",
         "$|x|$ is the distance from $x$ to 0, so $|x - 4|$ is the **distance between $x$ and 4**.\n\n"
         "An inequality like $|x - 4| < 0.5$ says \"$x$ is within 0.5 of 4\", which is the interval $3.5 < x < 4.5$ (shaded).\n\n"
         "This is the language of limits: \"within $epsilon$ of $L$\" is written $|f(x) - L| < epsilon$.",
         [](StepCtx& s) {
             graph(s, 1, {6}, 1, 7, -0.5, 3);
             shade(s, 1, 6, "3.5", "4.5", true);
             s.ask("The x with |x − 2| < 0.1 form an interval. How wide is it?", "0.2");
         }},
    };
    L.test = {
        typed("If $f(x) = x^2 - 3x$, what is $f(-2)$?", "10", 0),
        typed("The graph of $y = x^2$ is moved 5 left and 3 down. What is the new function at $x = 0$?", "22", 1),
        choice("$y = -(x + 2)^2 + 1$ has its highest point at", {"$(2, 1)$", "$(-2, 1)$", "$(-2, -1)$", "$(2, -1)$"}, 1, 2),
        typed("With $f(x) = x^2$ and $p(x) = 2x + 1$, what is $f(p(1))$?", "9", 3),
        typed("What is the inverse of $p(x) = 3x - 6$ at 0? (the $x$ with $p(x) = 0$)", "2", 4),
        choice("$|x - 1| < 3$ is the same as", {"$-2 < x < 4$", "$1 < x < 3$", "$-3 < x < 3$", "$x < 4$"}, 0, 5),
    };
    return L;
}

LessonDef pcPolynomials() {
    LessonDef L;
    L.id = "pc-polynomials";
    L.subject = kPrecalc;
    L.title = "2. Polynomials and rational functions";
    L.blurb = "Zeros and factors, the quadratic formula, end behaviour, asymptotes and holes.";
    L.setup = [](App& a) {
        setup(a, "Lesson: polynomials",
              {{"P(x) = (x + 2)(x - 1)(x - 3)", YELLOW_C, ""},       // 0
               {"Q(x) = x^2 - 4x + 1", BLUE_C, ""},                   // 1
               {"R(x) = (2x + 1)/(x - 3)", GREEN_C, ""},              // 2
               {"H(x) = 2", GREY, "y = 2"},                           // 3
               {"S(x) = (x^2 - 1)/(x - 1)", PURPLE_C, ""},            // 4
               {"T(x) = -x^3 + 2x", RED_C, ""}},                      // 5
              {}, 1, false);
    };
    L.steps = {
        {"Zeros and factors",
         "$$P(x) = (x + 2)(x - 1)(x - 3)$$\n\n"
         "A product is zero exactly when one factor is zero, so $P$ crosses the x-axis at $x = -2$, $x = 1$ and $x = 3$: its **zeros**.\n\n"
         "It works backwards too: if $r$ is a zero of a polynomial, then $x - r$ is a factor. Multiplied out, "
         "$P(x) = x^3 - 2x^2 - 5x + 6$, a polynomial of **degree** 3.",
         [](StepCtx& s) {
             graph(s, 1, {0}, -4, 5, -12, 12);
             s.ask("What is P(0)?", "6", "Multiply the factors at x = 0: (2)(−1)(−3).");
         }},
        {"Quadratics",
         "Every quadratic $a x^2 + b x + c$ can be written as a shifted parabola by **completing the square**:\n\n"
         "$$x^2 - 4x + 1 = (x - 2)^2 - 3$$\n\n"
         "so its lowest point is $(2, -3)$. Setting it to 0 gives the **quadratic formula**:\n\n"
         "$$x = (-b + sqrt(b^2 - 4 a c))/(2a)$$\n\n"
         "$$x = (-b - sqrt(b^2 - 4 a c))/(2a)$$\n\n"
         "Here $x = 2 ± sqrt(3)$. The **discriminant** $b^2 - 4ac$ says how many real zeros: two if positive, one if zero, none if negative.",
         [](StepCtx& s) {
             graph(s, 1, {1}, -2, 6, -4, 6);
             s.ask("How many real zeros does x² + 2x + 5 have?", "0", "Its discriminant is 4 − 20.");
         }},
        {"End behaviour",
         "For large $|x|$ the highest power wins: $-x^3 + 2x$ behaves like $-x^3$, rising to the left and falling to the right.\n\n"
         "- Even degree: both ends go the same way.\n"
         "- Odd degree: the ends go opposite ways.\n"
         "- A negative leading coefficient flips the picture.\n\n"
         "A polynomial of degree $n$ has at most $n$ zeros and at most $n - 1$ turning points.",
         [](StepCtx& s) {
             graph(s, 1, {0, 5}, -4, 4, -15, 15);
             s.active(1, 5);
             s.ask("At most how many turning points can a degree-5 polynomial have?", "4");
         }},
        {"Rational functions",
         "A **rational function** is a ratio of polynomials:\n\n"
         "$$R(x) = (2x + 1)/(x - 3)$$\n\n"
         "- Where the bottom is 0 and the top isn't ($x = 3$), the graph shoots off to infinity: a **vertical asymptote**.\n"
         "- For large $|x|$ only the leading terms matter: $2x/x = 2$, so the graph levels off at $y = 2$: a **horizontal asymptote**.",
         [](StepCtx& s) {
             graph(s, 1, {2, 3}, -8, 12, -8, 12);
             s.active(1, 2);
             s.ask("What horizontal asymptote does (6x − 1)/(3x + 4) have? Give y.", "2");
         }},
        {"Holes",
         "$$S(x) = (x^2 - 1)/(x - 1)$$\n\n"
         "Here top and bottom are **both** 0 at $x = 1$. Factor: $x^2 - 1 = (x - 1)(x + 1)$, so for $x != 1$ the factor cancels and $S(x) = x + 1$.\n\n"
         "The graph is the line $y = x + 1$ with one point missing at $x = 1$: a **hole**, not an asymptote. "
         "Asking what $S$ approaches at that hole (the answer is 2) is the first question of calculus.",
         [](StepCtx& s) {
             graph(s, 1, {4}, -3, 4, -2, 5);
             s.ask("S has a hole at x = 1. What y-value is missing there?", "2");
         }},
    };
    L.test = {
        typed("What is the product of the zeros of $(x - 2)(x + 5)(x - 1)$?", "10", 0),
        typed("Complete the square: $x^2 + 6x + 2 = (x + 3)^2 + c$. What is $c$?", "-7", 1),
        typed("How many real zeros does $x^2 - 6x + 9$ have?", "1", 1),
        choice("As $x$ grows large and positive, $-2x^4 + x^3$", {"goes to $inf$", "goes to $-inf$", "levels off at $-2$", "goes to 0"}, 1, 2),
        typed("Where is the vertical asymptote of $(x + 4)/(2x - 6)$? Give $x$.", "3", 3),
        typed("$(x^2 - 9)/(x - 3)$ has a hole at $x = 3$. What $y$-value is missing?", "6", 4),
    };
    return L;
}

LessonDef pcExpLog() {
    LessonDef L;
    L.id = "pc-explog";
    L.subject = kPrecalc;
    L.title = "3. Exponentials and logarithms";
    L.blurb = "Growth by a constant factor, the number e, logarithms as inverses, and the log laws.";
    L.setup = [](App& a) {
        setup(a, "Lesson: exponentials and logarithms",
              {{"E(x) = b^x", YELLOW_C, ""},                          // 0
               {"G(x) = ln(x)/ln(b)", BLUE_C, "log_b(x)"},            // 1
               {"l(x) = x", GREY, "y = x"},                           // 2
               {"N(x) = 100 (1/2)^(x/5)", GREEN_C, "half-life 5"},    // 3
               {"X(x) = exp(x)", RED_C, "e^x"}},                      // 4
              {{"b", 2, 0.2f, 5}}, 1, false);
    };
    L.steps = {
        {"Growth by a factor",
         "Linear functions add the same amount each step; **exponential** functions multiply by the same factor:\n\n"
         "$$E(x) = b^x$$\n\n"
         "With $b = 2$ the value doubles every time $x$ goes up by 1. Every exponential passes through $(0, 1)$, because $b^0 = 1$.\n\n"
         "- $b > 1$: growth. $0 < b < 1$: decay. Watch $b$ sweep.",
         [](StepCtx& s) {
             graph(s, 1, {0}, -4, 4, -1, 10);
             s.param("b", 2);
             s.play = {"b"};
             s.ask("What is 2 to the power −3?", "1/8");
         }},
        {"The exponent rules",
         "Because an exponent counts repeated factors:\n\n"
         "- $b^m b^n = b^(m + n)$\n"
         "- $(b^m)^n = b^(m n)$\n"
         "- $b^(-n) = 1/b^n$, and $b^(1/n)$ is the $n$th root of $b$\n\n"
         "So $8^(2/3)$ is the cube root of 8, squared: 4.",
         [](StepCtx& s) {
             s.param("b", 3);
             s.ask("What is 27^(2/3)?", "9");
         }},
        {"The number e",
         "One base is special. $e ≈ 2.71828$ is the base whose graph has slope exactly 1 where it crosses the y-axis, "
         "and in fact its slope everywhere equals its height. It comes from compounding:\n\n"
         "$$(1 + 1/n)^n → e$$\n\n"
         "as $n$ grows. In calculus $e^x$ is the function that is its own derivative, so it appears in every growth law.",
         [](StepCtx& s) {
             graph(s, 1, {0, 4}, -3, 3, -1, 8);
             s.active(1, 4);
             s.param("b", 2.71828);
             s.ask("What is ln of e³?", "3");
         }},
        {"Logarithms",
         "The **logarithm** undoes the exponential: $log_b(x)$ is the power you raise $b$ to to get $x$.\n\n"
         "For example $log_2(8) = 3$, because $2^3 = 8$.\n\n"
         "As an inverse, its graph is the mirror image of $b^x$ across $y = x$. It is only defined for $x > 0$, "
         "and it grows very slowly. With base $e$ it's the **natural log**, $ln(x)$.",
         [](StepCtx& s) {
             graph(s, 1, {0, 1, 2}, -4, 8, -4, 8);
             s.refit(1, true);
             s.active(1, 1);
             s.param("b", 2);
             s.ask("What is log base 2 of 32?", "5");
         }},
        {"The log laws",
         "Each exponent rule becomes a log rule:\n\n"
         "- $log(x y) = log(x) + log(y)$\n"
         "- $log(x/y) = log(x) - log(y)$\n"
         "- $log(x^n) = n log(x)$\n\n"
         "The last one solves equations with the unknown in the exponent: $2^x = 10$ gives $x ln(2) = ln(10)$, so $x = ln(10)/ln(2) ≈ 3.32$.",
         [](StepCtx& s) { s.ask("Solve 3^x = 20 (an expression is fine).", "ln(20)/ln(3)", "Take ln of both sides."); }},
        {"Half-life",
         "Radioactive decay halves the amount every fixed time, the **half-life**. Starting from 100 with half-life 5:\n\n"
         "$$N(t) = 100 (1/2)^(t/5)$$\n\n"
         "After 5 time units 50 are left, after 10 there are 25, and so on. Asking \"when are 10 left?\" needs a logarithm: "
         "$t = 5 ln(10)/ln(2) ≈ 16.6$.",
         [](StepCtx& s) {
             graph(s, 1, {3}, -1, 30, -5, 110);
             s.ask("With half-life 5 and N(0) = 100, how much is left at t = 15?", "12.5");
         }},
    };
    L.test = {
        typed("What is $4^(3/2)$?", "8", 1),
        typed("What is $log_3(81)$?", "4", 3),
        typed("Write $ln(8) - ln(2)$ as the log of one number: $ln(c)$. What is $c$?", "4", 4),
        typed("Solve $e^(2x) = 7$ (an expression is fine).", "ln(7)/2", 4),
        choice("The graph of $y = ln(x)$", {"passes through $(0, 1)$", "passes through $(1, 0)$", "is defined for every $x$", "has a horizontal asymptote"}, 1, 3),
        typed("A sample of 80 g has a half-life of 3 days. How many grams are left after 9 days?", "10", 5),
    };
    return L;
}

}  // namespace

std::vector<LessonDef> precalcLessons() { return {pcFunctions(), pcPolynomials(), pcExpLog()}; }
