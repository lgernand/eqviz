// Timeline: keyframes of the whole workspace (every pane's view, camera, mode switches, active
// and hidden rows, the variables, the sum player, scrolling), played back with easing and
// exported frame by frame to an MP4.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <sstream>

#include "app.hpp"
#include "theme.hpp"
#include "video.hpp"

using namespace theme;
namespace fs = std::filesystem;

namespace {

double lerp(double a, double b, double u) { return a + (b - a) * u; }
double lerpLog(double a, double b, double u) {
    if (a <= 0 || b <= 0) return lerp(a, b, u);
    return std::exp(lerp(std::log(a), std::log(b), u));
}

CamState camLerp(const CamState& a, const CamState& b, double u) {
    CamState c;
    c.target = a.target + (b.target - a.target) * u;
    c.theta = lerp(a.theta, b.theta, u);
    c.phi = lerp(a.phi, b.phi, u);
    c.dist = lerpLog(a.dist, b.dist, u);
    return c;
}

const char* kEaseNames[] = {"Smooth", "Linear", "Hold"};

void playIcon(ImDrawList* dl, ImVec2 mn, ImVec2 mx, bool pause, ImU32 c) {
    ImVec2 ctr((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
    float s = std::min(mx.x - mn.x, mx.y - mn.y) * 0.24f;
    if (pause) {
        dl->AddRectFilled(ImVec2(ctr.x - s * 0.9f, ctr.y - s), ImVec2(ctr.x - s * 0.25f, ctr.y + s), c, 1.5f);
        dl->AddRectFilled(ImVec2(ctr.x + s * 0.25f, ctr.y - s), ImVec2(ctr.x + s * 0.9f, ctr.y + s), c, 1.5f);
    } else {
        dl->AddTriangleFilled(ImVec2(ctr.x - s * 0.7f, ctr.y - s), ImVec2(ctr.x - s * 0.7f, ctr.y + s),
                              ImVec2(ctr.x + s * 1.0f, ctr.y), c);
    }
}

void diamond(ImDrawList* dl, ImVec2 c, float r, ImU32 fill, ImU32 edge) {
    ImVec2 p[4] = {ImVec2(c.x, c.y - r), ImVec2(c.x + r, c.y), ImVec2(c.x, c.y + r), ImVec2(c.x - r, c.y)};
    dl->AddConvexPolyFilled(p, 4, fill);
    dl->AddPolyline(p, 4, edge, 1.5f, ImDrawFlags_Closed);
}

std::string fmtTime(double t) {
    char b[32];
    snprintf(b, sizeof b, "%.2f s", t);
    return b;
}

}  // namespace

// ------------------------------------------------------------------ capture / apply

Keyframe App::captureKeyframe(double t) {
    Keyframe k;
    k.t = t;
    for (const Param& p : params_) k.params.push_back({p.name, p.value});
    k.scrollX = scroll_.x;
    k.scrollY = scroll_.y;
    k.fracCRe = fracC_.real();
    k.fracCIm = fracC_.imag();
    const int start = cur_;
    for (int i = 0; i < (int)panes_.size(); ++i) {
        enterPane(i);
        KeyPane kp;
        kp.cx = view_.cx;
        kp.cy = view_.cy;
        kp.scale = zooming_ ? targetScale_ : view_.scale;
        kp.aspect = view_.aspect;
        kp.threeD = dimAnimating_ ? dimTo3D_ : threeD_;
        kp.cam = dimAnimating_ ? camTo_ : cam_.s;
        if (!dimAnimating_ && threeD_) kp.cam.dist = targetDist_;
        kp.zScale = zScale_;
        kp.domainR = domainR_;
        kp.domainCx = domainCx_;
        kp.domainCy = domainCy_;
        kp.complex = fadeStage_ == 1 ? pendingComplex_ : complex_;  // stage 1 fades out towards the switch
        kp.deriv = deriv_;
        kp.domainColor = domainColor_;
        kp.fractal = fractal_;
        kp.julia = fracJulia_;
        kp.active = active_;
        kp.hidden = hiddenRows_;
        kp.noteScroll = noteScroll_;
        if (hasSum_ && k.sumPos < 0) k.sumPos = sumShowAll_ ? (double)sumCount_ : sumPos_;
        k.panes.push_back(kp);
    }
    enterPane(start);
    return k;
}

void App::addKeyframe() {
    tlShown_ = true;
    for (Keyframe& k : keys_)
        if (std::fabs(k.t - tlT_) < 1e-3) {  // on a keyframe: update it
            int ease = k.ease;
            k = captureKeyframe(k.t);
            k.ease = ease;
            return;
        }
    const bool atEnd = keys_.empty() || tlT_ > keys_.back().t;
    Keyframe k = captureKeyframe(tlT_);
    keys_.push_back(k);
    std::sort(keys_.begin(), keys_.end(), [](const Keyframe& a, const Keyframe& b) { return a.t < b.t; });
    if (atEnd) tlT_ += 2.0;  // ready for the next shot
}

void App::applyTimeline(double t, bool animate) {
    if (keys_.empty()) return;
    // the segment containing t: keys_[k] .. keys_[k + 1]
    int k = 0;
    while (k + 1 < (int)keys_.size() && keys_[k + 1].t <= t) ++k;
    const Keyframe& A = keys_[k];
    const Keyframe& B = k + 1 < (int)keys_.size() ? keys_[k + 1] : A;
    double u = 0;
    if (&A != &B && B.t > A.t) {
        u = std::clamp((t - A.t) / (B.t - A.t), 0.0, 1.0);
        if (A.ease == 0) u = smooth((float)u);
        else if (A.ease == 2) u = 0;
    }

    // variables (by name, so rows added later keep working)
    for (Param& p : params_) {
        const float* va = nullptr;
        const float* vb = nullptr;
        for (auto& [n, v] : A.params) if (n == p.name) va = &v;
        for (auto& [n, v] : B.params) if (n == p.name) vb = &v;
        if (!va) continue;
        p.playing = false;
        p.value = (float)(vb ? lerp(*va, *vb, u) : *va);
        if (p.integer) p.value = std::round(p.value);
    }
    if (A.sumPos >= 0) {
        const double target = B.sumPos >= 0 ? lerp(A.sumPos, B.sumPos, u) : A.sumPos;
        if (!animate || A.sumPos != B.sumPos) {
            sumPos_ = target;
            sumPlaying_ = false;
            sumShowAll_ = false;
        }
    }
    fracC_ = cplx(lerp(A.fracCRe, B.fracCRe, u), lerp(A.fracCIm, B.fracCIm, u));
    if (extendWorkspace_) {
        scroll_.x = (float)lerp(A.scrollX, B.scrollX, u);
        scroll_.y = (float)lerp(A.scrollY, B.scrollY, u);
    }

    const int start = cur_;
    const int n = std::min((int)panes_.size(), (int)A.panes.size());
    for (int i = 0; i < n; ++i) {
        enterPane(i);
        const KeyPane& a = A.panes[i];
        const KeyPane& b = i < (int)B.panes.size() ? B.panes[i] : a;
        if (isNote_) {
            noteScroll_ = (float)lerp(a.noteScroll, b.noteScroll, u);
            continue;
        }

        // ---- discrete state: what the last keyframe passed says
        if (a.active != active_ && a.active >= 0 && a.active < (int)funcs_.size() && funcs_[a.active].plotMarked) {
            active_ = a.active;
            applyActive(funcs_[active_].plotMarked, false);
            curveHash_ = surfHash_ = gridHash_ = sumYsHash_ = 0;
            rowCache_.clear();
            if (animate) create_ = 0;
        }
        hiddenRows_ = a.hidden;
        domainColor_ = a.domainColor;
        if (a.fractal != fractal_ || (a.fractal && a.julia != fracJulia_)) setFractal(a.fractal, a.julia);
        if (a.deriv != deriv_) {
            if (animate) toggleDeriv();
            else { deriv_ = a.deriv; derivCreate_ = 1; morph_ = 1; prev_ = Func{}; }
        }
        const bool complexNow = fadeStage_ == 1 ? pendingComplex_ : complex_;
        if (a.complex != complexNow) {
            if (animate) toggleComplex();
            else {
                complex_ = a.complex;
                pendingComplex_ = a.complex;
                fadeStage_ = 0;
                fade_ = 1;
                prev_ = Func{};
                morph_ = 1;
                if (complex_) { view_.aspect = 1; gridStep_ = 0; }
                rebuildParams();
                gridT_ = 1;
                gridAnim_ = false;
            }
        } else if (!animate && fadeStage_ == 0) {
            gridT_ = 1;
            gridAnim_ = false;
        }
        const bool threeDNow = dimAnimating_ ? dimTo3D_ : threeD_;
        if (a.threeD != threeDNow && !dimAnimating_) {
            if (!a.threeD) {  // fly down onto the key's 2D view
                view_.cx = a.cx; view_.cy = a.cy; view_.scale = targetScale_ = a.scale; zooming_ = false;
            }
            startDim(a.threeD);
            if (a.threeD) {
                camTo_ = a.cam;
                domainR_ = a.domainR; domainCx_ = a.domainCx; domainCy_ = a.domainCy; zScale_ = a.zScale;
            }
            if (!animate) dimP_ = 0.9999f;  // finishes in this frame's animation step
        }

        // ---- continuous state
        if (!dimAnimating_) {
            view_.cx = lerp(a.cx, b.cx, u);
            view_.cy = lerp(a.cy, b.cy, u);
            view_.scale = targetScale_ = lerpLog(a.scale, b.scale, u);
            view_.aspect = complex_ ? 1.0 : lerpLog(a.aspect, b.aspect, u);
            zooming_ = false;
            if (threeD_ && a.threeD) {
                const bool both = b.threeD;
                cam_.s = both ? camLerp(a.cam, b.cam, u) : a.cam;
                targetDist_ = cam_.s.dist;
                orbiting_ = false;
                domainR_ = (float)(both ? lerpLog(a.domainR, b.domainR, u) : a.domainR);
                domainCx_ = both ? lerp(a.domainCx, b.domainCx, u) : a.domainCx;
                domainCy_ = both ? lerp(a.domainCy, b.domainCy, u) : a.domainCy;
                zScale_ = (float)(both ? lerpLog(a.zScale, b.zScale, u) : a.zScale);
            }
        }
    }
    enterPane(start);
}

void App::updateTimeline(float dt) {
    if (exporting_) {
        if (exportWarm_ > 0) {  // settle the first keyframe before recording
            --exportWarm_;
            tlT_ = keys_.front().t;
            applyTimeline(tlT_, false);
        } else {
            applyTimeline(tlT_, true);
        }
        return;
    }
    if (tlScrub_) {
        tlScrub_ = false;
        applyTimeline(tlT_, false);
    }
    if (tlPlaying_) {
        if (keys_.size() < 2) { tlPlaying_ = false; return; }
        tlT_ += dt;
        bool wrapped = false;
        if (tlT_ >= tlLength()) {
            if (tlLoop_) { tlT_ = keys_.front().t; wrapped = true; }
            else { tlT_ = tlLength(); tlPlaying_ = false; }
        }
        applyTimeline(tlT_, !wrapped);
    }
}

// ------------------------------------------------------------------ export

void App::startExport() {
    if (keys_.size() < 2 || exporting_) return;
    const char* home = std::getenv("HOME");
    fs::path dir = fs::path(home ? home : ".") / "Movies" / "eqviz";
    std::error_code ec;
    fs::create_directories(dir, ec);
    std::string base;
    for (const char* c = projName_; *c; ++c)
        base += (std::isalnum((unsigned char)*c) || *c == '-' || *c == '_') ? *c : (*c == ' ' ? '-' : '\0');
    base.erase(std::remove(base.begin(), base.end(), '\0'), base.end());
    if (base.empty()) base = "eqviz";
    char stamp[32];
    std::time_t now = std::time(nullptr);
    std::strftime(stamp, sizeof stamp, "%Y%m%d-%H%M%S", std::localtime(&now));
    exportPath_ = (dir / (base + "-" + stamp + ".mp4")).string();
    exporting_ = true;
    exportOk_ = false;
    exportWarm_ = 3;
    exportFrames_ = 0;
    exportEnd_ = tlLength();
    exportTotal_ = (int)std::lround((exportEnd_ - keys_.front().t) * exportFps_) + 1;
    tlPlaying_ = false;
    tlT_ = keys_.front().t;
    exportStatus_ = "Exporting\xE2\x80\xA6";
}

void App::exportFrame(const unsigned char* rgba, int w, int h) {
    if (!videoCapturing()) return;
    std::string err;
    std::vector<unsigned char> half;
    if (exportHalf_) {  // 2x2 box filter
        const int hw = w / 2, hh = h / 2;
        half.resize((size_t)hw * hh * 4);
        for (int y = 0; y < hh; ++y)
            for (int x = 0; x < hw; ++x)
                for (int c = 0; c < 4; ++c) {
                    const size_t i0 = ((size_t)(2 * y) * w + 2 * x) * 4 + c, i1 = i0 + (size_t)w * 4;
                    half[((size_t)y * hw + x) * 4 + c] = (unsigned char)((rgba[i0] + rgba[i0 + 4] + rgba[i1] + rgba[i1 + 4] + 2) / 4);
                }
        rgba = half.data();
        w = hw;
        h = hh;
    }
    auto* v = static_cast<video::Writer*>(video_);
    if (!v) {
        v = video::open(exportPath_, w, h, exportFps_, err);
        video_ = v;
        if (!v) {
            exportStatus_ = "Export failed: " + err;
            finishExport(true);
            return;
        }
    }
    if (!video::append(v, rgba, w, h, err)) {
        exportStatus_ = "Export failed: " + err;
        finishExport(true);
        return;
    }
    ++exportFrames_;
    char title[96];
    snprintf(title, sizeof title, "eqviz \xE2\x80\x94 exporting video %d%%  (Esc cancels)",
             std::min(100, exportFrames_ * 100 / std::max(1, exportTotal_)));
    windowTitle_ = title;
    tlT_ = std::min(exportEnd_, keys_.front().t + (double)exportFrames_ / exportFps_);
    if (exportFrames_ >= exportTotal_) {
        // a mode switch on the last keyframe starts there: keep recording until it lands (at most 3 s)
        bool settling = false;
        const int start = cur_;
        for (int i = 0; i < (int)panes_.size(); ++i) {
            enterPane(i);
            if (!isNote_ && (dimAnimating_ || fadeStage_ != 0 || gridAnim_ || morph_ < 1 || derivCreate_ < 1 || create_ < 1))
                settling = true;
        }
        enterPane(start);
        if (!settling || exportFrames_ >= exportTotal_ + 3 * exportFps_) finishExport(false);
    }
}

void App::finishExport(bool cancelled) {
    std::string err;
    const bool hadWriter = video_ != nullptr;
    const bool ok = hadWriter && video::finish(static_cast<video::Writer*>(video_), cancelled, err);
    video_ = nullptr;
    exporting_ = false;
    exportWarm_ = 0;
    windowTitle_ = "eqviz \xE2\x80\x94 equation visualizer";
    tlT_ = std::min(tlT_, tlLength());
    if (ok) {
        exportOk_ = true;
        exportStatus_ = "Saved " + fs::path(exportPath_).filename().string();
        std::printf("exported %d frames to %s\n", exportFrames_, exportPath_.c_str());
    } else if (cancelled && exportStatus_.rfind("Export failed", 0) != 0) {
        exportStatus_ = "Export cancelled";
    } else if (!cancelled) {
        exportStatus_ = "Export failed: " + err;
    }
    if (!exportOk_) std::printf("%s\n", exportStatus_.c_str());
}

// ------------------------------------------------------------------ strip

void App::drawTimeline() {
    if (!tlShown_ || exporting_) return;
    ImGuiIO& io = ImGui::GetIO();
    const float x0 = visRect_.x, w = io.DisplaySize.x - x0;
    ImGui::SetNextWindowPos(ImVec2(x0, io.DisplaySize.y - tlHeight_));
    ImGui::SetNextWindowSize(ImVec2(w, tlHeight_));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, surface(0.05f, 0.97f));
    ImGui::Begin("##timeline", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PushFont(fonts_.ui, 14.5f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    {   // top edge
        ImVec2 wp = ImGui::GetWindowPos();
        dl->AddLine(wp, ImVec2(wp.x + w, wp.y), col(WHITE, 0.12f));
    }

    int atKey = -1;
    for (int i = 0; i < (int)keys_.size(); ++i)
        if (std::fabs(keys_[i].t - tlT_) < 1e-3) atKey = i;
    int segKey = -1;  // the keyframe whose easing governs the playhead
    for (int i = 0; i < (int)keys_.size(); ++i)
        if (keys_[i].t <= tlT_ + 1e-3) segKey = i;

    // ---- controls
    const float fh = ImGui::GetFrameHeight();
    ImGui::BeginDisabled(keys_.size() < 2);
    if (ImGui::Button("##play", ImVec2(fh * 1.3f, fh))) {
        tlPlaying_ = !tlPlaying_;
        if (tlPlaying_) {
            if (tlT_ >= tlLength() - 1e-3 || tlT_ < keys_.front().t) tlT_ = keys_.front().t;
            tlScrub_ = true;
        }
    }
    playIcon(dl, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), tlPlaying_, col(WHITE));
    ImGui::SetItemTooltip(keys_.size() < 2 ? "Add two keyframes to play" : "Play the timeline (Space)");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s / %s", fmtTime(tlT_).c_str(), fmtTime(tlLength()).c_str());
    ImGui::SameLine(0, 16);
    if (ImGui::Button(atKey >= 0 ? "Update keyframe" : "+ Keyframe")) addKeyframe();
    ImGui::SetItemTooltip(atKey >= 0 ? "Replace this keyframe with what the panes show now (K)"
                                     : "Record every pane's view, mode, rows and variables at the playhead (K)");
    if (atKey >= 0) {
        ImGui::SameLine();
        if (ImGui::Button("Delete")) keys_.erase(keys_.begin() + atKey), atKey = -1;
    }
    if (segKey >= 0 && segKey + 1 < (int)keys_.size()) {
        ImGui::SameLine(0, 16);
        ImGui::TextDisabled("Ease");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(92);
        ImGui::Combo("##ease", &keys_[segKey].ease, kEaseNames, 3);
        ImGui::SetItemTooltip("How the move from this keyframe to the next one runs");
    }
    ImGui::SameLine(0, 16);
    ImGui::Checkbox("Loop", &tlLoop_);
    if (keys_.size() >= 2) {
        ImGui::SameLine(0, 16);
        float len = (float)tlLength();
        ImGui::SetNextItemWidth(110);
        if (ImGui::DragFloat("##len", &len, 0.05f, 0.5f, 600.f, "length %.1f s")) {
            const double t0 = keys_.front().t, old = tlLength() - t0;
            const double nw = std::max(0.5, (double)len - t0);
            if (old > 1e-6)
                for (Keyframe& k : keys_) k.t = t0 + (k.t - t0) * nw / old;
            tlT_ = t0 + (tlT_ - t0) * (old > 1e-6 ? nw / old : 1);
        }
        ImGui::SetItemTooltip("Stretch or squeeze the whole timeline");
    }

    // right side: last export's status, export, close
    std::string status = exportStatus_;
    const float showW = exportOk_ ? 118.f : 0.f;
    const float statusW = status.empty() ? 0.f : ImGui::CalcTextSize(status.c_str()).x + 14 + showW;
    const float rightW = 132 + 30 + 8 + statusW;
    ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 12, w - 12 - rightW));
    if (!status.empty()) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(exportOk_ ? colv(GREEN_C) : colv(RED_C), "%s", status.c_str());
        ImGui::SetItemTooltip("%s", exportPath_.c_str());
        if (exportOk_) {
            ImGui::SameLine(0, 6);
            if (ImGui::Button("Show in Finder", ImVec2(showW - 6, 0))) {
#if defined(__APPLE__)
                std::string cmd = "open -R \"" + exportPath_ + "\"";
                std::system(cmd.c_str());
#endif
            }
        }
        ImGui::SameLine(0, 14);
    }
    ImGui::BeginDisabled(keys_.size() < 2 || !video::supported());
    if (ImGui::Button("Export video\xE2\x80\xA6", ImVec2(132, 0))) ImGui::OpenPopup("##export");
    ImGui::EndDisabled();
    ImGui::SetItemTooltip(video::supported() ? "Render the timeline to an MP4 in Movies/eqviz" : "Video export needs macOS");
    ImGui::SameLine(0, 8);
    if (ImGui::Button("\xC3\x97", ImVec2(30, 0))) tlShown_ = false;
    ImGui::SetItemTooltip("Hide the timeline (T)");
    if (ImGui::BeginPopup("##export")) {
        ImGui::TextUnformatted("Export the timeline as MP4 (H.264)");
        ImGui::Separator();
        ImGui::TextDisabled("Frame rate");
        for (int f : {24, 30, 60}) {
            ImGui::SameLine();
            char b[8];
            snprintf(b, sizeof b, "%d", f);
            if (ImGui::RadioButton(b, exportFps_ == f)) exportFps_ = f;
        }
        int fw = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x) & ~1;
        int fhh = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y) & ~1;
        char full[48], halfs[48];
        snprintf(full, sizeof full, "Full  %d\xC3\x97%d", fw, fhh);
        snprintf(halfs, sizeof halfs, "Half  %d\xC3\x97%d", (fw / 2) & ~1, (fhh / 2) & ~1);
        ImGui::TextDisabled("Size");
        ImGui::SameLine();
        if (ImGui::RadioButton(full, !exportHalf_)) exportHalf_ = false;
        ImGui::SameLine();
        if (ImGui::RadioButton(halfs, exportHalf_)) exportHalf_ = true;
        ImGui::TextDisabled("%d frames, %.1f s. The panel and timeline are hidden while recording.",
                            (int)std::lround((tlLength() - keys_.front().t) * exportFps_) + 1, tlLength() - keys_.front().t);
        if (ImGui::Button("Start export", ImVec2(-1, 0))) {
            startExport();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ---- track
    ImGui::Spacing();
    const ImVec2 tp = ImGui::GetCursorScreenPos();
    const float trackW = ImGui::GetContentRegionAvail().x;
    const float trackH = std::max(30.f, ImGui::GetContentRegionAvail().y - 2);
    ImGui::InvisibleButton("##track", ImVec2(trackW, trackH));
    const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
    const double tMax = std::max({4.0, tlLength() + 1.0, tlT_ + 0.5});
    const float pad = 10;
    auto t2x = [&](double t) { return tp.x + pad + (float)(t / tMax) * (trackW - 2 * pad); };
    auto x2t = [&](float x) { return std::clamp((double)(x - tp.x - pad) / (trackW - 2 * pad) * tMax, 0.0, tMax); };
    const float midY = tp.y + 15 + (trackH - 15) * 0.5f;

    dl->AddRectFilled(tp, ImVec2(tp.x + trackW, tp.y + trackH), col(BG, 0.7f), 6);
    {   // ruler
        double step = niceStep(tMax / 10);
        for (double t = 0; t <= tMax + 1e-9; t += step) {
            float x = t2x(t);
            dl->AddLine(ImVec2(x, tp.y + 3), ImVec2(x, tp.y + 8), col(WHITE, 0.3f));
            char b[16];
            snprintf(b, sizeof b, step < 1 ? "%.1f" : "%.0f", t);
            dl->AddText(nullptr, 11.5f, ImVec2(x + 4, tp.y + 1), col(WHITE, 0.4f), b);
        }
        dl->AddLine(ImVec2(t2x(0), midY), ImVec2(t2x(tMax), midY), col(WHITE, 0.12f), 1.f);
    }
    for (int i = 0; i + 1 < (int)keys_.size(); ++i) {  // segments, styled by easing
        float xa = t2x(keys_[i].t), xb = t2x(keys_[i + 1].t);
        if (keys_[i].ease == 2) dashedLine(dl, ImVec2(xa, midY), ImVec2(xb, midY), col(BLUE_C, 0.55f), 2.f, 5, 4);
        else dl->AddLine(ImVec2(xa, midY), ImVec2(xb, midY), col(BLUE_C, keys_[i].ease == 1 ? 0.45f : 0.7f), 3.f);
    }

    // interaction: drag a keyframe, or scrub
    const ImVec2 m = io.MousePos;
    int hotKey = -1;
    for (int i = 0; i < (int)keys_.size(); ++i)
        if (std::fabs(m.x - t2x(keys_[i].t)) < 8 && std::fabs(m.y - midY) < 12) hotKey = i;
    if (ImGui::IsItemActivated()) {
        tlPlaying_ = false;
        tlDragKey_ = hotKey;
    }
    if (active) {
        if (tlDragKey_ >= 0 && tlDragKey_ < (int)keys_.size()) {
            const double lo = tlDragKey_ > 0 ? keys_[tlDragKey_ - 1].t + 0.05 : 0.0;
            const double hi = tlDragKey_ + 1 < (int)keys_.size() ? keys_[tlDragKey_ + 1].t - 0.05 : 1e9;
            double t = std::clamp(std::round(x2t(m.x) * 20) / 20, lo, hi);  // 0.05 s grid
            if (ImGui::IsMouseDragging(0, 2)) keys_[tlDragKey_].t = t;
            tlT_ = keys_[tlDragKey_].t;
            tlScrub_ = true;
        } else {
            double t = x2t(m.x);
            for (const Keyframe& k : keys_)
                if (std::fabs(t2x(k.t) - m.x) < 6) t = k.t;  // snap onto keyframes
            tlT_ = t;
            tlScrub_ = true;
        }
    } else {
        tlDragKey_ = -1;
    }
    for (int i = 0; i < (int)keys_.size(); ++i) {
        const bool sel = i == atKey, hot = i == hotKey && hovered;
        diamond(dl, ImVec2(t2x(keys_[i].t), midY), sel || hot ? 8.f : 6.5f, sel ? col(YELLOW_C) : col(BLUE_C, hot ? 1.f : 0.85f),
                col(BG));
    }
    {   // playhead
        float x = t2x(tlT_);
        dl->AddLine(ImVec2(x, tp.y + 2), ImVec2(x, tp.y + trackH - 2), col(WHITE, 0.9f), 1.5f);
        dl->AddTriangleFilled(ImVec2(x - 5, tp.y + 1), ImVec2(x + 5, tp.y + 1), ImVec2(x, tp.y + 7), col(WHITE));
    }
    if (keys_.empty())
        dl->AddText(ImVec2(tp.x + 70, midY - 9), col(WHITE, 0.45f),
                    "Set up a shot, press + Keyframe (K), move the playhead, change the view, add another.");
    if (hovered && hotKey >= 0 && !active) ImGui::SetTooltip("Keyframe at %s: drag to retime", fmtTime(keys_[hotKey].t).c_str());

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

// ------------------------------------------------------------------ project files

void App::saveTimeline(std::ostream& out) const {
    if (keys_.empty()) return;
    std::map<int, int> rowOf;
    for (size_t r = 0; r < funcs_.size(); ++r) rowOf[funcs_[r].id] = (int)r;
    out << "timeline\t" << (tlLoop_ ? 1 : 0) << "\t" << exportFps_ << "\t" << (exportHalf_ ? 1 : 0) << "\n";
    for (const Keyframe& k : keys_) {
        std::string ps;
        for (auto& [n, v] : k.params) ps += (ps.empty() ? "" : ";") + n + "=" + std::to_string(v);
        char cbuf[80];
        snprintf(cbuf, sizeof cbuf, "\t%.17g\t%.17g", k.fracCRe, k.fracCIm);
        out << "key\t" << k.t << "\t" << k.ease << "\t" << k.sumPos << "\t" << k.scrollX << "\t" << k.scrollY << "\t"
            << (ps.empty() ? "-" : ps) << cbuf << "\n";
        for (const KeyPane& p : k.panes) {
            std::string hidden;
            for (int id : p.hidden)
                if (rowOf.count(id)) hidden += (hidden.empty() ? "" : ",") + std::to_string(rowOf[id]);
            char buf[640];
            snprintf(buf, sizeof buf,
                     "kpane\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.9g\t%.9g\t%.17g\t%.17g\t%d%d%d%d\t%d\t",
                     p.cx, p.cy, p.scale, p.aspect, p.cam.target.x, p.cam.target.y, p.cam.target.z, p.cam.theta, p.cam.phi,
                     p.cam.dist, p.zScale, p.domainR, p.domainCx, p.domainCy, p.complex, p.threeD, p.deriv, p.domainColor,
                     p.active);
            out << buf << (hidden.empty() ? "-" : hidden) << "\t" << p.noteScroll << "\t" << (p.fractal ? 1 : 0)
                << (p.julia ? 1 : 0) << "\n";
        }
    }
}

void App::loadTimelineLine(const std::vector<std::string>& f, const std::vector<int>& rowIds) {
    try {
        if (f[0] == "timeline" && f.size() > 3) {
            tlLoop_ = f[1] == "1";
            exportFps_ = std::clamp(std::stoi(f[2]), 1, 120);
            exportHalf_ = f[3] == "1";
        } else if (f[0] == "key" && f.size() > 6) {
            Keyframe k;
            k.t = std::stod(f[1]);
            k.ease = std::clamp(std::stoi(f[2]), 0, 2);
            k.sumPos = std::stod(f[3]);
            k.scrollX = std::stof(f[4]);
            k.scrollY = std::stof(f[5]);
            if (f[6] != "-") {
                std::stringstream ss(f[6]);
                std::string item;
                while (std::getline(ss, item, ';')) {
                    size_t eqp = item.find('=');
                    if (eqp != std::string::npos) k.params.push_back({item.substr(0, eqp), std::stof(item.substr(eqp + 1))});
                }
            }
            if (f.size() > 8) { k.fracCRe = std::stod(f[7]); k.fracCIm = std::stod(f[8]); }
            keys_.push_back(k);
        } else if (f[0] == "kpane" && f.size() > 18 && !keys_.empty()) {
            KeyPane p;
            double v[14];
            for (int i = 0; i < 14; ++i) v[i] = std::stod(f[1 + i]);
            p.cx = v[0]; p.cy = v[1]; p.scale = v[2]; p.aspect = v[3];
            p.cam.target = Vec3(v[4], v[5], v[6]);
            p.cam.theta = v[7]; p.cam.phi = v[8]; p.cam.dist = v[9];
            p.zScale = (float)v[10]; p.domainR = (float)v[11]; p.domainCx = v[12]; p.domainCy = v[13];
            const std::string& fl = f[15];
            if (fl.size() >= 4) { p.complex = fl[0] == '1'; p.threeD = fl[1] == '1'; p.deriv = fl[2] == '1'; p.domainColor = fl[3] == '1'; }
            p.active = std::stoi(f[16]);
            if (f[17] != "-") {
                std::stringstream hs(f[17]);
                std::string item;
                while (std::getline(hs, item, ',')) {
                    int r = std::stoi(item);
                    if (r >= 0 && r < (int)rowIds.size()) p.hidden.insert(rowIds[r]);
                }
            }
            p.noteScroll = std::stof(f[18]);
            if (f.size() > 19 && f[19].size() >= 2) { p.fractal = f[19][0] == '1'; p.julia = f[19][1] == '1'; }
            keys_.back().panes.push_back(p);
        }
    } catch (...) {
    }
}
