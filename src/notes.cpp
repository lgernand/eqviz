// Note panes: explanatory text with typeset mathematics.
//
// Markup (one block per line):
//   # Heading / ## Subheading       - bullet        blank line = new paragraph
//   $$ expression $$                displayed equation (relations = < > <= >= != ~ -> allowed)
//   inline $x^2$ maths, **bold** text, \name for upright words or symbols (\partial, \nabla, \Ric)
#include <algorithm>
#include <cctype>
#include <cstring>

#include "app.hpp"
#include "theme.hpp"
#include "misc/cpp/imgui_stdlib.h"

using namespace theme;

namespace {

std::string trim(const std::string& p) {
    size_t a = p.find_first_not_of(" \t"), b = p.find_last_not_of(" \t");
    return a == std::string::npos ? "" : p.substr(a, b - a + 1);
}

// Top-level comma split ("" when there is no top-level comma).
std::vector<std::string> commaSplit(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    int depth = 0;
    bool any = false;
    for (char c : s) {
        if (c == '(' || c == '[' || c == '{') ++depth;
        if (c == ')' || c == ']' || c == '}') --depth;
        if (c == ',' && depth == 0) { out.push_back(cur); cur.clear(); any = true; continue; }
        cur += c;
    }
    out.push_back(cur);
    if (!any) out.clear();
    return out;
}

// One side of a relation: an expression, a list "a, b, ...", a tuple "(p, q)" or f(a, b).
// Names written directly before "(" that aren't built-ins (R(x), pi_0(x), u(y, t)) are
// function notation in prose, not multiplication.
std::set<std::string> callNames(const std::string& s) {
    std::set<std::string> out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (!std::isalpha((unsigned char)s[i]) || (i > 0 && (std::isalnum((unsigned char)s[i - 1]) || s[i - 1] == '_' || s[i - 1] == '\\')))
            continue;
        size_t j = i;
        while (j < s.size() && (std::isalnum((unsigned char)s[j]) || s[j] == '_')) ++j;
        if (j < s.size() && s[j] == '(') {
            std::string n = s.substr(i, j - i);
            if (!eq::isBuiltinFunction(n)) out.insert(n);
        }
        i = j;
    }
    return out;
}

ts::Box mathPart(const std::string& pIn, float size) {
    const std::string p = trim(pIn);
    if (p == "..." || p == "\xE2\x80\xA6") return ts::text("\xE2\x80\xA6", size);
    const std::set<std::string> calls = callNames(p);
    eq::ParseResult r = eq::parse(p, calls.empty() ? nullptr : &calls, true);
    if (r.root) return ts::expr(r.root, size);
    auto list = [&](const std::vector<std::string>& parts) {
        std::vector<ts::Box> bs;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i) { bs.push_back(ts::text(",", size)); bs.push_back(ts::space(0.25f * size)); }
            bs.push_back(mathPart(parts[i], size));
        }
        return ts::hcat(bs);
    };
    // f(a, b) or (a, b)
    size_t open = p.find('(');
    if (open != std::string::npos && p.back() == ')') {
        std::vector<std::string> inner = commaSplit(p.substr(open + 1, p.size() - open - 2));
        std::string head = trim(p.substr(0, open));
        bool identHead = head.empty() || std::all_of(head.begin(), head.end(), [](unsigned char c) { return std::isalnum(c) || c == '_' || c == '\\'; });
        if (!inner.empty() && identHead) {
            ts::Box args = ts::parens(list(inner), size);
            return head.empty() ? args : ts::hcat({ts::ident(head, size), args});
        }
    }
    std::vector<std::string> parts = commaSplit(p);
    if (!parts.empty()) return list(parts);
    static std::set<std::string> warned;
    if (warned.insert(p).second) std::fprintf(stderr, "eqviz note: showing '%s' as text (%s)\n", p.c_str(), r.error.c_str());
    return ts::text(p, size);
}

