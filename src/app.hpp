#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "expr.hpp"
#include "platform.hpp"
#include "imgui.h"
#include "typeset.hpp"

using eq::cplx;

// ------------------------------------------------------------------ math

struct Vec3 {
    double x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double k) const { return {x * k, y * k, z * k}; }
};
inline double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline Vec3 normalize(const Vec3& v) {
    double l = std::sqrt(dot(v, v));
    return l > 0 ? v * (1.0 / l) : v;
}

struct Hasher {
    uint64_t h = 1469598103934665603ull;
    void u(uint64_t v) { h = (h ^ v) * 1099511628211ull; }
    void d(double v) {
        uint64_t x;
        std::memcpy(&x, &v, 8);
        u(x);
    }
};

template <class F>
void parallelFor(int n, F&& fn) {
    unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    int nt = (int)std::min<unsigned>(hw, (unsigned)std::max(1, n / 2));
    if (nt <= 1) {
        for (int i = 0; i < n; ++i) fn(i);
        return;
    }
    std::atomic<int> next{0};
    std::vector<std::thread> th;
    th.reserve(nt);
    for (int t = 0; t < nt; ++t)
        th.emplace_back([&] {
            for (;;) {
                int i = next++;
                if (i >= n) break;
                fn(i);
            }
        });
    for (auto& t : th) t.join();
}

double niceStep(double raw);

// ------------------------------------------------------------------ functions

struct Param {
    std::string name;
    float value = 1.f, min = -5.f, max = 5.f;
    bool playing = false;
    bool integer = false;
    double phase = 0;
};

// Compiled expression with its slots bound to x / y / parameters.
struct Func {
    eq::Program prog;
    std::vector<int> map;  // -1 primary var, -2 y, >= 0 param index, -3 unbound
    bool ok() const { return prog.valid() && (int)map.size() == prog.numSlots(); }
    void bind(const std::vector<Param>& ps);
};

// One row of a project: "name(params) = body".
struct FuncDef {
    char input[1024] = "";
    char label[128] = "";  // optional display name, shown on the graph instead of the formula
    bool editLabel = false;
    int color = 0xFFFF00;
    int id = -1;           // stable id (pane visibility refers to it)
    // derived by App::rebuildProject
    eq::DefHeader hdr;
    std::string name, error;
    std::vector<std::string> params;
    bool autoNamed = true;
    eq::NodeP body;  // as typed (with calls), for labels
    eq::NodeP plot;        // calls inlined, parameters renamed to x / y
    eq::NodeP plotMarked;  // same, with the row's own top-level sums marked for the player
    Func f;                // compiled plot
};

struct EvalEnv {
    const double* params = nullptr;
    double sumLimit = INFINITY;
    int maxTerms = 2048;
    int animTerms = 20;
    bool real = true;
};
cplx evalF(const Func& f, cplx x, double y, const EvalEnv& env);
double realPart(cplx v);  // NaN when the value is not (approximately) real
inline bool finiteC(cplx w) { return std::isfinite(w.real()) && std::isfinite(w.imag()); }

// ------------------------------------------------------------------ views

struct View2D {
    double cx = 0, cy = 0, scale = 70;  // world centre, pixels per unit (x)
    double aspect = 1;                  // y stretch: pixels per unit in y = scale * aspect
    ImVec2 cmin{0, 0}, cmax{1, 1};
    double yscale() const { return scale * aspect; }
    ImVec2 center() const { return ImVec2((cmin.x + cmax.x) * 0.5f, (cmin.y + cmax.y) * 0.5f); }
    ImVec2 w2s(double x, double y) const {
        ImVec2 c = center();
        return ImVec2((float)(c.x + (x - cx) * scale), (float)(c.y - (y - cy) * scale * aspect));
    }
    void s2w(ImVec2 p, double& x, double& y) const {
        ImVec2 c = center();
        x = cx + (p.x - c.x) / scale;
        y = cy - (p.y - c.y) / (scale * aspect);
    }
};

