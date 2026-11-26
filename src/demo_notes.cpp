// Context notes shown above each Millennium Prize demo (see notes.cpp for the markup).
#include "app.hpp"

namespace demo_notes {

const char* kRiemann = R"(# The Riemann Hypothesis

The Riemann zeta function begins life as a series that converges when $Re(s) > 1$:

$$ zeta(s) = sum(n, 1, inf, 1/n^s) $$

Euler noticed that it is also a product over the primes, one factor $1/(1 - p^(-s))$ for each prime $p$. That is why zeta knows where the primes are. Riemann extended $zeta(s)$ to every complex $s$ except $s = 1$, where it has a pole.

## The zeros

zeta vanishes at $s = -2, -4, -6, ...$ (the trivial zeros). All the other zeros lie in the critical strip $0 < Re(s) < 1$, and the first ones are at $1/2 + 14.1347 i$, $1/2 + 21.0220 i$, $1/2 + 25.0109 i$, ...

**The Riemann Hypothesis:** every nontrivial zero has real part exactly $1/2$. More than $10^13$ zeros have been checked; no proof is known.

## Why primes care

Riemann's explicit formula rebuilds the prime-counting staircase from the zeros. With Riemann's smooth approximation $R(x)$ and the zeros $1/2 + i gamma_k$,

$$ pi_0(x) = R(x) - sum(k, 1, inf, 2 Re(R(x^(1/2 + i gamma_k)))) $$

Each pair of zeros adds one wave; the right-hand pane adds them one at a time. If the hypothesis holds, the error in the prime count is as small as it can be:

$$ abs(pi(x) - li(x)) < sqrt(x) ln(x) / (8 pi) $$

for every $x >= 2657$ (Schoenfeld, 1976).

## The panes

- **Left:** $zeta(z)$ applied to the complex plane: the grid is carried to its image, with the pole at $z = 1$ near the middle.
- **Right:** the prime staircase $pi_0(x)$ (blue), $R(x)$ (green), $abs(zeta(1/2 + i x))$ (red, touching zero at each $gamma_k$), and the explicit formula built from the first $N$ zeros (yellow). Drag $N$ or play the sum.
)";

const char* kPoincare = R"(# The Poincaré conjecture (solved)

**Statement.** Every closed, simply connected 3-dimensional manifold is homeomorphic to the 3-sphere. "Simply connected" means every loop can be shrunk to a point; on a doughnut, a loop around the hole cannot.

Poincaré asked this in 1904. Grigori Perelman proved it in 2003, completing Richard Hamilton's programme; he declined both the Fields Medal (2006) and the Millennium Prize (2010), the only problem of the seven solved so far.

## Ricci flow

Hamilton's idea is to let the geometry of the manifold evolve so that it smooths itself out, like heat spreading through metal. The metric $g$ follows

$$ (\partial g)/(\partial t) = -2 \Ric $$

where $\Ric$ is the Ricci curvature. Positively curved regions shrink and the shape becomes rounder. A round 3-sphere of radius $r$ keeps its shape and shrinks:

$$ r(t)^2 = 1 - 4 t $$

$$ R = 6/r^2 $$

so its scalar curvature $R$ blows up as $t -> 1/4$ (bottom-right pane), and its volume $2 pi^2 r^3$ falls to zero.

## Surgery

Ricci flow can pinch a thin neck before the whole manifold becomes round. Perelman showed these singularities look like thin cylinders, which can be cut out and capped with round caps (surgery), and that only finitely many surgeries happen. For a simply connected manifold every piece eventually becomes a round sphere and disappears, which shows the original manifold was a sphere.

## The panes

- **Top left:** a dumbbell whose neck pinches first (schematic: each cross-section follows $r^2 -> r^2 - 4 t$).
- **Top right:** a round sphere shrinking to a point.
- **Bottom:** the dumbbell's profile over time, and $r$, $R$ and the volume against $t$.
)";

const char* kNavierStokes = R"(# Navier–Stokes existence and smoothness

The Navier–Stokes equations describe a viscous, incompressible fluid with velocity $u$, pressure $p$, density $rho$ and viscosity $nu$:

$$ (\partial u)/(\partial t) + (u \cdot \nabla) u = -\nabla p / rho + nu \nabla^2 u $$

$$ \nabla \cdot u = 0 $$

The left side is the fluid's acceleration; the right side is the pressure push plus viscous friction. The second equation says the fluid neither compresses nor expands.

## The problem

Start a 3D flow from smooth initial data with finite energy. **Does a smooth solution exist for all time, or can it blow up**, with the velocity becoming infinite somewhere in finite time? (The prize also accepts a proof of blow-up.)

- In 2D the answer is yes: smooth solutions exist forever (Ladyzhenskaya, 1960s).
- In 3D, Leray (1934) proved weak solutions exist for all time, but not that they stay smooth or unique.
- A blow-up would need the vorticity $omega = \nabla \times u$ to become unbounded (Beale–Kato–Majda, 1984).

