// Lesson text often writes maths as plain Unicode ("3x − 5", "|x − a| < δ", "A(x) = 2πx² + 2000/x").
// autoMath finds those runs in prose and wraps them in $…$ (rewritten in the parser's syntax) so the
// note pane typesets them like the graph labels. A run is only converted when it parses; anything
// doubtful stays as it was.
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "app.hpp"

bool noteMathOk(const std::string& src);  // notes.cpp

namespace {

uint32_t decode(const std::string& s, size_t i, size_t& len) {
    const unsigned char c = (unsigned char)s[i];
    if (c < 0x80) { len = 1; return c; }
    if ((c >> 5) == 6 && i + 1 < s.size()) { len = 2; return ((c & 31u) << 6) | (s[i + 1] & 63u); }
    if ((c >> 4) == 14 && i + 2 < s.size()) { len = 3; return ((c & 15u) << 12) | ((s[i + 1] & 63u) << 6) | (s[i + 2] & 63u); }
    if ((c >> 3) == 30 && i + 3 < s.size()) {
        len = 4;
        return ((c & 7u) << 18) | ((s[i + 1] & 63u) << 12) | ((s[i + 2] & 63u) << 6) | (s[i + 3] & 63u);
    }
    len = 1;
    return c;
}

char superChar(uint32_t c) {
    switch (c) {
        case 0x2070: return '0'; case 0xB9: return '1'; case 0xB2: return '2'; case 0xB3: return '3';
        case 0x2074: return '4'; case 0x2075: return '5'; case 0x2076: return '6'; case 0x2077: return '7';
        case 0x2078: return '8'; case 0x2079: return '9'; case 0x207A: return '+'; case 0x207B: return '-';
        case 0x207F: return 'n'; case 0x2E3: return 'x'; case 0x1D57: return 't'; case 0x1D4F: return 'k';
        case 0x1D56: return 'p'; case 0x2071: return 'i'; case 0x2E2: return 's';
        default: return 0;
    }
}
char subChar(uint32_t c) {
    if (c >= 0x2080 && c <= 0x2089) return (char)('0' + (c - 0x2080));
    switch (c) {
        case 0x2099: return 'n'; case 0x2096: return 'k'; case 0x2093: return 'x'; case 0x2090: return 'a';
        case 0x209C: return 't'; case 0x2098: return 'm'; case 0x1D62: return 'i'; case 0x2C7C: return 'j';
        default: return 0;
    }
}
const char* greekName(uint32_t c) {
    switch (c) {
        case 0x3B1: return "alpha"; case 0x3B2: return "beta"; case 0x3B3: return "gamma"; case 0x3B4: return "delta";
        case 0x3B5: return "epsilon"; case 0x3B6: return "zeta"; case 0x3B7: return "eta"; case 0x3B8: return "theta";
        case 0x3BA: return "kappa"; case 0x3BB: return "lambda"; case 0x3BC: return "mu"; case 0x3BD: return "nu";
        case 0x3BE: return "xi"; case 0x3C0: return "pi"; case 0x3C1: return "rho"; case 0x3C3: return "sigma";
        case 0x3C4: return "tau"; case 0x3C6: return "phi"; case 0x3C7: return "chi"; case 0x3C8: return "psi";
        case 0x3C9: return "omega"; case 0x394: return "Delta";
        default: return nullptr;
    }
}
bool isOp(uint32_t c) {
    switch (c) {
        case '=': case '+': case '-': case '/': case '<': case '>': case '^': case '*':
        case 0x2212: case 0xD7: case 0xB7: case 0xF7: case 0x2264: case 0x2265: case 0x2260: case 0x2248: case 0xB1: case 0x2192:
            return true;
        default: return false;
    }
}
bool isPrime(uint32_t c) { return c >= 0x2032 && c <= 0x2034; }

const std::set<std::string>& funcs() {
    static const std::set<std::string> f = {"sin", "cos", "tan", "sec", "csc", "cot", "ln", "log", "exp", "sqrt", "max", "min",
                                            "det", "atan", "arctan", "arcsin", "arccos", "sinh", "cosh", "tanh", "floor", "abs"};
    return f;
}
bool singleOk(char c) { return std::string("xyztnkfghuvwrcLM").find(c) != std::string::npos; }

enum Kind { None, Op, Num, Math };

std::string stripEdges(const std::string& w) {
    size_t a = 0, b = w.size();
    auto lead = [&](char c) { return c == '(' || c == '[' || c == '*' || c == '"'; };
    auto trail = [&](char c) { return c == ')' || c == ']' || c == '*' || c == '"' || c == ',' || c == '.' || c == ';' || c == ':' || c == '?' || c == '!'; };
    while (a < b && lead(w[a])) ++a;
    while (b > a && trail(w[b - 1])) --b;
    return w.substr(a, b - a);
}

Kind classify(const std::string& raw) {
    const std::string c = stripEdges(raw);
    if (c.empty()) return None;
    // not maths: problem labels "(a)", decades "1680s", units
    if (c.size() == 1 && raw.find('(') != std::string::npos && raw.find(')') != std::string::npos) return None;
    if (c.size() > 1 && c.back() == 's' && std::all_of(c.begin(), c.end() - 1, [](char d) { return std::isdigit((unsigned char)d); })) return None;
    static const std::set<std::string> units = {"m/s", "m/s\xC2\xB2", "m/s^2", "N/m", "N*s/m", "N\xC2\xB7s/m", "L/min", "kg", "cm", "km/h", "cm\xC2\xB3", "cm\xC2\xB2", "m\xC2\xB2", "m\xC2\xB3"};
    if (units.count(c)) return None;
    bool anyOp = false, anyDigit = false, anyLetter = false, signal = false, other = false;
    std::string run;
    char prevType = 0;  // 'd' digit, 'l' letter run, 'f' function name, 0 other
    int singles = 0;
    auto flush = [&]() -> bool {
        if (run.empty()) return true;
        anyLetter = true;
        if (run.size() == 1) { ++singles; prevType = 'l'; }
        else if (funcs().count(run)) prevType = 'f';
        else return false;
        run.clear();
        return true;
    };
    for (size_t i = 0; i < c.size();) {
        size_t len;
        const uint32_t cp = decode(c, i, len);
        if (cp < 128 && std::isalpha((int)cp)) {
            if (run.empty() && prevType == 'd') signal = true;  // 3x
            run += (char)cp;
            i += len;
            continue;
        }
        const bool afterLetter = !run.empty();
        if (!flush()) return None;
        if (cp < 128 && std::isdigit((int)cp)) {
            if (afterLetter) signal = true;  // x2
            anyDigit = true;
            prevType = 'd';
        } else if (cp == '.') {
            prevType = 0;
        } else if (isOp(cp)) {
            anyOp = true;
            prevType = 0;
        } else if (cp == '(' || cp == ')' || cp == ',' || cp == '[' || cp == ']') {
            if (cp == '(' && afterLetter) signal = true;  // f(x), sin(x)
            other = true;
            prevType = 0;
        } else if (superChar(cp) || subChar(cp) || isPrime(cp) || greekName(cp) || cp == 0x221A || cp == 0x221E || cp == '|') {
            signal = true;
            other = true;
            prevType = greekName(cp) ? 'l' : 0;
        } else {
            return None;  // apostrophes, dashes, accented letters...: prose
        }
        i += len;
    }
    if (!flush()) return None;
    if (!anyLetter && !other) {
        if (anyDigit) return anyOp && !(c.size() > 1 && (c[0] == '-' || c.compare(0, 3, "\xE2\x88\x92") == 0)) ? Math : Num;
        return anyOp ? Op : None;
    }
    if (anyOp && (anyLetter || anyDigit)) signal = true;  // x+1, 2/3
    if (c.size() == 1 && anyLetter) return singleOk(c[0]) ? Math : None;
    return signal ? Math : None;
}

// Unicode maths -> the parser's syntax.
std::string toParser(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size();) {
        size_t len;
        const uint32_t cp = decode(s, i, len);
        if (char sc = superChar(cp)) {
            std::string sup;
            size_t j = i;
            while (j < s.size()) {
                size_t l2;
                char k = superChar(decode(s, j, l2));
                if (!k) break;
                sup += k;
                j += l2;
            }
            out += sup.size() == 1 ? "^" + sup : "^(" + sup + ")";
            i = j;
            (void)sc;
            continue;
        }
        if (char k = subChar(cp)) {
            std::string sub;
            size_t j = i;
            while (j < s.size()) {
                size_t l2;
                char q = subChar(decode(s, j, l2));
                if (!q) break;
                sub += q;
                j += l2;
            }
            out += "_" + sub;
            i = j;
            (void)k;
            continue;
        }
        if (const char* g = greekName(cp)) { out += std::string(" ") + g + " "; i += len; continue; }
        switch (cp) {
            case 0x2212: out += "-"; break;
            case 0xB7: out += "*"; break;
            case 0xF7: out += "/"; break;
            case 0xD7: out += " \\times "; break;
            case 0x221E: out += " inf "; break;
            case 0x221A: {  // √x, √(x + 1)
                size_t j = i + len;
                if (j < s.size() && s[j] == '(') { out += "sqrt"; break; }
                std::string arg;
                while (j < s.size()) {
                    size_t l2;
                    uint32_t q = decode(s, j, l2);
                    if (q < 128 && std::isalnum((int)q)) { arg += (char)q; j += l2; }
                    else if (const char* g = greekName(q)) { arg += std::string(" ") + g + " "; j += l2; }
                    else break;
                }
                out += "sqrt(" + arg + ")";
                i = j;
                continue;
            }
            default: out.append(s, i, len);
        }
        i += len;
    }
    // tidy doubled spaces
    std::string t;
    for (char c : out)
        if (!(c == ' ' && (t.empty() || t.back() == ' '))) t += c;
    while (!t.empty() && t.back() == ' ') t.pop_back();
    return t;
}

