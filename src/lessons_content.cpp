// The built-in lessons: trigonometry, calculus and linear algebra.
// Each lesson sets up rows, sliders and panes (pane 0 holds the text), then lists steps.
// A step's `set` changes the target state; steps build on the ones before them.
#include "app.hpp"
#include "theme.hpp"

using namespace theme;

namespace {

void setup(App& a, const char* name, std::vector<DemoRow> rows, std::vector<DemoParam> params, int graphs, bool stacked,
           float noteFrac = 0.37f) {
    std::vector<DemoPane> panes(graphs + 1);
    for (auto& p : panes) p.show = {-1};  // every row hidden; the steps decide what each pane shows
    a.loadDemo(name, "", rows, params, panes, 1, 1);
    a.lessonLayout(graphs, stacked, noteFrac);
}

void circle(StepCtx& s, int pane, int opts) {
    KeyPane& p = s.pane(pane);
    p.unitCircle = true;
    p.circleOpts = opts;
}

void matrixPane(StepCtx& s, int pane, int opts) {
    KeyPane& p = s.pane(pane);
    p.matrix = true;
    p.matOpts = opts;
}

constexpr double PI = 3.14159265358979323846;

// ==================================================================== trigonometry

LessonDef trigCircle() {
    LessonDef L;
    L.id = "trig-circle";
    L.subject = "Trigonometry";
    L.title = "1. The unit circle";
    L.blurb = "Angles in radians, and sine and cosine as the coordinates of a point on a circle.";
    L.setup = [](App& a) { setup(a, "Lesson: the unit circle", {{"s(x) = sin(x)", RED_C, ""}}, {}, 1, false); };
    L.steps = {
        {"Angles on a circle",
         "Draw a circle of radius 1 around the origin: the **unit circle**.\n\n"
         "An angle $theta$ is measured from the positive x-axis, turning **counterclockwise**. "
         "Turning by $theta$ from the point $(1, 0)$ lands on a point $P$ on the circle.\n\n"
         "- Drag the yellow point around the circle.\n"
         "- The yellow arc is how far $P$ has travelled.",
         [](StepCtx& s) {
             circle(s, 1, kCircArc | kCircSnap);
             s.fit(1, -1.7, 1.7, -1.35, 1.35);
             s.param("theta", 0.6);
         }},
        {"Radians",
         "On the unit circle, the length of the arc **is** the angle. Measured this way, angles are in **radians**.\n\n"
         "A full turn is the whole circumference, $2 pi r = 2 pi$ when $r = 1$, so\n\n"
         "360\xC2\xB0 is the same angle as $2 pi$ radians.\n\n"
         "- a half turn is $pi$ (180°)\n"
         "- a quarter turn is $pi/2$ (90°)\n"
         "- one radian is about 57.3°\n\n"
         "Watch $theta$ sweep a full turn.",
         [](StepCtx& s) {
             s.param("theta", 2 * PI);
             s.dur = 4.5f;
             s.ask("How many radians is 90°?", "pi/2", "A full turn (360°) is 2\xCF\x80.");
         }},
        {"Cosine and sine",
         "Here is the definition everything else follows from:\n\n"
         "$$P = (cos(theta), sin(theta))$$\n\n"
         "- **cos θ** is the x-coordinate of $P$ (blue)\n"
         "- **sin θ** is the y-coordinate of $P$ (red)\n\n"
         "So both are always between $-1$ and $1$. The table at the top right shows their values; drag $P$ and watch them change.",
         [](StepCtx& s) {
             circle(s, 1, kCircArc | kCircSnap | kCircSinCos);
             s.param("theta", 0.9);
             s.ask("What is cos \xCE\xB8 when \xCE\xB8 = 0?", "1", "At \xCE\xB8 = 0 the point P is at (1, 0).");
         }},
        {"All the way around",
         "The definition works for any angle. In each quarter of the circle the signs change:\n\n"
         "- first quadrant ($0 < theta < pi/2$): both positive\n"
         "- second: $cos(theta) < 0$, $sin(theta) > 0$\n"
         "- third: both negative\n"
         "- fourth: $cos(theta) > 0$, $sin(theta) < 0$\n\n"
         "Angles past $2 pi$ just go round again, so $sin(theta + 2 pi) = sin(theta)$.",
         [](StepCtx& s) {
             s.param("theta", 2.4);
             s.dur = 2.f;
             s.ask("What is sin(4), to two decimals?", "sin(4)", "4 radians is between \xCF\x80 and 3\xCF\x80/2: the third quadrant.");
         }},
        {"Special angles",
         "Some angles have exact values worth knowing. Dragging $P$ snaps to them, and the table shows exact forms.\n\n"
         "- 30\xC2\xB0 = $pi/6$: half of an equilateral triangle gives $sin(pi/6) = 1/2$ and $cos(pi/6) = sqrt(3)/2$\n"
         "- 45\xC2\xB0 = $pi/4$: an isosceles right triangle gives $sin(pi/4) = cos(pi/4) = sqrt(2)/2$\n"
         "- 60\xC2\xB0 = $pi/3$: the same triangle as 30°, turned: $sin(pi/3) = sqrt(3)/2$, $cos(pi/3) = 1/2$\n\n"
         "Try dragging to 120°, 135°, 150°: the same numbers appear with different signs.",
         [](StepCtx& s) {
             s.param("theta", PI / 6);
             s.ask("What is sin(\xCF\x80/4)?", "sqrt(2)/2", "Both legs of a 45\xC2\xB0 right triangle are equal, and the hypotenuse is 1.");
         }},
        {"The Pythagorean identity",
         "The blue leg, the red leg and the radius form a right triangle with hypotenuse 1. Pythagoras gives\n\n"
         "$$cos(theta)^2 + sin(theta)^2 = 1$$\n\n"
         "for every angle. If you know one of them, you know the other up to its sign.",
         [](StepCtx& s) {
             s.param("theta", 0.927295);
             s.ask("If cos \xCE\xB8 = 0.6 and P is above the x-axis, what is sin \xCE\xB8?", "0.8",
                   "sin\xC2\xB2 \xCE\xB8 = 1 \xE2\x88\x92 0.36");
         }},
    };
    return L;
}

LessonDef trigWaves() {
    LessonDef L;
    L.id = "trig-waves";
    L.subject = "Trigonometry";
    L.title = "2. Sine and cosine waves";
    L.blurb = "Unroll the circle: the height of P against the angle traces the sine wave.";
    L.setup = [](App& a) { setup(a, "Lesson: sine and cosine waves", {{"sin(x)", RED_C, ""}, {"cos(x)", BLUE_C, ""}}, {}, 2, true); };
    L.steps = {
        {"Unrolling the circle",
         "Top: the unit circle. Bottom: a graph whose horizontal axis is the **angle**.\n\n"
         "For each angle $theta$, plot the height of $P$, which is $sin(theta)$. The dashed line in the graph marks the current $theta$, "
         "and its dot sits at height $sin(theta)$.",
         [](StepCtx& s) {
             circle(s, 1, kCircSinCos | kCircArc | kCircSnap);
             s.fit(1, -1.6, 1.6, -1.25, 1.25);
             s.show(2, {0});
             s.active(2, 0);
             s.pane(2).trace = "theta";
             s.fit(2, -0.6, 7.2, -1.5, 1.5);
             s.param("theta", 0.0);
         }},
        {"Watch it trace",
         "As $theta$ goes once round, the height rises to 1 at $pi/2$, falls through 0 at $pi$, reaches $-1$ at $3 pi/2$ and comes back at $2 pi$.\n\n"
         "That is one full **cycle** of $y = sin(x)$.",
         [](StepCtx& s) {
             s.param("theta", 2 * PI);
             s.dur = 6.f;
         }},
        {"Cosine",
         "Cosine is the horizontal coordinate. Its graph (blue) is the same wave, shifted:\n\n"
         "$$cos(x) = sin(x + pi/2)$$\n\n"
         "Cosine starts at its top, $cos(0) = 1$, while sine starts at 0.",
         [](StepCtx& s) {
             s.show(2, {0, 1});
             s.param("theta", 1.0);
         }},
        {"Repeating forever",
         "Both waves repeat every $2 pi$: their **period** is $2 pi$.\n\n"
         "- $sin(x) = 0$ at every multiple of $pi$\n"
         "- $sin(x)$ peaks at $pi/2 + 2 k pi$ and bottoms out at $3 pi/2 + 2 k pi$",
         [](StepCtx& s) {
             s.fit(2, -7, 13.5, -1.5, 1.5);
             s.param("theta", 2.0);
             s.ask("sin(3\xCF\x80/2) = ?", "-1", "Where is P after three quarter turns?");
         }},
        {"Odd and even",
         "Reflect the angle, $theta$ to $-theta$, and $P$ flips over the x-axis:\n\n"
         "- $sin(-x) = -sin(x)$: sine is **odd** (its graph has rotational symmetry about the origin)\n"
         "- $cos(-x) = cos(x)$: cosine is **even** (its graph is mirror-symmetric about the y-axis)",
         [](StepCtx& s) {
             s.fit(2, -7, 7, -1.5, 1.5);
             s.param("theta", 5.5);
             s.ask("cos(\xE2\x88\x92\xCF\x80/3) = ?", "0.5", "Cosine is even.");
         }},
    };
    return L;
}

LessonDef trigTan() {
    LessonDef L;
    L.id = "trig-tan";
    L.subject = "Trigonometry";
    L.title = "3. Tangent and its friends";
    L.blurb = "tan, sec, cot and csc as lengths on the unit circle, and why tan has asymptotes.";
    L.setup = [](App& a) {
        setup(a, "Lesson: tangent", {{"tan(x)", GOLD_C, ""}, {"sin(x)", RED_C, ""}, {"cos(x)", BLUE_C, ""}}, {}, 2, true);
    };
    L.steps = {
        {"Tangent",
         "$$tan(theta) = sin(theta)/cos(theta)$$\n\n"
         "It is the **slope** of the radius $OP$. On the circle it is also a length: draw the tangent line to the circle at $P$. "
         "The gold piece from $P$ to the x-axis has length $|tan(theta)|$ (similar triangles).",
         [](StepCtx& s) {
             circle(s, 1, kCircSinCos | kCircTan | kCircSnap);
             s.fit(1, -1.4, 2.6, -1.25, 1.6);
             s.show(2, {0});
             s.active(2, 0);
             s.pane(2).trace = "theta";
             s.fit(2, -1.7, 4.8, -4, 4);
             s.param("theta", 0.6);
         }},
        {"Where tangent blows up",
         "As $theta$ approaches $pi/2$, $cos(theta)$ shrinks to 0 and $tan(theta)$ grows without bound. The graph has a "
         "**vertical asymptote** at $x = pi/2$, and another every $pi$.\n\n"
         "Tangent repeats every $pi$, not $2 pi$: opposite points of the circle give the same slope.",
         [](StepCtx& s) {
             s.param("theta", 1.52);
             s.dur = 3.5f;
         }},
        {"Secant",
         "The purple segment on the x-axis, from $O$ to where the tangent line crosses, has length\n\n"
         "$$sec(theta) = 1/cos(theta)$$\n\n"
         "The triangle $O$, $P$, crossing point is right-angled at $P$, so $1 + tan(theta)^2 = sec(theta)^2$.",
         [](StepCtx& s) { s.param("theta", 0.8); }},
        {"Cotangent and cosecant",
         "Do the same towards the y-axis:\n\n"
         "- teal: $cot(theta) = cos(theta)/sin(theta)$, from $P$ to the y-axis\n"
         "- pink: $csc(theta) = 1/sin(theta)$, from $O$ up the y-axis\n\n"
         "and $1 + cot(theta)^2 = csc(theta)^2$.",
         [](StepCtx& s) {
             circle(s, 1, kCircSinCos | kCircTan | kCircCot | kCircSnap);
             s.fit(1, -1.4, 2.6, -1.25, 2.2);
             s.param("theta", 1.0);
         }},
        {"Exact values",
         "At the special angles tangent is exact too:\n\n"
         "- $tan(pi/6) = sqrt(3)/3$\n"
         "- $tan(pi/4) = 1$: the radius has slope 1\n"
         "- $tan(pi/3) = sqrt(3)$\n"
         "- $tan(pi/2)$ is undefined",
         [](StepCtx& s) {
             s.param("theta", PI / 3);
             s.ask("tan(\xCF\x80/3) = ?", "sqrt(3)", "sin(\xCF\x80/3) = \xE2\x88\x9A" "3/2 and cos(\xCF\x80/3) = 1/2.");
         }},
    };
    return L;
}

LessonDef trigTransform() {
    LessonDef L;
    L.id = "trig-transform";
    L.subject = "Trigonometry";
    L.title = "4. Shaping a wave";
    L.blurb = "Amplitude, period, phase shift and midline of a sin(b(x \xE2\x88\x92 c)) + d.";
    L.setup = [](App& a) {
        setup(a, "Lesson: shaping a wave", {{"f(x) = a sin(b (x - c)) + d", YELLOW_C, ""}, {"sin(x)", GREY, ""}},
              {{"a", 1, -3, 3}, {"b", 1, 0.2f, 4}, {"c", 0, -3.2f, 3.2f}, {"d", 0, -2, 2}}, 1, false);
    };
    L.steps = {
        {"Four knobs",
         "Every sinusoid can be written\n\n"
         "$$f(x) = a sin(b (x - c)) + d$$\n\n"
         "With $a = 1$, $b = 1$, $c = 0$, $d = 0$ it is plain $sin(x)$ (grey). Each step turns one knob; "
         "the sliders in the panel let you turn them yourself.",
         [](StepCtx& s) {
             s.show(1, {0, 1});
             s.active(1, 0);
             s.fit(1, -7, 7, -3.5, 3.5);
             s.param("a", 1).param("b", 1).param("c", 0).param("d", 0);
         }},
        {"Amplitude",
         "$a$ stretches the wave vertically. The **amplitude**, the height of a peak above the midline, is $|a|$. "
         "A negative $a$ flips the wave upside down.",
         [](StepCtx& s) {
             s.param("a", 2);
             s.dur = 2.f;
         }},
        {"Period",
         "$b$ squeezes the wave horizontally. One cycle now takes\n\n"
         "$$T = (2 pi)/b$$\n\n"
         "so $b = 2$ fits two cycles where there used to be one.",
         [](StepCtx& s) {
             s.param("a", 1).param("b", 2);
             s.dur = 2.f;
             s.ask("What is the period of sin(3x)?", "2pi/3", "T = 2\xCF\x80 / b");
         }},
        {"Phase shift",
         "$c$ slides the wave sideways: $sin(x - c)$ is $sin(x)$ moved **right** by $c$. "
         "(Watch the sign: $sin(x + 1)$ moves left.)",
         [](StepCtx& s) {
             s.param("b", 1).param("c", 1.2);
             s.dur = 2.f;
         }},
        {"Midline",
         "$d$ lifts the whole wave: the **midline** is $y = d$, and the wave runs from $d - |a|$ to $d + |a|$.",
         [](StepCtx& s) {
             s.param("c", 0).param("d", 1).param("a", 2);
             s.dur = 2.f;
             s.ask("What is the largest value of 2 sin(x) + 1?", "3", "Midline plus amplitude.");
         }},
    };
    return L;
}

// ==================================================================== calculus

LessonDef calcLimits() {
    LessonDef L;
    L.id = "calc-limits";
    L.subject = "Calculus";
    L.title = "1. Limits";
    L.blurb = "What a function approaches, even where it isn't defined.";
    L.setup = [](App& a) {
        setup(a, "Lesson: limits",
              {{"f(x) = sin(x)/x", YELLOW_C, ""}, {"g(x) = (x^2 - 1)/(x - 1)", BLUE_C, ""}, {"h(x) = floor(x)", GREEN_C, ""}},
              {{"a", 2, -4, 4}}, 1, false);
    };
    L.steps = {
        {"A gap at zero",
         "$$f(x) = sin(x)/x$$\n\n"
         "At $x = 0$ this is $0/0$: undefined. But we can ask what $f(x)$ gets close to as $x$ gets close to 0.\n\n"
         "The dashed line is at $x = a$, and the dot shows $f(a)$. Drag the $a$ slider towards 0.",
         [](StepCtx& s) {
             s.show(1, {0});
             s.active(1, 0);
             s.pane(1).trace = "a";
             s.fit(1, -9, 9, -0.6, 1.4);
             s.param("a", 2);
         }},
        {"Closer and closer",
         "As $a$ approaches 0 from either side, $f(a)$ approaches 1. We write\n\n"
         "**the limit of $sin(x)/x$ as $x$ approaches 0 is 1.**\n\n"
         "The limit is about the values **near** 0; the value **at** 0 doesn't matter (here it doesn't even exist).",
         [](StepCtx& s) {
             s.param("a", 0.001);
             s.dur = 5.f;
             s.ask("What is the limit of sin(x)/x as x \xE2\x86\x92 0?", "1");
         }},
        {"A hole in the graph",
         "$$g(x) = (x^2 - 1)/(x - 1)$$\n\n"
         "Factor the top: $x^2 - 1 = (x - 1)(x + 1)$. For x \xE2\x89\xA0 1 the factors cancel and $g(x) = x + 1$, a straight line "
         "with a single missing point.",
         [](StepCtx& s) {
             s.show(1, {1});
             s.active(1, 1);
             s.fit(1, -2, 4, -1, 5);
             s.param("a", 1.001);
             s.dur = 3.f;
             s.ask("What is the limit of (x\xC2\xB2 \xE2\x88\x92 1)/(x \xE2\x88\x92 1) as x \xE2\x86\x92 1?", "2", "It equals x + 1 near x = 1.");
         }},
        {"One-sided limits",
         "$h(x)$ rounds down to a whole number. Approaching 2 from the **left**, $h(x) = 1$; from the **right**, $h(x) = 2$.\n\n"
         "The two **one-sided limits** differ, so the limit at 2 does not exist. A limit exists only when both sides agree.",
         [](StepCtx& s) {
             s.show(1, {2});
             s.active(1, 2);
             s.fit(1, -0.5, 4.5, -0.8, 4.2);
             s.param("a", 1.98);
             s.ask("What is the limit of floor(x) as x \xE2\x86\x92 2 from the left?", "1");
         }},
    };
    return L;
}

LessonDef calcDerivative() {
    LessonDef L;
    L.id = "calc-derivative";
    L.subject = "Calculus";
    L.title = "2. The derivative";
    L.blurb = "From the slope of a secant line to the slope of the tangent: the derivative.";
    L.setup = [](App& a) {
        setup(a, "Lesson: the derivative",
              {{"f(x) = x^2", YELLOW_C, ""},
               {"s(x) = f(a) + (f(a + h) - f(a))/h (x - a)", BLUE_C, "secant line"},
               {"p(x) = x^3", PURPLE_C, ""}},
              {{"a", 1, -2, 2}, {"h", 1.5f, 0.001f, 2}}, 1, false);
    };
    L.steps = {
        {"Average rate of change",
         "How steep is $f(x) = x^2$ at $x = a$? Start with two points: $x = a$ and $x = a + h$.\n\n"
         "The blue **secant line** through them has slope\n\n"
         "$$(f(a + h) - f(a))/h$$\n\n"
         "the average rate of change of $f$ over that step.",
         [](StepCtx& s) {
             s.show(1, {0, 1});
             s.active(1, 0);
             s.pane(1).trace = "a";
             s.fit(1, -2.5, 3.6, -1, 7);
             s.param("a", 1).param("h", 1.5);
             s.ask("With a = 1 and h = 1.5, what is the secant's slope?", "3.5", "(f(2.5) \xE2\x88\x92 f(1)) / 1.5 = (6.25 \xE2\x88\x92 1) / 1.5");
         }},
        {"Shrink the step",
         "Now let $h$ shrink towards 0. The second point slides down the curve, and the secant line settles onto the line "
         "that just touches the curve at $x = a$: the **tangent line**.\n\n"
         "Its slope settles at 2.",
         [](StepCtx& s) {
             s.param("h", 0.001);
             s.dur = 5.f;
         }},
        {"The derivative",
         "That limiting slope is the **derivative** of $f$ at $a$:\n\n"
         "**f\xE2\x80\xB2(a)** = the limit, as $h$ approaches 0, of\n\n$$(f(a + h) - f(a))/h$$\n\n"
         "For $f(x) = x^2$: $((a + h)^2 - a^2)/h = 2 a + h$, which tends to $2 a$. So f\xE2\x80\xB2(x) = $2 x$.\n\n"
         "The green curve is f\xE2\x80\xB2. The tangent at the probe point (drag it along the curve) has slope f\xE2\x80\xB2 there.",
         [](StepCtx& s) {
             s.show(1, {0});
             s.pane(1).deriv = true;
             s.pane(1).probeX = 1;
         }},
        {"Moving the point",
         "The derivative is itself a function: at each $x$ it gives the slope of $f$ there. "
         "Left of 0 the parabola falls (negative slope), right of 0 it rises.",
         [](StepCtx& s) {
             s.pane(1).probeX = -1.5;
             s.param("a", -1.5);
             s.dur = 3.f;
             s.ask("What is the slope of x\xC2\xB2 at x = 3?", "6", "f\xE2\x80\xB2(x) = 2x");
         }},
        {"The power rule",
         "The same calculation for $x^3$ gives $3 x^2$, and in general\n\n"
         "**the derivative of $x^n$ is $n x^(n - 1)$**\n\n"
         "Here is $p(x) = x^3$ with its derivative $3 x^2$ in green.",
         [](StepCtx& s) {
             s.show(1, {2});
             s.active(1, 2);
             s.pane(1).trace = "";
             s.pane(1).probeX = 1;
             s.fit(1, -2.2, 2.2, -4, 6);
             s.ask("What is the slope of x\xC2\xB3 at x = 2?", "12", "3x\xC2\xB2 at x = 2");
         }},
    };
    return L;
}

LessonDef calcShape() {
    LessonDef L;
    L.id = "calc-shape";
    L.subject = "Calculus";
    L.title = "3. What f\xE2\x80\xB2 tells you";
    L.blurb = "Increasing, decreasing, maxima, minima and concavity, read from derivatives.";
    L.setup = [](App& a) {
        setup(a, "Lesson: what the derivative tells you",
              {{"f(x) = x^3 - 3x", YELLOW_C, ""}, {"d(x) = 3x^2 - 3", GREEN_C, "f\xE2\x80\xB2(x) = 3x\xC2\xB2 \xE2\x88\x92 3"},
               {"e(x) = 6x", PINK, "f\xE2\x80\xB3(x) = 6x"}},
              {{"a", -1.8f, -2.5f, 2.5f}}, 1, false);
    };
    L.steps = {
        {"Rising and falling",
         "$$f(x) = x^3 - 3 x$$\n\n"
         "and its derivative (green) f\xE2\x80\xB2(x) = $3 x^2 - 3$.\n\n"
         "- where f\xE2\x80\xB2 > 0, $f$ is **increasing**\n"
         "- where f\xE2\x80\xB2 < 0, $f$ is **decreasing**\n\n"
         "Move $a$ and compare the two dots.",
         [](StepCtx& s) {
             s.show(1, {0, 1});
             s.active(1, 0);
             s.pane(1).trace = "a";
             s.fit(1, -2.7, 2.7, -4.2, 4.2);
             s.param("a", -1.8);
         }},
        {"Flat spots",
         "Where f\xE2\x80\xB2(x) = 0 the tangent is flat: a **critical point**. Solve $3 x^2 - 3 = 0$: $x = -1$ or $x = 1$.\n\n"
         "- at $x = -1$, f\xE2\x80\xB2 changes from + to \xE2\x88\x92: a **local maximum**, $f(-1) = 2$\n"
         "- at $x = 1$, f\xE2\x80\xB2 changes from \xE2\x88\x92 to +: a **local minimum**, $f(1) = -2$",
         [](StepCtx& s) {
             s.param("a", 1);
             s.dur = 3.f;
             s.ask("At what x is the local maximum?", "-1");
         }},
        {"Concavity",
         "The **second derivative** f\xE2\x80\xB3(x) = $6 x$ (pink) is the slope of the slope.\n\n"
         "- f\xE2\x80\xB3 < 0: the curve bends down (**concave down**), like a cap\n"
         "- f\xE2\x80\xB3 > 0: it bends up (**concave up**), like a cup\n\n"
         "It switches at an **inflection point**.",
         [](StepCtx& s) {
             s.show(1, {0, 2});
             s.param("a", 0);
             s.ask("At what x is the inflection point?", "0");
         }},
    };
    return L;
}

LessonDef calcIntegral() {
    LessonDef L;
    L.id = "calc-integral";
    L.subject = "Calculus";
    L.title = "4. Area under a curve";
    L.blurb = "Riemann sums of rectangles, and their limit: the definite integral.";
    L.setup = [](App& a) {
        setup(a, "Lesson: area under a curve",
              {{"f(x) = x^2", YELLOW_C, ""}, {"R(x) = (2 floor(n x / 2) / n)^2 (sign(x (2 - x)) + 1) / 2", BLUE_C, "rectangles"}},
              {{"n", 4, 1, 64}}, 1, false);
    };
    L.steps = {
        {"The area problem",
         "What is the area between $f(x) = x^2$ and the x-axis, from $x = 0$ to $x = 2$?\n\n"
         "The curve isn't made of straight edges, so there is no simple formula. The trick: approximate with shapes we can measure.",
         [](StepCtx& s) {
             s.show(1, {0});
             s.active(1, 0);
             KeyPane& p = s.pane(1);
             p.shadeRow = 0;
             p.shadeLo = "0";
             p.shadeHi = "2";
             s.fit(1, -0.4, 2.6, -0.6, 4.6);
         }},
        {"Rectangles",
         "Split [0, 2] into $n$ strips of width $2/n$. Over each strip, use a rectangle as tall as $f$ at the strip's **left** edge.\n\n"
         "With $n = 4$: heights $f(0), f(0.5), f(1), f(1.5)$, so the area is\n\n"
         "$$0.5 (0 + 0.25 + 1 + 2.25) = 1.75$$\n\n"
         "an underestimate, since the curve rises.",
         [](StepCtx& s) {
             s.show(1, {0, 1});
             s.pane(1).shadeRow = 1;
             s.param("n", 4);
             s.ask("What is the left sum with n = 2 strips?", "1", "Widths 1, heights f(0) and f(1).");
         }},
        {"More rectangles",
         "Let $n$ grow. The rectangles hug the curve more and more closely and the total settles down: this is a **Riemann sum**, "
         "and it approaches $8/3 = 2.667$.",
         [](StepCtx& s) {
             s.param("n", 64);
             s.dur = 5.f;
         }},
        {"The definite integral",
         "The limit of the sums is the **definite integral**:\n\n"
         "**the integral of $x^2$ from 0 to 2 is $8/3$**\n\n"
         "It can be computed exactly with an **antiderivative**, a function whose derivative is $x^2$: "
         "$F(x) = x^3/3$, and the area is $F(2) - F(0) = 8/3$. The next lesson shows why that works.",
         [](StepCtx& s) {
             s.show(1, {0});
             s.pane(1).shadeRow = 0;
             s.ask("What is the area under x\xC2\xB2 from 0 to 3?", "9", "F(3) \xE2\x88\x92 F(0) with F(x) = x\xC2\xB3/3");
         }},
    };
    return L;
}

LessonDef calcFTC() {
    LessonDef L;
    L.id = "calc-ftc";
    L.subject = "Calculus";
    L.title = "5. The fundamental theorem";
    L.blurb = "The area function's slope is the original curve: integration undoes differentiation.";
    L.setup = [](App& a) {
        setup(a, "Lesson: the fundamental theorem of calculus",
              {{"f(x) = cos(x)", YELLOW_C, ""}, {"A(x) = sum(k, 1, 240, f((k - 1/2) x / 240) x / 240)", BLUE_C, "area from 0 to x"}},
              {{"a", 1, 0, 6.5f}}, 1, false);
    };
    L.steps = {
        {"An area function",
         "Fix the left end at 0 and let the right end move. The shaded **signed area** under $f(x) = cos(x)$ from 0 to $a$ "
         "is a number that depends on $a$: call it $A(a)$.\n\n"
         "Area below the x-axis counts as negative.",
         [](StepCtx& s) {
             s.show(1, {0});
             s.active(1, 0);
             KeyPane& p = s.pane(1);
             p.shadeRow = 0;
             p.shadeLo = "0";
             p.shadeHi = "a";
             p.trace = "a";
             s.fit(1, -0.5, 7, -1.6, 1.6);
             s.param("a", 1);
         }},
        {"Plot the area",
         "The blue curve is $A(x)$. Watch its height at $a$: it always equals the shaded area. It rises while cos is positive, "
         "falls once the new area is negative, and is back near 0 at $a = pi$.",
         [](StepCtx& s) {
             s.show(1, {0, 1});
             s.param("a", 5.5);
             s.dur = 6.f;
         }},
        {"The slope of the area",
         "Nudge $a$ by a tiny $h$: the area grows by a thin strip about $f(a)$ tall and $h$ wide. So the rate the area grows is the height of the curve:\n\n"
         "**A\xE2\x80\xB2(x) = f(x)**\n\n"
         "That is the **fundamental theorem of calculus**. The blue curve is $sin(x)$, whose derivative is $cos(x)$. "
         "Every integral can be computed from an antiderivative.",
         [](StepCtx& s) {
             s.param("a", 1.5708);
             s.dur = 2.f;
             s.ask("What is the area under cos(x) from 0 to \xCF\x80/2?", "1", "An antiderivative of cos is sin.");
         }},
    };
    return L;
}

LessonDef calcTaylor() {
    LessonDef L;
    L.id = "calc-taylor";
    L.subject = "Calculus";
    L.title = "6. Taylor series";
    L.blurb = "Polynomials that copy a function's derivatives at a point, and get better with every term.";
    L.setup = [](App& a) {
        setup(a, "Lesson: Taylor series",
              {{"sin(x)", RED_C, ""}, {"T(x) = sum(k, 0, n, (-1)^k x^(2k + 1)/(2k + 1)!)", YELLOW_C, "Taylor polynomial"}},
              {{"n", 0, 0, 12}}, 1, false);
    };
    L.steps = {
        {"Matching at a point",
         "Can a polynomial imitate $sin(x)$? Start at $x = 0$, where $sin(0) = 0$ and the slope is $cos(0) = 1$.\n\n"
         "The line $T(x) = x$ has the same value and slope there. Close to 0 it is a good fit; further away it is not.",
         [](StepCtx& s) {
             s.show(1, {0, 1});
             s.active(1, 1);
             s.fit(1, -9, 9, -3, 3);
             s.param("n", 0);
         }},
        {"Adding a term",
         "Match the next derivatives too. The derivatives of sin at 0 cycle $0, 1, 0, -1, ...$, which gives\n\n"
         "$$T(x) = x - x^3/(3!)$$\n\n"
         "It follows the curve much further.",
         [](StepCtx& s) {
             s.param("n", 1);
             s.dur = 1.5f;
         }},
        {"More terms",
         "Keep going:\n\n"
         "$$sin(x) = x - x^3/(3!) + x^5/(5!) - x^7/(7!)$$\n\nand so on forever. "
         "Every term widens the stretch where the polynomial and the curve agree. With infinitely many terms they agree everywhere.",
         [](StepCtx& s) {
             s.param("n", 7);
             s.dur = 4.f;
         }},
        {"Why it works",
         "A polynomial $c_0 + c_1 x + c_2 x^2$ + \xE2\x8B\xAF has $k$-th derivative $k! c_k$ at 0. To copy $f$'s derivatives, choose\n\n"
         "$c_k$ = (the $k$-th derivative of $f$ at 0) divided by $k!$\n\n"
         "The factorials grow so fast that the terms eventually shrink for any $x$.",
         [](StepCtx& s) {
             s.param("n", 2);
             s.ask("What is x \xE2\x88\x92 x\xC2\xB3/6 at x = 1?", "1 - 1/6", "", 0.001);
         }},
    };
    return L;
}

// ==================================================================== linear algebra

const std::vector<DemoRow> kNoRows = {{"x", GREY, ""}};

LessonDef laVectors() {
    LessonDef L;
    L.id = "la-vectors";
    L.subject = "Linear algebra";
    L.title = "1. Vectors and combinations";
    L.blurb = "Vectors as arrows, adding and scaling them, and the span of two vectors.";
    L.setup = [](App& a) { setup(a, "Lesson: vectors", kNoRows, {}, 1, false); };
    L.steps = {
        {"Arrows with coordinates",
         "A vector like $(2, 1)$ is an arrow from the origin: 2 steps right, 1 step up.\n\n"
         "Two special vectors: **î** $= (1, 0)$ (green) and **ĵ** $= (0, 1)$ (red). Every vector is built from them:\n\n"
         "**(2, 1) = 2\xC3\xAE + 1\xC4\xB5**\n\n"
         "Drag the tip of the yellow arrow; it snaps to half-steps.",
         [](StepCtx& s) {
             matrixPane(s, 1, kMatGrid | kMatBasis | kMatVector);
             s.matrix(1, 1, 0, 0, 1);
             s.fit(1, -4.5, 4.5, -3.4, 3.4);
             s.pane(1).vx = 2;
             s.pane(1).vy = 1;
         }},
        {"Adding and scaling",
         "- **Scaling** multiplies each coordinate: 3\xC2\xB7(1, 2) = (3, 6), three times as long, same direction\n"
         "- **Adding** adds coordinates: (1, 2) + (2, \xE2\x88\x92" "1) = (3, 1), the tip of one arrow placed at the tail of the other\n\n"
         "Scaling and adding together give a **linear combination**: $a v + b w$.",
         [](StepCtx& s) {
             s.pane(1).vx = -1;
             s.pane(1).vy = 2;
             s.ask("What is the x-coordinate of 3(1, 2) + (2, \xE2\x88\x92" "1)?", "5");
         }},
        {"Other building blocks",
         "Nothing forces us to build from î and ĵ. Take $u = (1, 1)$ (green) and $w = (-1, 2)$ (red). "
         "The yellow arrow is now $v_1 u + v_2 w$, built from the same two numbers.\n\n"
         "The grid shows every combination with whole-number weights. With these two vectors you can still reach every point of the plane.",
         [](StepCtx& s) {
             s.matrix(1, 1, -1, 1, 2);
             s.pane(1).vx = 1;
             s.pane(1).vy = 1;
             s.dur = 2.5f;
         }},
        {"Span",
         "If the two vectors point along the same line, as $(1, 0.5)$ and $(2, 1)$ do, every combination stays on that line. "
         "The set of all combinations, the **span**, is only a line, not the plane.\n\n"
         "Vectors like this are **linearly dependent**: one is a multiple of the other.",
         [](StepCtx& s) {
             s.matrix(1, 1, 2, 0.5, 1);
             s.dur = 2.5f;
             s.ask("(2, 1) is what multiple of (1, 0.5)?", "2");
         }},
    };
    return L;
}

LessonDef laTransform() {
    LessonDef L;
    L.id = "la-transform";
    L.subject = "Linear algebra";
    L.title = "2. Matrices are transformations";
    L.blurb = "A matrix moves the whole plane, and its columns say where \xC3\xAE and \xC4\xB5 land.";
    L.setup = [](App& a) { setup(a, "Lesson: matrices as transformations", kNoRows, {}, 1, false); };
    L.steps = {
        {"Moving the plane",
         "A **linear transformation** moves every point of the plane so that\n\n"
         "- grid lines stay straight, parallel and evenly spaced\n"
         "- the origin stays put\n\n"
         "Right now nothing has moved: this is the **identity**.",
         [](StepCtx& s) {
             matrixPane(s, 1, kMatGrid | kMatBasis);
             s.matrix(1, 1, 0, 0, 1);
             s.fit(1, -5, 5, -3.8, 3.8);
         }},
        {"Rotation",
         "Rotate everything a quarter turn. î lands on $(0, 1)$ and ĵ lands on $(-1, 0)$.\n\n"
         "Write those two landing spots as the **columns** of a grid of numbers, the **matrix** at the top left. "
         "First column: where î goes (green). Second column: where ĵ goes (red).",
         [](StepCtx& s) {
             s.matrix(1, 0, -1, 1, 0);
             s.dur = 2.5f;
         }},
        {"Shear",
         "Keep î where it is and push ĵ to $(1, 1)$: a **shear**. Squares become slanted parallelograms.",
         [](StepCtx& s) {
             s.matrix(1, 1, 1, 0, 1);
             s.dur = 2.5f;
         }},
        {"Any matrix",
         "Any four numbers make a transformation: the first column says where î lands, the second where ĵ lands, "
         "and everything else follows. Use the panel to type your own, or pick a preset.",
         [](StepCtx& s) {
             s.matrix(1, 2, -1, 1, 1);
             s.dur = 2.5f;
             s.ask("A matrix has rows (3, 1) and (2, 4). What is the y-coordinate of where \xC3\xAE lands?", "2", "Read down the first column.");
         }},
        {"Matrix times vector",
         "Since $v = v_1 i + v_2 j$ and lines stay evenly spaced, $v$ lands on $v_1$ (first column) + $v_2$ (second column):\n\n"
         "**Mv = v\xE2\x82\x81\xC2\xB7(2, 1) + v\xE2\x82\x82\xC2\xB7(\xE2\x88\x92" "1, 1)**\n\n"
         "That is matrix-vector multiplication. The dashed arrow is $v$, the yellow one is $M v$; drag it.",
         [](StepCtx& s) {
             matrixPane(s, 1, kMatGrid | kMatBasis | kMatVector);
             s.pane(1).vx = 1;
             s.pane(1).vy = 2;
             s.ask("For this matrix and v = (2, 1), what is the x-coordinate of Mv?", "3", "2\xC2\xB7(2, 1) + 1\xC2\xB7(\xE2\x88\x92" "1, 1)");
         }},
    };
    return L;
}

LessonDef laDeterminant() {
    LessonDef L;
    L.id = "la-det";
    L.subject = "Linear algebra";
    L.title = "3. The determinant";
    L.blurb = "How much a matrix scales area, when it flips orientation, and when it squashes the plane flat.";
    L.setup = [](App& a) { setup(a, "Lesson: the determinant", kNoRows, {}, 1, false); };
    L.steps = {
        {"Area scale factor",
         "Watch the unit square, the one with sides î and ĵ (yellow). After the transformation it is a parallelogram.\n\n"
         "Its area is the **determinant**. Every region's area is scaled by the same factor. Here the plane is stretched "
         "2 times one way and 1.5 times the other, so areas grow 3 times.",
         [](StepCtx& s) {
             matrixPane(s, 1, kMatGrid | kMatBasis | kMatDet);
             s.matrix(1, 2, 0, 0, 1.5);
             s.fit(1, -3.5, 5, -2.8, 3.6);
             s.dur = 2.f;
         }},
        {"Shear keeps area",
         "A shear slants the square but keeps its base and height, so the area stays 1: $\\det = 1$.",
         [](StepCtx& s) { s.matrix(1, 1, 1, 0, 1); s.dur = 2.f; }},
        {"The formula",
         "For the matrix with rows $(a, b)$ and $(c, d)$:\n\n"
         "$$\\det = a d - b c$$\n\n"
         "Here $3 * 2 - 1 * 1 = 5$.",
         [](StepCtx& s) {
             s.matrix(1, 3, 1, 1, 2);
             s.fit(1, -3, 6, -2.5, 4.5);
             s.dur = 2.f;
             s.ask("What is the determinant of the matrix with rows (3, 1) and (2, 4)?", "10", "ad \xE2\x88\x92 bc");
         }},
        {"Negative determinant",
         "Here î and ĵ have swapped sides: going from green to red now turns clockwise. The plane has been **flipped**, "
         "like a mirror image, and the determinant is negative. Its size is still the area factor: $|\\det| = 3$.",
         [](StepCtx& s) { s.matrix(1, 1, 2, 2, 1); s.dur = 2.5f; }},
        {"Determinant zero",
         "When $\\det = 0$ the square has no area: the whole plane is squashed onto a line (or a point). "
         "Information is lost, so there is no way to undo it. This matrix has columns $(1, 0.5)$ and $(2, 1)$, which line up.",
         [](StepCtx& s) {
             s.matrix(1, 1, 2, 0.5, 1);
             s.dur = 2.5f;
             s.ask("What is the determinant of the matrix with rows (1, 2) and (0.5, 1)?", "0");
         }},
    };
    return L;
}

LessonDef laSystems() {
    LessonDef L;
    L.id = "la-systems";
    L.subject = "Linear algebra";
    L.title = "4. Solving Mx = b";
    L.blurb = "Systems of equations as finding the vector that lands on a target, and the inverse matrix.";
    L.setup = [](App& a) { setup(a, "Lesson: solving Mx = b", kNoRows, {}, 1, false); };
    L.steps = {
        {"Two equations, one picture",
         "The system\n\n"
         "$$2 x_1 + x_2 = 3$$\n\n"
         "$$x_1 + x_2 = 1$$\n\n"
         "says $M x = b$, with $M$ the matrix at the top left and $b = (3, 1)$ (the gold target). "
         "Solving it means finding the input $x$ that $M$ sends onto $b$.\n\n"
         "Drag the yellow arrow onto the target.",
         [](StepCtx& s) {
             matrixPane(s, 1, kMatGrid | kMatBasis | kMatVector | kMatTarget);
             s.matrix(1, 2, 1, 1, 1);
             s.fit(1, -3.5, 5.5, -2.8, 3.8);
             KeyPane& p = s.pane(1);
             p.vx = 1; p.vy = 1; p.bx = 3; p.by = 1;
             s.ask("What is x\xE2\x82\x81 in the solution?", "2", "Subtract the second equation from the first.");
         }},
        {"Undoing the transformation",
         "To solve for any $b$, run the transformation **backwards**: the **inverse matrix** $M^(-1)$ undoes $M$.\n\n"
         "$$x = M^(-1) b$$\n\n"
         "For rows $(a, b)$, $(c, d)$ the inverse is $1/(a d - b c)$ times the matrix with rows $(d, -b)$ and $(-c, a)$. "
         "Here $\\det = 1$, so $M^(-1)$ has rows $(1, -1)$ and $(-1, 2)$, and $x = (3 - 1, -3 + 2) = (2, -1)$.\n\n"
         "Watch the plane unwind back to the grid you started with.",
         [](StepCtx& s) {
             s.pane(1).vx = 2; s.pane(1).vy = -1;
             s.matrix(1, 1, 0, 0, 1);
             s.dur = 3.f;
         }},
        {"When there is no inverse",
         "If $\\det = 0$ the plane collapses onto a line. A target $b$ off that line can't be reached at all, and one on the line "
         "is reached by infinitely many inputs. There is no inverse.\n\n"
         "Here the columns are $(1, 0.5)$ and $(2, 1)$; the target $(3, 1)$ is not on their line.",
         [](StepCtx& s) {
             s.matrix(1, 1, 2, 0.5, 1);
             s.pane(1).vx = 1; s.pane(1).vy = 1;
             s.dur = 2.5f;
             s.ask("What is this matrix's determinant?", "0");
         }},
    };
    return L;
}

LessonDef laEigen() {
    LessonDef L;
    L.id = "la-eigen";
    L.subject = "Linear algebra";
    L.title = "5. Eigenvectors";
    L.blurb = "The directions a matrix only stretches, and by how much: eigenvectors and eigenvalues.";
    L.setup = [](App& a) { setup(a, "Lesson: eigenvectors", kNoRows, {}, 1, false); };
    L.steps = {
        {"Knocked off their line",
         "Most vectors change direction when a matrix acts on them. Here $v = (1, 1)$ (dashed) lands on $M v = (4, 2)$: "
         "a different direction.\n\n"
         "Drag the yellow tip around and watch how $v$ and $M v$ point different ways.",
         [](StepCtx& s) {
             matrixPane(s, 1, kMatGrid | kMatBasis | kMatVector);
             s.matrix(1, 3, 1, 0, 2);
             s.fit(1, -4.5, 6.5, -3.6, 4.6);
             s.pane(1).vx = 1;
             s.pane(1).vy = 1;
             s.dur = 2.f;
         }},
        {"Vectors that stay on their line",
         "Some special directions are only **stretched**: $M v = lambda v$. These are **eigenvectors**, and the stretch factor "
         "$lambda$ is the **eigenvalue**.\n\n"
         "The pink and teal lines are this matrix's two eigen-directions: everything on the pink line is stretched by 3, "
         "everything on the teal line by 2.",
         [](StepCtx& s) { matrixPane(s, 1, kMatGrid | kMatBasis | kMatVector | kMatEigen); }},
        {"Try one",
         "Put $v = (-1, 1)$ on the teal line: $M v = (-2, 2) = 2 v$. Same direction, twice as long.",
         [](StepCtx& s) {
             s.pane(1).vx = -1;
             s.pane(1).vy = 1;
             s.ask("For v = (1, 0), Mv is v times what number?", "3", "Mv = (3, 0)");
         }},
        {"Rotations have none",
         "A rotation turns **every** direction, so it has no real eigenvectors. (Its eigenvalues are complex numbers.)",
         [](StepCtx& s) { s.matrix(1, 0, -1, 1, 0); s.dur = 2.5f; }},
        {"Finding them",
         "$M v = lambda v$ means $(M - lambda I) v = 0$ for some nonzero $v$, so $M - lambda I$ must squash the plane: "
         "its determinant is 0.\n\n"
         "$$(a - lambda)(d - lambda) - b c = 0$$\n\n"
         "For rows $(2, 1)$ and $(1, 2)$: $(2 - lambda)^2 - 1 = 0$, so $lambda = 1$ or $lambda = 3$.",
         [](StepCtx& s) {
             s.matrix(1, 2, 1, 1, 2);
             s.pane(1).vx = 1;
             s.pane(1).vy = 1;
             s.dur = 2.5f;
             s.ask("What is the larger eigenvalue of the matrix with rows (2, 0) and (0, 5)?", "5", "A diagonal matrix stretches along the axes.");
         }},
    };
    return L;
}

}  // namespace

const std::vector<LessonDef>& lessonLibrary() {
    static const std::vector<LessonDef> lib = {
        trigCircle(),  trigWaves(),      trigTan(),      trigTransform(),
        calcLimits(),  calcDerivative(), calcShape(),    calcIntegral(), calcFTC(), calcTaylor(),
        laVectors(),   laTransform(),    laDeterminant(), laSystems(),   laEigen(),
    };
    return lib;
}