The difficulty is the nonlinear term $(u \cdot \nabla) u$: it moves energy to ever smaller scales, and nobody can show viscosity always wins.

## Exact solutions shown here

- **Taylor–Green vortex** (top left): a lattice of counter-rotating cells whose vorticity decays as $omega = 2 sin(x) sin(y) e^(-2 nu t)$.
- **Lamb–Oseen vortex** (top right): a single vortex that spreads by diffusion, with core width $sqrt(4 nu t)$.
- **Stokes' first problem** (bottom left): a plate set suddenly in motion drags the fluid with $u(y, t) = erfc(y / (2 sqrt(nu t)))$, a boundary layer that thickens like $sqrt(nu t)$.
- **Poiseuille and Couette flow** (bottom right): steady flows in a channel, $G (1 - y^2)/2$ and $U (1 + y)/2$.

All of these are smooth forever; the open question is about general 3D initial data.
)";

const char* kPvsNP = R"(# P vs NP

**P** is the class of problems a computer can *solve* in polynomial time: the number of steps grows at most like $n^k$ for an input of size $n$. **NP** is the class of problems whose solutions can be *checked* in polynomial time.

Every problem in P is in NP. **The question: is every problem in NP also in P?** If you can recognise a correct answer quickly, can you always find one quickly?

## NP-complete problems

Cook and Levin (1971) showed that Boolean satisfiability (SAT) is NP-complete: every NP problem can be translated into it. Thousands more followed: the travelling salesman, graph colouring, Sudoku on $n^2 \times n^2$ grids, protein folding models, and so on. A polynomial algorithm for any one of them would give one for all of NP.

## Why it matters

The best known algorithms for NP-complete problems take exponential time, essentially trying many of the $2^n$ possibilities. For $n = 100$ that is

$$ 2^100 ~ 1.27 \times 10^30 $$

steps, while $n^3$ is only $10^6$. However large $k$ is, $2^n$ eventually overtakes $n^k$: drag $k$ in the left pane and watch where they cross.

Most experts believe $P != NP$. Modern cryptography relies on it: if $P = NP$, finding a key would be as easy as checking one.

## The panes

- **Left:** running times $n^k$ (in P), $2^n$ (brute-force search) and $n!$ (trying every ordering).
- **Right:** the same on a logarithmic scale: $ln(n^k) = k ln(n)$ grows slowly, while $ln(2^n) = n ln(2)$ is a straight line.
)";

const char* kHodge = R"(# The Hodge conjecture

Algebraic geometry studies shapes cut out by polynomial equations, like the Fermat cubic surface

$$ x^3 + y^3 + z^3 = 1 $$

or the elliptic curve $w^2 = z^3 - z$. Over the complex numbers these are smooth manifolds, and topology measures them with **cohomology**: groups that record their holes of each dimension.

## Hodge classes

On a smooth complex projective variety, Hodge showed that the cohomology splits into pieces of type $(p, q)$, according to how many holomorphic and antiholomorphic directions a form uses. A **Hodge class** is a rational cohomology class of type $(p, p)$.

Any algebraic subvariety (a smaller shape cut out by more polynomial equations) gives a Hodge class.

**The Hodge conjecture:** every Hodge class is a rational combination of classes of algebraic subvarieties. In other words, the "shapes" that calculus and topology can see are all built from shapes defined by polynomials.

## What is known

- Classes of type $(1, 1)$: true (Lefschetz's $(1,1)$ theorem, 1924).
- It is known to fail with integer instead of rational coefficients (Atiyah–Hirzebruch), and for general Kähler manifolds (Voisin, 2002). The rational, projective case remains open.

## The panes

These are illustrations: the conjecture is about the topology of complex varieties of complex dimension 2 and higher, which cannot be drawn directly.

- **Left:** the real points of the Fermat cubic surface.
- **Middle:** $abs(w)$ over the complex plane for $w = sqrt(z^3 - z)$, one sheet of the curve's Riemann surface (a torus), coloured by phase.
- **Right:** the real points of $w^2 = z^3 - z$: an oval and an infinite branch.
)";

const char* kYangMills = R"(# Yang–Mills existence and mass gap

Yang–Mills theory generalises electromagnetism. Its field strength for a gauge group such as $SU(3)$ includes a term where the field interacts with itself, and its classical action is

$$ S = 1/(4 g^2) F^2 $$

integrated over spacetime ($g$ is the coupling constant). It underlies the Standard Model's strong and weak forces.

## The problem

Prove that a quantum Yang–Mills theory exists rigorously on four-dimensional spacetime (satisfying the Wightman or Osterwalder–Schrader axioms), and that it has a **mass gap**: a number $\Delta > 0$ such that every particle has energy at least $\Delta$. Classically the field has massless waves, like light. The claim is that quantum effects give the lightest particles (glueballs) a mass.