int count(const std::string& s, char c) {
    int n = 0;
    for (char d : s) n += d == c;
    return n;
}

// Convert the maths runs in one stretch of prose (no $…$, no links inside).
std::string convertProse(const std::string& seg) {
    std::vector<std::string> words;
    {
        size_t a = 0;
        for (size_t i = 0; i <= seg.size(); ++i)
            if (i == seg.size() || seg[i] == ' ') { words.push_back(seg.substr(a, i - a)); a = i + 1; }
    }
    std::vector<Kind> kinds;
    for (auto& w : words) kinds.push_back(classify(w));
    auto endsClause = [](const std::string& w) {
        if (w.empty()) return false;
        const char c = w.back();
        return c == ',' || c == ';' || c == ':' || c == '.' || c == '?' || c == '!';
    };
    std::string out;
    size_t i = 0;
    bool firstWord = true;
    auto emit = [&](const std::string& w) { out += (firstWord ? "" : " ") + w; firstWord = false; };
    while (i < words.size()) {
        if (kinds[i] == None) { emit(words[i]); ++i; continue; }
        size_t j = i;
        while (j < words.size() && kinds[j] != None) {
            ++j;
            if (endsClause(words[j - 1])) break;
        }
        // trim leading/trailing operators and lone numbers
        size_t a = i, b = j;
        auto numOk = [&](size_t k, bool lead) {
            return lead ? (k + 1 < b && kinds[k + 1] == Op) : (k > a && kinds[k - 1] == Op);
        };
        bool changed = true;
        while (changed && a < b) {
            changed = false;
            if (kinds[a] == Op || (kinds[a] == Num && !numOk(a, true))) { ++a; changed = true; continue; }
            if (kinds[b - 1] == Op || (kinds[b - 1] == Num && !numOk(b - 1, false))) { --b; changed = true; }
        }
        bool hasMath = false;
        for (size_t k = a; k < b; ++k) hasMath |= kinds[k] == Math;
        for (size_t k = i; k < a; ++k) emit(words[k]);
        if (a < b && hasMath) {
            std::string core;
            for (size_t k = a; k < b; ++k) core += (k > a ? " " : "") + words[k];
            std::string pre, post;
            for (bool again = true; again && !core.empty();) {
                again = false;
                const char f = core.front(), l = core.back();
                if (f == '*' || f == '"' || f == '[') { pre += f; core.erase(0, 1); again = true; continue; }
                if (l == '*' || l == '"' || l == ']' || l == ',' || l == '.' || l == ';' || l == ':' || l == '?' || l == '!') {
                    post.insert(post.begin(), l); core.pop_back(); again = true; continue;
                }
                if (f == '(' && count(core, '(') > count(core, ')')) { pre += f; core.erase(0, 1); again = true; continue; }
                if (l == ')' && count(core, ')') > count(core, '(')) { post.insert(post.begin(), l); core.pop_back(); again = true; continue; }
            }
            const std::string conv = toParser(core);
            if (!core.empty() && core.find("**") == std::string::npos && noteMathOk(conv)) emit(pre + "$" + conv + "$" + post);
            else for (size_t k = a; k < b; ++k) emit(words[k]);
        } else {
            for (size_t k = a; k < b; ++k) emit(words[k]);
        }
        for (size_t k = b; k < j; ++k) emit(words[k]);
        i = j;
    }
    return out;
}

