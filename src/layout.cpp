// Pane layout: a binary tree of splits that the user can grow, shrink and resize,
// inside a workspace that may be larger than the window (with scrollbars).
#include <algorithm>

#include "app.hpp"
#include "gl.hpp"
#include "theme.hpp"

using namespace theme;

static const float kGap = 6.f;        // gap between panes (the splitter handle)
static const float kMinPane = 140.f;  // smallest pane, in pixels
static const float kBar = 11.f;       // scrollbar thickness

// ------------------------------------------------------------------ geometry

void App::layoutRects(int n, ImVec4 r) {
    if (n < 0 || n >= (int)nodes_.size()) return;
    const LayoutNode& nd = nodes_[n];
    if (nd.pane >= 0) {
        if (nd.pane < (int)paneRects_.size()) paneRects_[nd.pane] = r;
        return;
    }
    if (!nd.stacked) {
        float xm = r.x + (r.z - r.x) * nd.ratio;
        layoutRects(nd.a, ImVec4(r.x, r.y, xm - kGap / 2, r.w));
        layoutRects(nd.b, ImVec4(xm + kGap / 2, r.y, r.z, r.w));
        splitters_.push_back({n, ImVec4(xm - kGap / 2 - 2, r.y, xm + kGap / 2 + 2, r.w), r, false});
    } else {
        float ym = r.y + (r.w - r.y) * nd.ratio;
        layoutRects(nd.a, ImVec4(r.x, r.y, r.z, ym - kGap / 2));
        layoutRects(nd.b, ImVec4(r.x, ym + kGap / 2, r.z, r.w));
        splitters_.push_back({n, ImVec4(r.x, ym - kGap / 2 - 2, r.z, ym + kGap / 2 + 2), r, true});
    }
}

void App::computePaneRects() {
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    visRect_ = ImVec4(panelW_, 0, ds.x, ds.y);
    const float vw = visRect_.z - visRect_.x, vh = visRect_.w - visRect_.y;
    float ww = vw, wh = vh;
    if (extendWorkspace_) {
        ww = vw * std::max(1.f, wsW_);
        wh = vh * std::max(1.f, wsH_);
    }
    scroll_.x = std::clamp(scroll_.x, 0.f, std::max(0.f, ww - vw));
    scroll_.y = std::clamp(scroll_.y, 0.f, std::max(0.f, wh - vh));
    if (nodes_.empty()) {  // always keep a valid tree
        nodes_.push_back(LayoutNode{});
        nodes_[0].pane = 0;
        rootNode_ = 0;
    }
    paneRects_.assign(panes_.size(), ImVec4(visRect_.x, visRect_.y, visRect_.z, visRect_.w));
    splitters_.clear();
    const float x0 = visRect_.x - scroll_.x, y0 = visRect_.y - scroll_.y;
    layoutRects(rootNode_, ImVec4(x0, y0, x0 + ww, y0 + wh));
}

// ------------------------------------------------------------------ tree edits

static int buildRow(std::vector<App::LayoutNode>& nodes, int firstPane, int count, bool stacked) {
    if (count == 1) {
        App::LayoutNode leaf;
        leaf.pane = firstPane;
        nodes.push_back(leaf);
        return (int)nodes.size() - 1;
    }
    App::LayoutNode split;
    split.stacked = stacked;
    split.ratio = 1.f / count;
    int a = buildRow(nodes, firstPane, 1, stacked);
    int b = buildRow(nodes, firstPane + 1, count - 1, stacked);
    split.a = a;
    split.b = b;
    nodes.push_back(split);
    return (int)nodes.size() - 1;
}

void App::setPanes(int count, int layout) {
    count = std::clamp(count, 1, 16);
    layout_ = layout;
    const int oldN = (int)panes_.size();
    if (count < oldN) {
        enterPane(0);
        for (int i = count; i < oldN; ++i)
            if (panes_[i].dcTex_) {
                GLuint t = panes_[i].dcTex_;
                glDeleteTextures(1, &t);
            }
        panes_.resize(count);
        focus_ = std::min(focus_, count - 1);
    }
    // default arrangement: a row (or column), or a 2 x 2 grid for four
    nodes_.clear();
    if (count == 4) {
        int top = buildRow(nodes_, 0, 2, false), bottom = buildRow(nodes_, 2, 2, false);
        LayoutNode split;
        split.stacked = true;
        split.a = top;
        split.b = bottom;
        nodes_.push_back(split);
        rootNode_ = (int)nodes_.size() - 1;
    } else {
        rootNode_ = buildRow(nodes_, 0, count, layout == 2);
    }
    if (count > oldN) {
        const int act = active_;
        panes_.resize(count);
        for (int i = oldN; i < count; ++i) {
            enterPane(i);
            initPane();
            active_ = act;
        }
        rebuildProject(true);
    }
    computePaneRects();
    enterPane(focus_);
}

