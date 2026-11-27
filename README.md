# eqviz: equation visualizer

A C++17 tool for exploring equations, styled after manim / 3Blue1Brown videos: a dark
background, a blue number plane, crisp yellow curves, TeX-style typeset labels and smooth
"smooth"-eased animations.

## Features

- **Equation input** with implicit multiplication (`2x`, `a sin(bx)`), `^`, `!`, `|x|`, `sqrt`,
  trig/hyperbolic/log functions, `pi`, `e`, `i`, `gamma(x)`, `zeta(s)`, and `y = …` / `f(x) = …` forms.
- **Sliders for variables**: any unknown letter (`a`, `omega`, `a_1`) becomes a slider.
  Each one has a ▶ button that sweeps it back and forth, and its range can be edited.
- **Real axes ⇄ complex plane**: on the complex plane, `f(z)` is shown 3b1b-style as a grid
  being transformed by the function, with the real axis (yellow) and imaginary axis (red)
  tracked. Domain colouring is optional; on macOS it is rendered on the GPU at full
  resolution (the expression is compiled to a GLSL shader), falling back to the CPU for
  infinite series, number-theory functions, morphs and zoom depths beyond single precision. The input grid stays fixed while you zoom and pan,
  so zooming magnifies the transformed lines; use *Fit grid to view* or the spacing slider
  to rebuild it.
- **2D ⇄ 3D**: an animated camera move tilts the plane up while the surface rises out of it.
  Real mode plots `z = f(x, y)` as a checkerboard manim surface; complex mode plots the
  analytic landscape `|f(z)|`, coloured by `arg f(z)`.
- **First derivative**: computed symbolically and typeset.
  - Real 2D: `f′` curve plus a draggable tangent line with a live slope readout.
  - Real 3D: `∂f/∂x`, `∂f/∂y` and the tangent plane at a chosen point.
  - Complex: the view morphs from `f(z)` to `f′(z)`.
- **Summation / products**: `sum(k, 1, n, 1/k^2)` (or `sum(k=1, n, …)`, `prod(…)`). A player
  adds the terms one at a time with smooth morphs between partial sums. You set the loop
  speed in terms per second (optionally accelerating), and can play, pause, step, reset,
  show all, or loop. In 2D the previous partial sums trail behind.
- **Infinite series**: use `inf` (or `∞`) as a bound: `sum(k, 0, inf, x^k)`, or over all
  integers with `sum(n, -inf, inf, …)`. The player animates the first N partial sums, then
  glides to the true limit. Limits are computed by sampling partial sums at doubling
  checkpoints and extrapolating the tail geometrically. That converges p-series and
  Dirichlet series like `sum 1/n^z` in a few hundred terms. Where a series diverges (e.g.
  `x^k` for |x| ≥ 1, `1/n^z` for Re z ≤ 1) or does not settle within *Max terms*, the graph
  is left blank, so the region of convergence shows up directly.
- **Riemann zeta**: `zeta(s)` is the full analytic continuation (Borwein's accelerated
  series, plus the functional equation for Re s < 0), accurate to ~1e-15 near the critical
  line. The examples include the critical line |ζ(1/2 + ix)|, the complex grid transformed
  by ζ, ζ as the series Σ 1/nᶻ, and the landscape around the first zero.
- **Projects with multiple functions**: each row is a function with its own colour and
  visibility toggle, and all visible rows are drawn together. Name a row (`f(x) = …`,
  `L(u) = …`, `h(a, b) = …`) and any other row can call it: `g(x) = f(x)^2 + f(2x)`. Calls
  are inlined before evaluation, with checks for circular definitions and argument counts.
  Each row can also get a display name (the **Aa** button), shown on the graph and in hover
  readouts instead of the formula. The highlighted row drives the sum player, derivative, 3D
  and complex views. Projects
  save to and open from `~/Documents/eqviz` (plain text `.eqviz` files).
- **Multiple coordinate planes**: add panes to the left, right, top or bottom of the focused
  pane, close them, and drag the gaps between panes to resize. Each pane has its own mode
  (real or complex, 2D or 3D), view, main function and visible rows; click a pane to edit it.
  Optionally the workspace can be larger than the window (up to 5× each way); scroll it with
  two fingers on a trackpad or with the scrollbars.