struct CamState {
    Vec3 target;
    double theta = 0, phi = 0, dist = 10;
};

struct Camera {
    CamState s;
    double fov = 0.62;
    Vec3 eye, right, up, fwd;
    double focal = 1;
    ImVec2 center;
    void update(ImVec2 cmin, ImVec2 cmax);
    bool project(const Vec3& p, ImVec2& out, float& depth) const;
};

struct Prim3 {
    float depth = 0;
    enum Kind : unsigned char { Quad, QuadG, Line, Tri, Dot } kind = Quad;
    ImU32 fill = 0, stroke = 0;
    ImU32 vc[4] = {0, 0, 0, 0};  // per-vertex colours for QuadG
    float thick = 1;
    ImVec2 p[4];
};

struct Fonts {
    ImFont* ui = nullptr;
    ImFont* uiBold = nullptr;
    ImFont* math = nullptr;
    bool mathAlnum = false;
};

struct LaunchOptions {
    std::string eq;
    bool complex = false, threeD = false, deriv = false, domainColor = false, autoRotate = false;
    int frames = 0;
    std::string shot;
    double sumPos = -1;
    int width = 1560, height = 940;
    std::string actions;
    int bench = 0;        // testing: 1 = pan every pane each frame, 2 = zoom every pane each frame
    std::string project;  // project file to open, or "demo:riemann" / "demo:poincare"
    float mouseX = -1, mouseY = -1;  // testing: fake cursor position
    float drag[5] = {-1, 0, 0, 0, 0};  // testing: x0, y0, x1, y1, start frame of a simulated drag  // testing: "frame:action,..." with action c | 3 | d | p | e<N>
};

// ------------------------------------------------------------------ panes

// Samples of a curve on a column grid fixed in world coordinates (x = k / scale), so a
// pan only computes the newly exposed columns and a vertical pan computes nothing.
struct ColumnCache {
    uint64_t key = 0;   // what the values depend on (function, parameters...), not the view
    double scale = 0;
    long k0 = 0;        // column index of ys[0]
    bool coarse = false;  // some columns interpolated during a zoom; refine once it settles
    std::vector<double> ys;
};

struct RowCache {
    std::vector<std::vector<ImVec2>> segs;
    uint64_t hash = 0;
    ColumnCache cc;
};
using SegList = std::vector<std::vector<ImVec2>>;
using GridLines = std::vector<std::vector<std::vector<cplx>>>;
using SumYsMap = std::map<int, ColumnCache>;
using RowCacheMap = std::map<int, RowCache>;
using IntSet = std::set<int>;
using U32Vec = std::vector<uint32_t>;
using IntVec = std::vector<int>;
using CplxVec = std::vector<cplx>;