// Typeset "lhs = rhs < ..." by splitting at top-level relations and parsing each side.
ts::Box mathBox(const std::string& src, float size) {
    static const std::pair<const char*, const char*> rels[] = {
        {"<=", "\xE2\x89\xA4"}, {">=", "\xE2\x89\xA5"}, {"!=", "\xE2\x89\xA0"}, {"->", "\xE2\x86\x92"},
        {"\xE2\x89\xA4", "\xE2\x89\xA4"}, {"\xE2\x89\xA5", "\xE2\x89\xA5"}, {"\xE2\x89\xA0", "\xE2\x89\xA0"},
        {"\xE2\x89\x88", "\xE2\x89\x88"}, {"\xE2\x86\x92", "\xE2\x86\x92"}, {"\xE2\x89\x85", "\xE2\x89\x85"},
        {"\xC2\xB1", "\xC2\xB1"}, {"~", "\xE2\x89\x88"}, {"=", "="}, {"<", "<"}, {">", ">"}};
    std::vector<std::string> parts, ops;
    std::string cur;
    int depth = 0;
    for (size_t i = 0; i < src.size();) {
        char c = src[i];
        if (c == '(' || c == '[' || c == '{') ++depth;
        if (c == ')' || c == ']' || c == '}') --depth;
        bool matched = false;
        if (depth == 0)
            for (auto& r : rels) {
                size_t n = std::strlen(r.first);
                if (src.compare(i, n, r.first) == 0) {
                    parts.push_back(cur);
                    ops.push_back(r.second);
                    cur.clear();
                    i += n;
                    matched = true;
                    break;
                }
            }
        if (!matched) { cur += c; ++i; }
    }
    parts.push_back(cur);
    std::vector<ts::Box> boxes;
    for (size_t k = 0; k < parts.size(); ++k) {
        const std::string p = trim(parts[k]);
        if (!p.empty()) boxes.push_back(mathPart(p, size));
        if (k < ops.size()) boxes.push_back(ts::binop(ops[k].c_str(), size));
    }
    return ts::hcat(boxes);
}

}  // namespace

// Whether every side of "lhs = rhs < ..." parses as an expression (so it typesets as maths, not as text).
bool noteMathOk(const std::string& src) {
    std::string t = src;
    for (const char* r : {"<=", ">=", "!=", "->", "\xE2\x89\xA4", "\xE2\x89\xA5", "\xE2\x89\xA0", "\xE2\x89\x88", "\xE2\x86\x92", "\xC2\xB1"}) {
        for (size_t at; (at = t.find(r)) != std::string::npos;) t.replace(at, std::strlen(r), "=");
    }
    for (char& c : t)
        if (c == '<' || c == '>' || c == '~') c = '=';
    size_t a = 0;
    int depth = 0;
    for (size_t i = 0; i <= t.size(); ++i) {
        if (i < t.size() && (t[i] == '(' || t[i] == '[')) ++depth;
        if (i < t.size() && (t[i] == ')' || t[i] == ']')) --depth;
        if (depth < 0) return false;
        if (i == t.size() || (t[i] == '=' && depth == 0)) {
            const std::string p = trim(t.substr(a, i - a));
            if (p.empty()) return false;
            const std::set<std::string> calls = callNames(p);
            if (!eq::parse(p, calls.empty() ? nullptr : &calls, true).root) return false;
            a = i + 1;
        }
    }
    return depth == 0;
}

// Draw one line of text with inline $maths$ at the ImGui cursor (for the lesson bar).
void richTextLine(const std::string& src, ImU32 textCol, ImU32 mathCol) {
    const std::string s = autoMath(src);
    ImGuiStyle& st = ImGui::GetStyle();
    bool first = true;
    auto sameLine = [&] { if (!first) ImGui::SameLine(0, 0); first = false; };
    for (size_t i = 0; i < s.size();) {
        size_t d = s.find('$', i);
        if (d != i) {  // prose
            const std::string t = s.substr(i, d == std::string::npos ? std::string::npos : d - i);
            sameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, textCol);
            ImGui::TextUnformatted(t.c_str());
            ImGui::PopStyleColor();
            if (d == std::string::npos) break;
            i = d;
            continue;
        }
        size_t e = s.find('$', i + 1);
        if (e == std::string::npos) e = s.size();
        const ts::Box b = mathBox(s.substr(i + 1, e - i - 1), ImGui::GetFontSize() * 1.12f);
        sameLine();
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float h = ImGui::GetFrameHeight();
        ImFontBaked* fb = ImGui::GetFontBaked();
        const float base = p.y + st.FramePadding.y + (fb ? fb->Ascent : ImGui::GetFontSize() * 0.8f);
        ImGui::Dummy(ImVec2(b.w + 2, h));
        ts::draw(ImGui::GetWindowDrawList(), b, ImVec2(p.x + 1, base), mathCol);
        i = e + 1;
    }
}