std::string convertLine(const std::string& ln) {
    std::string out, prose;
    auto flush = [&] { out += convertProse(prose); prose.clear(); };
    for (size_t i = 0; i < ln.size();) {
        if (ln[i] == '$') {  // existing maths: copy through
            size_t e = ln.find('$', i + 1);
            if (ln.compare(i, 2, "$$") == 0) e = ln.find("$$", i + 2), e = e == std::string::npos ? e : e + 1;
            if (e == std::string::npos) { prose += ln.substr(i); break; }
            flush();
            out += ln.substr(i, e + 1 - i);
            i = e + 1;
            continue;
        }
        if (ln[i] == '[') {  // a link [label](target): copy through
            size_t m = ln.find("](", i), e = m == std::string::npos ? m : ln.find(')', m);
            if (e != std::string::npos) {
                flush();
                out += ln.substr(i, e + 1 - i);
                i = e + 1;
                continue;
            }
        }
        prose += ln[i++];
    }
    flush();
    return out;
}

}  // namespace

std::string autoMath(const std::string& text) {
    std::string out;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string ln = text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        const size_t lead = ln.find_first_not_of(' ');
        const bool skip = lead == std::string::npos || ln.compare(lead, 1, "#") == 0 || ln.compare(lead, 2, "$$") == 0;
        if (skip) out += ln;
        else if (ln.compare(lead, 2, "- ") == 0) out += ln.substr(0, lead + 2) + convertLine(ln.substr(lead + 2));
        else out += ln.substr(0, lead) + convertLine(ln.substr(lead));
        if (nl == std::string::npos) break;
        out += '\n';
        pos = nl + 1;
    }
    return out;
}
