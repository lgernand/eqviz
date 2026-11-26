// Projects: several named functions that can call each other, saved as text files.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

#include "app.hpp"
#include "theme.hpp"

using namespace theme;
namespace fs = std::filesystem;

static const int kPalette[] = {YELLOW_C, BLUE_C, GREEN_C, RED_C, PURPLE_C, GOLD_C, TEAL_C, PINK};

int App::nextColor() const {
    for (int c : kPalette) {
        bool used = false;
        for (auto& d : funcs_) used |= d.color == c;
        if (!used) return c;
    }
    return kPalette[funcs_.size() % 8];
}

// ------------------------------------------------------------------ rebuild

void App::rebuildProject(bool sameRow) {
    if (funcs_.empty()) funcs_.emplace_back();
    for (auto& d : funcs_)
        if (d.id < 0) d.id = nextRowId_++;

    // 1. names: explicit "name(params) =" headers first, then automatic names for the rest
    std::set<std::string> used;
    for (auto& d : funcs_) {
        d.hdr = eq::parseHeader(d.input);
        if (d.hdr.named) used.insert(d.hdr.name);
    }
    static const char* autoNames[] = {"f", "g", "h", "p", "q", "r", "s", "u", "v", "w"};
    for (size_t i = 0; i < funcs_.size(); ++i) {
        FuncDef& d = funcs_[i];
        d.autoNamed = !d.hdr.named;
        if (d.hdr.named) {
            d.name = d.hdr.name;
            d.params = d.hdr.params;
            continue;
        }
        d.name.clear();
        for (const char* n : autoNames)
            if (!used.count(n)) { d.name = n; break; }
        if (d.name.empty()) d.name = "f_" + std::to_string(i + 1);
        used.insert(d.name);
        d.params = {"x"};
    }

    // 2. parse every body, knowing every function name
    eq::FnTable table;
    for (auto& d : funcs_) {
        d.error.clear();
        d.body = d.plot = d.plotMarked = nullptr;
        d.f = Func{};
        const std::string& text = d.hdr.body;
        if (std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isspace(c); })) continue;
        eq::ParseResult r = eq::parse(text, &used);
        if (!r.root) { d.error = r.error; continue; }
        d.body = d.autoNamed ? eq::renameVar(r.root, "z", "x") : r.root;
        if (table.defs.count(d.name)) { d.error = "'" + d.name + "' is defined more than once"; d.body = nullptr; continue; }
        table.defs[d.name] = {d.params, d.body};
    }

    // 3. inline calls. plotMarked keeps the row's own top-level sums animated (used
    //    when the row is a pane's active row); calls inlined from other rows never animate.
    for (auto& d : funcs_) {
        if (!d.body) continue;
        std::string err;
        eq::NodeP plain = eq::expandCalls(d.body, table, err);
        eq::NodeP marked = plain ? eq::expandCalls(eq::markAnimatedSums(d.body), table, err) : nullptr;
        if (!plain || !marked) { d.error = err; continue; }
        std::map<std::string, eq::NodeP> rep;
        if (!d.params.empty() && d.params[0] != "x") rep[d.params[0]] = eq::var("x");
        if (d.params.size() > 1 && d.params[1] != "y") rep[d.params[1]] = eq::var("y");
        d.plot = eq::substitute(plain, rep);
        d.plotMarked = eq::substitute(marked, rep);
        d.f = Func{eq::Program(d.plot), {}};
    }

    // 4. each pane's active row drives its animated / derivative / 3D / complex machinery
    const int start = cur_;
    for (int p = 0; p < (int)panes_.size(); ++p) {
        enterPane(p);
        active_ = std::clamp(active_, 0, (int)funcs_.size() - 1);
        const bool same = p == start ? sameRow : true;
        FuncDef& a = funcs_[active_];
        if (a.plotMarked) {
            if (!expr_ || !same || eq::toString(a.plotMarked) != eq::toString(expr_)) applyActive(a.plotMarked, same);
        } else if (!a.body && a.error.empty()) {  // blank row
            expr_ = dx_ = dy_ = nullptr;
            f_ = fx_ = fy_ = prev_ = Func{};
            hasSum_ = false;
        }  // else: keep showing the last good version while the row has an error
        curveHash_ = surfHash_ = gridHash_ = sumYsHash_ = 0;
        rowCache_.clear();
    }
    enterPane(start);
    rebuildParams();
}

