// eqviz — an equation visualizer in the visual style of manim / 3Blue1Brown.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "app.hpp"
#include "gl.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "theme.hpp"
#include "typeset.hpp"

static bool fileExists(const char* p) {
    FILE* f = std::fopen(p, "rb");
    if (!f) return false;
    std::fclose(f);
    return true;
}

static ImFont* loadFont(const std::vector<std::pair<const char*, int>>& candidates, float size) {
    for (auto& c : candidates) {
        if (!fileExists(c.first)) continue;
        ImFontConfig cfg;
        cfg.FontNo = c.second;
        cfg.OversampleH = 2;
        if (ImFont* f = ImGui::GetIO().Fonts->AddFontFromFileTTF(c.first, size, &cfg)) return f;
    }
    return nullptr;
}

static void mergeFallback(const char* path, float size) {
    if (!fileExists(path)) return;
    ImFontConfig cfg;
    cfg.MergeMode = true;
    ImGui::GetIO().Fonts->AddFontFromFileTTF(path, size, &cfg);
}

static void saveBMP(const char* path, int w, int h, const std::vector<unsigned char>& rgba) {
    FILE* f = std::fopen(path, "wb");
    if (!f) { std::fprintf(stderr, "cannot write %s\n", path); return; }
    int rowBytes = (w * 3 + 3) & ~3;
    unsigned int dataSize = rowBytes * h, fileSize = 54 + dataSize;
    unsigned char hdr[54] = {'B', 'M'};
    auto put32 = [&](int off, unsigned int v) { for (int i = 0; i < 4; ++i) hdr[off + i] = (v >> (8 * i)) & 255; };
    put32(2, fileSize); put32(10, 54); put32(14, 40); put32(18, w); put32(22, h);
    hdr[26] = 1; hdr[28] = 24; put32(34, dataSize);
    std::fwrite(hdr, 1, 54, f);
    std::vector<unsigned char> row(rowBytes, 0);
    for (int y = 0; y < h; ++y) {  // GL rows are bottom-up, as BMP expects
        for (int x = 0; x < w; ++x) {
            const unsigned char* p = &rgba[(size_t)(y * w + x) * 4];
            row[x * 3 + 0] = p[2]; row[x * 3 + 1] = p[1]; row[x * 3 + 2] = p[0];
        }
        std::fwrite(row.data(), 1, rowBytes, f);
    }
    std::fclose(f);
}

