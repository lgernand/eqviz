// H.264 MP4 writer on AVFoundation. Frames arrive as bottom-up RGBA (glReadPixels) and are
// flipped / swizzled into BGRA pixel buffers.
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#include <unistd.h>

#include <algorithm>

#include "video.hpp"

namespace video {

struct Writer {
    AVAssetWriter* writer = nil;
    AVAssetWriterInput* input = nil;
    AVAssetWriterInputPixelBufferAdaptor* adaptor = nil;
    int w = 0, h = 0, fps = 30;
    long long frame = 0;
    NSString* path = nil;
};

bool supported() { return true; }

Writer* open(const std::string& path, int w, int h, int fps, std::string& err) {
    @autoreleasepool {
        w &= ~1;
        h &= ~1;
        if (w < 16 || h < 16) { err = "frame too small"; return nullptr; }
        NSString* p = [NSString stringWithUTF8String:path.c_str()];
        NSURL* url = [NSURL fileURLWithPath:p];
        [[NSFileManager defaultManager] removeItemAtURL:url error:nil];
        NSError* e = nil;
        AVAssetWriter* writer = [[AVAssetWriter alloc] initWithURL:url fileType:AVFileTypeMPEG4 error:&e];
        if (!writer) { err = e ? e.localizedDescription.UTF8String : "could not create the file"; return nullptr; }
        // generous bitrate: thin lines and text on flat backgrounds compress badly at low rates
        const double bitsPerPixel = 0.12;
        NSNumber* rate = @((long long)std::min(150e6, std::max(8e6, w * (double)h * fps * bitsPerPixel)));
        NSDictionary* settings = @{
            AVVideoCodecKey : AVVideoCodecTypeH264,
            AVVideoWidthKey : @(w),
            AVVideoHeightKey : @(h),
            AVVideoCompressionPropertiesKey : @{
                AVVideoAverageBitRateKey : rate,
                AVVideoProfileLevelKey : AVVideoProfileLevelH264HighAutoLevel,
                AVVideoMaxKeyFrameIntervalKey : @(fps * 2),
                AVVideoExpectedSourceFrameRateKey : @(fps),
            },
            AVVideoColorPropertiesKey : @{
                AVVideoColorPrimariesKey : AVVideoColorPrimaries_ITU_R_709_2,
                AVVideoTransferFunctionKey : AVVideoTransferFunction_ITU_R_709_2,
                AVVideoYCbCrMatrixKey : AVVideoYCbCrMatrix_ITU_R_709_2,
            },
        };
        AVAssetWriterInput* input = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:settings];
        input.expectsMediaDataInRealTime = NO;
        NSDictionary* attrs = @{
            (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
            (id)kCVPixelBufferWidthKey : @(w),
            (id)kCVPixelBufferHeightKey : @(h),
        };
        AVAssetWriterInputPixelBufferAdaptor* adaptor =
            [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:input
                                                                             sourcePixelBufferAttributes:attrs];
        if (![writer canAddInput:input]) { err = "the encoder rejected the video settings"; return nullptr; }
        [writer addInput:input];
        if (![writer startWriting]) {
            err = writer.error ? writer.error.localizedDescription.UTF8String : "could not start writing";
            return nullptr;
        }
        [writer startSessionAtSourceTime:kCMTimeZero];
        Writer* v = new Writer;
        v->writer = writer;
        v->input = input;
        v->adaptor = adaptor;
        v->w = w;
        v->h = h;
        v->fps = fps;
        v->path = p;
        return v;
    }
}

bool append(Writer* v, const unsigned char* rgba, int w, int h, std::string& err) {
    if (!v) return false;
    @autoreleasepool {
        for (int spins = 0; !v->input.readyForMoreMediaData; ++spins) {
            if (v->writer.status == AVAssetWriterStatusFailed || spins > 20000) {
                err = v->writer.error ? v->writer.error.localizedDescription.UTF8String : "the encoder stalled";
                return false;
            }
            usleep(500);
        }
        CVPixelBufferRef pb = nullptr;
        if (CVPixelBufferPoolCreatePixelBuffer(nullptr, v->adaptor.pixelBufferPool, &pb) != kCVReturnSuccess || !pb) {
            err = "could not allocate a frame buffer";
            return false;
        }
        CVPixelBufferLockBaseAddress(pb, 0);
        unsigned char* dst = (unsigned char*)CVPixelBufferGetBaseAddress(pb);
        const size_t stride = CVPixelBufferGetBytesPerRow(pb);
        const int cw = std::min(w, v->w), ch = std::min(h, v->h);
        for (int y = 0; y < v->h; ++y) {
            unsigned char* d = dst + stride * y;
            if (y >= ch) { std::fill(d, d + v->w * 4, 0); continue; }
            const unsigned char* s = rgba + (size_t)(h - 1 - y) * w * 4;  // flip: GL rows are bottom-up
            for (int x = 0; x < cw; ++x) {
                d[4 * x + 0] = s[4 * x + 2];
                d[4 * x + 1] = s[4 * x + 1];
                d[4 * x + 2] = s[4 * x + 0];
                d[4 * x + 3] = 255;
            }
        }
        CVPixelBufferUnlockBaseAddress(pb, 0);
        BOOL ok = [v->adaptor appendPixelBuffer:pb withPresentationTime:CMTimeMake(v->frame, v->fps)];
        CVPixelBufferRelease(pb);
        if (!ok) {
            err = v->writer.error ? v->writer.error.localizedDescription.UTF8String : "could not add the frame";
            return false;
        }
        ++v->frame;
        return true;
    }
}

bool finish(Writer* v, bool cancel, std::string& err) {
    if (!v) return false;
    bool ok = true;
    @autoreleasepool {
        if (cancel || v->frame == 0) {
            [v->writer cancelWriting];
            [[NSFileManager defaultManager] removeItemAtPath:v->path error:nil];
            ok = false;
        } else {
            [v->input markAsFinished];
            dispatch_semaphore_t done = dispatch_semaphore_create(0);
            [v->writer finishWritingWithCompletionHandler:^{ dispatch_semaphore_signal(done); }];
            dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
            if (v->writer.status != AVAssetWriterStatusCompleted) {
                err = v->writer.error ? v->writer.error.localizedDescription.UTF8String : "could not finish the file";
                ok = false;
            }
        }
    }
    delete v;
    return ok;
}

}  // namespace video