// ------------------------------------------------------------------ project lifecycle

void App::resetForProject() {
    projNote_.clear();
    const int start = cur_;
    for (int i = 0; i < (int)panes_.size(); ++i) {
        enterPane(i);
        expr_ = dx_ = dy_ = nullptr;
        f_ = fx_ = fy_ = prev_ = Func{};
        morph_ = 1;
        active_ = 0;
        hiddenRows_.clear();
        rowCache_.clear();
        gridStep_ = 0;
        lockZScale_ = false;
    }
    enterPane(start);
    funcs_.clear();
}

void App::setSingleFunction(const char* text) {
    resetForProject();
    FuncDef d;
    std::strncpy(d.input, text, sizeof d.input - 1);
    d.color = YELLOW_C;
    funcs_.push_back(d);
    rebuildProject(true);
}

void App::newProject() {
    std::strncpy(projName_, "Untitled", sizeof projName_);
    setPanes(1, layout_);
    params_.clear();
    view_.cx = view_.cy = 0;
    view_.aspect = 1;
    view_.scale = targetScale_ = 72;
    if (threeD_ && !dimAnimating_) startDim(false);
    setSingleFunction("");
    focusRow_ = 0;
}

void App::loadDemoRiemann() {
    struct Row { const char* eq; int color; };
    static const Row rows[] = {
        {"zeta(z)", WHITE},                                                              // 0: the complex pane
        {"pi_0(x) = sum(n, 1, x, isprime(n))", BLUE_C},                                  // 1: prime staircase
        {"L(u) = sum(n, 1, 12, mobius(n)/n Ei(u/n))", GREY},                             // 2: helper
        {"R(x) = L(ln x)", GREEN_C},                                                     // 3: Riemann's R
        {"pi_N(x) = R(x) - sum(k, 1, N, 2 re(L((1/2 + i zetazero(k)) ln x)))", YELLOW_C},  // 4: from the zeros
        {"Z(x) = abs(zeta(1/2 + i x))", RED_C},                                          // 5: critical line
    };
    if (threeD_ && !dimAnimating_) startDim(false);
    setPanes(2, 1);
    resetForProject();
    for (const Row& r : rows) {
        FuncDef d;
        std::strncpy(d.input, r.eq, sizeof d.input - 1);
        d.color = r.color;
        d.id = nextRowId_++;
        funcs_.push_back(d);
    }
    std::strncpy(projName_, "Riemann Hypothesis", sizeof projName_);
    projNote_ = "Riemann's explicit formula rebuilds the prime staircase from the zeros of zeta. "
                "The Riemann Hypothesis says every nontrivial zero lies on Re(s) = 1/2, which keeps "
                "the error in the prime count as small as possible.";
    Param n;
    n.name = "N";
    n.value = 30;
    n.min = 0;
    n.max = 400;
    n.integer = true;
    params_ = {n};
    sumSpeed_ = 2.5f;
    sumAccel_ = true;

    // left: zeta transforming the complex plane
    enterPane(0);
    active_ = 0;
    if (threeD_) startDim(false);
    complex_ = true;
    fade_ = 1;
    fadeStage_ = 0;
    gridT_ = 0; gridAnimP_ = 0; gridAnim_ = true;
    fitPending_ = true;
    fitRange_ = ImVec4(-4.5f, 5.5f, -6.f, 6.f);
    // right: the prime staircase and the curve built from the zeros
    enterPane(1);
    active_ = 4;
    if (threeD_) startDim(false);
    complex_ = false;
    fade_ = 1;
    fadeStage_ = 0;
    hiddenRows_ = {funcs_[0].id, funcs_[2].id};
    fitPending_ = true;
    fitRange_ = ImVec4(1.5f, 62.f, -1.5f, 19.f);
    focus_ = 1;
    rebuildProject(true);
}