void App::layoutNote(float width) {
    Hasher hh;
    hh.u(std::hash<std::string>{}(noteText_));
    hh.d(width);
    hh.d(labelScale_);
    hh.u((uint64_t)(uintptr_t)fonts_.ui);
    hh.u((uint64_t)(uintptr_t)fonts_.math);
    if (hh.h == noteLayoutHash_) return;
    noteLayoutHash_ = hh.h;
    noteItems_.clear();
    noteColW_ = width;

    const float k = labelScale_;
    const float body = 16.5f * k, h1 = 27.f * k, h2 = 20.f * k, mInline = 18.5f * k, mDisplay = 23.f * k;
    float y = 0;
    ImFont* regular = fonts_.ui;
    ImFont* bold = fonts_.uiBold ? fonts_.uiBold : fonts_.ui;

    // flow a run of prose (with inline $maths$ and **bold**) into wrapped lines
    auto flow = [&](const std::string& textIn, ImFont* baseFont, float size, float indent, int role, bool bullet) {
        struct Tok { bool math; bool bold; std::string s; std::string link; };
        std::vector<Tok> toks;
        bool inMath = false, isBold = false;
        std::string word;
        auto pushWord = [&] {
            if (!word.empty()) toks.push_back({false, isBold, word, ""});
            word.clear();
        };
        for (size_t i = 0; i < textIn.size(); ++i) {
            char c = textIn[i];
            if (inMath) {
                if (c == '$') { toks.push_back({true, false, word, ""}); word.clear(); inMath = false; }
                else word += c;
                continue;
            }
            if (c == '$') { pushWord(); inMath = true; continue; }
            if (c == '[') {  // [label](target)
                const size_t m = textIn.find("](", i), e = m == std::string::npos ? m : textIn.find(')', m);
                if (e != std::string::npos) {
                    pushWord();
                    const std::string label = textIn.substr(i + 1, m - i - 1), target = textIn.substr(m + 2, e - m - 2);
                    size_t a = 0;
                    for (size_t q = 0; q <= label.size(); ++q)
                        if (q == label.size() || label[q] == ' ') {
                            if (a > 0) toks.push_back({false, isBold, " ", ""});
                            if (q > a) toks.push_back({false, isBold, label.substr(a, q - a), target});
                            a = q + 1;
                        }
                    i = e;
                    continue;
                }
            }
            if (c == '*' && i + 1 < textIn.size() && textIn[i + 1] == '*') { pushWord(); isBold = !isBold; ++i; continue; }
            if (c == ' ') { pushWord(); toks.push_back({false, isBold, " ", ""}); continue; }
            word += c;
        }
        pushWord();

        struct Piece { NoteItem item; float asc, desc; };
        std::vector<Piece> line;
        float x = indent;
        bool first = true;
        const float spaceW = baseFont->CalcTextSizeA(size, FLT_MAX, 0, " ").x;
        bool pendingSpace = false;
        auto finish = [&] {
            float asc = 0.8f * size, desc = 0.26f * size;
            for (auto& p : line) { asc = std::max(asc, p.asc); desc = std::max(desc, p.desc); }
            const float base = y + asc;
            if (bullet && first) {
                NoteItem d;
                d.kind = NoteItem::Dot;
                d.x = indent - 13 * k;
                d.y = base - 0.3f * size;
                d.role = 3;
                noteItems_.push_back(d);
            }
            for (auto& p : line) { p.item.y = base; noteItems_.push_back(p.item); }
            y = base + desc + 0.38f * size;
            line.clear();
            x = indent;
            first = false;
            pendingSpace = false;
        };
        for (const Tok& t : toks) {
            if (!t.math && t.s == " ") { pendingSpace = !line.empty(); continue; }
            Piece p;
            if (t.math) {
                p.item.kind = NoteItem::Math;
                p.item.box = mathBox(t.s, std::max(mInline, size * 1.1f));
                p.item.role = 2;
                p.item.w = p.item.box.w;
                p.asc = p.item.box.asc;
                p.desc = p.item.box.desc;
            } else {
                ImFont* f = t.bold ? bold : baseFont;
                p.item.kind = NoteItem::Text;
                p.item.font = f;
                p.item.size = size;
                p.item.s = t.s;
                p.item.role = t.bold && role == 0 ? 1 : role;
                if (!t.link.empty()) { p.item.link = t.link; p.item.role = 3; }
                p.item.w = f->CalcTextSizeA(size, FLT_MAX, 0, t.s.c_str()).x;
                p.asc = 0.8f * size;
                p.desc = 0.26f * size;
            }
            float gap = pendingSpace ? spaceW : 0;
            if (!line.empty() && x + gap + p.item.w > width) { finish(); gap = 0; }
            p.item.x = x + gap;
            x = p.item.x + p.item.w;
            line.push_back(p);
            pendingSpace = false;
        }
        if (!line.empty()) finish();
    };

    std::string para;
    auto flushPara = [&] {
        if (!para.empty()) { flow(para, regular, body, 0, 0, false); y += 0.45f * body; }
        para.clear();
    };
    std::string text = noteText_;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string ln = text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        pos = nl == std::string::npos ? text.size() + 1 : nl + 1;
        while (!ln.empty() && (ln.back() == ' ' || ln.back() == '\r')) ln.pop_back();
        size_t lead = ln.find_first_not_of(' ');
        std::string t = lead == std::string::npos ? "" : ln.substr(lead);
        if (t.empty()) { flushPara(); continue; }
        if (t.rfind("$$", 0) == 0) {
            flushPara();
            std::string m = t.substr(2);
            size_t end = m.rfind("$$");
            if (end != std::string::npos) m = m.substr(0, end);
            NoteItem it;
            it.kind = NoteItem::Math;
            it.box = mathBox(m, mDisplay);
            it.role = 2;
            it.w = it.box.w;
            it.x = std::max(0.f, (width - it.w) * 0.5f);
            y += 0.35f * body;
            it.y = y + it.box.asc;
            y = it.y + it.box.desc + 0.75f * body;
            noteItems_.push_back(it);
            continue;
        }
        if (t.rfind("# ", 0) == 0 || t.rfind("## ", 0) == 0) {
            flushPara();
            const bool top = t[1] == ' ';
            if (!noteItems_.empty()) y += (top ? 0.9f : 0.6f) * body;
            flow(t.substr(top ? 2 : 3), bold, top ? h1 : h2, 0, 1, false);
            if (top) {
                NoteItem r;
                r.kind = NoteItem::Rule;
                r.y = y - 0.15f * body;
                r.w = 54 * k;
                r.role = 3;
                noteItems_.push_back(r);
                y += 0.55f * body;
            }
            continue;
        }
        if (t.rfind("- ", 0) == 0) {
            flushPara();
            flow(t.substr(2), regular, body, 24 * k, 0, true);
            y += 0.1f * body;
            continue;
        }
        para += (para.empty() ? "" : " ") + t;
    }
    flushPara();
    noteContentH_ = y;
}