// Minimal PNG writer (stored deflate blocks), used for the app icon (needs alpha).
static void savePNG(const char* path, int w, int h, const std::vector<unsigned char>& rgbaBottomUp) {
    auto crc = [](const unsigned char* d, size_t n, unsigned c = 0xFFFFFFFFu) {
        for (size_t i = 0; i < n; ++i) {
            c ^= d[i];
            for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
        }
        return c;
    };
    std::vector<unsigned char> raw;
    for (int y = h - 1; y >= 0; --y) {  // GL rows are bottom-up; PNG is top-down
        raw.push_back(0);
        raw.insert(raw.end(), &rgbaBottomUp[(size_t)y * w * 4], &rgbaBottomUp[(size_t)y * w * 4] + w * 4);
    }
    std::vector<unsigned char> z = {0x78, 0x01};
    unsigned a = 1, b = 0;
    for (unsigned char c : raw) { a = (a + c) % 65521; b = (b + a) % 65521; }
    for (size_t off = 0; off < raw.size();) {
        size_t n = std::min<size_t>(65535, raw.size() - off);
        bool last = off + n == raw.size();
        z.push_back(last ? 1 : 0);
        z.push_back(n & 255); z.push_back(n >> 8); z.push_back(~n & 255); z.push_back((~n >> 8) & 255);
        z.insert(z.end(), raw.begin() + off, raw.begin() + off + n);
        off += n;
    }
    unsigned ad = (b << 16) | a;
    for (int s = 24; s >= 0; s -= 8) z.push_back((ad >> s) & 255);
    FILE* f = std::fopen(path, "wb");
    if (!f) return;
    auto be32 = [&](unsigned v) { unsigned char t[4] = {(unsigned char)(v >> 24), (unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v}; std::fwrite(t, 1, 4, f); };
    auto chunk = [&](const char* type, const std::vector<unsigned char>& data) {
        be32((unsigned)data.size());
        std::vector<unsigned char> td(type, type + 4);
        td.insert(td.end(), data.begin(), data.end());
        std::fwrite(td.data(), 1, td.size(), f);
        be32(crc(td.data(), td.size()) ^ 0xFFFFFFFFu);
    };
    const unsigned char sig[8] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
    std::fwrite(sig, 1, 8, f);
    std::vector<unsigned char> ihdr = {(unsigned char)(w >> 24), (unsigned char)(w >> 16), (unsigned char)(w >> 8), (unsigned char)w,
                                       (unsigned char)(h >> 24), (unsigned char)(h >> 16), (unsigned char)(h >> 8), (unsigned char)h,
                                       8, 6, 0, 0, 0};
    chunk("IHDR", ihdr);
    chunk("IDAT", z);
    chunk("IEND", {});
    std::fclose(f);
}

// App icon in the same style: dark rounded square, blue plane, yellow wave, sigma.
static void drawIcon(ImDrawList* dl, ImVec2 size, ImFont* math) {
    using namespace theme;
    const float S = std::min(size.x, size.y), m = S * 0.098f, r = S * 0.18f;
    const ImVec2 a(m, m), b(S - m, S - m);
    dl->AddRectFilled(a, b, col(BG), r);
    const float w = b.x - a.x;
    for (int i = 1; i < 8; ++i) {  // grid stays clear of the rounded corners
        float t = a.x + w * i / 8.f;
        bool axis = i == 4;
        ImU32 c = axis ? col(WHITE, 0.85f) : col(BLUE_D, 0.5f);
        float th = axis ? S * 0.006f : S * 0.003f;
        if (t > a.x + r * 0.95f && t < b.x - r * 0.95f) {
            dl->AddLine(ImVec2(t, a.y + S * 0.01f), ImVec2(t, b.y - S * 0.01f), c, th);
            dl->AddLine(ImVec2(a.x + S * 0.01f, t), ImVec2(b.x - S * 0.01f, t), c, th);
        } else {
            dl->AddLine(ImVec2(t, a.y + r), ImVec2(t, b.y - r), c, th);
            dl->AddLine(ImVec2(a.x + r, t), ImVec2(b.x - r, t), c, th);
        }
    }
    std::vector<ImVec2> pts;
    for (int i = 0; i <= 200; ++i) {
        float u = i / 200.f, x = a.x + w * (0.06f + 0.88f * u);
        pts.push_back(ImVec2(x, (a.y + b.y) * 0.5f + w * 0.05f + w * 0.24f * std::sin(u * 6.2831853f * 1.25f)));
    }
    dl->AddPolyline(pts.data(), (int)pts.size(), col(BG, 0.9f), S * 0.045f, 0);
    dl->AddPolyline(pts.data(), (int)pts.size(), col(YELLOW_C), S * 0.026f, 0);
    (void)math;
    ts::Box sig = ts::text("\xE2\x88\x91", S * 0.24f);
    ts::draw(dl, sig, ImVec2(a.x + w * 0.1f, a.y + w * 0.27f), col(BLUE_C));
}

static void usage() {
    std::printf(
        "eqviz [options]\n"
        "  --eq \"<expr>\"     equation to plot (e.g. \"sin(x)\", \"z^2\", \"sum(k,1,n,1/k^2)\")\n"
        "  --complex          start on the complex plane\n"
        "  --3d               start in 3D\n"
        "  --deriv            show the first derivative\n"
        "  --domain-color     domain colouring (complex plane)\n"
        "  --rotate           auto-rotate the 3D camera\n"
        "  --size WxH         window size\n"
        "  --frames N         render N frames at 60 fps, then exit\n"
        "  --shot file.bmp    with --frames: save the last frame\n"
        "  --sum-pos T        freeze the summation player at T terms\n"
        "  --project FILE     open a saved project, or demo:riemann / demo:poincare\n");
}

int main(int argc, char** argv) {
    LaunchOptions opt;
    std::string renderIcon;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : std::string(); };
        if (a == "--eq") opt.eq = next();
        else if (a == "--complex") opt.complex = true;
        else if (a == "--3d") opt.threeD = true;
        else if (a == "--deriv") opt.deriv = true;
        else if (a == "--domain-color") opt.domainColor = true;
        else if (a == "--rotate") opt.autoRotate = true;
        else if (a == "--frames") opt.frames = std::atoi(next().c_str());
        else if (a == "--shot") opt.shot = next();
        else if (a == "--sum-pos") opt.sumPos = std::atof(next().c_str());
        else if (a == "--size") std::sscanf(next().c_str(), "%dx%d", &opt.width, &opt.height);
        else if (a == "--actions") opt.actions = next();
        else if (a == "--project") opt.project = next();
        else if (a == "--render-icon") { renderIcon = next(); opt.width = opt.height = 512; }
        else if (a == "--mouse") std::sscanf(next().c_str(), "%f,%f", &opt.mouseX, &opt.mouseY);
        else if (a == "-h" || a == "--help") { usage(); return 0; }
        else { std::fprintf(stderr, "unknown option %s\n", a.c_str()); usage(); return 1; }
    }

    glfwSetErrorCallback([](int code, const char* msg) { std::fprintf(stderr, "GLFW %d: %s\n", code, msg); });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* window = glfwCreateWindow(opt.width, opt.height, "eqviz \xE2\x80\x94 equation visualizer", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Fonts: a clean sans for the UI, an OpenType math font for the equations.
    const char* mathPaths[] = {
        "/System/Library/Fonts/Supplemental/STIXTwoMath.otf",
        "/Library/Fonts/latinmodern-math.otf",
        "/usr/share/fonts/opentype/stix/STIXTwoMath-Regular.otf",
        "/usr/share/texmf/fonts/opentype/public/lm-math/latinmodern-math.otf",
        "C:\\Windows\\Fonts\\cambria.ttc",
    };
    const char* mathFallback = nullptr;
    for (const char* p : mathPaths) if (fileExists(p)) { mathFallback = p; break; }

    Fonts fonts;
    fonts.ui = loadFont({{"/System/Library/Fonts/Avenir Next.ttc", 5}, {"/System/Library/Fonts/HelveticaNeue.ttc", 0},
                         {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0}, {"C:\\Windows\\Fonts\\segoeui.ttf", 0}},
                        16.f);
    if (!fonts.ui) fonts.ui = io.Fonts->AddFontDefault();
    if (mathFallback) mergeFallback(mathFallback, 16.f);
    fonts.uiBold = loadFont({{"/System/Library/Fonts/Avenir Next.ttc", 2}, {"/System/Library/Fonts/HelveticaNeue.ttc", 1},
                             {"/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 0}, {"C:\\Windows\\Fonts\\segoeuib.ttf", 0}},
                            16.f);
    if (fonts.uiBold && mathFallback) mergeFallback(mathFallback, 16.f);
    if (!fonts.uiBold) fonts.uiBold = fonts.ui;
    if (mathFallback) {
        fonts.math = loadFont({{mathFallback, 0}}, 24.f);
        fonts.mathAlnum = fonts.math != nullptr;
    }
    if (!fonts.math) {
        fonts.math = loadFont({{"/System/Library/Fonts/Supplemental/Times New Roman Italic.ttf", 0}}, 24.f);
        if (!fonts.math) fonts.math = fonts.ui;
    }
    io.FontDefault = fonts.ui;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    if (!renderIcon.empty()) {  // render the icon for packaging, then exit
        glfwSetWindowSize(window, opt.width, opt.height);
        ts::init(fonts.math, fonts.mathAlnum);
        for (int k = 0; k < 3; ++k) {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            drawIcon(ImGui::GetBackgroundDrawList(), io.DisplaySize, fonts.math);
            ImGui::Render();
            int fw, fh;
            glfwGetFramebufferSize(window, &fw, &fh);
            glViewport(0, 0, fw, fh);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if (k == 2) {
                std::vector<unsigned char> px((size_t)fw * fh * 4);
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glReadPixels(0, 0, fw, fh, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
                savePNG(renderIcon.c_str(), fw, fh, px);
            }
            glfwSwapBuffers(window);
        }
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwTerminate();
        return 0;
    }

    App app;
    app.init(fonts, opt);

    double last = glfwGetTime();
    int frame = 0;
    double cpuMs = 0;
    ImVec4 bg = theme::colv(theme::BG);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double now = glfwGetTime();
        float dt = opt.frames > 0 ? 1.f / 60.f : (float)std::min(0.1, now - last);
        last = now;

        auto t0 = std::chrono::steady_clock::now();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        if (opt.mouseX >= 0) io.AddMousePosEvent(opt.mouseX, opt.mouseY);
        ImGui::NewFrame();
        app.frame(dt);
        ImGui::Render();
        cpuMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();

        int fw, fh;
        glfwGetFramebufferSize(window, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glClearColor(bg.x, bg.y, bg.z, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (opt.frames > 0 && ++frame >= opt.frames) {
            if (!opt.shot.empty()) {
                std::vector<unsigned char> px((size_t)fw * fh * 4);
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glReadPixels(0, 0, fw, fh, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
                saveBMP(opt.shot.c_str(), fw, fh, px);
            }
            std::printf("average CPU time per frame: %.2f ms over %d frames\n", cpuMs / frame, frame);
            break;
        }
        glfwSwapBuffers(window);
    }

    app.shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