// Poincare conjecture (Perelman 2003): every closed, simply connected 3-manifold is
// homeomorphic to the 3-sphere. The proof runs Hamilton's Ricci flow
// dg/dt = -2 Ric(g): round spheres shrink to points, and thin necks pinch off and are
// removed by surgery, so the manifold is shown to be a union of round spheres.
void App::loadDemoPoincare() {
    struct Row { const char* eq; int color; };
    static const Row rows[] = {
        // a dumbbell: two round bulbs (radius 0.95) joined by a thin neck; P is the squared radius
        {"P(x) = 0.9025 - (abs(x) - 1)^2 + 0.22 e^(-6x^2)", GREY},              // 0
        {"r(x) = sqrt(P(x) - 4t)", YELLOW_C},                                    // 1 profile at time t
        {"q(x) = -r(x)", YELLOW_C},                                              // 2
        {"b(x) = sqrt(P(x))", GREY},                                             // 3 profile at t = 0
        {"c(x) = -b(x)", GREY},                                                  // 4
        {"M(x, y) = sqrt(r(x)^2 - y^2)", BLUE_C},                                // 5 the neck, in 3D
        {"rho(s) = sqrt(1 - 4s)", BLUE_C},                                       // 6 radius of S^3 under the flow
        {"R(s) = 6 / rho(s)^2", RED_C},                                          // 7 scalar curvature 6/r^2
        {"V(s) = 2 pi^2 rho(s)^3", GREEN_C},                                     // 8 volume of S^3
        {"S(x, y) = sqrt(rho(t)^2 - x^2 - y^2)", PURPLE_C},                      // 9 a round sphere shrinking
    };
    if (threeD_ && !dimAnimating_) startDim(false);
    setPanes(4, 1);
    resetForProject();
    for (const Row& r : rows) {
        FuncDef d;
        std::strncpy(d.input, r.eq, sizeof d.input - 1);
        d.color = r.color;
        d.id = nextRowId_++;
        funcs_.push_back(d);
    }
    std::strncpy(projName_, "Poincare conjecture", sizeof projName_);
    projNote_ = "Every closed, simply connected 3-manifold is a 3-sphere (Perelman, 2003). The proof runs "
                "Ricci flow  \xE2\x88\x82g/\xE2\x88\x82t = \xE2\x88\x92" "2 Ric(g): round spheres shrink to a point "
                "(radius\xC2\xB2 = 1 \xE2\x88\x92 4t, curvature R = 6/radius\xC2\xB2), while thin necks pinch off "
                "first and are removed by surgery. The neck here is schematic: each cross-section follows "
                "radius\xC2\xB2 \xE2\x86\x92 radius\xC2\xB2 \xE2\x88\x92 4t. Slider t is the flow's time.";
    Param t;
    t.name = "t";
    t.value = 0;
    t.min = 0;
    t.max = 0.24f;
    t.playing = true;  // run the flow back and forth
    params_ = {t};

    auto hideAllBut = [&](std::initializer_list<int> keep) {
        hiddenRows_.clear();
        for (int i = 0; i < (int)funcs_.size(); ++i)
            if (std::find(keep.begin(), keep.end(), i) == keep.end()) hiddenRows_.insert(funcs_[i].id);
    };
    auto setup = [&](int pane, int active, bool want3D, ImVec4 fit) {
        enterPane(pane);
        active_ = active;
        if (threeD_) startDim(false);
        complex_ = false;
        fade_ = 1;
        fadeStage_ = 0;
        want3D_ = want3D;
        fitPending_ = true;
        fitRange_ = fit;
    };
    setup(0, 5, true, ImVec4(-3.f, 3.f, -3.f, 3.f));          // 3D: dumbbell pinching at the neck
    surfRes_ = 110;
    lockZScale_ = true;  // true proportions: these are shapes, not graphs
    zScale_ = 1;
    setup(1, 9, true, ImVec4(-1.6f, 1.6f, -1.6f, 1.6f));      // 3D: round sphere shrinking to a point
    surfRes_ = 90;
    lockZScale_ = true;
    zScale_ = 1;
    setup(2, 1, false, ImVec4(-2.2f, 2.2f, -1.25f, 1.25f));   // profile of the dumbbell over time
    hideAllBut({1, 2, 3, 4});
    setup(3, 7, false, ImVec4(-0.015f, 0.265f, -1.f, 24.f));  // radius, curvature, volume vs time
    hideAllBut({6, 7, 8});
    focus_ = 2;
    rebuildProject(true);
}

