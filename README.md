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
| ⟳ Rotate (3D panes, bottom-left) | | spin around the z-axis; drag its slider for the speed (°/s, negative reverses) |
| Scrollbars | click the track to page, drag the thumb to scroll | |

On macOS the app reads native trackpad events to tell two-finger scrolling from a mouse wheel
and to get pinch gestures (`src/platform_mac.mm`); on other platforms the wheel zooms.

The control panel can be collapsed with the « button (or `Tab`) to give the panes the whole
window; the floating ☰ button brings it back.

Keys (when not typing): `Tab` hide/show the panel · `Space` play/pause the sum · `D` derivative · `C` real/complex ·
`3` 2D/3D · `R` reset view.

## Command line

```
--eq "<expr>"  --complex  --3d  --deriv  --domain-color  --rotate  --size WxH
--frames N --shot out.bmp     render N frames at 60 fps, save the last one, exit
--sum-pos T                   freeze the summation player at T terms
--project FILE                open a saved project, or demo:riemann / demo:poincare / demo:navier-stokes
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
| `src/gpu.cpp` | expression → GLSL compiler and GPU domain colouring |
| `src/project.cpp` | functions list, calls between rows, save/open, demo project |
| `src/app.*` | state, animations, input, mode transitions |
| `tests/expr_test.cpp` | expression engine checks: `cmake --build build --target expr_test && ./build/expr_test` |
