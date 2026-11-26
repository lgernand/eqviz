// Video export needs AVFoundation; other platforms report that it is unavailable.
#include "video.hpp"

namespace video {
struct Writer {};
Writer* open(const std::string&, int, int, int, std::string& err) {
    err = "Video export is only available on macOS";
    return nullptr;
}
bool append(Writer*, const unsigned char*, int, int, std::string&) { return false; }
bool finish(Writer* v, bool, std::string&) { delete v; return false; }
bool supported() { return false; }
}  // namespace video