std::string App::projectDir() const {
    const char* home = std::getenv("HOME");
#ifdef _WIN32
    if (!home) home = std::getenv("USERPROFILE");
#endif
    fs::path p = fs::path(home ? home : ".") / "Documents" / "eqviz";
    std::error_code ec;
    fs::create_directories(p, ec);
    return p.string();
}

bool App::saveProject(const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    out << "eqviz-project 1\n";
    out << "name\t" << projName_ << "\n";
    if (!projNote_.empty()) out << "note\t" << projNote_ << "\n";
    out << "mode\t" << (complex_ ? 1 : 0) << "\t" << (threeD_ ? 1 : 0) << "\n";
    out << "view\t" << view_.cx << "\t" << view_.cy << "\t" << view_.scale << "\t" << view_.aspect << "\n";
    out << "sum\t" << sumSpeed_ << "\t" << (sumAccel_ ? 1 : 0) << "\t" << animTerms_ << "\t" << maxTerms_ << "\n";
    for (auto& p : params_)
        out << "param\t" << p.name << "\t" << p.value << "\t" << p.min << "\t" << p.max << "\n";
    out << "layout\t" << panes_.size() << "\t" << layout_ << "\t" << focus_ << "\n";
    out << "workspace\t" << (extendWorkspace_ ? 1 : 0) << "\t" << wsW_ << "\t" << wsH_ << "\n";
    out << "layoutroot\t" << rootNode_ << "\n";
    for (const LayoutNode& nd : nodes_)
        out << "layoutnode\t" << nd.pane << "\t" << (nd.stacked ? 1 : 0) << "\t" << nd.ratio << "\t" << nd.a << "\t" << nd.b << "\n";
    const int start = cur_;
    for (int i = 0; i < (int)panes_.size(); ++i) {
        enterPane(i);
        std::string hidden;
        for (size_t r = 0; r < funcs_.size(); ++r)
            if (!rowVisible(funcs_[r])) hidden += (hidden.empty() ? "" : ",") + std::to_string(r);
        out << "pane\t" << (complex_ ? 1 : 0) << "\t" << (threeD_ ? 1 : 0) << "\t" << active_ << "\t" << view_.cx << "\t"
            << view_.cy << "\t" << view_.scale << "\t" << view_.aspect << "\t" << (hidden.empty() ? "-" : hidden) << "\n";
    }
    enterPane(start);
    enterPane(focus_);
    bool firstVisible = true;
    for (size_t i = 0; i < funcs_.size(); ++i) {
        const FuncDef& d = funcs_[i];
        char hex[16];
        snprintf(hex, sizeof hex, "%06X", d.color & 0xFFFFFF);
        out << "fn\t" << hex << "\t" << (rowVisible(d) ? 1 : 0) << "\t" << ((int)i == active_ ? 1 : 0) << "\t" << d.input << "\n";
        (void)firstVisible;
    }
    return (bool)out;
}