void App::addPane(Side side) {
    int leaf = -1;
    for (int i = 0; i < (int)nodes_.size(); ++i)
        if (nodes_[i].pane == focus_) leaf = i;
    if (leaf < 0) return;
    const int act = active_, newPane = (int)panes_.size();
    panes_.emplace_back();
    enterPane(newPane);
    initPane();
    active_ = act;  // the new pane starts on the focused pane's main function, real axes

    LayoutNode oldLeaf;
    oldLeaf.pane = focus_;
    LayoutNode newLeaf;
    newLeaf.pane = newPane;
    nodes_.push_back(oldLeaf);
    int ia = (int)nodes_.size() - 1;
    nodes_.push_back(newLeaf);
    int ib = (int)nodes_.size() - 1;
    LayoutNode& split = nodes_[leaf];
    split.pane = -1;
    split.stacked = side == kAbove || side == kBelow;
    split.ratio = 0.5f;
    if (side == kLeft || side == kAbove) { split.a = ib; split.b = ia; }
    else { split.a = ia; split.b = ib; }
    focus_ = newPane;
    computePaneRects();
    rebuildProject(true);
    enterPane(focus_);
}

void App::closePane(int i) {
    if (panes_.size() <= 1 || i < 0 || i >= (int)panes_.size()) return;
    int leaf = -1, parent = -1;
    for (int n = 0; n < (int)nodes_.size(); ++n) {
        if (nodes_[n].pane == i) leaf = n;
        if (nodes_[n].pane < 0 && (nodes_[n].a >= 0 || nodes_[n].b >= 0)) {}
    }
    for (int n = 0; n < (int)nodes_.size(); ++n)
        if (nodes_[n].pane < 0 && (nodes_[n].a == leaf || nodes_[n].b == leaf)) parent = n;
    if (leaf < 0 || parent < 0) return;
    const int sibling = nodes_[parent].a == leaf ? nodes_[parent].b : nodes_[parent].a;
    nodes_[parent] = nodes_[sibling];  // the sibling takes the parent's place
    nodes_[leaf].pane = -2;            // orphaned (kept so indices stay valid)
    nodes_[sibling].pane = -2;
    nodes_[sibling].a = nodes_[sibling].b = -1;

    enterPane(i == 0 ? 1 : 0);  // pane i's state is now stored in panes_[i]
    if (panes_[i].dcTex_) {
        GLuint t = panes_[i].dcTex_;
        glDeleteTextures(1, &t);
    }
    panes_.erase(panes_.begin() + i);
    if (cur_ > i) --cur_;
    for (auto& nd : nodes_)
        if (nd.pane > i) --nd.pane;
    if (focus_ >= i && focus_ > 0) --focus_;
    focus_ = std::min(focus_, (int)panes_.size() - 1);
    computePaneRects();
    enterPane(focus_);
}

// ------------------------------------------------------------------ interaction

