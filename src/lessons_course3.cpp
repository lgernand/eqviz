// Calculus course: Part IV, multivariable and vector calculus (Units 9-12).
#include "lesson_kit.hpp"

using namespace theme;
using namespace kit;

namespace {

// ==================================================================== Unit 9

LessonDef unit9() {
    LessonDef L = unit("cc-u9", kPart4, "Unit 9. Vectors and motion in space", "Dot and cross products, curvature, projectiles, and Kepler's laws from Newton's.");
    L.setup = [](App& a) {
        setup(a, "Unit 9: vectors and motion in space", {{"x", GREY, ""}},
              {{"v", 50, 5, 80}, {"alpha", 0.5236f, 0.05f, 1.55f}, {"s", 1, 0, 8}, {"k", 0.6f, 0, 0.9f}, {"R", 2, 0.5f, 4}}, 1, false);
    };
    UnitText u;
    u.bigIdea = "Vectors let one equation describe motion in three dimensions. With calculus, they derive Kepler's laws from Newton's.";
    u.topics = "Vectors; dot and cross products; lines and planes; vector-valued functions and their derivatives and integrals; arc length and curvature; "
               "unit tangent and normal vectors; velocity and acceleration, with tangential and normal components; Kepler's laws from Newton's law of gravitation.";
    u.weeks = "3 weeks";
    u.people = "Kepler, Tycho Brahe, Halley, Newton, Hamilton, Gibbs, Heaviside";
    u.story = "Kepler drew his laws of planetary motion from Tycho Brahe's data (1609 and 1619). In 1684 Edmond Halley asked Newton what orbit an inverse-square "
              "force would produce. Newton's answer grew into the Principia (1687).\n\n"
              "William Rowan Hamilton invented quaternions on 16 October 1843 and carved the formula into Brougham Bridge in Dublin. In the 1880s Josiah Willard Gibbs "
              "and Oliver Heaviside cut quaternions down to the dot and cross products you use today, over loud protests from Peter Guthrie Tait.\n\n"
              "Right: an orbit with the Sun at the focus (the origin).";
    u.read = "- **Math:** Simmons, chapters on vectors and vector functions; OpenStax Volume 3, ch. 2–3; Spivak ch. 17 (Planetary Motion).\n"
             "- **History:** Michael J. Crowe, A History of Vector Analysis (Dover); David Bressoud, Second Year Calculus: From Celestial Mechanics to Special Relativity (1991); Westfall, Never at Rest.\n"
             "- **Primary source:** Newton, Principia, Book I, Propositions 1–11 (Cohen & Whitman translation, 1999).";
    u.prove = "- **(a)** Curvature κ = |v × a| / |v|³.\n- **(b)** Kepler's second law (equal areas in equal times) for any central force.\n"
              "- **(c)** Kepler's first law (elliptical orbits) from the inverse-square law.";
    u.testMinutes = 150;
    u.testName = "Unit 9 test";
    u.test = "**1.** Find the equation of the plane through (1, 0, 2), (2, 1, 0) and (0, 3, 1).\n\n"
             "**2.** Find the distance from (1, 1, 1) to that plane.\n\n"
             "**3.** For the helix r(t) = ⟨cos t, sin t, t⟩, find T, N, the curvature, and the arc length from t = 0 to 2π.\n\n"
             "**4.** A projectile is fired at 50 m/s at 30° above level ground (g = 9.8 m/s²). Find its time of flight, range and maximum height.\n\n"
             "**5.** Prove that if acceleration is always parallel to position, then r × v is constant. Deduce Kepler's second law.\n\n"
             "**6.** Prove that |a × b|² + (a · b)² = |a|²|b|².\n\n"
             "**7.** History: What did Halley's 1684 question require mathematically, and why did Newton take until 1687 to publish?";
    auto projectile = [](StepCtx& s) {
        graph(s, 1, {}, -10, 260, -10, 140);
        curve(s, 1, "v cos(alpha) t", "v sin(alpha) t - 4.9 t^2", "0", "2 v sin(alpha)/9.8", "s");
    };
    auto orbit = [](StepCtx& s) {
        graph(s, 1, {}, -5.6, 1.6, -2.4, 2.4);
        curve(s, 1, "cos(t)/(1 + k cos(t))", "sin(t)/(1 + k cos(t))", "0", "2 pi", "s");
    };
    L.steps = {
        overview(u, [projectile](StepCtx& s) {
            projectile(s);
            s.param("v", 50).param("alpha", 0.5236).param("s", 2);
        }),
        story(u, [orbit](StepCtx& s) {
            orbit(s);
            s.param("k", 0.6).param("s", 1);
        }),
        {"Projectile motion",
         "With position $r(t) = (v cos(alpha) t, v sin(alpha) t - 4.9 t^2)$, the velocity is $r'(t)$ (green arrow) and the acceleration is the constant "
         "$(0, -9.8)$. Each coordinate is a separate one-variable problem. Watch the launch angle rise to 45°, which maximizes the range.",
         [projectile](StepCtx& s) {
             projectile(s);
             s.param("alpha", 0.7854).param("s", 3.6);
             s.dur = 3.f;
             s.ask("A projectile is fired at 50 m/s at 30°. What is its range? (an expression is fine)", "2500 sin(pi/3)/9.8", "Range = v\xC2\xB2 sin(2\xCE\xB1)/g");
         }},
        {"Orbits",
         "Newton showed an inverse-square force gives a conic with the Sun at a focus. Raise the eccentricity $k$ and the circle stretches into an ellipse. "
         "Kepler's second law: the line to the Sun sweeps equal areas in equal times, because $r \\times v$ is constant for any central force.",
         [orbit](StepCtx& s) {
             orbit(s);
             s.param("k", 0.8).param("s", 2.5);
             s.dur = 3.f;
         }},
        {"Curvature",
         "Curvature measures how fast the direction turns per unit length: $kappa = |v \\times a| / |v|^3$. A circle of radius $R$ has $kappa = 1/R$ everywhere: "
         "small circles bend hard.",
         [](StepCtx& s) {
             graph(s, 1, {}, -4.5, 4.5, -4.5, 4.5);
             curve(s, 1, "R cos(t)", "R sin(t)", "0", "2 pi", "s");
             s.param("R", 3).param("s", 1);
             s.ask("What is the curvature of a circle of radius 4?", "1/4");
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is (1, 2, 3) · (4, −5, 6)?", "12", 0),
        typed("What is the z-component of (1, 0, 0) × (0, 1, 0)?", "1", 0),
        typed("What is the length of the vector (2, 3, 6)?", "7", 0),
        typed("A projectile is fired at 20 m/s at 45° (g = 9.8). What is its range?", "400/9.8", 2),
        typed("What is the curvature of a circle of radius 5?", "1/5", 4),
        choice("Kepler's second law holds for", {"any central force", "only the inverse-square force", "only circular orbits", "only the Earth"}, 0, 3),
    };
    return L;
}

// ==================================================================== Unit 10

LessonDef unit10() {
    LessonDef L = unit("cc-u10", kPart4, "Unit 10. Partial derivatives and optimization", "Gradients, tangent planes, the second-derivative test and Lagrange multipliers.");
    L.setup = [](App& a) {
        setup(a, "Unit 10: partial derivatives",
              {{"f(x, y) = x^2 + 3y^2", BLUE_C, ""},                 // 0
               {"g(x, y) = x^3 - 3x y + y^3", PURPLE_C, ""},          // 1
               {"h(x, y) = x y/(x^2 + y^2)", GREEN_C, ""}},           // 2
              {}, 2, true);
    };
    UnitText u;
    u.bigIdea = "The derivative of a function of several variables is a linear map. The gradient points uphill, and Lagrange multipliers optimize under constraints.";
    u.topics = "Functions of several variables and level sets; limits and continuity in several variables; partial derivatives and the equality of mixed partials; "
               "differentiability and tangent planes; the multivariable chain rule; directional derivatives and the gradient; the second-derivative test; "
               "Lagrange multipliers; Taylor polynomials in two variables (optional).";
    u.weeks = "4 weeks";
    u.people = "Euler, Clairaut, Legendre, Lagrange, Jacobi, Schwarz";
    u.story = "Euler and Alexis Clairaut worked out the equality of mixed partial derivatives in the 1730s and 40s. Legendre introduced the symbol ∂ in 1786, and "
              "Jacobi revived it in 1841. Lagrange's Mécanique analytique (1788) introduced his multipliers and famously contains no diagrams at all. "
              "Hermann Schwarz gave the rigorous proof about mixed partials in 1873.";
    u.read = "- **Math:** Simmons, chapter on partial derivatives; OpenStax Volume 3, ch. 4; for the linear-map view, Hubbard & Hubbard, Vector Calculus, Linear Algebra, and Differential Forms (5th ed., 2015).\n"
             "- **History:** Bressoud, Second Year Calculus; Nahin, When Least Is Best, on Lagrange.\n"
             "- **Primary source:** Lagrange, Analytical Mechanics (trans. Boissonnade & Vagliente, 1997), the preface and the opening statics sections.";
    u.prove = "- **(a)** f(x, y) = xy/(x² + y²) has no limit at the origin, though both partials exist there.\n- **(b)** The directional derivative equals ∇f · u.\n"
              "- **(c)** The gradient is perpendicular to level curves.\n- **(d)** The second-derivative test, from the quadratic approximation.";
    u.testMinutes = 150;
    u.testName = "Unit 10 test";
    u.test = "**1.** Show that the limit of x²y/(x⁴ + y²) as (x, y) → (0, 0) does not exist.\n\n"
             "**2.** For f(x, y) = x³y − 2xy² + e^(xy), find all first and second partial derivatives, and verify that the two mixed partials agree.\n\n"
             "**3.** Find the tangent plane to z = x² + 3y² at (1, 1, 4), and use it to approximate z at (1.1, 0.9).\n\n"
             "**4.** Let z = f(x, y) with x = r cos θ and y = r sin θ. Show that (∂z/∂x)² + (∂z/∂y)² = (∂z/∂r)² + (1/r²)(∂z/∂θ)².\n\n"
             "**5.** For f(x, y, z) = xyz, find the directional derivative at (1, 2, 3) toward (2, 4, 5), and the direction and rate of steepest increase.\n\n"
             "**6.** Find and classify all critical points of f(x, y) = x³ − 3xy + y³.\n\n"
             "**7.** Use Lagrange multipliers to find the largest rectangular box, with faces parallel to the coordinate planes, that fits inside the ellipsoid x²/a² + y²/b² + z²/c² = 1.\n\n"
             "**8.** History: The preface to Lagrange's Mécanique analytique promises no figures. What did it mean to do mechanics purely by analysis, and how do multipliers fit that program?";
    auto bowl = [](StepCtx& s) {
        surface(s, 1, 0, 2, 0.3f);
        graph(s, 2, {}, -2.2, 2.2, -2.2, 2.2);
        field(s, 2, "2x", "6y", kFieldArrows | kFieldStream);
    };
    L.steps = {
        overview(u, bowl),
        story(u),
        {"Limits in two variables",
         "In the plane you can approach a point from infinitely many directions. $h(x, y) = x y/(x^2 + y^2)$ is 0 along both axes but $1/2$ along $y = x$: "
         "the surface (top) is a pleated sheet with no single height at the origin. Both partial derivatives exist there anyway.",
         [](StepCtx& s) {
             surface(s, 1, 2, 1.5, 1.f);
             s.ask("What is the value of xy/(x\xC2\xB2 + y\xC2\xB2) along the line y = x?", "1/2");
         }},
        {"The gradient",
         "The partial derivatives form the **gradient** $\\nabla f = (f_x, f_y)$. Bottom: the gradient field of $f = x^2 + 3 y^2$, with flow lines that follow it. "
         "The gradient points straight uphill, perpendicular to the level curves, and its length is the steepest slope.",
         [bowl](StepCtx& s) {
             bowl(s);
             s.ask("What is |\xE2\x88\x87" "f(1, 1)| for f = x\xC2\xB2 + 3y\xC2\xB2?", "sqrt(40)", "\xE2\x88\x87" "f = (2x, 6y)");
         }},
        {"Critical points",
         "Where $\\nabla f = 0$ the tangent plane is flat. For $g = x^3 - 3 x y + y^3$ that happens at $(0, 0)$ and $(1, 1)$. The second-derivative test uses "
         "$g_xx g_yy - g_xy^2$: negative at the origin (a **saddle**), positive with $g_xx > 0$ at $(1, 1)$ (a **minimum**). "
         "In the flow lines below, the saddle is where they come in and leave.",
         [](StepCtx& s) {
             surface(s, 1, 1, 1.5, 0.12f, false, 0.4, 0.4);
             graph(s, 2, {}, -1.4, 2.2, -1.4, 2.2);
             field(s, 2, "3x^2 - 3y", "3y^2 - 3x", kFieldArrows | kFieldStream);
         }},
        {"Lagrange multipliers",
         "To maximize $f$ on a curve $g = c$, look where the level curve of $f$ just touches the constraint: there $\\nabla f = lambda \\nabla g$. Bottom: the gradient "
         "field of $x y$ and the unit circle. At the touching points the arrows are perpendicular to the circle, so $x y$ is largest at $(1/sqrt(2), 1/sqrt(2))$.",
         [](StepCtx& s) {
             graph(s, 2, {}, -1.8, 1.8, -1.8, 1.8);
             field(s, 2, "y", "x", kFieldArrows);
             curve(s, 2, "cos(t)", "sin(t)", "0", "2 pi", "pi/4");
             s.ask("What is the largest value of xy when x + y = 10?", "25");
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("$f = x^2 y$. What is $f_x$ at $(1, 2)$?", "4", 3),
        typed("$f = x^2 + y^2 - 4 x + 6 y$ has one critical point. What is its x-coordinate?", "2", 4),
        choice("For $x^3 - 3 x y + y^3$, the origin is", {"a saddle point", "a local minimum", "a local maximum", "not a critical point"}, 0, 4),
        typed("What is the directional derivative of $x^2 + y^2$ at $(1, 1)$ in the direction $(1, 0)$?", "2", 3),
        typed("Use the tangent plane of $z = x^2 + y^2$ at $(1, 1)$ to estimate $z$ at $(1.1, 1)$.", "2.2", 3),
        choice("$f_xy = f_yx$ is guaranteed when", {"the second partials are continuous", "f is a polynomial in x only", "f is bounded", "always, with no conditions"}, 0, 1),
    };
    return L;
}

// ==================================================================== Unit 11

LessonDef unit11() {
    LessonDef L = unit("cc-u11", kPart4, "Unit 11. Multiple integrals", "Double and triple integrals, polar and spherical coordinates, and the Jacobian.");
    L.setup = [](App& a) {
        setup(a, "Unit 11: multiple integrals",
              {{"p(x, y) = 4 - x^2 - y^2", BLUE_C, ""},              // 0
               {"G(x, y) = exp(-x^2 - y^2)", PURPLE_C, ""},          // 1
               {"c(x, y) = sqrt(4 - x^2 - y^2)", GREEN_C, ""},       // 2
               {"g(x) = exp(-x^2)", YELLOW_C, ""}},                  // 3
              {}, 2, true);
    };
    UnitText u;
    u.bigIdea = "Double and triple integrals add up quantities over regions. Changing variables, with the Jacobian, is substitution in higher dimensions.";
    u.topics = "Double integrals over rectangles and general regions; Fubini's theorem and reversing the order of integration; polar coordinates; mass, center of mass, "
               "moments of inertia and probability; triple integrals; cylindrical and spherical coordinates; general change of variables and the Jacobian.";
    u.weeks = "3 weeks";
    u.people = "Newton, Euler, Lagrange, Jacobi, Fubini";
    u.story = "Euler worked out double integrals in 1769, and Lagrange used triple integrals in spherical coordinates in 1773 to compute the gravitational pull of ellipsoids. "
              "Jacobi's 1841 memoir on functional determinants gave the Jacobian its name. Guido Fubini proved his theorem on iterated integrals in 1907.\n\n"
              "Newton had proved geometrically, in Principia Book I, Propositions 70–71, that a uniform sphere attracts outside bodies as if its mass sat at its center. "
              "You will prove it with an integral.";
    u.read = "- **Math:** Simmons, chapter on multiple integrals; OpenStax Volume 3, ch. 5; Jerrold Marsden & Anthony Tromba, Vector Calculus (6th ed., 2012), for change of variables.\n"
             "- **History:** Bressoud, Second Year Calculus; Victor Katz, A History of Mathematics (3rd ed., 2009), on multiple integrals.\n"
             "- **Primary source:** Newton, Principia, Book I, Propositions 70–71.";
    u.prove = "- **(a)** The integral of e^(−x²) over the whole real line is √π, via polar coordinates.\n- **(b)** The Jacobians for polar and spherical coordinates.\n"
              "- **(c)** A function whose two iterated integrals differ, showing why Fubini needs hypotheses.";
    u.testMinutes = 150;
    u.testName = "Unit 11 test";
    u.test = "**1.** Evaluate ∫₀¹ ∫ₓ¹ e^(y²) dy dx by reversing the order of integration.\n\n"
             "**2.** Find the volume under z = 4 − x² − y² and above the xy-plane.\n\n"
             "**3.** Find the center of mass of a uniform semicircular plate of radius a.\n\n"
             "**4.** Derive the volume of a ball using spherical coordinates. Then find the volume of the \"ice-cream cone\" inside x² + y² + z² = 4 and above z = √(x² + y²).\n\n"
             "**5.** Prove that the integral of e^(−x²) from −∞ to ∞ equals √π.\n\n"
             "**6.** Use u = x − y, v = x + y to evaluate the double integral of cos((x − y)/(x + y)) over the triangle with vertices (0, 0), (1, 0) and (0, 1).\n\n"
             "**7.** Prove Newton's shell theorem: a uniform spherical shell attracts an outside particle as if all its mass were at its center.";
    L.steps = {
        overview(u, [](StepCtx& s) {
            surface(s, 1, 0, 2.2, 0.5f);
            graph(s, 2, {3}, -3, 3, -0.2, 1.2);
            shade(s, 2, 3, "-4", "4");
        }),
        story(u),
        {"Iterated integrals",
         "A double integral adds $f(x, y)$ times tiny areas $d x d y$. Fubini: for nice functions, integrate in $y$ first and then $x$, or the other way round, "
         "and get the same number. Reversing the order can turn an impossible inner integral into an easy one.",
         [](StepCtx& s) { s.ask("What is ∫₀¹ ∫₀¹ (x + y) dy dx?", "1"); }},
        {"Polar coordinates",
         "The paraboloid $z = 4 - x^2 - y^2$ (top) is round, so use polar coordinates. The area element becomes $r d r d theta$: the factor $r$ is the **Jacobian**. "
         "The volume is the integral of $(4 - r^2) r$ over $0$ to $2$ in $r$ and $0$ to $2 pi$ in $theta$.",
         [](StepCtx& s) { s.ask("What is the volume under z = 4 \xE2\x88\x92 x\xC2\xB2 \xE2\x88\x92 y\xC2\xB2 and above the plane?", "8 pi"); }},
        {"The Gaussian integral",
         "Nobody can integrate $e^(-x^2)$ in elementary terms, but its square is easy. Multiply the integral by itself to get the volume under $e^(-x^2 - y^2)$ (top), "
         "switch to polar coordinates, and the extra $r$ makes it integrable: the volume is $pi$. So the area under the bell curve (bottom) is $sqrt(pi)$, about 1.772.",
         [](StepCtx& s) { surface(s, 1, 1, 2.5, 1.2f); }},
        {"Spherical coordinates",
         "For balls and cones use spherical coordinates $(rho, phi, theta)$, with volume element $rho^2 sin(phi) d rho d phi d theta$. Top: the upper half of the ball of radius 2.",
         [](StepCtx& s) {
             surface(s, 1, 2, 2.2, 1.f);
             s.ask("What is the volume of a ball of radius 2?", "32 pi/3");
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is ∫₀² ∫₀¹ xy dy dx?", "1", 2),
        typed("What is the volume under $z = 1 - x^2 - y^2$ and above the xy-plane?", "pi/2", 3),
        choice("The Jacobian for polar coordinates is", {"r", "1", "r\xC2\xB2", "sin \xCE\xB8"}, 0, 3),
        typed("What is ∫ e^(−x²) dx over the whole real line?", "sqrt(pi)", 4),
        typed("Use polar coordinates: what is the area of a disk of radius 3?", "9 pi", 3),
        choice("The spherical volume element is", {"\xCF\x81\xC2\xB2 sin \xCF\x86 d\xCF\x81 d\xCF\x86 d\xCE\xB8", "\xCF\x81 d\xCF\x81 d\xCF\x86 d\xCE\xB8", "sin \xCF\x86 d\xCF\x81 d\xCF\x86 d\xCE\xB8", "\xCF\x81\xC2\xB3 d\xCF\x81 d\xCF\x86 d\xCE\xB8"}, 0, 5),
    };
    return L;
}

// ==================================================================== Unit 12

LessonDef unit12() {
    LessonDef L = unit("cc-u12", kPart4, "Unit 12. Vector calculus", "Line and surface integrals, curl and divergence, and the theorems of Green, Stokes and Gauss.");
    L.setup = [](App& a) { setup(a, "Unit 12: vector calculus", {{"x", GREY, ""}}, {}, 1, false); };
    UnitText u;
    u.bigIdea = "Green's, Stokes' and the divergence theorems are one idea: integrating a derivative over a region equals integrating the function over the region's boundary, ∫_M dω = ∫_∂M ω.";
    u.topics = "Vector fields; line integrals; conservative fields, potentials and the fundamental theorem for line integrals; Green's theorem; curl and divergence; "
               "parametric surfaces and surface integrals; flux; Stokes' theorem; the divergence theorem; fluid flow and Maxwell's equations; a first look at differential forms (optional).";
    u.weeks = "4 weeks";
    u.people = "Green, Ostrogradsky, Gauss, Kelvin, Stokes, Maxwell, Cartan";
    u.story = "George Green, a miller's son from Nottingham with about a year of schooling, taught himself mathematics. In 1828 he published his Essay on electricity "
              "and magnetism by subscription, to about 50 readers. He entered Cambridge at 40, and William Thomson (Lord Kelvin) rescued the Essay from obscurity in 1845.\n\n"
              "Mikhail Ostrogradsky proved the divergence theorem in 1826, and Gauss had special cases. Stokes' theorem first appears in a letter from Thomson to Stokes "
              "in 1850. Stokes set it as a Cambridge prize-exam question in 1854, and James Clerk Maxwell sat that exam. Élie Cartan's differential forms (1899) "
              "unified all three theorems into one formula.";
    u.read = "- **Math:** Simmons, chapter on vector analysis; Schey, Div, Grad, Curl, and All That; OpenStax Volume 3, ch. 6.\n"
             "- **History:** Crowe, A History of Vector Analysis; D. M. Cannell, George Green: Mathematician and Physicist 1793–1841 (2nd ed., 2001); Victor Katz, \"The History of Stokes' Theorem,\" Mathematics Magazine (1979).\n"
             "- **Primary source:** Maxwell, A Treatise on Electricity and Magnetism (1873; Dover), the preliminary chapter on vector quantities.";
    u.prove = "- **(a)** The fundamental theorem for line integrals.\n- **(b)** Green's theorem for a rectangle, then for a region bounded by two graphs.\n"
              "- **(c)** curl(∇f) = 0 and div(curl F) = 0.\n- **(d)** The divergence theorem for a box.";
    u.testMinutes = 180;
    u.testName = "Unit 12 test";
    u.test = "**1.** Show that F = ⟨2xy + z, x² + 2yz, y² + x⟩ is conservative, find a potential, and compute the line integral of F from (0, 0, 0) to (1, 2, 3).\n\n"
             "**2.** Use Green's theorem to find the area inside the ellipse x = a cos t, y = b sin t.\n\n"
             "**3.** Evaluate the line integral of y² dx + 3xy dy counterclockwise around the boundary of the upper half of the unit disk.\n\n"
             "**4.** Compute the divergence and curl of F = ⟨x²y, yz², zx²⟩.\n\n"
             "**5.** Use the divergence theorem to find the outward flux of F = ⟨x³, y³, z³⟩ through the sphere x² + y² + z² = a².\n\n"
             "**6.** Verify Stokes' theorem for F = ⟨−y, x, z⟩ on the upper unit hemisphere, oriented upward.\n\n"
             "**7.** Let F = ⟨−y, x⟩/(x² + y²). Show that curl F = 0 away from the origin, yet the integral of F around the unit circle is 2π. Why doesn't this contradict Green's theorem?\n\n"
             "**8.** History: Trace how Green's 1828 Essay went from about 50 copies in Nottingham to the foundation of mathematical physics.";
    auto fieldPane = [](StepCtx& s, const char* P, const char* Q, int opts) {
        graph(s, 1, {}, -3, 3, -3, 3);
        field(s, 1, P, Q, opts);
    };
    L.steps = {
        overview(u, [fieldPane](StepCtx& s) { fieldPane(s, "-y", "x", kFieldArrows | kFieldStream); }),
        story(u),
        {"Circulation and curl",
         "The field $F = (-y, x)$ spins around the origin. Its **curl** measures spin per unit area: $Q_x - P_y = 2$ everywhere. "
         "**Green's theorem**: the circulation around a closed curve equals the total curl inside. Around the unit circle that is $2 \\times pi = 2 pi$.",
         [fieldPane](StepCtx& s) {
             fieldPane(s, "-y", "x", kFieldArrows | kFieldStream);
             curve(s, 1, "cos(t)", "sin(t)", "0", "2 pi");
             s.ask("What is the z-component of the curl of F = (\xE2\x88\x92y, x)?", "2");
         }},
        {"Divergence",
         "$F = (x, y)$ streams out of the origin. Its **divergence** $P_x + Q_y = 2$ measures outflow per unit area. The **divergence theorem**: the flux out "
         "through a closed boundary equals the total divergence inside. In space, $(x, y, z)$ has divergence 3.",
         [fieldPane](StepCtx& s) {
             fieldPane(s, "x", "y", kFieldArrows | kFieldStream);
             s.ask("What is the divergence of (x, y, z)?", "3");
         }},
        {"Conservative fields",
         "$F = (2 x y, x^2)$ is the gradient of $f = x^2 y$. Its line integrals depend only on the endpoints: f(end) − f(start), the "
         "fundamental theorem for line integrals. Its flow lines never loop, and its curl is 0.",
         [fieldPane](StepCtx& s) {
             fieldPane(s, "2 x y", "x^2", kFieldArrows | kFieldStream);
             s.ask("What is the line integral of \xE2\x88\x87(x\xC2\xB2y) from (0, 0) to (1, 2)?", "2");
         }},
        {"Curl-free but still circulating",
         "F = (−y, x)/(x² + y²) has zero curl everywhere except the origin, where it is undefined. Yet its circulation around the unit circle is $2 pi$. "
         "Green's theorem needs the field defined everywhere inside: the hole at the origin carries all the circulation.",
         [fieldPane](StepCtx& s) {
             fieldPane(s, "-y/(x^2 + y^2)", "x/(x^2 + y^2)", kFieldArrows | kFieldStream);
             curve(s, 1, "cos(t)", "sin(t)", "0", "2 pi");
         }},
        reading(u),
        proofs(u),
        writtenTest(u),
    };
    L.test = {
        typed("What is the divergence of (x², y², z²) at (1, 1, 1)?", "6", 3),
        typed("What is the circulation of (−y, x) counterclockwise around the unit circle?", "2 pi", 2),
        typed("What is the outward flux of (x, y, z) through the unit sphere?", "4 pi", 3),
        choice("The curl of a gradient field is", {"always zero", "always the divergence", "the Laplacian", "undefined"}, 0, 4),
        typed("Use Green's theorem: what area does the ellipse x = 2 cos t, y = 3 sin t enclose?", "6 pi", 2),
    };
    return L;
}

}  // namespace

std::vector<LessonDef> calculusCourse3() { return {unit9(), unit10(), unit11(), unit12()}; }