bool App::loadProject(const std::string& path) {
    std::ifstream in(path);
    std::string line;
    if (!in || !std::getline(in, line) || line.rfind("eqviz-project", 0) != 0) return false;
    std::vector<FuncDef> fs;
    std::vector<Param> ps;
    std::vector<int> fnVisible;
    struct PaneSpec { bool complex = false, threeD = false; int active = 0; double cx = 0, cy = 0, s = 72, a = 1; std::vector<int> hidden; };
    std::vector<PaneSpec> paneSpecs;
    int layoutN = 0, layoutKind = 1, layoutFocus = 0, treeRoot = -1;
    std::vector<LayoutNode> tree;
    bool wsExtend = false;
    float wsw = 1.6f, wsh = 1.f;
    int act = 0;
    bool wantComplex = false, want3D = false;
    std::string name = fs::path(path).stem().string(), note;
    double vcx = 0, vcy = 0, vs = 72, va = 1;
    auto split = [](const std::string& s) {
        std::vector<std::string> v;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, '\t')) v.push_back(item);
        return v;
    };
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto f = split(line);
        if (f.empty()) continue;
        try {
            if (f[0] == "name" && f.size() > 1) name = f[1];
            else if (f[0] == "note" && f.size() > 1) note = f[1];
            else if (f[0] == "mode" && f.size() > 2) { wantComplex = f[1] == "1"; want3D = f[2] == "1"; }
            else if (f[0] == "view" && f.size() > 4) { vcx = std::stod(f[1]); vcy = std::stod(f[2]); vs = std::stod(f[3]); va = std::stod(f[4]); }
            else if (f[0] == "sum" && f.size() > 4) {
                sumSpeed_ = std::stof(f[1]); sumAccel_ = f[2] == "1"; animTerms_ = std::stoi(f[3]); maxTerms_ = std::stoi(f[4]);
            } else if (f[0] == "param" && f.size() > 4) {
                Param p;
                p.name = f[1]; p.value = std::stof(f[2]); p.min = std::stof(f[3]); p.max = std::stof(f[4]);
                ps.push_back(p);
            } else if (f[0] == "workspace" && f.size() > 3) {
                wsExtend = f[1] == "1"; wsw = std::stof(f[2]); wsh = std::stof(f[3]);
            } else if (f[0] == "layoutroot" && f.size() > 1) {
                treeRoot = std::stoi(f[1]);
            } else if (f[0] == "layoutnode" && f.size() > 5) {
                LayoutNode nd;
                nd.pane = std::stoi(f[1]); nd.stacked = f[2] == "1"; nd.ratio = std::stof(f[3]);
                nd.a = std::stoi(f[4]); nd.b = std::stoi(f[5]);
                tree.push_back(nd);
            } else if (f[0] == "layout" && f.size() > 3) {
                layoutN = std::stoi(f[1]); layoutKind = std::stoi(f[2]); layoutFocus = std::stoi(f[3]);
            } else if (f[0] == "pane" && f.size() > 8) {
                PaneSpec p;
                p.complex = f[1] == "1"; p.threeD = f[2] == "1"; p.active = std::stoi(f[3]);
                p.cx = std::stod(f[4]); p.cy = std::stod(f[5]); p.s = std::stod(f[6]); p.a = std::stod(f[7]);
                if (f[8] != "-") {
                    std::stringstream hs(f[8]);
                    std::string item;
                    while (std::getline(hs, item, ',')) p.hidden.push_back(std::stoi(item));
                }
                paneSpecs.push_back(p);
            } else if (f[0] == "fn" && f.size() > 3) {
                FuncDef d;
                d.color = (int)std::strtol(f[1].c_str(), nullptr, 16);
                fnVisible.push_back(f[2] == "1");
                if (f[3] == "1") act = (int)fs.size();
                std::string text;
                for (size_t k = 4; k < f.size(); ++k) text += (k > 4 ? "\t" : "") + f[k];
                std::strncpy(d.input, text.c_str(), sizeof d.input - 1);
                fs.push_back(d);
            }
        } catch (...) {
            // skip malformed lines
        }
    }
    if (fs.empty()) return false;
    if (paneSpecs.empty()) {  // older single-view files
        PaneSpec p;
        p.complex = wantComplex; p.threeD = want3D; p.active = act;
        p.cx = vcx; p.cy = vcy; p.s = vs; p.a = va;
        for (size_t i = 0; i < fnVisible.size(); ++i)
            if (!fnVisible[i]) p.hidden.push_back((int)i);
        paneSpecs.push_back(p);
        layoutN = 1;
    }
    for (auto& d : fs) d.id = nextRowId_++;
    if (threeD_ && !dimAnimating_) startDim(false);
    setPanes(std::max(1, std::min((int)paneSpecs.size(), layoutN ? layoutN : 1)), layoutKind);
    resetForProject();
    funcs_ = std::move(fs);
    params_ = ps;
    std::strncpy(projName_, name.c_str(), sizeof projName_ - 1);
    projNote_ = note;
    for (int i = 0; i < (int)panes_.size(); ++i) {
        const PaneSpec& p = paneSpecs[i];
        enterPane(i);
        active_ = std::clamp(p.active, 0, (int)funcs_.size() - 1);
        if (threeD_ && !p.threeD && !dimAnimating_) startDim(false);
        complex_ = p.complex;
        fade_ = 1;
        fadeStage_ = 0;
        view_.cx = p.cx; view_.cy = p.cy; view_.scale = targetScale_ = p.s; view_.aspect = p.a;
        zooming_ = false;
        for (int r : p.hidden)
            if (r >= 0 && r < (int)funcs_.size()) hiddenRows_.insert(funcs_[r].id);
        want3D_ = p.threeD && !threeD_;
        if (complex_ && !p.threeD) { gridT_ = 0; gridAnimP_ = 0; gridAnim_ = true; }
    }
    extendWorkspace_ = wsExtend;
    wsW_ = wsw;
    wsH_ = wsh;
    {   // restore the saved arrangement if it is consistent with the panes
        bool ok = treeRoot >= 0 && treeRoot < (int)tree.size();
        std::vector<int> seen(panes_.size(), 0);
        for (const LayoutNode& nd : tree) {
            if (nd.pane >= (int)panes_.size()) ok = false;
            else if (nd.pane >= 0) seen[nd.pane]++;
            if (nd.pane == -1 && (nd.a < 0 || nd.b < 0 || nd.a >= (int)tree.size() || nd.b >= (int)tree.size())) ok = false;
        }
        for (int c : seen) ok = ok && c == 1;
        if (ok) { nodes_ = tree; rootNode_ = treeRoot; }
    }
    focus_ = std::clamp(layoutFocus, 0, (int)panes_.size() - 1);
    computePaneRects();
    enterPane(focus_);
    rebuildProject(true);
    return true;
}