void App::drawNotePane(ImDrawList* dl) {
    const float W = canvasMax_.x - canvasMin_.x, H = canvasMax_.y - canvasMin_.y;
    dl->AddRectFilled(canvasMin_, canvasMax_, ImGui::ColorConvertFloat4ToU32(surface(0.025f)));
    const float pad = std::min(40.f, W * 0.06f + 12);
    const float colW = std::max(120.f, std::min(W - 2 * pad, 820.f * labelScale_));
    layoutNote(colW);
    const float maxScroll = std::max(0.f, noteContentH_ + 2 * pad - H);
    noteScroll_ = std::clamp(noteScroll_, 0.f, maxScroll);
    const float ox = canvasMin_.x + std::max(pad, (W - colW) * 0.5f);
    // leave room for the floating "show panel" button when the panel is hidden
    const bool underButton = panelSlide_ > 0.5f && canvasMin_.x < 8 && canvasMin_.y < 8;
    const float oy = canvasMin_.y + pad + (underButton ? 36.f : 0.f) - noteScroll_;
    const ImU32 cBody = ImGui::ColorConvertFloat4ToU32(surface(0.84f)), cHead = col(WHITE), cMath = col(WHITE),
                cAccent = col(BLUE_C);
    for (const NoteItem& it : noteItems_) {
        const float by = oy + it.y;
        if (by < canvasMin_.y - 80 || by > canvasMax_.y + 80) continue;  // off screen
        const ImU32 c = it.role == 1 ? cHead : it.role == 2 ? cMath : it.role == 3 ? cAccent : cBody;
        switch (it.kind) {
            case NoteItem::Text: {
                ImFontBaked* b = it.font->GetFontBaked(it.size);
                float asc = b ? std::round(b->Ascent) : it.size * 0.8f;
                dl->AddText(it.font, it.size, ImVec2(ox + it.x, by - asc), c, it.s.c_str());
                if (!it.link.empty()) {  // links: underline, brighter on hover, click to follow
                    const ImVec2 m = ImGui::GetIO().MousePos;
                    const bool hov = !ImGui::GetIO().WantCaptureMouse && inVisible(m) && m.x >= canvasMin_.x && m.x < canvasMax_.x &&
                                     m.y >= canvasMin_.y && m.y < canvasMax_.y && m.x >= ox + it.x && m.x < ox + it.x + it.w &&
                                     m.y >= by - asc && m.y < by + 0.3f * it.size;
                    dl->AddLine(ImVec2(ox + it.x, by + 2.5f), ImVec2(ox + it.x + it.w, by + 2.5f), hov ? col(WHITE) : col(BLUE_C, 0.5f), 1.2f);
                    if (hov) {
                        dl->AddText(it.font, it.size, ImVec2(ox + it.x, by - asc), col(WHITE), it.s.c_str());
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        if (ImGui::IsMouseClicked(0)) pendingLink_ = it.link;
                    }
                }
                break;
            }
            case NoteItem::Math: ts::draw(dl, it.box, ImVec2(ox + it.x, by), c); break;
            case NoteItem::Rule: dl->AddLine(ImVec2(ox, by), ImVec2(ox + it.w, by), c, 2.5f); break;
            case NoteItem::Dot: dl->AddCircleFilled(ImVec2(ox + it.x, by), 3.2f, c); break;
        }
    }
    if (maxScroll > 0) {  // slim position indicator
        float th = std::max(30.f, H * H / (noteContentH_ + 2 * pad));
        float ty = canvasMin_.y + (H - th) * (noteScroll_ / maxScroll);
        dl->AddRectFilled(ImVec2(canvasMax_.x - 6, ty + 4), ImVec2(canvasMax_.x - 3, ty + th - 4), col(WHITE, 0.25f), 2);
    }
}