// Everything that belongs to one coordinate plane. App holds the current pane's
// copy of these members; App::enterPane swaps panes in and out.
#define EQVIZ_PANE_FIELDS(X)                                                                          \
    X(int, active_, 0) X(eq::NodeP, expr_, {}) X(eq::NodeP, dx_, {}) X(eq::NodeP, dy_, {})              \
    X(Func, f_, {}) X(Func, fx_, {}) X(Func, fy_, {}) X(Func, prev_, {})                                \
    X(float, morph_, 1.f) X(float, create_, 1.f) X(float, derivCreate_, 1.f)                            \
    X(bool, hasSum_, false) X(bool, sumInfinite_, false) X(std::string, sumIndex_, {})                  \
    X(Func, sumLoF_, {}) X(Func, sumHiF_, {}) X(int, sumCount_, 0)                                      \
    X(bool, complex_, false) X(bool, threeD_, false) X(bool, deriv_, false)                             \
    X(bool, dimAnimating_, false) X(bool, dimTo3D_, false) X(float, dimP_, 1.f)                         \
    X(CamState, camFrom_, {}) X(CamState, camTo_, {}) X(int, fadeStage_, 0) X(float, fade_, 1.f)        \
    X(bool, pendingComplex_, false) X(float, gridT_, 1.f) X(float, gridAnimP_, 1.f) X(bool, gridAnim_, false) \
    X(View2D, view_, {}) X(double, targetScale_, 72) X(bool, zooming_, false) X(bool, panning_, false)  \
    X(bool, draggingProbe_, false) X(ImVec2, zoomAnchorS_, {}) X(double, zoomAnchorX_, 0)               \
    X(double, zoomAnchorY_, 0) X(double, planeX0_, -8) X(double, planeX1_, 8) X(double, planeY0_, -5)   \
    X(double, planeY1_, 5) X(Camera, cam_, {}) X(double, targetDist_, 14) X(bool, orbiting_, false)     \
    X(bool, autoRotate_, false) X(float, domainR_, 4.f) X(float, zScale_, 1.f) X(float, surfOpacity_, 0.88f) \
    X(double, domainCx_, 0) X(double, domainCy_, 0) X(int, surfRes_, 48) X(int, colorMode_, 0)          \
    X(float, probeX_, 1.f) X(float, probeY_, 0.5f) X(ImVec2, probeScreen_, {}) X(bool, probeVisible_, false) \
    X(bool, domainColor_, false) X(float, dcOpacity_, 0.75f) X(unsigned int, dcTex_, 0u) X(int, dcW_, 0) \
    X(int, dcH_, 0) X(uint64_t, dcHash_, 0) X(U32Vec, dcPixels_, {})                                   \
    X(GridLines, gridLines_, {}) X(IntVec, gridKinds_, {}) X(uint64_t, gridHash_, 0)                    \
    X(CplxVec, surfVals_, {}) X(uint64_t, surfHash_, 0) X(SegList, curveSegs_, {}) X(uint64_t, curveHash_, 0) \
    X(SumYsMap, sumYs_, {}) X(uint64_t, sumYsHash_, 0) X(IntSet, hiddenRows_, {}) X(RowCacheMap, rowCache_, {}) \
    X(bool, fitPending_, false) X(ImVec4, fitRange_, {}) X(bool, want3D_, false)                        \
    X(double, gridX0_, 0) X(double, gridX1_, 0) X(double, gridY0_, 0) X(double, gridY1_, 0) X(double, gridStep_, 0)               \
    X(bool, lockZScale_, false) X(ColumnCache, activeCC_, {}) X(ColumnCache, derivCC_, {})            \
    X(double, lastScale_, 0) X(double, scaleChangeT_, -10) X(bool, zoomingNow_, false)                 \
    X(double, gridVcx_, 0) X(double, gridVcy_, 0) X(double, gridVscale_, 0) X(double, gridVw_, 0)      \
    X(double, gridVh_, 0) X(uint64_t, gridContentHash_, 0)

#define EQVIZ_DECLARE_FIELD(type, name, init) type name = init;

struct PaneState {
    EQVIZ_PANE_FIELDS(EQVIZ_DECLARE_FIELD)
};

// ------------------------------------------------------------------ app

struct DemoRow;
struct DemoParam;
struct DemoPane;

class App {
public:
    void init(const Fonts& f, const LaunchOptions& o);
    void frame(float dt);
    void shutdown();

    PlatformInput plat_;  // native trackpad scroll / pinch for this frame (set by main)

    // ---- state (shared across app.cpp / panel.cpp / scene*.cpp)
    Fonts fonts_;
    LaunchOptions opt_;
    double time_ = 0;
    bool firstFrame_ = true;
    int frameNo_ = 0;
    void runScriptedActions();

    // project: several functions, one of them active (animated, derivative, 3D/complex)
    std::vector<FuncDef> funcs_;
    int focusRow_ = -1, nextRowId_ = 0;
    char projName_[128] = "Untitled";
    std::string projNote_;  // short description shown under the project name
    std::string projStatus_;
    double projStatusTime_ = -10;