- **Millennium Prize Problems**: Project, then Open lists one demo project per problem:
  - **P vs NP**: polynomial n^k against 2ⁿ and n! running times, linear and log scale.
  - **Hodge conjecture**: the Fermat cubic surface, |w| on the curve w² = z³ − z over ℂ, and
    its real points (illustrations; the conjecture concerns complex varieties).
  - **Yang–Mills mass gap**: Coulomb vs Yukawa forces, the gap in E = √(p² + m²), the
    confining quark potential, and a BPST instanton.
  - **Birch and Swinnerton-Dyer**: an adjustable elliptic curve, and their original experiment,
    log ∏ N_p/p against ln ln x for curves of rank 0–3 (point counts via `legendre`).
  - Riemann, Poincaré and Navier–Stokes, below.
- **Note panes**: choose "New panes: note" and add a pane to get explanatory text with typeset
  maths: `# Heading`, `## Subheading`, `- bullet`, `**bold**`, inline `$x^2$`, and displayed
  `$$ e^(i pi) + 1 = 0 $$` equations (relations `= < > <= >= != ~ ->`, lists and tuples, and
  `\partial`, `\nabla`, `\Ric` style upright symbols). Edit the text in the panel; notes are saved
  with the project. Each Millennium demo opens with a note explaining the problem.
- **Tall panes**: with "Let panes extend past the window" on, panes added above or below get a
  full window of height, dragging a gap down makes a pane taller than the window, and you scroll
  down through the workspace.
- **Riemann Hypothesis demo** (Project, then Open): on the left, ζ(z) transforming the
  complex plane; on the right, the prime staircase π₀(x), Riemann's
  smooth R(x), |ζ(½ + ix)| touching zero at each γₖ, and the explicit-formula curve
  π_N(x) = R(x) − Σ 2 Re R(x^ρₖ), built from the first N zeros. The player adds zeros one
  at a time and the curve locks onto the staircase.
- **Poincaré conjecture demo** (Project, then Open): Ricci flow ∂g/∂t = −2 Ric(g) in four
  panes driven by the time slider t. A dumbbell's neck pinches off first and the pieces round
  out (schematic: each cross-section follows r² → r² − 4t). A round sphere shrinks to a point
  with radius² = 1 − 4t. Curvature R = 6/ρ² and volume V = 2π²ρ³ are plotted against time.
- **Navier–Stokes demo** (Project, then Open): exact viscous flows driven by time t and
  viscosity ν: the decaying Taylor–Green vortex, a diffusing Lamb–Oseen vortex and its swirl
  profile, Stokes' suddenly started plate (u = erfc(y / 2√(νt))), and Poiseuille/Couette flows.