void App::handleLayoutInput() {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 m = io.MousePos;
    layoutBusy_ = dragSplit_ >= 0 || dragScroll_ != 0;
    const float vw = visRect_.z - visRect_.x, vh = visRect_.w - visRect_.y;
    const float ww = extendWorkspace_ ? vw * std::max(1.f, wsW_) : vw;
    const float wh = extendWorkspace_ ? vh * std::max(1.f, wsH_) : vh;

    // scroll the workspace: horizontal trackpad swipe, or Option + wheel for vertical
    if (extendWorkspace_ && !io.WantCaptureMouse && inVisible(m)) {
        if (io.MouseWheelH != 0) scroll_.x -= io.MouseWheelH * 40.f;
        if (io.KeyAlt && io.MouseWheel != 0) {
            scroll_.y -= io.MouseWheel * 40.f;
            io.MouseWheel = 0;  // don't also zoom the pane
        }
    }

    // scrollbars
    const bool hBar = extendWorkspace_ && ww > vw + 1, vBar = extendWorkspace_ && wh > vh + 1;
    auto hThumb = [&] {
        float len = std::max(40.f, vw * vw / ww), x = visRect_.x + (vw - len) * (scroll_.x / std::max(1.f, ww - vw));
        return ImVec4(x, visRect_.w - kBar, x + len, visRect_.w);
    };
    auto vThumb = [&] {
        float len = std::max(40.f, vh * vh / wh), y = visRect_.y + (vh - len) * (scroll_.y / std::max(1.f, wh - vh));
        return ImVec4(visRect_.z - kBar, y, visRect_.z, y + len);
    };
    auto inR = [&](const ImVec4& r) { return m.x >= r.x && m.x < r.z && m.y >= r.y && m.y < r.w; };
    if (dragScroll_ == 1) {
        if (!ImGui::IsMouseDown(0)) dragScroll_ = 0;
        else {
            ImVec4 t = hThumb();
            float len = t.z - t.x;
            scroll_.x = (m.x - dragGrab_ - visRect_.x) / std::max(1.f, vw - len) * (ww - vw);
        }
    } else if (dragScroll_ == 2) {
        if (!ImGui::IsMouseDown(0)) dragScroll_ = 0;
        else {
            ImVec4 t = vThumb();
            float len = t.w - t.y;
            scroll_.y = (m.y - dragGrab_ - visRect_.y) / std::max(1.f, vh - len) * (wh - vh);
        }
    } else if (!io.WantCaptureMouse && ImGui::IsMouseClicked(0)) {
        if (hBar && m.y >= visRect_.w - kBar && inVisible(m)) {
            ImVec4 t = hThumb();
            if (!inR(t)) scroll_.x += (m.x < t.x ? -vw : vw) * 0.8f;  // page
            else { dragScroll_ = 1; dragGrab_ = m.x - t.x; }
            layoutBusy_ = true;
        } else if (vBar && m.x >= visRect_.z - kBar && inVisible(m)) {
            ImVec4 t = vThumb();
            if (!inR(t)) scroll_.y += (m.y < t.y ? -vh : vh) * 0.8f;
            else { dragScroll_ = 2; dragGrab_ = m.y - t.y; }
            layoutBusy_ = true;
        }
    }

    // splitters
    if (dragSplit_ >= 0) {
        if (!ImGui::IsMouseDown(0)) dragSplit_ = -1;
        else {
            for (const Splitter& s : splitters_) {
                if (s.node != dragSplit_) continue;
                LayoutNode& nd = nodes_[s.node];
                float span = s.stacked ? s.area.w - s.area.y : s.area.z - s.area.x;
                float pos = s.stacked ? m.y - s.area.y : m.x - s.area.x;
                float lo = std::min(0.5f, kMinPane / std::max(1.f, span));
                nd.ratio = std::clamp(pos / std::max(1.f, span), lo, 1.f - lo);
                ImGui::SetMouseCursor(s.stacked ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
            }
        }
    } else if (!io.WantCaptureMouse && !layoutBusy_ && inVisible(m)) {
        for (const Splitter& s : splitters_) {
            if (!inR(s.handle)) continue;
            ImGui::SetMouseCursor(s.stacked ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
            if (ImGui::IsMouseClicked(0)) { dragSplit_ = s.node; layoutBusy_ = true; }
            break;
        }
    }
    if (dragSplit_ >= 0 || dragScroll_ != 0) computePaneRects();
}

void App::drawLayoutChrome(ImDrawList* dl) {
    const ImVec2 m = ImGui::GetIO().MousePos;
    if (panes_.size() > 1) {
        for (const Splitter& s : splitters_) {
            bool hot = s.node == dragSplit_ ||
                       (dragSplit_ < 0 && m.x >= s.handle.x && m.x < s.handle.z && m.y >= s.handle.y && m.y < s.handle.w);
            ImVec2 a, b;
            if (s.stacked) { float y = (s.handle.y + s.handle.w) * 0.5f; a = ImVec2(s.handle.x, y); b = ImVec2(s.handle.z, y); }
            else { float x = (s.handle.x + s.handle.z) * 0.5f; a = ImVec2(x, s.handle.y); b = ImVec2(x, s.handle.w); }
            dl->AddLine(a, b, hot ? col(BLUE_C, 0.9f) : col(WHITE, 0.10f), hot ? 3.f : 1.f);
        }
        const ImVec4& r = paneRects_[focus_];
        dl->AddRect(ImVec2(r.x + 1, r.y + 1), ImVec2(r.z - 1, r.w - 1), col(BLUE_C, 0.55f), 0, 2.f);
    }
    if (!extendWorkspace_) return;
    const float vw = visRect_.z - visRect_.x, vh = visRect_.w - visRect_.y;
    const float ww = vw * std::max(1.f, wsW_), wh = vh * std::max(1.f, wsH_);
    if (ww > vw + 1) {
        float len = std::max(40.f, vw * vw / ww), x = visRect_.x + (vw - len) * (scroll_.x / std::max(1.f, ww - vw));
        dl->AddRectFilled(ImVec2(visRect_.x, visRect_.w - kBar), ImVec2(visRect_.z, visRect_.w), col(BG, 0.85f));
        dl->AddRectFilled(ImVec2(x + 2, visRect_.w - kBar + 2), ImVec2(x + len - 2, visRect_.w - 2),
                          col(WHITE, dragScroll_ == 1 ? 0.45f : 0.25f), 4);
    }
    if (wh > vh + 1) {
        float len = std::max(40.f, vh * vh / wh), y = visRect_.y + (vh - len) * (scroll_.y / std::max(1.f, wh - vh));
        dl->AddRectFilled(ImVec2(visRect_.z - kBar, visRect_.y), ImVec2(visRect_.z, visRect_.w), col(BG, 0.85f));
        dl->AddRectFilled(ImVec2(visRect_.z - kBar + 2, y + 2), ImVec2(visRect_.z - 2, y + len - 2),
                          col(WHITE, dragScroll_ == 2 ? 0.45f : 0.25f), 4);
    }
}