bool App::scrollNoteAt(ImVec2 m, float delta) {
    for (int i = 0; i < (int)paneRects_.size(); ++i) {
        const ImVec4& r = paneRects_[i];
        if (m.x < r.x || m.x >= r.z || m.y < r.y || m.y >= r.w) continue;
        const bool here = i == cur_;
        const bool note = here ? isNote_ : panes_[i].isNote_;
        if (!note) return false;
        float& scroll = here ? noteScroll_ : panes_[i].noteScroll_;
        const float content = here ? noteContentH_ : panes_[i].noteContentH_;
        const float W = r.z - r.x, H = r.w - r.y, pad = std::min(40.f, W * 0.06f + 12);
        const float maxScroll = std::max(0.f, content + 2 * pad - H);
        const float next = std::clamp(scroll + delta, 0.f, maxScroll);
        if (next == scroll) return false;  // at an end (or fits): let the workspace scroll
        scroll = next;
        return true;
    }
    return false;
}

void App::drawNoteEditor() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::PushFont(nullptr, 12.5f);
    ImGui::TextWrapped("This pane is a note. Markup: # Heading, ## Subheading, - bullet, blank line for a new "
                       "paragraph, **bold**, $x^2$ inline maths, and a line $$ e^(i pi) + 1 = 0 $$ for a displayed "
                       "equation. \\partial, \\nabla, \\Ric etc. give upright symbols and words.");
    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::PushFont(nullptr, 14.f);
    ImGui::InputTextMultiline("##note", &noteText_, ImVec2(-1, 340), ImGuiInputTextFlags_AllowTabInput);
    ImGui::PopFont();
}

void App::addContextNote(const char* text, float heightFrac) {
    computePaneRects();
    std::vector<float> h = paneHeights();
    const int keepFocus = focus_;
    const int np = (int)panes_.size();
    panes_.emplace_back();
    enterPane(np);
    initPane();
    isNote_ = true;
    noteText_ = text;
    LayoutNode leaf;
    leaf.pane = np;
    nodes_.push_back(leaf);
    LayoutNode split;
    split.stacked = true;
    split.a = (int)nodes_.size() - 1;
    split.b = rootNode_;
    nodes_.push_back(split);
    rootNode_ = (int)nodes_.size() - 1;
    wsW_ = 1.f;
    scroll_ = ImVec2(0, 0);
    if (visRect_.w - visRect_.y < 50) {  // window size not known yet (called before the first frame)
        pendingNotePane_ = np;
        pendingNoteFrac_ = heightFrac;
        extendWorkspace_ = true;
        wsH_ = 1.f + heightFrac;
    } else {
        h.push_back((visRect_.w - visRect_.y) * heightFrac);
        relayoutFromHeights(h);
    }
    focus_ = keepFocus;
    enterPane(focus_);
}
