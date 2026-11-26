// End-of-lesson tests for the trigonometry, calculus and linear algebra lessons.
// A question's `set` adjusts the picture (applied after all of the lesson's steps).
#include "app.hpp"

namespace {

constexpr double PI = 3.14159265358979323846;
using Set = std::function<void(StepCtx&)>;

TestQuestion typed(std::string text, std::string answer, int review, Set set = {}, double tol = 0.01) {
    TestQuestion q;
    q.text = std::move(text);
    q.answer = std::move(answer);
    q.review = review;
    q.set = std::move(set);
    q.tol = tol;
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

// a unit circle showing only the angle (no values to read off)
Set angleOnly(double theta) {
    return [theta](StepCtx& s) {
        s.pane(1).circleOpts = kCircArc | kCircSnap;
        s.param("theta", theta);
    };
}

Set showMatrix(double a, double b, double c, double d, int opts = kMatGrid | kMatBasis) {
    return [=](StepCtx& s) {
        s.matrix(1, a, b, c, d);
        s.pane(1).matOpts = opts;
        s.dur = 2.f;
    };
}

}  // namespace

std::vector<TestQuestion> lessonTests(const std::string& id) {
    if (id == "trig-circle")
        return {
            typed("The point $P$ is at the angle shown, $theta = (2 pi)/3$. What is $cos(theta)$?", "-1/2", 2, angleOnly(2 * PI / 3)),
            choice("In which quadrant are both $cos(theta)$ and $sin(theta)$ negative?", {"first", "second", "third", "fourth"}, 2, 3),
            typed("Convert 45\xC2\xB0 to radians.", "pi/4", 1),
            typed("$sin(theta) = 0.28$ and $P$ is in the first quadrant. What is $cos(theta)$?", "0.96", 5),
            choice("What is $sin(pi/3)$?", {"$1/2$", "$sqrt(2)/2$", "$sqrt(3)/2$", "$1$"}, 2, 4, angleOnly(PI / 3)),
        };
    if (id == "trig-waves")
        return {
            typed("What is the period of $sin(x)$?", "2pi", 3),
            choice("Which wave has the value 1 at $x = 0$?", {"$sin(x)$", "$cos(x)$", "both", "neither"}, 1, 2),
            typed("$sin((7 pi)/2) = $ ?", "-1", 3),
            choice("$sin(-x)$ equals", {"$sin(x)$", "$-sin(x)$", "$cos(x)$", "$-cos(x)$"}, 1, 4),
        };
    if (id == "trig-tan")
        return {
            typed("$tan(pi/4) = $ ?", "1", 4, angleOnly(PI / 4)),
            choice("Where does $tan(x)$ have vertical asymptotes?",
                   {"at every multiple of $pi$", "at $pi/2 + k pi$", "at every multiple of $2 pi$", "nowhere"}, 1, 1),
            typed("If $cos(theta) = 0.5$, what is $sec(theta)$?", "2", 2),
            typed("What is the period of $tan(x)$?", "pi", 1),
        };
    if (id == "trig-transform")
        return {
            typed("What is the amplitude of $3 sin(2 x) - 1$?", "3", 1),
            typed("What is the period of $sin(4 x)$?", "pi/2", 2),
            choice("$sin(x - 2)$ is $sin(x)$ shifted", {"left by 2", "right by 2", "up by 2", "down by 2"}, 1, 3),
            typed("What is the smallest value of $2 sin(x) + 5$?", "3", 4),
            typed("The yellow wave is $a sin(b (x - c)) + d$. Read its midline from the graph: what is $d$?", "0.5", 4,
                  [](StepCtx& s) { s.param("a", 1.5).param("b", 2).param("c", 0).param("d", 0.5); }),
        };
    if (id == "calc-limits")
        return {
            typed("What is the limit of $sin(x)/x$ as $x$ approaches 0?", "1", 1),
            typed("What is the limit of $(x^2 - 9)/(x - 3)$ as $x$ approaches 3?", "6", 2),
            choice("The limit of $floor(x)$ as $x$ approaches 2", {"is 1", "is 2", "does not exist", "is 1.5"}, 2, 3),
            typed("What is the limit of $floor(x)$ as $x$ approaches 1 from the right?", "1", 3),
        };
    if (id == "calc-derivative")
        return {
            typed("For $f(x) = x^2$, what is the slope of the secant line from $x = 1$ to $x = 3$?", "4", 0),
            typed("What is the derivative of $x^2$ at $x = -2$?", "-4", 3),
            typed("What is the derivative of $x^3$ at $x = -1$?", "3", 4),
            choice("As $h$ approaches 0, the secant line becomes", {"the x-axis", "the tangent line", "vertical", "the curve itself"}, 1, 1),
            typed("What is the derivative of $x^5$ at $x = 1$?", "5", 4),
        };
    if (id == "calc-shape")
        return {
            typed("For $f(x) = x^3 - 3 x$, what is the value of $f$ at its local minimum?", "-2", 1),
            choice("Where the derivative is negative, $f$ is", {"increasing", "decreasing", "concave up", "zero"}, 1, 0),
            typed("$x^2 - 4 x$ has one critical point. At what $x$?", "2", 1),
            choice("Where the second derivative is positive, the curve is", {"concave up", "concave down", "decreasing", "at a maximum"}, 0, 2),
        };
    if (id == "calc-integral")
        return {
            typed("What is the area under $x^2$ from 0 to 2?", "8/3", 3),
            typed("What is the left Riemann sum for $x^2$ on [0, 2] with $n = 4$?", "1.75", 1),
            choice("For a rising curve, left rectangles", {"overestimate the area", "underestimate the area", "are exact", "could do either"}, 1, 1),
            typed("What is the area under $2 x$ from 0 to 3?", "9", 3),
        };
    if (id == "calc-ftc")
        return {
            typed("What is the signed area under $cos(x)$ from 0 to $pi$?", "0", 1),
            choice("If $A(x)$ is the area under $f$ from 0 to $x$, the derivative of $A$ is", {"$f(x)$", "$A(x)$", "the derivative of $f$", "0"}, 0, 2),
            typed("What is the area under $3 x^2$ from 0 to 2?", "8", 2),
            typed("What is the area under $cos(x)$ from 0 to $pi/6$?", "0.5", 2),
        };
    if (id == "calc-taylor")
        return {
            typed("What is $x - x^3/6$ at $x = 0.5$?", "0.5 - 0.5^3/6", 1, {}, 0.0005),
            choice("Adding more terms makes the polynomial match $sin(x)$", {"over a wider range", "only at 0", "less well", "only for negative x"}, 0, 2),
            typed("What is the coefficient of $x^5$ in the Taylor series of $sin(x)$?", "1/120", 3, {}, 0.0001),
            choice("$5!$ equals", {"25", "60", "120", "720"}, 2, 3),
        };
    if (id == "la-vectors")
        return {
            typed("What is the x-coordinate of 2\xC2\xB7(3, \xE2\x88\x92" "1) + (\xE2\x88\x92" "1, 4)?", "5", 1),
            typed("And its y-coordinate?", "2", 1),
            choice("The span of $(1, 2)$ and $(2, 4)$ is", {"a line", "the whole plane", "a single point", "two lines"}, 0, 3),
            choice("Can every vector in the plane be built from $(1, 1)$ and $(-1, 2)$?", {"yes", "no"}, 0, 2),
        };
    if (id == "la-transform")
        return {
            typed("Where does the matrix shown send \xC3\xAE? Give the x-coordinate.", "3", 1, showMatrix(3, 1, -1, 2)),
            choice("Which matrix rotates the plane a quarter turn counterclockwise?",
                   {"rows $(0, -1)$ and $(1, 0)$", "rows $(0, 1)$ and $(1, 0)$", "rows $(-1, 0)$ and $(0, -1)$", "rows $(1, 0)$ and $(0, 1)$"}, 0, 1),
            typed("$M$ has columns $(2, 1)$ and $(0, 3)$. What is the y-coordinate of $M$ times $(1, 1)$?", "4", 4),
            choice("A shear like the one in this lesson", {"keeps \xC3\xAE fixed and slants \xC4\xB5", "keeps both fixed", "rotates everything", "squashes the plane onto a line"}, 0, 2),
        };
    if (id == "la-det")
        return {
            typed("What is the determinant of the matrix with rows $(4, 1)$ and $(2, 3)$?", "10", 2),
            typed("What is the determinant of the matrix shown?", "1", 2, showMatrix(2, 1, 1, 1)),
            choice("A determinant of 0 means the plane is", {"squashed onto a line or a point", "rotated", "doubled in area", "flipped"}, 0, 4),
            choice("A negative determinant means the transformation", {"flips orientation", "shrinks areas", "is a rotation", "is the identity"}, 0, 3),
            typed("A matrix with determinant 3 maps a region of area 2 to a region of what area?", "6", 0),
        };
    if (id == "la-systems")
        return {
            typed("Solve $2 x_1 + x_2 = 5$ and $x_1 + x_2 = 3$. What is $x_1$?", "2", 0),
            typed("And $x_2$?", "1", 0),
            typed("The inverse of the matrix with rows $(2, 1)$ and $(1, 1)$ has rows $(1, -1)$ and $(-1, k)$. What is $k$?", "2", 1),
            choice("If the determinant of $M$ is 0, the system $M x = b$",
                   {"always has exactly one solution", "has no solution or infinitely many", "has exactly two solutions", "can always be solved with the inverse"}, 1, 2),
        };
    if (id == "la-eigen")
        return {
            typed("What is the larger eigenvalue of the matrix with rows $(4, 0)$ and $(0, -1)$?", "4", 4),
            typed("$M$ has rows $(2, 1)$ and $(1, 2)$, and $v = (1, 1)$. $M v = lambda v$: what is $lambda$?", "3", 4),
            choice("Which of these has no real eigenvectors?", {"a quarter-turn rotation", "a stretch along the axes", "a shear", "the identity"}, 0, 3),
            typed("What is the smaller eigenvalue of the matrix shown?", "2", 1, showMatrix(3, 1, 0, 2, kMatGrid | kMatBasis | kMatVector)),
        };
    return {};
}