- **Number theory and special functions**: `isprime(n)` (deterministic Miller–Rabin, exact
  up to 2⁵³), `primepi(x)` (exact prime count up to 2×10⁹ from a cached segmented sieve;
  `sum(k, a, b, isprime(k))` is evaluated with it automatically), `mobius(n)`, `zetazero(k)` (the
  k-th zero's imaginary part, 414 tabulated up to t = 700), `Ei(z)`, `li(z)`, `erf(x)`, `erfc(x)`, and two-argument `mod(a, m)` and `legendre(a, p)`.
- **Y stretch** for real graphs, so curves like |ζ(1/2 + ix)| don't look flat.

## Handwriting

Choose **New panes: handwriting** in the View section and add a pane to write equations by hand, with the mouse,
trackpad or a pen. Left-drag writes and right-drag erases. **Undo** and **Clear** are at the top. The strip at the
bottom shows the recognized equation, typeset in the selected equation font, which the pane's **Equation font**
picker (or Appearance) changes. **Add as function** adds it as a row so graph panes can plot it, and **Copy text**
copies it.

- Write fractions with a bar, exponents small and raised, and square roots with a long top line. Function names
  like sin, cos, ln and log are recognized from their letters.
- Every symbol carries a tag showing what it was read as. Click a tag to correct it: the correction sticks, and the
  strokes become a template, so the recognizer learns your handwriting. Learned templates are saved in
  `~/Library/Application Support/eqviz/ink_templates.txt`.
- Recognition runs entirely on your Mac:
  - Each symbol is matched against point-cloud templates ($P recognizer), generated by thinning glyphs from macOS's
    handwriting-like fonts (Chalkboard, Noteworthy, Marker Felt, Comic Sans, Trattatello) and STIX Two.
  - Lines, dots, = and radicals are recognized from their shape.
  - Strokes are grouped into symbols by dynamic programming over the writing order.
  - On macOS 11+, Apple's on-device Vision text recognition also reads each line, and its characters are aligned
    with the symbols to break ties (you can turn this off).
- Ink and corrections are saved with the project.

## Lessons

**Lessons** (the button under the project name) teaches trigonometry, calculus, linear algebra and the Millennium Prize Problems with live graphs. Each lesson puts its explanation in a
pane on the left. Every step moves the graphs into place with smooth transitions: sliders sweep, matrices morph,
and curves appear. Many steps end with a **Check yourself** question. Answers can be numbers or expressions, such
as `0.5`, `sqrt(3)/2` or `2pi/3`, and come with hints. Move between steps with **Back** and **Next**, with
**←** and **→**, or by clicking the progress dots. Your progress is remembered.

- **Trigonometry:** the unit circle and radians, sine and cosine waves, tangent and its friends, and shaping a
  wave with a sin(b(x − c)) + d.
- **Calculus:** limits, the derivative (secant to tangent), what f′ tells you, area under a curve (Riemann sums),
  the fundamental theorem, and Taylor series.
- **Linear algebra:** vectors and combinations, matrices as transformations, the determinant, solving Mx = b,
  and eigenvectors.
- **Millennium Prize Problems:** one lesson per problem. The Riemann Hypothesis (primes rebuilt from zeta zeros),
  P vs NP (polynomial against exponential time), Navier–Stokes (exact flows, decay, and the blow-up question),
  Yang–Mills and the mass gap (range, energy gaps, confinement), the Hodge conjecture (varieties, holes and Hodge
  classes), the Poincaré conjecture (Ricci flow with surgery) and Birch and Swinnerton-Dyer (rank and counting
  points mod p).

**The calculus course.** A complete course from limits to differential equations, built from the "Calculus: From Limits
to Differential Equations" lesson plan:
- a course guide and placement test
- the optional Unit 0 bridge on rigorous limits
- Units 1–18 in five parts (derivatives; integrals; series, polar coordinates and Euler's formula; multivariable and vector calculus; differential equations)
- a midterm and a final

Each unit follows the plan's order:
- an overview, then the story of the people behind the ideas
- the ideas themselves, with live graphs and checks
- the readings (Simmons, Spivak, Boyce & DiPrima, Strogatz and the history shelf)
- the milestone proofs to write closed-book
- the plan's written unit test, to grade with your tutor

A quick check with fresh problems ends each unit, so the written tests stay sealed; it needs 85% to pass. **Next lesson** runs
straight through the course, and the lesson list shows each part's progress.

The course adds two teaching tools for real-axes panes, also in the panel's **Graph** section:
- **Parametric curve:** (x(t), y(t)), including polar curves, with an optional point travelling along it and its velocity arrow.
- **Vector / slope field:** arrows of (P, Q); with P = 1 it is the slope field of dy/dx = Q. Flow lines give phase portraits, and a draggable point shows the solution through it.

Formulas in notes can use primes: f′(x), y″.

**Tests.** Every lesson ends with a short test: **Take the test** on the last step. The questions mix typed
answers and multiple choice, and some set up their own picture. Your first answer to each question counts. A
wrong answer shows the right one and the step to review. The results screen lists every question with its
answer, and offers **Retry test** and **Next lesson**. Your best score is saved, and the lesson list marks
lessons passed with 80% or more.

The lessons use four teaching tools. You can also turn them on yourself for any real-axes pane, in the panel's
**Graph** section:

- **Unit circle:** the angle θ (a slider, or drag the point) with cos, sin, tan, sec, cot and csc drawn as
  coloured lengths. Special angles snap and show exact values such as √3/2. Radians or degrees.
- **Matrix transformation:** a 2×2 matrix applied to the whole plane, shown as the transformed grid, the images of
  î and ĵ (the matrix columns), the unit square with its area (the determinant), the eigenvector lines with their
  eigenvalues, and a draggable vector v with Mv and a target b. Presets cover rotation, shear, stretch, reflection
  and a det-0 squash. **Play** animates from the identity.
- **Trace x =** a slider: a vertical line at that x, with every curve's value there.
- **Shade area:** fills the region under a row between two bounds (numbers or expressions in the sliders) and
  shows the integral.

## Fractals

On the complex plane, tick **Fractal: iterate f** and the pane's function becomes a map that is applied over
and over, z → f(z, c). Use `z` for the iterate and `c` for the constant (in fractal panes `c` is not a slider).

- **Mandelbrot-style** panes colour each point c by how fast the orbit of z₀ = 0 escapes. **Julia-style**
  panes start the orbit at the point and use a fixed c. That c is shared by every Julia pane: drag the yellow
  **c** in a Mandelbrot pane showing the same function, or double-click there, and the Julia set follows live.
- Escaping points use a smooth iteration count, so the colour bands blend. Maps without `c` (Newton's method,
  `z - (z^3 - 1)/(3 z^2)`) colour each point by the fixed point it settles on.
- Hover any point to see its orbit traced on the plane, with where it starts and whether it escapes, settles or
  stays bounded.
- Rendering runs on the GPU (the expression is compiled to a shader). Past single precision, about a few
  thousand times magnification, it switches to double precision on the CPU. That path shows a coarse image
  while you move, and a background thread then refines it to full resolution. You can zoom about 10¹² times
  in. Iterations grow with the zoom (**Auto**). Palettes are Theme, Fire, Ice, Rainbow and Greyscale, and
  **Colour bands** sets the band density.
- Examples: Mandelbrot, Julia, Multibrot z³ + c, burning ship, Newton's method for z³ = 1, and c·eᶻ. The
  **Fractals** demo project shows Mandelbrot and Julia side by side with a draggable c, plus Newton's
  method, the burning ship and an explanatory note.
- The Julia constant and each pane's fractal mode are saved in projects and can be keyframed on the timeline.
  Moving c between keyframes morphs the Julia set.

## Timeline & video export

Press `T` (or **Timeline & video export** in the View section) for a strip under the panes.

- **+ Keyframe** (`K`) records the whole workspace at the playhead. That covers every pane's
  2D view, 3D camera, surface region and height scale, real/complex, 2D/3D, derivative, domain
  colouring, active and hidden rows, note scrolling, the variables, the sum player and the
  workspace scroll. Adding a keyframe at the end moves the playhead 2 s on, ready for the next shot.
- Between keyframes, views, cameras, variables and the term count are interpolated. Zoom and
  camera distance interpolate on a log scale, so zooms feel even. Each segment eases
  **Smooth** (manim's `smooth`), **Linear**, or **Hold** (jump at the next key).
- Mode switches (complex, 3D, derivative, active row) run their usual animations when the
  playhead passes the keyframe that changes them. Scrubbing applies them instantly.
- Drag a diamond to retime a keyframe, or drag the track to scrub. **Update keyframe** and
  **Delete** act on the keyframe under the playhead. **length** stretches the whole timeline.
  `Space` plays, and **Loop** repeats.
- **Export video…** renders the timeline frame by frame at 24, 30 or 60 fps, at full or half
  the window's pixel size. The output is an H.264 MP4 in `~/Movies/eqviz/` (AVFoundation,
  no ffmpeg needed). The panel, the strip, hover readouts and the focus border are hidden
  while recording, and frames use video time, so the output is smooth even when rendering is
  slow. Recording continues briefly past the last keyframe while a transition it started
  finishes. `Esc` cancels.
- Timelines are saved in project files.

## Appearance

The **Appearance** section at the bottom of the panel sets:
- **Theme**: 3Blue1Brown (default), Chalkboard, Paper (light), Solarized, Neon, High contrast,
  or **Custom**: open *Customise colours* to change the background, text, grid, accent and
  curve colours. Existing curves follow the theme's palette.
- **Interface font** (Avenir Next, Helvetica Neue, Charter, Iowan Old Style, New York, Menlo) and
  **equation font** (STIX Two, Charter, Palatino, Baskerville, Iowan Old Style, New York, Georgia,
  Times New Roman, Avenir Next), with a live preview. Text fonts set variables in their italic
  face and symbols fall back to STIX Two Math.
- **Panel text** and **equation size** scales.

Choices are saved to `~/Library/Application Support/eqviz/settings.txt`.

## Build

macOS (Homebrew):

```bash
brew install cmake glfw
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/eqviz
```

Dear ImGui v1.92.9b is vendored in `third_party/imgui`. If GLFW isn't installed, CMake
fetches it from source. Linux and Windows should also build; the app falls back to other
fonts there.

Fonts: the UI uses Avenir Next and equations use STIX Two Math, both bundled with macOS.

## Desktop app (macOS)

```bash
scripts/package_mac.sh --install
```

This builds `dist/eqviz.app` and a shareable `dist/eqviz.dmg`, and copies the app to
`~/Applications` (Launchpad, Spotlight, Dock). GLFW is linked statically, so the app runs
without Homebrew. The icon is rendered by the app itself (`--render-icon`). Builds are
ad-hoc signed, so a copy sent to another Mac needs right-click, then Open, the first time.

## Controls

| | 2D | 3D |
|---|---|---|
| Left-drag | pan (or drag the tangent point) | pan |
| Right-drag | | orbit |
| Two-finger scroll (trackpad) | pan, or scroll the workspace when it extends past the window | pan, or scroll the workspace |
| Pinch (trackpad) | zoom about the cursor | zoom |
| Mouse wheel | zoom about the cursor | zoom |
| "pan │ x y" (bottom-left, on hover) | click "pan" to drift continuously; drag the x / y speeds (units/s) | same, plus z; the plotted region travels with the camera |
| "rotate │ speed" (3D panes, bottom-left, on hover) | | click "rotate" to spin around the z-axis; drag the speed sideways (°/s, negative reverses) |
| Scrollbars | click the track to page, drag the thumb to scroll | |

On macOS the app reads native trackpad events to tell two-finger scrolling from a mouse wheel
and to get pinch gestures (`src/platform_mac.mm`); on other platforms the wheel zooms.

The control panel can be collapsed with the « button (or `Tab`) to give the panes the whole
window; the floating ☰ button brings it back.

Keys (when not typing): `Tab` hide/show the panel · `Space` play/pause the sum · `D` derivative · `C` real/complex ·
`3` 2D/3D · `R` reset view · `←` `→` lesson steps · `T` timeline · `K` add/update keyframe (`Space` plays the timeline when it is open
and has two or more keyframes).

## Command line

```
--eq "<expr>"  --complex  --3d  --deriv  --domain-color  --rotate  --size WxH
--frames N --shot out.bmp     render N frames at 60 fps, save the last one, exit
--sum-pos T                   freeze the summation player at T terms
--project FILE                open a saved project, demo:riemann / poincare / navier-stokes / fractals, or lesson:<id> (e.g. lesson:calc-derivative)
--actions "10:c,90:3,150:d"   scripted toggles at given frames (c, 3, d, p, e<N>)
--mouse X,Y                   fake cursor position (testing hover readouts)
```

## Layout

| File | Purpose |
|---|---|
| `src/expr.*` | tokenizer, parser, simplifier, symbolic derivative, complex evaluator, infinite-series convergence, zeta |
| `src/typeset.*` | small TeX-like box layout (fractions, scripts, radicals, Σ with limits) |
| `src/scene2d.cpp` | number plane, adaptive curve sampling, warped complex grid, domain colouring |
| `src/scene3d.cpp` | camera, surface meshes, painter's-algorithm depth sorting |
| `src/panel.cpp` | control panel and on-canvas equation labels |
| `src/themes.cpp`, `src/fonts.cpp`, `src/appearance.cpp` | colour themes, font choices, Appearance section and settings |
| `src/notes.cpp`, `src/demo_notes.cpp` | note panes (markup, layout, editor) and the demos' explanations |
| `src/timeline.cpp`, `src/video*` | keyframes, playback, timeline strip; MP4 writer (AVFoundation) |
| `src/lesson.cpp`, `src/lessons_content.cpp`, `src/lessons_millennium.cpp`, `src/lesson_tests.cpp` | lesson engine (steps, transitions, questions, tests, progress) and the lessons |
| `src/ink.cpp`, `src/ink_vision.mm` | handwriting panes: templates, $P matching, stroke grouping, layout parsing, Vision fusion |
| `src/overlays.cpp` | unit circle, matrix transformation, trace line, shaded area, parametric curves, vector and slope fields |
| `src/lesson_kit.hpp`, `src/lessons_course*.cpp` | helpers for writing lessons; the calculus course (placement, Units 0–18, midterm, final) |
| `src/fractal.cpp` | fractal mode: escape-time iteration, CPU double-precision renderer, orbits, panel section |
| `src/gpu.cpp` | expression → GLSL compiler and GPU domain colouring |
| `src/project.cpp` | functions list, calls between rows, save/open, demo project |
| `src/app.*` | state, animations, input, mode transitions |
| `tests/expr_test.cpp` | expression engine checks: `cmake --build build --target expr_test && ./build/expr_test` |
