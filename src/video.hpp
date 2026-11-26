// Video file writer: H.264 MP4 through AVFoundation on macOS (video_mac.mm).
#pragma once
#include <string>

namespace video {

struct Writer;
// Opens path for frames of w x h pixels (made even). Returns nullptr and sets err on failure.
Writer* open(const std::string& path, int w, int h, int fps, std::string& err);
// Appends one frame: tightly packed RGBA rows, bottom row first (as glReadPixels returns them).
bool append(Writer* v, const unsigned char* rgba, int w, int h, std::string& err);
// Finishes the file (or deletes it when cancel is set) and frees the writer.
bool finish(Writer* v, bool cancel, std::string& err);
bool supported();

}  // namespace video