// ------------------------------------------------------------------ UI

static void status(App& a, const std::string& s) {
    a.projStatus_ = s;
    a.projStatusTime_ = a.time_;
}

void App::drawProjectSection() {
    const float bw = 54, gap = 6;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 3 * (bw + gap));
    ImGui::InputTextWithHint("##pname", "Project name", projName_, sizeof projName_);
    ImGui::SameLine(0, gap);
    if (ImGui::Button("Save", ImVec2(bw, 0))) {
        std::string fname = projName_;
        for (char& c : fname)
            if (!std::isalnum((unsigned char)c) && c != ' ' && c != '-' && c != '_') c = '_';
        if (fname.empty()) fname = "Untitled";
        std::string path = (fs::path(projectDir()) / (fname + ".eqviz")).string();
        status(*this, saveProject(path) ? "Saved to " + path : "Could not write " + path);
    }
    ImGui::SetItemTooltip("Save to ~/Documents/eqviz");
    ImGui::SameLine(0, gap);
    if (ImGui::Button("Open", ImVec2(bw, 0))) ImGui::OpenPopup("##open");
    ImGui::SameLine(0, gap);
    if (ImGui::Button("New", ImVec2(bw, 0))) newProject();
    if (ImGui::BeginPopup("##open")) {
        ImGui::TextDisabled("Demo");
        if (ImGui::Selectable("Riemann Hypothesis: primes from zeta zeros")) loadDemoRiemann();
        if (ImGui::Selectable("Poincar\xC3\xA9 conjecture: Ricci flow with surgery")) loadDemoPoincare();
        ImGui::Separator();
        ImGui::TextDisabled("Saved in ~/Documents/eqviz");
        std::vector<fs::path> files;
        std::error_code ec;
        for (auto& e : fs::directory_iterator(projectDir(), ec))
            if (e.path().extension() == ".eqviz") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        if (files.empty()) ImGui::TextDisabled("  (none yet)");
        for (auto& p : files)
            if (ImGui::Selectable(p.stem().string().c_str()))
                status(*this, loadProject(p.string()) ? "Opened " + p.filename().string() : "Could not read " + p.string());
        ImGui::EndPopup();
    }
    if (!projNote_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::PushFont(nullptr, 13.f);
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted(projNote_.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopFont();
        ImGui::PopStyleColor();
    }
    if (time_ - projStatusTime_ < 5 && !projStatus_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::PushTextWrapPos(0);
        ImGui::TextUnformatted(projStatus_.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }
}

void App::drawFunctionList() {
    ImDrawList* wdl = ImGui::GetWindowDrawList();
    const float fh = ImGui::GetFrameHeight();
    int newActive = -1, del = -1;
    bool commit = false;
    for (size_t i = 0; i < funcs_.size(); ++i) {
        FuncDef& d = funcs_[i];
        const bool isActive = (int)i == active_;
        ImGui::PushID((int)i);
        ImVec2 row = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        if (isActive)
            wdl->AddRectFilled(ImVec2(row.x - 6, row.y - 3), ImVec2(row.x + w + 4, row.y + fh + 3), col(d.color, 0.13f), 8);

        // colour swatch = show / hide
        const bool vis = rowVisible(d);
        if (ImGui::InvisibleButton("##vis", ImVec2(22, fh))) {
            if (vis) hiddenRows_.insert(d.id);
            else hiddenRows_.erase(d.id);
        }
        ImGui::SetItemTooltip(vis ? (panes_.size() > 1 ? "Hide in this pane" : "Hide this graph")
                                  : (panes_.size() > 1 ? "Show in this pane" : "Show this graph"));
        ImVec2 c(row.x + 11, row.y + fh * 0.5f);
        if (vis) wdl->AddCircleFilled(c, 7, col(d.color));
        else wdl->AddCircle(c, 6.5f, col(d.color, 0.8f), 0, 1.6f);
        ImGui::SameLine(0, 6);

        if (focusRow_ == (int)i) { ImGui::SetKeyboardFocusHere(); focusRow_ = -1; }
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - fh - 6);
        bool enter = ImGui::InputTextWithHint("##in", i == 0 ? "e.g.  sin(x),  z^2,  sum(k,1,n,1/k^2)" : "e.g.  g(x) = f(x)^2 + 1",
                                              d.input, sizeof d.input, ImGuiInputTextFlags_EnterReturnsTrue);
        if (ImGui::IsItemActivated() && !isActive) newActive = (int)i;
        if (enter || ImGui::IsItemDeactivatedAfterEdit()) commit = true;
        ImGui::SameLine(0, 6);
        if (ImGui::Button("\xC3\x97", ImVec2(fh, fh))) del = (int)i;
        ImGui::SetItemTooltip("Remove");
        if (!d.error.empty()) {
            ImGui::Indent(28);
            ImGui::PushStyleColor(ImGuiCol_Text, colv(RED_C));
            ImGui::PushTextWrapPos(0);
            ImGui::TextUnformatted(d.error.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
            ImGui::Unindent(28);
        }
        ImGui::PopID();
    }
    if (newActive >= 0) {
        active_ = newActive;
        rebuildProject(false);
    } else if (commit) {
        rebuildProject(true);
    }
    if (del >= 0) {
        if (funcs_.size() > 1) {
            bool wasActive = del == active_;
            funcs_.erase(funcs_.begin() + del);
            if (active_ > del || active_ >= (int)funcs_.size()) --active_;
            rebuildProject(!wasActive);
        } else {
            funcs_[0].input[0] = 0;
            rebuildProject(true);
        }
    }

    float half = (ImGui::GetContentRegionAvail().x - 6) * 0.5f;
    ImVec4 a = colv(BLUE_C);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(a.x * 0.42f, a.y * 0.42f, a.z * 0.42f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(a.x * 0.55f, a.y * 0.55f, a.z * 0.55f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(a.x * 0.68f, a.y * 0.68f, a.z * 0.68f, 1));
    if (ImGui::Button("+  Add function", ImVec2(half, 34))) {
        FuncDef d;
        d.color = nextColor();
        d.id = nextRowId_++;
        funcs_.push_back(d);
        active_ = (int)funcs_.size() - 1;
        focusRow_ = active_;
        rebuildProject(false);
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine(0, 6);
    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 9));
    if (ImGui::BeginCombo("##examples", "Examples", ImGuiComboFlags_HeightLarge)) {
        for (int i = 0; i < kNumExamples; ++i)
            if (ImGui::Selectable(kExamples[i].label, false)) loadExample(i);
        ImGui::EndCombo();
    }
    ImGui::PopStyleVar();
    if (funcs_.size() > 1) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::PushFont(nullptr, 12.5f);
        ImGui::TextWrapped(panes_.size() > 1
                               ? "Rows are shared by every pane. The highlighted row is the focused pane's main "
                                 "function (sum player, derivative, 3D, complex); dots show or hide rows in that pane."
                               : "The highlighted row drives the sum player, derivative, 3D and complex views. "
                                 "Name rows like  f(x) = ...  to call them from other rows.");
        ImGui::PopFont();
        ImGui::PopStyleColor();
    }
}
