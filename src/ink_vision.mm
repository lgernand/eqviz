// A second opinion for the handwriting recognizer: Apple's on-device text recognition (Vision)
// reads one symbol at a time. Nothing leaves the machine.
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <Vision/Vision.h>

#include <algorithm>
#include <cmath>

#include "ink_vision.hpp"

namespace inkvision {

bool available() {
    if (@available(macOS 11.0, *)) return true;
    return false;
}

std::vector<std::pair<std::string, float>> read(const std::vector<std::vector<ImVec2>>& strokes) {
    std::vector<std::pair<std::string, float>> out;
    for (const Char& c : readChars(strokes, 1)) out.push_back({c.text, c.conf});
    return out;
}

std::vector<Char> readChars(const std::vector<std::vector<ImVec2>>& strokes, int candidates) {
    std::vector<Char> out;
    if (@available(macOS 11.0, *)) {
        @autoreleasepool {
            float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f;
            for (auto& s : strokes)
                for (auto& p : s) { x0 = std::min(x0, p.x); y0 = std::min(y0, p.y); x1 = std::max(x1, p.x); y1 = std::max(y1, p.y); }
            if (x1 < x0) return out;
            // black ink on white, the symbol about 90 px tall with generous margins
            // keep the ink's proportions: about 100 px of ink height, with margins
            const float hInk = std::max(8.f, y1 - y0), wInk = std::max(8.f, x1 - x0);
            const float k = std::min(100.f / hInk, 1800.f / wInk);
            const int Wd = (int)(wInk * k) + 120, S = (int)(hInk * k) + 120;
            const float ox = 60, oy = 60;
            CGColorSpaceRef cs = CGColorSpaceCreateDeviceGray();
            CGContextRef ctx = CGBitmapContextCreate(nullptr, Wd, S, 8, Wd, cs, (CGBitmapInfo)kCGImageAlphaNone);
            CGColorSpaceRelease(cs);
            if (!ctx) return out;
            CGContextSetGrayFillColor(ctx, 1, 1);
            CGContextFillRect(ctx, CGRectMake(0, 0, Wd, S));
            CGContextSetGrayStrokeColor(ctx, 0, 1);
            CGContextSetLineWidth(ctx, 6.f);
            CGContextSetLineCap(ctx, kCGLineCapRound);
            CGContextSetLineJoin(ctx, kCGLineJoinRound);
            for (auto& s : strokes) {
                if (s.empty()) continue;
                auto P = [&](ImVec2 p) { return CGPointMake(ox + (p.x - x0) * k, S - (oy + (p.y - y0) * k)); };  // CG is y-up
                CGContextBeginPath(ctx);
                CGContextMoveToPoint(ctx, P(s[0]).x, P(s[0]).y);
                if (s.size() == 1) CGContextAddLineToPoint(ctx, P(s[0]).x + 0.5, P(s[0]).y);
                for (size_t i = 1; i < s.size(); ++i) CGContextAddLineToPoint(ctx, P(s[i]).x, P(s[i]).y);
                CGContextStrokePath(ctx);
            }
            CGImageRef img = CGBitmapContextCreateImage(ctx);
            CGContextRelease(ctx);
            if (!img) return out;
            VNRecognizeTextRequest* req = [[VNRecognizeTextRequest alloc] init];
            req.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
            req.usesLanguageCorrection = NO;
            req.minimumTextHeight = 0.05;
            VNImageRequestHandler* h = [[VNImageRequestHandler alloc] initWithCGImage:img options:@{}];
            NSError* err = nil;
            [h performRequests:@[ req ] error:&err];
            CGImageRelease(img);
            if (err) return out;
            // each character of each line's best readings, with its box mapped back to ink coordinates
            int line = 0;
            for (VNRecognizedTextObservation* obs in req.results) {
                int rank = 0;
                const CGRect ob = obs.boundingBox;  // per-character boxes are not reliable for handwriting; the line's is
                for (VNRecognizedText* t in [obs topCandidates:candidates]) {
                    NSString* str = t.string;
                    for (NSUInteger i = 0; i < str.length; ++i) {
                        NSRange r = [str rangeOfComposedCharacterSequenceAtIndex:i];
                        NSString* ch = [str substringWithRange:r];
                        i = r.location + r.length - 1;
                        if ([ch isEqualToString:@" "]) continue;
                        const CGRect b = ob;  // normalized, origin bottom-left
                        Char c;
                        c.text = ch.UTF8String ? ch.UTF8String : "";
                        c.conf = (float)t.confidence;
                        c.line = line;
                        c.rank = rank;
                        c.x0 = (float)((b.origin.x * Wd - ox) / k + x0);
                        c.x1 = (float)(((b.origin.x + b.size.width) * Wd - ox) / k + x0);
                        c.y0 = (float)((S - (b.origin.y + b.size.height) * S - oy) / k + y0);
                        c.y1 = (float)((S - b.origin.y * S - oy) / k + y0);
                        out.push_back(c);
                    }
                    ++rank;
                }
                ++line;
            }
        }
    }
    return out;
}

}  // namespace inkvision
