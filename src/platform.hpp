// Native input the windowing layer does not report: trackpad vs mouse-wheel scrolling
// and pinch-to-zoom (macOS). Other platforms report nothing, so the wheel always zooms.
#pragma once

struct PlatformInput {
    float scrollX = 0, scrollY = 0;  // trackpad scrolling this frame, in points (content follows fingers)
    bool trackpad = false;           // true if this frame's scroll events came from a trackpad
    float magnify = 0;               // pinch this frame (+0.1 = 10% larger)
};

void platformInstall();
PlatformInput platformPoll();  // returns and clears the input gathered since the last poll