## Why a mass gap matters

- **Short range.** A massless particle gives a long-range force $-1/r$; a mass $m$ gives the Yukawa force $-e^(-m r)/r$, which dies off over a distance $1/m$ (top left).
- **Energy gap.** A particle of mass $m$ has $E = sqrt(p^2 + m^2)$, which never drops below $m$ (top right).
- **Confinement.** Quarks are never seen alone. The quark–antiquark potential rises linearly at long distance:

$$ V(r) = -4 alpha / (3 r) + sigma r $$

Pulling them apart costs ever more energy (bottom left). Numerical lattice simulations show the gap and confinement clearly; a proof is missing.

## Instantons

Instantons are tunnelling solutions of the Euclidean equations, each with action $8 pi^2 / g^2$. Their size $rho$ is free: shrinking $rho$ concentrates the same action into a taller, narrower peak (bottom right), whose height grows like $1/rho^4$.
)";

const char* kBSD = R"(# The Birch and Swinnerton-Dyer conjecture

An **elliptic curve** over the rationals is a curve

$$ y^2 = x^3 + a x + b $$

with no singular points. Its rational points form a group: draw a line through two points, find the third intersection, and reflect it. Mordell (1922) showed the group is generated by finitely many points. The number of independent generators of infinite order is the curve's **rank** $r$.

## Counting points mod p

For each prime $p$, let $N_p$ be the number of solutions modulo $p$ (plus the point at infinity). Hasse proved $abs(N_p - p - 1) <= 2 sqrt(p)$. These counts assemble into the L-function $L(E, s)$, defined as a product over primes, which continues to the whole complex plane (Wiles and others, 2001).

## The conjecture

In the 1960s Birch and Swinnerton-Dyer computed products of $N_p/p$ over primes $p <= x$ on an early computer, and found they grow like

$$ C (ln(x))^r $$

with $r$ the rank. In the language of L-functions:

**The conjecture:** the order of vanishing of $L(E, s)$ at $s = 1$ equals the rank of $E$. The full conjecture also predicts the leading coefficient exactly.

## What is known

It is proved when the order of vanishing is 0 or 1 (Coates–Wiles, Gross–Zagier, Kolyvagin, 1977–1990). For rank 2 and beyond it is open, even though the data agree spectacularly.

## The panes

- **Left:** the curve $y^2 = x^3 + a x + b$. Drag $a$ and $b$: with three real roots it has an oval and a branch.
- **Right:** their experiment: the log of the product of $N_p/p$ over primes $p <= x$, plotted against $u = ln(ln(x))$, for curves of rank 0, 1, 2 and 3. Higher rank climbs faster; the grey lines have slopes 1, 2 and 3.
)";


const char* kFractals = R"(# Fractals from iteration

Take a map and apply it again and again:

$$z_{n+1} = z_n^2 + c$$

Starting from $z_0 = 0$, the orbit $z_1 = c$, $z_2 = c^2 + c$, ... either stays bounded or runs off to infinity. Once $|z_n| > 2$ it must escape, so a computer can tell which happens.

## The Mandelbrot set

The **Mandelbrot set** is every $c$ whose orbit stays bounded. Points outside are coloured by how quickly they escape, using a smooth count

$$\nu = n + 1 - ln(ln(|z_n|))/ln(2)$$

so the bands blend instead of stepping. The main cardioid holds the $c$ whose orbit settles on a fixed point, the big disc to its left those that settle into a 2-cycle, and every bulb a cycle of its own length.

## Julia sets

Fix $c$ and colour the starting points $z_0$ instead: that is the **Julia set** of $c$. The two pictures are linked:

- the Julia set is connected exactly when $c$ is in the Mandelbrot set
- for $c$ outside, it shatters into a cloud of dust (a Cantor set)
- near the boundary of the Mandelbrot set, the Julia set looks like the Mandelbrot set around $c$

Drag the yellow $c$ in the Mandelbrot pane to see this.

## Newton's method

To solve $z^3 = 1$, Newton's method repeats

$$z_{n+1} = z_n - (z_n^3 - 1)/(3 z_n^2)$$

Each start point runs to one of the three cube roots of $1$, coloured by which. The boundaries between the three basins are fractal: wherever two colours meet, the third is there too.

## The burning ship

Taking absolute values of the real and imaginary parts before squaring breaks the symmetry and gives the **burning ship**. Flipping the sign of the imaginary part stands it upright:

$$z_{n+1} = (|re(z_n)| - i |im(z_n)|)^2 + c$$

## Exploring

Zoom anywhere on a boundary: the detail never runs out. The GPU renders in single precision; past a few thousand times magnification eqviz switches to double precision on the CPU, which goes about a trillion times deeper. Iterations grow with the zoom so fine filaments stay sharp.
)";
}  // namespace demo_notes
