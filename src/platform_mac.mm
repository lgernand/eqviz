#import <Cocoa/Cocoa.h>

#include "platform.hpp"

static PlatformInput g_input;
static id g_monitor = nil;

void platformInstall() {
    if (g_monitor) return;
    g_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:(NSEventMaskScrollWheel | NSEventMaskMagnify)
                                                      handler:^NSEvent*(NSEvent* e) {
        if (e.type == NSEventTypeMagnify) {
            g_input.magnify += (float)e.magnification;
        } else if (e.type == NSEventTypeScrollWheel && e.hasPreciseScrollingDeltas) {
            // trackpads (and Magic Mouse) send precise deltas; classic wheels do not
            g_input.trackpad = true;
            g_input.scrollX += (float)e.scrollingDeltaX;
            g_input.scrollY += (float)e.scrollingDeltaY;
        }
        return e;  // still delivered to GLFW / ImGui
    }];
}

PlatformInput platformPoll() {
    PlatformInput r = g_input;
    g_input = PlatformInput{};
    return r;
}