    std::vector<Param> params_;
    std::vector<double> pvals_;
    bool editRanges_ = false;
    int exampleIdx_ = -1;

    // summation player (shared by all panes)
    bool sumAccel_ = false;
    double sumPos_ = 0, sumStopAt_ = -1, sumHold_ = 0;
    bool sumPlaying_ = false, sumShowAll_ = false, sumLoop_ = false, sumTrail_ = true;
    float sumSpeed_ = 1.5f;
    int maxTerms_ = 2048, animTerms_ = 20;

    // the current pane's state (see EQVIZ_PANE_FIELDS)
    EQVIZ_PANE_FIELDS(EQVIZ_DECLARE_FIELD)

    // panes, arranged by a binary layout tree (layout.cpp)
    struct LayoutNode {
        int pane = -1;         // leaf: pane index; split: -1
        bool stacked = false;  // split: children above/below (else left/right)
        float ratio = 0.5f;    // split: share of the first child
        int a = -1, b = -1;    // split: children (node indices)
    };
    struct Splitter {
        int node;
        ImVec4 handle, area;  // grab rect, rect of the split node
        bool stacked;
    };
    enum Side { kLeft, kRight, kAbove, kBelow };
    std::vector<PaneState> panes_;
    int cur_ = 0, focus_ = 0, layout_ = 1;
    std::vector<ImVec4> paneRects_;
    std::vector<LayoutNode> nodes_;
    int rootNode_ = 0;
    std::vector<Splitter> splitters_;
    int dragSplit_ = -1, dragScroll_ = 0;  // dragScroll_: 1 horizontal bar, 2 vertical bar
    float dragGrab_ = 0;
    bool layoutBusy_ = false;              // a splitter or scrollbar owns the mouse this frame
    bool extendWorkspace_ = false;         // panes may extend past the window (scrollbars)
    float wsW_ = 1.6f, wsH_ = 1.f;         // workspace size in multiples of the visible area
    ImVec2 scroll_{0, 0};
    ImVec4 visRect_;                       // visible canvas area (right of the panel)
    void exchange(PaneState& p);
    void enterPane(int i);      // make pane i current (canvas rect included)
    void setPanes(int count, int layout);  // default arrangement for count panes
    void addPane(Side side);    // split the focused pane
    void closePane(int i);
    void computePaneRects();
    void layoutRects(int node, ImVec4 r);
    void handleLayoutInput();
    void drawLayoutChrome(ImDrawList* dl);
    bool inVisible(ImVec2 p) const { return p.x >= visRect_.x && p.x < visRect_.z && p.y >= visRect_.y && p.y < visRect_.w; }
    void initPane();            // defaults for a freshly created pane
    void applyFit();
    bool rowVisible(const FuncDef& d) const { return !hiddenRows_.count(d.id); }
    void updatePaneAnimations(float dt);
    void handleKeys();
    uint64_t evalHash() const;  // everything that changes function values

    // layout
    ImVec2 canvasMin_, canvasMax_;
    float panelW_ = 390.f;     // current width of the panel area (0 when collapsed)
    float panelFullW_ = 390.f; // width of the panel when open
    bool panelCollapsed_ = false;
    float panelSlide_ = 0.f;   // 0 open .. 1 collapsed (animated)
    void togglePanel() { panelCollapsed_ = !panelCollapsed_; }

    // ---- logic (app.cpp)
    // ---- projects (project.cpp)
    void rebuildProject(bool sameRow = true);
    void applyActive(const eq::NodeP& plot, bool sameRow);
    void setSingleFunction(const char* eq);
    void newProject();
    void loadDemoRiemann();
    void loadDemoPoincare();
    void loadDemoNavierStokes();
    void loadDemo(const char* name, const char* note, const std::vector<DemoRow>& rows, const std::vector<DemoParam>& params,
                  const std::vector<DemoPane>& panes, int layoutKind, int focus);
    void loadDemoPvsNP();
    void loadDemoHodge();
    void loadDemoYangMills();
    void loadDemoBSD();
    bool saveProject(const std::string& path);
    bool loadProject(const std::string& path);
    std::string projectDir() const;
    void resetForProject();
    void drawProjectSection();
    void drawFunctionList();
    int nextColor() const;
    const FuncDef* activeDef() const { return active_ >= 0 && active_ < (int)funcs_.size() ? &funcs_[active_] : nullptr; }
    void rebuildParams();
    void bindAll();
    void updateParamValues();
    void updateAnimations(float dt);
    void updateSum(float dt);
    int computeSumCount();
    double sumLimit() const;
    EvalEnv makeEnv(double limit = NAN) const;
    const Func& displayFunc() const { return (complex_ && deriv_ && fx_.ok()) ? fx_ : f_; }
    void handleInput(float dt);
    void toggleComplex();
    void toggle3D();
    void toggleDeriv();
    void startDim(bool to3D);
    float heightScale() const;
    void autoZScale();
    void resetView();
    void loadExample(int i);

    // ---- UI (panel.cpp)
    void setupStyle();
    void drawPanel();
    void drawOverlay(ImDrawList* dl);

    // ---- 2D (scene2d.cpp)
    void drawScene2D(ImDrawList* dl);
    void drawPlane2D(ImDrawList* dl, float lineAlpha, bool labels);
    void drawRealCurves(ImDrawList* dl, float alpha);
    void drawComplexGrid(ImDrawList* dl, float alpha);
    void drawDomainColoring(ImDrawList* dl, float alpha);
    void sampleCurve(const std::function<double(double)>& F, std::vector<std::vector<ImVec2>>& segs) const;
    // Values for the columns covering the view (x0 = x of ys[0], spacing 1 / scale), reusing cc.
    void sampleCached(const std::function<double(double)>& F, uint64_t key, ColumnCache& cc,
                      std::vector<double>& ys, double& x0) const;
    void segsFromYs(const std::vector<double>& ys, double x0, const std::function<double(double)>& F,
                    std::vector<std::vector<ImVec2>>& segs) const;
    void warpedGrid();     // fills gridLines_ / gridKinds_
    void fitGridToView();  // input grid of the complex transform = the current view

    // ---- hover readout
    bool canHover() const;  // mouse over the canvas, not over UI, no button held
    void hoverLabel(ImDrawList* dl, ImVec2 at, const ts::Box& b, ImU32 accent) const;

    // ---- 3D (scene3d.cpp)
    void drawScene3D(ImDrawList* dl);
    void fillSurface(int N);
};

// Building blocks for the built-in demo projects (demos.cpp).
struct DemoRow {
    const char* eq;
    int color;
    const char* label;  // display name ("" = show the formula)
};
struct DemoParam {
    const char* name;
    float value, lo, hi;
    bool play = false;
};
struct DemoPane {
    int active = 0;
    bool threeD = false, complex = false;
    ImVec4 fit{-8, 8, -5, 5};  // x0, x1, y0, y1
    std::vector<int> show;     // rows visible in this pane (empty = all)
    int colorMode = 0;
    float zScale = 0;          // 0 = automatic
    int res = 48;
};

struct Example {
    const char* label;
    const char* eq;
    bool complex, threeD;
    double cx = 0, cy = 0, scale = 0;  // 2D view to use (scale 0 = default view)
    double aspect = 1;                 // y stretch for real graphs
};
extern const Example kExamples[];
extern const int kNumExamples;

void strokePolylines(ImDrawList* dl, const std::vector<std::vector<ImVec2>>& segs, ImU32 color, float thick,
                     float progress = 1.f, bool backstroke = true);
void dashedLine(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color, float thick, float dash = 7.f, float gap = 5.f);
ImU32 domainColor(cplx w, float alpha = 1.f);
