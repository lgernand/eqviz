#include "expr.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace eq {

static const double PI = 3.14159265358979323846;
static const cplx NaNc(NAN, NAN);

// ---------------------------------------------------------------- helpers

static std::shared_ptr<Node> newNode(Op op) {
    auto n = std::make_shared<Node>();
    n->op = op;
    return n;
}
NodeP num(cplx v) { auto n = newNode(Op::Num); n->val = v; return n; }
NodeP cnst(const std::string& s) {
    auto n = newNode(Op::Const);
    n->name = s;
    if (s == "pi") n->val = PI;
    else if (s == "e") n->val = std::exp(1.0);
    else if (s == "i") n->val = cplx(0, 1);
    else if (s == "inf") n->val = INFINITY;
    return n;
}
NodeP var(const std::string& s) { auto n = newNode(Op::Var); n->name = s; return n; }
NodeP bin(Op op, NodeP a, NodeP b) { auto n = newNode(op); n->kids = {std::move(a), std::move(b)}; return n; }
NodeP neg(NodeP a) { auto n = newNode(Op::Neg); n->kids = {std::move(a)}; return n; }
NodeP func(Fn f, NodeP a) { auto n = newNode(Op::Func); n->fn = f; n->kids = {std::move(a)}; return n; }
static NodeP withKids(const NodeP& src, std::vector<NodeP> kids) {
    auto n = std::make_shared<Node>(*src);
    n->kids = std::move(kids);
    return n;
}

std::string utf8(uint32_t cp) {
    std::string s;
    if (cp < 0x80) s += (char)cp;
    else if (cp < 0x800) { s += (char)(0xC0 | (cp >> 6)); s += (char)(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) {
        s += (char)(0xE0 | (cp >> 12)); s += (char)(0x80 | ((cp >> 6) & 0x3F)); s += (char)(0x80 | (cp & 0x3F));
    } else {
        s += (char)(0xF0 | (cp >> 18)); s += (char)(0x80 | ((cp >> 12) & 0x3F));
        s += (char)(0x80 | ((cp >> 6) & 0x3F)); s += (char)(0x80 | (cp & 0x3F));
    }
    return s;
}

struct GreekEntry { const char* name; uint32_t cp; };
static const GreekEntry kGreek[] = {
    {"alpha", 0x3B1}, {"beta", 0x3B2}, {"gamma", 0x3B3}, {"delta", 0x3B4}, {"epsilon", 0x3B5},
    {"zeta", 0x3B6}, {"eta", 0x3B7}, {"theta", 0x3B8}, {"iota", 0x3B9}, {"kappa", 0x3BA},
    {"lambda", 0x3BB}, {"mu", 0x3BC}, {"nu", 0x3BD}, {"xi", 0x3BE}, {"pi", 0x3C0},
    {"rho", 0x3C1}, {"sigma", 0x3C3}, {"tau", 0x3C4}, {"upsilon", 0x3C5}, {"phi", 0x3C6},
    {"chi", 0x3C7}, {"psi", 0x3C8}, {"omega", 0x3C9},
    {"Gamma", 0x393}, {"Delta", 0x394}, {"Theta", 0x398}, {"Lambda", 0x39B}, {"Xi", 0x39E},
    {"Pi", 0x3A0}, {"Sigma", 0x3A3}, {"Phi", 0x3A6}, {"Psi", 0x3A8}, {"Omega", 0x3A9},
};
uint32_t greekCodepoint(const std::string& name) {
    for (auto& g : kGreek) if (name == g.name) return g.cp;
    return 0;
}
const char* greekNameFor(uint32_t cp) {
    for (auto& g : kGreek) if (g.cp == cp) return g.name;
    if (cp == 0x3C2) return "sigma";
    return nullptr;
}

struct FnEntry { const char* name; Fn fn; bool splittable; };
static const FnEntry kFns[] = {
    {"sin", Fn::Sin, true}, {"cos", Fn::Cos, true}, {"tan", Fn::Tan, true},
    {"sec", Fn::Sec, true}, {"csc", Fn::Csc, true}, {"cot", Fn::Cot, true},
    {"asin", Fn::Asin, true}, {"acos", Fn::Acos, true}, {"atan", Fn::Atan, true},
    {"arcsin", Fn::Asin, true}, {"arccos", Fn::Acos, true}, {"arctan", Fn::Atan, true},
    {"sinh", Fn::Sinh, true}, {"cosh", Fn::Cosh, true}, {"tanh", Fn::Tanh, true},
    {"exp", Fn::Exp, true}, {"ln", Fn::Ln, true}, {"log", Fn::Log, true}, {"sqrt", Fn::Sqrt, true},
    {"abs", Fn::Abs, false}, {"re", Fn::Re, false}, {"Re", Fn::Re, false}, {"im", Fn::Im, false},
    {"Im", Fn::Im, false}, {"conj", Fn::Conj, false}, {"arg", Fn::Arg, false},
    {"floor", Fn::Floor, false}, {"ceil", Fn::Ceil, false}, {"round", Fn::Round, false},
    {"sign", Fn::Sign, false}, {"sgn", Fn::Sign, false}, {"gamma", Fn::Gamma, false},
    {"Gamma", Fn::Gamma, false}, {"zeta", Fn::Zeta, false},
    {"isprime", Fn::IsPrime, false}, {"mobius", Fn::Mobius, false}, {"zetazero", Fn::ZetaZero, false},
    {"Ei", Fn::Ei, false}, {"li", Fn::Li, false}, {"erf", Fn::Erf, true}, {"erfc", Fn::Erfc, true},
    {"primepi", Fn::PrimePi, false}, {"legendre", Fn::Legendre, false}, {"mod", Fn::Mod, false},
};
static bool lookupFn(const std::string& w, Fn& out) {
    for (auto& f : kFns) if (w == f.name) { out = f.fn; return true; }
    return false;
}

const char* fnDisplayName(Fn f) {
    switch (f) {
        case Fn::Sin: return "sin"; case Fn::Cos: return "cos"; case Fn::Tan: return "tan";
        case Fn::Sec: return "sec"; case Fn::Csc: return "csc"; case Fn::Cot: return "cot";
        case Fn::Asin: return "arcsin"; case Fn::Acos: return "arccos"; case Fn::Atan: return "arctan";
        case Fn::Sinh: return "sinh"; case Fn::Cosh: return "cosh"; case Fn::Tanh: return "tanh";
        case Fn::Exp: return "exp"; case Fn::Ln: return "ln"; case Fn::Log: return "log";
        case Fn::Sqrt: return "sqrt"; case Fn::Abs: return "abs"; case Fn::Re: return "Re";
        case Fn::Im: return "Im"; case Fn::Conj: return "conj"; case Fn::Arg: return "arg";
        case Fn::Floor: return "floor"; case Fn::Ceil: return "ceil"; case Fn::Round: return "round";
        case Fn::Sign: return "sgn"; case Fn::Gamma: return "Gamma"; case Fn::Zeta: return "zeta";
        case Fn::IsPrime: return "isprime"; case Fn::Mobius: return "mobius";
        case Fn::ZetaZero: return "zetazero"; case Fn::Ei: return "Ei"; case Fn::Li: return "li";
        case Fn::Erf: return "erf"; case Fn::Erfc: return "erfc"; case Fn::PrimePi: return "primepi";
        case Fn::Legendre: return "legendre"; case Fn::Mod: return "mod";
    }
    return "?";
}

// ---------------------------------------------------------------- lexer

namespace {

enum class T { Num, Ident, LP, RP, Comma, Plus, Minus, Star, Slash, Caret, Bang, Pipe, Eq, Sqrt, End };

struct Tok {
    T t = T::End;
    std::string s;
    double v = 0;
    int pos = 0;
};

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

[[noreturn]] void fail(const std::string& msg, int pos) {
    throw ParseError(msg + " (at column " + std::to_string(pos + 1) + ")");
}

uint32_t decode(const std::string& s, size_t& i) {
    unsigned char c = (unsigned char)s[i];
    uint32_t cp; int extra;
    if (c < 0x80) { cp = c; extra = 0; }
    else if ((c >> 5) == 6) { cp = c & 0x1F; extra = 1; }
    else if ((c >> 4) == 14) { cp = c & 0x0F; extra = 2; }
    else { cp = c & 0x07; extra = 3; }
    ++i;
    for (int k = 0; k < extra && i < s.size(); ++k, ++i) cp = (cp << 6) | ((unsigned char)s[i] & 0x3F);
    return cp;
}

thread_local const std::set<std::string>* gUserFns = nullptr;
thread_local bool gKeepParens = false;

bool isKnownWord(const std::string& w) {
    if (gUserFns && gUserFns->count(w)) return true;
    Fn f;
    if (lookupFn(w, f)) return true;
    if (greekCodepoint(w)) return true;
    return w == "pi" || w == "inf" || w == "infinity" || w == "sum" || w == "prod";
}

const std::vector<std::string>& splitCandidates() {
    static std::vector<std::string> c = [] {
        std::vector<std::string> v = {"pi", "inf", "sum", "prod"};
        for (auto& f : kFns) if (f.splittable) v.push_back(f.name);
        for (auto& g : kGreek) {
            std::string n = g.name;
            if (n != "xi" && n != "nu" && n != "pi") v.push_back(n);
        }
        return v;
    }();
    return c;
}

// "ax" -> a, x ; "sinx" -> sin, x ; "omega" -> omega
std::vector<std::string> splitWord(const std::string& w) {
    if (isKnownWord(w) || w.size() == 1) return {w};
    std::vector<std::string> out;
    size_t i = 0;
    while (i < w.size()) {
        size_t best = 0;
        for (const auto& cand : splitCandidates())
            if (cand.size() > best && w.compare(i, cand.size(), cand) == 0) best = cand.size();
        if (best == 0) best = 1;
        out.push_back(w.substr(i, best));
        i += best;
    }
    return out;
}

std::vector<Tok> lex(const std::string& s) {
    std::vector<Tok> toks;
    auto push = [&](T t, int pos, std::string str = {}, double v = 0) {
        Tok k; k.t = t; k.pos = pos; k.s = std::move(str); k.v = v; toks.push_back(k);
    };
    size_t i = 0;
    while (i < s.size()) {
        int pos = (int)i;
        unsigned char c = (unsigned char)s[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { ++i; continue; }
        if (std::isdigit(c) || (c == '.' && i + 1 < s.size() && std::isdigit((unsigned char)s[i + 1]))) {
            size_t j = i;
            while (j < s.size() && (std::isdigit((unsigned char)s[j]) || s[j] == '.')) ++j;
            std::string numStr = s.substr(i, j - i);
            if (std::count(numStr.begin(), numStr.end(), '.') > 1) fail("Malformed number '" + numStr + "'", pos);
            push(T::Num, pos, numStr, std::stod(numStr));
            i = j;
            continue;
        }
        if (c == '\\' && i + 1 < s.size() && std::isalpha((unsigned char)s[i + 1])) {  // \name: upright word / symbol
            size_t j = i + 1;
            while (j < s.size() && std::isalpha((unsigned char)s[j])) ++j;
            push(T::Ident, pos, s.substr(i, j - i));
            i = j;
            continue;
        }
        if (std::isalpha(c)) {
            size_t j = i;
            while (j < s.size() && std::isalpha((unsigned char)s[j])) ++j;
            std::string word = s.substr(i, j - i);
            std::string sub;
            if (j < s.size() && s[j] == '_') {
                ++j;
                if (j < s.size() && s[j] == '{') {
                    size_t k = s.find('}', j);
                    if (k == std::string::npos) fail("Missing '}' in subscript", (int)j);
                    sub = s.substr(j + 1, k - j - 1);
                    j = k + 1;
                } else {
                    size_t k = j;
                    while (k < s.size() && std::isalnum((unsigned char)s[k])) ++k;
                    sub = s.substr(j, k - j);
                    j = k;
                }
                if (sub.empty()) fail("Empty subscript", pos);
            }
            std::vector<std::string> parts = splitWord(word);
            Fn dummy;
            // a1 -> a_1 for single-letter / Greek names (not functions)
            if (sub.empty() && j < s.size() && std::isdigit((unsigned char)s[j])) {
                const std::string& last = parts.back();
                if ((last.size() == 1 || greekCodepoint(last)) && !lookupFn(last, dummy) && last != "e" && last != "i") {
                    size_t k = j;
                    while (k < s.size() && std::isdigit((unsigned char)s[k])) ++k;
                    sub = s.substr(j, k - j);
                    j = k;
                }
            }
            for (size_t p = 0; p < parts.size(); ++p) {
                std::string name = parts[p];
                if (p + 1 == parts.size() && !sub.empty()) name += "_" + sub;
                push(T::Ident, pos, name);
            }
            i = j;
            continue;
        }
        switch (c) {
            case '(': case '[': case '{': push(T::LP, pos); ++i; continue;
            case ')': case ']': case '}': push(T::RP, pos); ++i; continue;
            case ',': push(T::Comma, pos); ++i; continue;
            case '+': push(T::Plus, pos); ++i; continue;
            case '-': push(T::Minus, pos); ++i; continue;
            case '*': push(T::Star, pos); ++i; continue;
            case '/': push(T::Slash, pos); ++i; continue;
            case '^': push(T::Caret, pos); ++i; continue;
            case '!': push(T::Bang, pos); ++i; continue;
            case '|': push(T::Pipe, pos); ++i; continue;
            case '=': push(T::Eq, pos); ++i; continue;
            default: break;
        }
        if (c >= 0x80) {
            uint32_t cp = decode(s, i);
            switch (cp) {
                case 0xB7: case 0xD7: case 0x22C5: case 0x2217: push(T::Star, pos); continue;
                case 0xF7: case 0x2215: push(T::Slash, pos); continue;
                case 0x2212: case 0x2013: push(T::Minus, pos); continue;
                case 0x221A: push(T::Sqrt, pos); continue;
                case 0x2202: push(T::Ident, pos, "\\partial"); continue;
                case 0x2207: push(T::Ident, pos, "\\nabla"); continue;
                case 0x221E: push(T::Ident, pos, "inf"); continue;
                case 0x2211: case 0x3A3: push(T::Ident, pos, "sum"); continue;
                case 0x220F: push(T::Ident, pos, "prod"); continue;
                case 0xB2: push(T::Caret, pos); push(T::Num, pos, "2", 2); continue;
                case 0xB3: push(T::Caret, pos); push(T::Num, pos, "3", 3); continue;
                default: break;
            }
            if (const char* g = greekNameFor(cp)) { push(T::Ident, pos, g); continue; }
            fail("Unexpected character '" + utf8(cp) + "'", pos);
        }
        fail(std::string("Unexpected character '") + (char)c + "'", pos);
    }
    push(T::End, (int)s.size());
    return toks;
}

// ---------------------------------------------------------------- parser

class Parser {
public:
    explicit Parser(std::vector<Tok> t) : t_(std::move(t)) {}

    NodeP parseAll() {
        NodeP e = additive();
        if (peek().t != T::End) fail("Unexpected '" + describe(peek()) + "'", peek().pos);
        return e;
    }

private:
    std::vector<Tok> t_;
    size_t p_ = 0;
    int absDepth_ = 0;
    std::vector<std::string> bound_;

    const Tok& peek(int k = 0) const { return t_[std::min(p_ + k, t_.size() - 1)]; }
    void next() { if (p_ < t_.size() - 1) ++p_; }
    bool accept(T t) { if (peek().t == t) { next(); return true; } return false; }
    void expect(T t, const char* what) {
        if (!accept(t)) fail(std::string("Expected ") + what + ", found '" + describe(peek()) + "'", peek().pos);
    }
    static std::string describe(const Tok& k) {
        switch (k.t) {
            case T::Num: case T::Ident: return k.s;
            case T::LP: return "("; case T::RP: return ")"; case T::Comma: return ",";
            case T::Plus: return "+"; case T::Minus: return "-"; case T::Star: return "*";
            case T::Slash: return "/"; case T::Caret: return "^"; case T::Bang: return "!";
            case T::Pipe: return "|"; case T::Eq: return "="; case T::Sqrt: return "sqrt";
            case T::End: return "end of input";
        }
        return "?";
    }
    bool isBound(const std::string& n) const { return std::find(bound_.begin(), bound_.end(), n) != bound_.end(); }
    bool isFunctionIdent(const Tok& k) const {
        Fn f;
        return k.t == T::Ident && !isBound(k.s) && (lookupFn(k.s, f) || k.s == "sum" || k.s == "prod");
    }
    bool startsImplicit() const {
        T t = peek().t;
        return t == T::Num || t == T::Ident || t == T::LP || t == T::Sqrt || (t == T::Pipe && absDepth_ == 0);
    }

    NodeP additive() {
        NodeP a = term();
        for (;;) {
            if (accept(T::Plus)) a = bin(Op::Add, a, term());
            else if (accept(T::Minus)) a = bin(Op::Sub, a, term());
            else return a;
        }
    }
    NodeP term() {
        NodeP a = unary();
        for (;;) {
            if (accept(T::Star)) a = bin(Op::Mul, a, unary());
            else if (accept(T::Slash)) a = bin(Op::Div, a, unary());
            else if (startsImplicit()) a = bin(Op::Mul, a, power());
            else return a;
        }
    }
    NodeP unary() {
        if (accept(T::Minus)) return neg(unary());
        if (accept(T::Plus)) return unary();
        return power();
    }
    NodeP power() {
        NodeP base = postfix();
        if (accept(T::Caret)) return bin(Op::Pow, base, unary());
        return base;
    }
    NodeP postfix() {
        NodeP a = primary();
        while (accept(T::Bang)) { auto n = newNode(Op::Fact); n->kids = {a}; a = n; }
        return a;
    }
    NodeP primary() {
        const Tok& k = peek();
        switch (k.t) {
            case T::Num: { double v = k.v; next(); return num(v); }
            case T::LP: {
                next();
                NodeP e = additive();
                expect(T::RP, "')'");
                if (gKeepParens) {  // display-only grouping
                    auto g = newNode(Op::Call);
                    g->kids = {e};
                    return g;
                }
                return e;
            }
            case T::Pipe: {
                next(); ++absDepth_;
                NodeP e = additive();
                expect(T::Pipe, "closing '|'");
                --absDepth_;
                return func(Fn::Abs, e);
            }
            case T::Sqrt: { next(); return func(Fn::Sqrt, postfix()); }
            case T::Ident: return identifier();
            case T::End: fail("Expression ended unexpectedly", k.pos);
            default: fail("Unexpected '" + describe(k) + "'", k.pos);
        }
    }
    NodeP identifier() {
        std::string name = peek().s;
        int pos = peek().pos;
        next();
        if (isBound(name)) return var(name);
        if (name[0] == '\\') return var(name);  // upright name, e.g. \Ric, \partial
        if (gUserFns && gUserFns->count(name) && peek().t == T::LP) {
            next();
            auto call = newNode(Op::Call);
            call->name = name;
            if (peek().t != T::RP) {
                call->kids.push_back(additive());
                while (accept(T::Comma)) call->kids.push_back(additive());
            }
            expect(T::RP, "')' to close the call");
            return call;
        }
        if (name == "sum" || name == "prod") return sumLike(name == "prod", pos);
        Fn f;
        // gamma / zeta double as Greek letters; they are functions only when called
        bool greekToo = name == "gamma" || name == "Gamma" || name == "zeta";
        if (lookupFn(name, f) && (!greekToo || peek().t == T::LP)) return funcCall(f, pos);
        if (name == "pi") return cnst("pi");
        if (name == "e") return cnst("e");
        if (name == "i") return cnst("i");
        if (name == "inf" || name == "infinity") return cnst("inf");
        return var(name);
    }
    NodeP funcCall(Fn f, int pos) {
        NodeP exponent;
        if (accept(T::Caret)) {  // sin^2(x), sin^-1(x)
            bool negExp = accept(T::Minus);
            exponent = primary();
            if (negExp) exponent = neg(exponent);
        }
        NodeP arg;
        if (isBinaryFn(f)) {  // name(a, b)
            if (!accept(T::LP)) fail(std::string(fnDisplayName(f)) + " needs two arguments: " + fnDisplayName(f) + "(a, b)", pos);
            NodeP a = additive();
            expect(T::Comma, "',' between the two arguments");
            NodeP b = additive();
            expect(T::RP, "')'");
            auto n = newNode(Op::Func);
            n->fn = f;
            n->kids = {a, b};
            return n;
        }
        if (accept(T::LP)) {
            arg = additive();
            if (peek().t == T::Comma) fail("Function takes a single argument", peek().pos);
            expect(T::RP, "')'");
        } else {
            T t = peek().t;
            if (!(t == T::Num || t == T::Ident || t == T::Sqrt || t == T::Pipe))
                fail("Missing argument for function", pos);
            arg = power();
            while (peek().t == T::Num || (peek().t == T::Ident && !isFunctionIdent(peek())))
                arg = bin(Op::Mul, arg, power());
        }
        if (exponent && exponent->op == Op::Neg && exponent->kids[0]->op == Op::Num && exponent->kids[0]->val == 1.0) {
            if (f == Fn::Sin) return func(Fn::Asin, arg);
            if (f == Fn::Cos) return func(Fn::Acos, arg);
            if (f == Fn::Tan) return func(Fn::Atan, arg);
        }
        NodeP r = func(f, arg);
        if (exponent) r = bin(Op::Pow, r, exponent);
        return r;
    }
    NodeP sumLike(bool isProd, int pos) {
        const char* nm = isProd ? "prod" : "sum";
        expect(T::LP, "'(' after sum");
        if (peek().t != T::Ident)
            fail(std::string(nm) + " needs an index first, e.g. " + nm + "(k, 1, n, 1/k^2)", peek().pos);
        std::string idx = peek().s;
        next();
        NodeP lo, hi;
        if (accept(T::Eq)) {
            lo = additive();
            expect(T::Comma, "',' after the lower bound");
            hi = additive();
        } else {
            expect(T::Comma, "',' after the index");
            lo = additive();
            expect(T::Comma, "',' after the lower bound");
            hi = additive();
        }
        expect(T::Comma, "',' before the summand");
        bound_.push_back(idx);
        NodeP body = additive();
        bound_.pop_back();
        expect(T::RP, "')' to close the sum");
        auto n = newNode(isProd ? Op::Prod : Op::Sum);
        n->name = idx;
        n->kids = {lo, hi, body};
        (void)pos;
        return n;
    }
};

}  // namespace

ParseResult parse(const std::string& src, const std::set<std::string>* userFns, bool keepParens) {
    struct Guard {
        Guard(const std::set<std::string>* u, bool k) { gUserFns = u; gKeepParens = k; }
        ~Guard() { gUserFns = nullptr; gKeepParens = false; }
    } guard(userFns, keepParens);
    try {
        std::vector<Tok> toks = lex(src);
        int depth = 0, eqPos = -1;
        for (size_t i = 0; i < toks.size(); ++i) {
            if (toks[i].t == T::LP) ++depth;
            else if (toks[i].t == T::RP) --depth;
            else if (toks[i].t == T::Eq && depth == 0) {
                if (eqPos >= 0) fail("Only one '=' is allowed", toks[i].pos);
                eqPos = (int)i;
            }
        }
        std::vector<Tok> use = toks;
        if (eqPos >= 0) {
            std::vector<Tok> L(toks.begin(), toks.begin() + eqPos);
            std::vector<Tok> R(toks.begin() + eqPos + 1, toks.end() - 1);
            auto isDef = [](const std::vector<Tok>& v) {
                if (v.size() == 1 && v[0].t == T::Ident) return true;
                return v.size() >= 3 && v[0].t == T::Ident && v[1].t == T::LP && v.back().t == T::RP;
            };
            if (isDef(L)) use = R;
            else if (isDef(R)) use = L;
            else fail("Write the equation as y = ... or f(x) = ...", toks[eqPos].pos);
            use.push_back(toks.back());
        }
        if (use.size() <= 1) return {nullptr, "Type an expression, e.g. sin(x)"};
        Parser p(use);
        return {p.parseAll(), ""};
    } catch (const std::exception& e) {
        return {nullptr, e.what()};
    }
}

// ---------------------------------------------------------------- tree utilities

bool dependsOn(const NodeP& n, const std::string& v) {
    switch (n->op) {
        case Op::Num: case Op::Const: return false;
        case Op::Var: return n->name == v;
        case Op::Sum: case Op::Prod:
            if (dependsOn(n->kids[0], v) || dependsOn(n->kids[1], v)) return true;
            return n->name != v && dependsOn(n->kids[2], v);
        default:
            for (auto& k : n->kids) if (dependsOn(k, v)) return true;
            return false;
    }
}

static void fv(const NodeP& n, std::vector<std::string>& bound, std::set<std::string>& out) {
    if (n->op == Op::Var) {
        if (std::find(bound.begin(), bound.end(), n->name) == bound.end()) out.insert(n->name);
        return;
    }
    if (n->op == Op::Sum || n->op == Op::Prod) {
        fv(n->kids[0], bound, out);
        fv(n->kids[1], bound, out);
        bound.push_back(n->name);
        fv(n->kids[2], bound, out);
        bound.pop_back();
        return;
    }
    for (auto& k : n->kids) fv(k, bound, out);
}
void freeVars(const NodeP& n, std::set<std::string>& out) {
    std::vector<std::string> b;
    if (n) fv(n, b, out);
}
void sumBoundNames(const NodeP& n, std::set<std::string>& out) {
    if (!n) return;
    if (n->op == Op::Sum || n->op == Op::Prod) {
        freeVars(n->kids[0], out);
        freeVars(n->kids[1], out);
    }
    for (auto& k : n->kids) sumBoundNames(k, out);
}

NodeP renameVar(const NodeP& n, const std::string& from, const std::string& to) {
    if (n->op == Op::Var) return n->name == from ? var(to) : n;
    if (n->kids.empty()) return n;
    std::vector<NodeP> k;
    bool shadow = (n->op == Op::Sum || n->op == Op::Prod) && n->name == from;
    for (size_t i = 0; i < n->kids.size(); ++i)
        k.push_back(shadow && i == 2 ? n->kids[i] : renameVar(n->kids[i], from, to));
    return withKids(n, std::move(k));
}

static NodeP markImpl(const NodeP& n, bool inside) {
    if (n->kids.empty()) return n;
    bool isSum = n->op == Op::Sum || n->op == Op::Prod;
    std::vector<NodeP> k;
    for (auto& c : n->kids) k.push_back(markImpl(c, inside || isSum));
    auto m = std::make_shared<Node>(*n);
    m->kids = std::move(k);
    if (isSum && !inside) m->animated = true;
    return m;
}
NodeP markAnimatedSums(const NodeP& n) { return markImpl(n, false); }

void collectAnimatedSums(const NodeP& n, std::vector<const Node*>& out) {
    if (!n) return;
    if ((n->op == Op::Sum || n->op == Op::Prod) && n->animated) out.push_back(n.get());
    for (auto& k : n->kids) collectAnimatedSums(k, out);
}

std::string toString(const NodeP& n) {
    if (!n) return "";
    auto k = [&](int i) { return toString(n->kids[i]); };
    switch (n->op) {
        case Op::Num: {
            char b[64];
            if (n->val.imag() == 0) snprintf(b, sizeof b, "%g", n->val.real());
            else snprintf(b, sizeof b, "(%g%+gi)", n->val.real(), n->val.imag());
            return b;
        }
        case Op::Const: case Op::Var: return n->name;
        case Op::Add: return "(" + k(0) + " + " + k(1) + ")";
        case Op::Sub: return "(" + k(0) + " - " + k(1) + ")";
        case Op::Mul: return "(" + k(0) + "*" + k(1) + ")";
        case Op::Div: return "(" + k(0) + "/" + k(1) + ")";
        case Op::Pow: return "(" + k(0) + "^" + k(1) + ")";
        case Op::Neg: return "-" + k(0);
        case Op::Func:
            return std::string(fnDisplayName(n->fn)) + "(" + k(0) + (n->kids.size() > 1 ? ", " + k(1) : std::string()) + ")";
        case Op::Sum: case Op::Prod:
            return std::string(n->op == Op::Sum ? "sum(" : "prod(") + n->name + ", " + k(0) + ", " + k(1) + ", " + k(2) + ")";
        case Op::Fact: return k(0) + "!";
        case Op::NumDeriv: return "d/d" + n->name + "[" + k(0) + "]";
        case Op::Call: {
            std::string s = n->name + "(";
            for (size_t i = 0; i < n->kids.size(); ++i) s += (i ? ", " : "") + k((int)i);
            return s + ")";
        }
    }
    return "?";
}

// ---------------------------------------------------------------- simplify

static bool isN(const NodeP& n) { return n->op == Op::Num; }
static bool isNV(const NodeP& n, double v) { return n->op == Op::Num && n->val == cplx(v, 0); }
static bool isNice(double v) { return std::fabs(v * 1000.0 - std::round(v * 1000.0)) < 1e-9; }

static bool same(const NodeP& a, const NodeP& b) {
    if (a == b) return true;
    if (a->op != b->op || a->name != b->name || a->kids.size() != b->kids.size()) return false;
    if (a->op == Op::Func && a->fn != b->fn) return false;
    if ((a->op == Op::Num || a->op == Op::Const) && a->val != b->val) return false;
    for (size_t i = 0; i < a->kids.size(); ++i)
        if (!same(a->kids[i], b->kids[i])) return false;
    return true;
}

static NodeP simp1(const NodeP& n) {
    if (n->kids.empty()) return n;
    std::vector<NodeP> k;
    for (auto& c : n->kids) k.push_back(simp1(c));
    const NodeP& a = k[0];
    NodeP b = k.size() > 1 ? k[1] : nullptr;
    switch (n->op) {
        case Op::Add:
            if (isN(a) && isN(b)) return num(a->val + b->val);
            if (isNV(a, 0)) return b;
            if (isNV(b, 0)) return a;
            if (b->op == Op::Neg) return bin(Op::Sub, a, b->kids[0]);
            if (isN(b) && b->val.imag() == 0 && b->val.real() < 0) return bin(Op::Sub, a, num(-b->val));
            if (a->op == Op::Neg) return bin(Op::Sub, b, a->kids[0]);
            if (same(a, b)) return bin(Op::Mul, num(2), a);
            if (isN(b) && a->op == Op::Add && isN(a->kids[1])) return bin(Op::Add, a->kids[0], num(a->kids[1]->val + b->val));
            break;
        case Op::Sub:
            if (isN(a) && isN(b)) return num(a->val - b->val);
            if (isNV(b, 0)) return a;
            if (isNV(a, 0)) return neg(b);
            if (b->op == Op::Neg) return bin(Op::Add, a, b->kids[0]);
            if (same(a, b)) return num(0);
            if (isN(b) && a->op == Op::Add && isN(a->kids[1])) {
                cplx c = a->kids[1]->val - b->val;
                return c == 0.0 ? a->kids[0] : bin(Op::Add, a->kids[0], num(c));
            }
            if (isN(b) && a->op == Op::Sub && isN(a->kids[1])) return bin(Op::Sub, a->kids[0], num(a->kids[1]->val + b->val));
            break;
        case Op::Mul:
            if (isN(a) && isN(b)) return num(a->val * b->val);
            if (isNV(a, 0) || isNV(b, 0)) return num(0);
            if (isNV(a, 1)) return b;
            if (isNV(b, 1)) return a;
            if (isNV(a, -1)) return neg(b);
            if (isNV(b, -1)) return neg(a);
            if (isN(b) && !isN(a)) return bin(Op::Mul, b, a);
            if (isN(a) && b->op == Op::Mul && isN(b->kids[0])) return bin(Op::Mul, num(a->val * b->kids[0]->val), b->kids[1]);
            if (a->op == Op::Neg) return neg(bin(Op::Mul, a->kids[0], b));
            if (b->op == Op::Neg) return neg(bin(Op::Mul, a, b->kids[0]));
            if (b->op == Op::Div && isNV(b->kids[0], 1)) return bin(Op::Div, a, b->kids[1]);
            if (a->op == Op::Div && isNV(a->kids[0], 1)) return bin(Op::Div, b, a->kids[1]);
            if (same(a, b)) return bin(Op::Pow, a, num(2));
            if (b->op == Op::Pow && same(a, b->kids[0]) && isN(b->kids[1])) return bin(Op::Pow, a, num(b->kids[1]->val + 1.0));
            if (a->op == Op::Pow && same(b, a->kids[0]) && isN(a->kids[1])) return bin(Op::Pow, b, num(a->kids[1]->val + 1.0));
            if (!isN(a) && b->op == Op::Mul && isN(b->kids[0])) return bin(Op::Mul, b->kids[0], bin(Op::Mul, a, b->kids[1]));
            break;
        case Op::Div:
            if (isN(a) && isN(b) && b->val != 0.0) {
                cplx q = a->val / b->val;
                if (q.imag() == 0 && isNice(q.real())) return num(q);
            }
            if (isNV(a, 0)) return num(0);
            if (isNV(b, 1)) return a;
            if (a->op == Op::Neg) return neg(bin(Op::Div, a->kids[0], b));
            if (b->op == Op::Neg) return neg(bin(Op::Div, a, b->kids[0]));
            if (same(a, b)) return num(1);
            if (a->op == Op::Div) return bin(Op::Div, a->kids[0], bin(Op::Mul, a->kids[1], b));
            break;
        case Op::Pow:
            if (isN(a) && isN(b) && a->val.imag() == 0 && b->val.imag() == 0) {
                double e = b->val.real(), r = std::pow(a->val.real(), e);
                if (e == std::floor(e) && std::fabs(r) < 1e9 && std::isfinite(r)) return num(r);
            }
            if (isNV(b, 0)) return num(1);
            if (isNV(b, 1)) return a;
            if (isNV(a, 1)) return num(1);
            if (a->op == Op::Pow && isN(a->kids[1]) && isN(b)) return bin(Op::Pow, a->kids[0], num(a->kids[1]->val * b->val));
            break;
        case Op::Neg:
            if (isN(a)) return num(-a->val);
            if (a->op == Op::Neg) return a->kids[0];
            break;
        case Op::Sum:
            if (isNV(k[2], 0)) return num(0);
            break;
        default: break;
    }
    return withKids(n, std::move(k));
}

NodeP simplify(NodeP n) {
    if (!n) return n;
    for (int i = 0; i < 16; ++i) {
        NodeP m = simp1(n);
        if (same(m, n)) return m;
        n = m;
    }
    return n;
}

// ---------------------------------------------------------------- derivative

static NodeP D(const NodeP& n, const std::string& v);

static NodeP numericD(const NodeP& n, const std::string& v) {
    auto d = newNode(Op::NumDeriv);
    d->name = v;
    d->kids = {n};
    return d;
}

static NodeP fnPrime(Fn f, const NodeP& u) {
    NodeP one = num(1), two = num(2);
    switch (f) {
        case Fn::Sin: return func(Fn::Cos, u);
        case Fn::Cos: return neg(func(Fn::Sin, u));
        case Fn::Tan: return bin(Op::Div, one, bin(Op::Pow, func(Fn::Cos, u), two));
        case Fn::Sec: return bin(Op::Mul, func(Fn::Sec, u), func(Fn::Tan, u));
        case Fn::Csc: return neg(bin(Op::Mul, func(Fn::Csc, u), func(Fn::Cot, u)));
        case Fn::Cot: return neg(bin(Op::Div, one, bin(Op::Pow, func(Fn::Sin, u), two)));
        case Fn::Asin: return bin(Op::Div, one, func(Fn::Sqrt, bin(Op::Sub, one, bin(Op::Pow, u, two))));
        case Fn::Acos: return neg(bin(Op::Div, one, func(Fn::Sqrt, bin(Op::Sub, one, bin(Op::Pow, u, two)))));
        case Fn::Atan: return bin(Op::Div, one, bin(Op::Add, one, bin(Op::Pow, u, two)));
        case Fn::Sinh: return func(Fn::Cosh, u);
        case Fn::Cosh: return func(Fn::Sinh, u);
        case Fn::Tanh: return bin(Op::Div, one, bin(Op::Pow, func(Fn::Cosh, u), two));
        case Fn::Exp: return func(Fn::Exp, u);
        case Fn::Ln: return bin(Op::Div, one, u);
        case Fn::Log: return bin(Op::Div, one, bin(Op::Mul, u, func(Fn::Ln, num(10))));
        case Fn::Sqrt: return bin(Op::Div, one, bin(Op::Mul, two, func(Fn::Sqrt, u)));
        case Fn::Abs: return func(Fn::Sign, u);
        case Fn::Floor: case Fn::Ceil: case Fn::Round: case Fn::Sign: return num(0);
        case Fn::IsPrime: case Fn::Mobius: case Fn::ZetaZero: case Fn::PrimePi: return num(0);
        case Fn::Ei: return bin(Op::Div, func(Fn::Exp, u), u);
        case Fn::Erf: return bin(Op::Mul, bin(Op::Div, two, func(Fn::Sqrt, cnst("pi"))), func(Fn::Exp, neg(bin(Op::Pow, u, two))));
        case Fn::Erfc: return neg(bin(Op::Mul, bin(Op::Div, two, func(Fn::Sqrt, cnst("pi"))), func(Fn::Exp, neg(bin(Op::Pow, u, two)))));
        case Fn::Li: return bin(Op::Div, one, func(Fn::Ln, u));
        default: return nullptr;
    }
}

static NodeP D(const NodeP& n, const std::string& v) {
    if (!dependsOn(n, v)) return num(0);
    const auto& k = n->kids;
    switch (n->op) {
        case Op::Var: return num(1);
        case Op::Add: return bin(Op::Add, D(k[0], v), D(k[1], v));
        case Op::Sub: return bin(Op::Sub, D(k[0], v), D(k[1], v));
        case Op::Neg: return neg(D(k[0], v));
        case Op::Mul:
            return bin(Op::Add, bin(Op::Mul, D(k[0], v), k[1]), bin(Op::Mul, k[0], D(k[1], v)));
        case Op::Div:
            if (!dependsOn(k[1], v)) return bin(Op::Div, D(k[0], v), k[1]);
            if (!dependsOn(k[0], v))
                return neg(bin(Op::Div, bin(Op::Mul, k[0], D(k[1], v)), bin(Op::Pow, k[1], num(2))));
            return bin(Op::Div, bin(Op::Sub, bin(Op::Mul, D(k[0], v), k[1]), bin(Op::Mul, k[0], D(k[1], v))),
                       bin(Op::Pow, k[1], num(2)));
        case Op::Pow: {
            const NodeP &a = k[0], &b = k[1];
            if (!dependsOn(b, v))
                return bin(Op::Mul, bin(Op::Mul, b, bin(Op::Pow, a, bin(Op::Sub, b, num(1)))), D(a, v));
            if (!dependsOn(a, v)) {
                if (a->op == Op::Const && a->name == "e") return bin(Op::Mul, n, D(b, v));
                return bin(Op::Mul, bin(Op::Mul, n, func(Fn::Ln, a)), D(b, v));
            }
            return bin(Op::Mul, n, bin(Op::Add, bin(Op::Mul, D(b, v), func(Fn::Ln, a)),
                                       bin(Op::Div, bin(Op::Mul, b, D(a, v)), a)));
        }
        case Op::Func: {
            if (isBinaryFn(n->fn)) return num(0);  // integer-valued step functions
            NodeP fp = fnPrime(n->fn, k[0]);
            if (!fp) return numericD(n, v);
            return bin(Op::Mul, fp, D(k[0], v));
        }
        case Op::Sum: {
            if (dependsOn(k[0], v) || dependsOn(k[1], v)) return numericD(n, v);
            return withKids(n, {k[0], k[1], D(k[2], v)});
        }
        default: return numericD(n, v);
    }
}

NodeP derivative(const NodeP& n, const std::string& v) {
    if (!n) return nullptr;
    return simplify(D(n, v));
}

// ---------------------------------------------------------------- evaluation

Program::Program(const NodeP& root) {
    if (root) root_ = compile(root);
}

int Program::slotFor(const std::string& name) {
    for (size_t i = 0; i < names_.size(); ++i)
        if (names_[i] == name) return (int)i;
    names_.push_back(name);
    return (int)names_.size() - 1;
}

int Program::compile(const NodeP& n) {
    int idx = (int)code_.size();
    code_.emplace_back();
    Inst in;
    in.op = n->op;
    in.fn = n->fn;
    in.val = n->val;
    in.animated = n->animated;
    switch (n->op) {
        case Op::Var: in.slot = slotFor(n->name); break;
        case Op::Sum: case Op::Prod:
            in.slot = slotFor(n->name);
            in.a = compile(n->kids[0]);
            in.b = compile(n->kids[1]);
            in.c = compile(n->kids[2]);
            {
                const NodeP& body = n->kids[2];
                in.primeCount = n->op == Op::Sum && body->op == Op::Func && body->fn == Fn::IsPrime &&
                                body->kids[0]->op == Op::Var && body->kids[0]->name == n->name;
            }
            break;
        case Op::NumDeriv:
            in.slot = slotFor(n->name);
            in.a = compile(n->kids[0]);
            break;
        default:
            if (n->kids.size() > 0) in.a = compile(n->kids[0]);
            if (n->kids.size() > 1) in.b = compile(n->kids[1]);
            break;
    }
    code_[idx] = in;
    return idx;
}

struct Program::Ctx {
    cplx* s;
    double lim;
    bool real;
    int maxTerms;
    int animTerms;
};

// Index range of a sum. Unilateral: lo, lo+1, ... (or hi, hi-1, ... when only lo
// is -inf). Bilateral (-inf..inf): 0, 1, -1, 2, -2, ...
struct Program::Range {
    double lo, hi;
    bool loInf, hiInf;
    double kAt(long j) const {
        if (!loInf) return lo + (double)j;
        if (!hiInf) return hi - (double)j;
        if (j == 0) return 0;
        return (j & 1) ? (double)((j + 1) / 2) : -(double)(j / 2);
    }
};

namespace {

inline bool finiteC(cplx w) { return std::isfinite(w.real()) && std::isfinite(w.imag()); }

inline cplx cmul(cplx a, cplx b) {
    return {a.real() * b.real() - a.imag() * b.imag(), a.real() * b.imag() + a.imag() * b.real()};
}
inline cplx cdiv(cplx a, cplx b) {
    if (b.imag() == 0) return {a.real() / b.real(), a.imag() == 0 ? (b.real() == 0 ? NAN : 0.0) : a.imag() / b.real()};
    double d = b.real() * b.real() + b.imag() * b.imag();
    return {(a.real() * b.real() + a.imag() * b.imag()) / d, (a.imag() * b.real() - a.real() * b.imag()) / d};
}
cplx ipow(cplx b, long n) {
    bool inv = n < 0;
    unsigned long m = inv ? (unsigned long)(-n) : (unsigned long)n;
    cplx r(1, 0);
    while (m) {
        if (m & 1) r = cmul(r, b);
        m >>= 1;
        if (m) b = cmul(b, b);
    }
    return inv ? cdiv(cplx(1, 0), r) : r;
}

cplx gammaC(cplx z) {
    if (z.imag() == 0) {
        double x = z.real();
        if (x > 0 && x == std::floor(x) && x <= 171) {
            double r = 1;
            for (int k = 2; k < (int)x; ++k) r *= k;
            return r;
        }
        if (x <= 0 && x == std::floor(x)) return NaNc;
        return std::tgamma(x);
    }
    static const double p[] = {0.99999999999980993, 676.5203681218851, -1259.1392167224028,
                               771.32342877765313, -176.61502916214059, 12.507343278686905,
                               -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7};
    if (z.real() < 0.5) return PI / (std::sin(PI * z) * gammaC(1.0 - z));
    z -= 1.0;
    cplx x = p[0];
    for (int i = 1; i < 9; ++i) x += p[i] / (z + (double)i);
    cplx t = z + 7.5;
    return std::sqrt(2 * PI) * std::pow(t, z + 0.5) * std::exp(-t) * x;
}

// Riemann zeta. Borwein's accelerated alternating series (algorithm 2) for
// Re(s) >= 0, and the functional equation
//   zeta(s) = 2^s pi^(s-1) sin(pi s / 2) Gamma(1 - s) zeta(1 - s)
// for Re(s) < 0. The number of terms grows with |Im s| to keep the error tiny.
// w_k = (d_k - d_n) / d_n for Borwein's weights, computed in log space so that
// n can go far beyond where d_n itself would overflow a double.
const std::vector<double>& borweinW(int n) {
    static const int kMax = 1000;
    static const std::vector<std::vector<double>> tables = [] {
        std::vector<std::vector<double>> t(kMax + 1);
        for (int m = 16; m <= kMax; ++m) {
            std::vector<double> lt(m + 1);
            lt[0] = -std::log((double)m);
            double mx = lt[0];
            for (int i = 0; i < m; ++i) {
                lt[i + 1] = lt[i] + std::log(4.0 * (m + i) * (m - i)) - std::log((2.0 * i + 1) * (2.0 * i + 2));
                mx = std::max(mx, lt[i + 1]);
            }
            std::vector<double> pre(m + 1);
            double acc = 0;
            for (int i = 0; i <= m; ++i) { acc += std::exp(lt[i] - mx); pre[i] = acc; }
            std::vector<double> w(m + 1);
            for (int k = 0; k <= m; ++k) w[k] = (pre[m] - pre[k]) / pre[m] * -1.0;
            t[m] = std::move(w);
        }
        return t;
    }();
    return tables[std::clamp(n, 16, kMax)];
}

cplx zetaC(cplx s) {
    if (!finiteC(s)) return NaNc;
    if (s == cplx(1, 0)) return cplx(INFINITY, 0);
    if (s.real() < 0) {
        cplx r = 1.0 - s;
        return std::exp(s * std::log(2.0)) * std::exp((s - 1.0) * std::log(PI)) * std::sin(PI * s / 2.0) *
               gammaC(r) * zetaC(r);
    }
    const int n = std::clamp((int)(22 + 1.3 * std::fabs(s.imag())), 22, 1000);
    const std::vector<double>& w = borweinW(n);
    static const std::vector<double> logs = [] {
        std::vector<double> v(1002);
        for (size_t k = 0; k < v.size(); ++k) v[k] = std::log((double)k + 1);
        return v;
    }();
    cplx sum(0);
    for (int k = 0; k < n; ++k) {
        if (w[k] == 0) break;
        double lk = logs[k];
        double m = std::exp(-s.real() * lk), ph = -s.imag() * lk;
        cplx term = w[k] * cplx(m * std::cos(ph), m * std::sin(ph));
        sum += (k & 1) ? -term : term;
    }
    return -sum / (1.0 - std::exp((1.0 - s) * std::log(2.0)));
}

// ---- number theory

// Deterministic Miller-Rabin for 64-bit integers (these bases are exact below 2^64).
static uint64_t mulmod(uint64_t a, uint64_t b, uint64_t m) { return (unsigned __int128)a * b % m; }
static uint64_t powmod(uint64_t a, uint64_t e, uint64_t m) {
    uint64_t r = 1;
    a %= m;
    while (e) {
        if (e & 1) r = mulmod(r, a, m);
        a = mulmod(a, a, m);
        e >>= 1;
    }
    return r;
}
bool isPrimeU64(uint64_t n) {
    if (n < 2) return false;
    for (uint64_t p : {2ull, 3ull, 5ull, 7ull, 11ull, 13ull, 17ull, 19ull, 23ull, 29ull, 31ull, 37ull}) {
        if (n % p == 0) return n == p;
    }
    uint64_t d = n - 1;
    int sCount = 0;
    while ((d & 1) == 0) { d >>= 1; ++sCount; }
    for (uint64_t a : {2ull, 325ull, 9375ull, 28178ull, 450775ull, 9780504ull, 1795265022ull}) {
        uint64_t x = powmod(a % n, d, n);
        if (x == 0 || x == 1 || x == n - 1) continue;
        bool composite = true;
        for (int r = 1; r < sCount; ++r) {
            x = mulmod(x, x, n);
            if (x == n - 1) { composite = false; break; }
        }
        if (composite) return false;
    }
    return true;
}
bool isPrimeInt(double v) {
    if (!(v >= 2) || v != std::floor(v) || v > 9007199254740992.0) return false;  // exact integers only (2^53)
    return isPrimeU64((uint64_t)v);
}

double mobiusInt(double v) {
    if (!(v >= 1) || v != std::floor(v) || v > 9e15) return NAN;
    long long n = (long long)v;
    int sign = 1;
    for (long long p = 2; p * p <= n; ++p) {
        if (n % p) continue;
        n /= p;
        if (n % p == 0) return 0;
        sign = -sign;
    }
    if (n > 1) sign = -sign;
    return sign;
}

// ---- exponential integral Ei(z) and logarithmic integral li(z) = Ei(ln z)

// Hot path of the Riemann demo, so it avoids std::complex division and abs (which go
// through the slow NaN-safe __divdc3 and hypot) in favour of plain arithmetic.
cplx eiC(cplx z) {
    const double zr = z.real(), zi = z.imag();
    if (zr == 0 && zi == 0) return cplx(-INFINITY, 0);
    const double EULER = 0.57721566490153286;
    const double n2 = zr * zr + zi * zi;
    const bool realAxis = zi == 0;
    if (n2 <= 256 || (realAxis && zr > 0)) {  // power series
        double sr = 0, si = 0, tr = 1, ti = 0;
        for (int k = 1; k < 4000; ++k) {
            const double inv = 1.0 / k;
            const double nr = (tr * zr - ti * zi) * inv, ni = (tr * zi + ti * zr) * inv;
            tr = nr; ti = ni;
            const double ar = tr * inv, ai = ti * inv;
            sr += ar; si += ai;
            if (k > 2 && ar * ar + ai * ai < 1e-34 * (sr * sr + si * si)) break;
        }
        cplx lg = realAxis ? cplx(std::log(std::fabs(zr)), 0) : std::log(z);
        return EULER + lg + cplx(sr, si);
    }
    // continued fraction for E1(w), w = -z (modified Lentz)
    const double wr = -zr, wi = -zi;
    auto inv = [](double a, double b, double& r, double& i) {  // 1 / (a + ib)
        const double d = 1.0 / (a * a + b * b);
        r = a * d; i = -b * d;
    };
    double br = wr + 1, bi = wi, cr = 1e300, ci = 0, dr, di;
    inv(br, bi, dr, di);
    double hr = dr, hi = di;
    for (int i = 1; i < 2000; ++i) {
        const double an = -(double)i * i;
        br += 2.0;
        inv(an * dr + br, an * di + bi, dr, di);  // d = 1 / (an d + b)
        double icr, ici;
        inv(cr, ci, icr, ici);
        cr = br + an * icr; ci = bi + an * ici;    // c = b + an / c
        const double er = cr * dr - ci * di, ei = cr * di + ci * dr;  // del = c d
        const double nhr = hr * er - hi * ei, nhi = hr * ei + hi * er;
        hr = nhr; hi = nhi;
        const double xr = er - 1, xi = ei;
        if (xr * xr + xi * xi < 1e-30) break;
    }
    const double m = std::exp(-wr), ph = -wi;  // exp(-w)
    const double exr = m * std::cos(ph), exi = m * std::sin(ph);
    const double e1r = hr * exr - hi * exi, e1i = hr * exi + hi * exr;
    const double jump = realAxis ? 0.0 : (zi > 0 ? PI : -PI);
    return cplx(-e1r, -e1i + jump);
}

// ---- nontrivial zeros of zeta via the Riemann-Siegel Z function

double rsTheta(double t) {
    return t / 2 * std::log(t / (2 * PI)) - t / 2 - PI / 8 + 1 / (48 * t) + 7 / (5760 * t * t * t);
}
double rsZ(double t) {
    cplx z = zetaC(cplx(0.5, t));
    double th = rsTheta(t);
    return (cplx(std::cos(th), std::sin(th)) * z).real();
}

const std::vector<double>& zeroTable() {
    static const std::vector<double> zeros = [] {
        std::vector<double> out;
        const double maxT = 700, step = 0.05;
        double a = 10, za = rsZ(a);
        for (double b = a + step; b <= maxT; b += step) {
            double zb = rsZ(b);
            if (za * zb < 0) {  // Illinois (modified regula falsi)
                double lo = a, hi = b, flo = za, fhi = zb, x = lo;
                int side = 0;
                for (int it = 0; it < 60 && hi - lo > 1e-13; ++it) {
                    x = (lo * fhi - hi * flo) / (fhi - flo);
                    double fx = rsZ(x);
                    if (fx * fhi > 0) { hi = x; fhi = fx; if (side == -1) flo /= 2; side = -1; }
                    else if (fx * flo > 0) { lo = x; flo = fx; if (side == 1) fhi /= 2; side = 1; }
                    else break;
                }
                out.push_back(x);
            }
            a = b;
            za = zb;
        }
        return out;
    }();
    return zeros;
}

cplx cpow(cplx a, cplx b, bool realMode) {
    if (b.imag() == 0) {
        double e = b.real();
        if (e == std::floor(e) && std::fabs(e) <= 1024) return ipow(a, (long)e);
        if (a.imag() == 0) {
            double x = a.real();
            if (x > 0) return std::pow(x, e);
            if (x == 0) return e > 0 ? 0.0 : INFINITY;
            if (realMode) {
                for (int q = 3; q <= 15; q += 2) {
                    double p = e * q, rp = std::round(p);
                    if (std::fabs(p - rp) < 1e-9) {
                        double r = std::pow(-x, e);
                        return ((long)rp % 2 != 0) ? -r : r;
                    }
                }
                return NaNc;
            }
        }
    }
    if (a == cplx(0, 0)) return b.real() > 0 ? cplx(0, 0) : NaNc;
    if (a.imag() == 0 && a.real() > 0) {  // n^z, the workhorse of Dirichlet series
        double la = std::log(a.real()), m = std::exp(b.real() * la), ph = b.imag() * la;
        return {m * std::cos(ph), m * std::sin(ph)};
    }
    if (a.imag() == 0) a = cplx(a.real(), 0.0);
    return std::exp(cmul(b, std::log(a)));
}

cplx cfunc(Fn f, cplx a) {
    if (a.imag() == 0) a = cplx(a.real(), 0.0);  // drop signed zero so branch cuts behave
    const bool r = a.imag() == 0;
    const double x = a.real();
    switch (f) {
        case Fn::Sin: return r ? cplx(std::sin(x)) : std::sin(a);
        case Fn::Cos: return r ? cplx(std::cos(x)) : std::cos(a);
        case Fn::Tan: return r ? cplx(std::tan(x)) : std::tan(a);
        case Fn::Sec: return r ? cplx(1.0 / std::cos(x)) : cdiv(1.0, std::cos(a));
        case Fn::Csc: return r ? cplx(1.0 / std::sin(x)) : cdiv(1.0, std::sin(a));
        case Fn::Cot: return r ? cplx(std::cos(x) / std::sin(x)) : cdiv(std::cos(a), std::sin(a));
        case Fn::Asin: return (r && std::fabs(x) <= 1) ? cplx(std::asin(x)) : std::asin(a);
        case Fn::Acos: return (r && std::fabs(x) <= 1) ? cplx(std::acos(x)) : std::acos(a);
        case Fn::Atan: return r ? cplx(std::atan(x)) : std::atan(a);
        case Fn::Sinh: return r ? cplx(std::sinh(x)) : std::sinh(a);
        case Fn::Cosh: return r ? cplx(std::cosh(x)) : std::cosh(a);
        case Fn::Tanh: return r ? cplx(std::tanh(x)) : std::tanh(a);
        case Fn::Exp: return r ? cplx(std::exp(x)) : std::exp(a);
        case Fn::Ln:
            if (r && x > 0) return std::log(x);
            if (a == 0.0) return cplx(-INFINITY, 0);
            return std::log(a);
        case Fn::Log:
            if (r && x > 0) return std::log10(x);
            if (a == 0.0) return cplx(-INFINITY, 0);
            return std::log(a) / std::log(10.0);
        case Fn::Sqrt: return (r && x >= 0) ? cplx(std::sqrt(x)) : std::sqrt(a);
        case Fn::Abs: return std::abs(a);
        case Fn::Re: return a.real();
        case Fn::Im: return a.imag();
        case Fn::Conj: return std::conj(a);
        case Fn::Arg: return std::arg(a);
        case Fn::Floor: return cplx(std::floor(a.real()), std::floor(a.imag()));
        case Fn::Ceil: return cplx(std::ceil(a.real()), std::ceil(a.imag()));
        case Fn::Round: return cplx(std::round(a.real()), std::round(a.imag()));
        case Fn::Sign:
            if (r) return x > 0 ? 1.0 : (x < 0 ? -1.0 : 0.0);
            return a / std::abs(a);
        case Fn::Gamma: return gammaC(a);
        case Fn::Zeta: return zetaC(a);
        case Fn::Legendre: case Fn::Mod: return NaNc;  // two-argument; handled in Program::ev
        case Fn::IsPrime: return isPrimeInt(std::round(x * 1e9) / 1e9) ? 1.0 : 0.0;
        case Fn::Mobius: return mobiusInt(std::round(x * 1e9) / 1e9);
        case Fn::ZetaZero: {
            double k = std::round(x);
            if (std::fabs(x - k) > 1e-9 || k == 0) return NaNc;
            const auto& z = zeroTable();
            long i = (long)std::fabs(k) - 1;
            if (i >= (long)z.size()) return NaNc;
            return k > 0 ? z[i] : -z[i];
        }
        case Fn::Ei: return eiC(a);
        case Fn::PrimePi: return primePi(x);
        case Fn::Erf: return r ? cplx(std::erf(x)) : NaNc;   // real arguments only
        case Fn::Erfc: return r ? cplx(std::erfc(x)) : NaNc;
        case Fn::Li:
            if (a == 0.0) return 0.0;
            return eiC(r && x > 0 ? cplx(std::log(x), 0) : std::log(a));
    }
    return NaNc;
}

}  // namespace

cplx Program::partialSum(const Inst& in, Ctx& c, const Range& r, double terms, double count) const {
    const bool isSum = in.op == Op::Sum;
    long nfull = (long)std::min(count, std::floor(terms));
    double frac = terms < count ? terms - std::floor(terms) : 0.0;
    cplx acc = isSum ? cplx(0) : cplx(1);
    cplx saved = c.s[in.slot];
    for (long j = 0; j < nfull; ++j) {
        c.s[in.slot] = r.kAt(j);
        cplx t = ev(in.c, c);
        acc = isSum ? acc + t : cmul(acc, t);
    }
    if (frac > 1e-9 && (double)nfull < count) {
        c.s[in.slot] = r.kAt(nfull);
        cplx t = ev(in.c, c);
        acc = isSum ? acc + frac * t : cmul(acc, 1.0 + frac * (t - 1.0));
    }
    c.s[in.slot] = saved;
    return acc;
}

// Converge an infinite series (or product). Partial sums are sampled at doubling
// checkpoints N, 2N, 4N...; successive differences D give a ratio q = D/D_prev,
// and the tail is extrapolated geometrically, E = S + D q/(1 - q). That is exact
// for geometric tails and captures the N^(1-p) tails of p-series and the steady
// rotation of Dirichlet series like sum 1/n^s. Returns NaN when the series
// diverges or does not converge within maxTerms.
cplx Program::infiniteSum(const Inst& in, Ctx& c, const Range& r) const {
    const bool isSum = in.op == Op::Sum;
    const bool bilateral = r.loInf && r.hiInf;
    const long cap = std::max(64, c.maxTerms);
    cplx saved = c.s[in.slot];
    cplx S = isSum ? cplx(0) : cplx(1), Sprev, Dprev, E0, E1;
    bool haveS = false, haveD = false, haveE = false, haveE0 = false;
    int slow = 0;
    long checkpoint = bilateral ? 17 : 16;
    cplx t(0);
    cplx result = NaNc;
    bool done = false;
    for (long j = 0; j < cap && !done; ++j) {
        c.s[in.slot] = r.kAt(j);
        t = ev(in.c, c);
        S = isSum ? S + t : cmul(S, t);
        if (!finiteC(S)) { done = true; break; }
        if (j + 1 != checkpoint) continue;
        checkpoint = bilateral ? 2 * (checkpoint - 1) + 1 : 2 * checkpoint;
        if (haveS) {
            cplx D = S - Sprev;
            double scale = 1 + std::abs(S);
            double tmag = std::abs(isSum ? t : t - 1.0);
            if (std::abs(D) <= 1e-13 * scale && tmag <= 1e-10 * scale) { result = S; done = true; break; }
            if (haveD && std::abs(Dprev) > 0) {
                cplx q = D / Dprev;
                if (std::abs(q) < 0.95) {
                    cplx E = S + D * q / (1.0 - q);
                    if (haveE && std::abs(E - E1) <= 1e-7 * (1 + std::abs(E))) { result = E; done = true; break; }
                    E0 = E1; haveE0 = haveE;
                    E1 = E; haveE = true;
                    slow = 0;
                } else {
                    haveE = haveE0 = false;
                    if (std::abs(D) > 1e-3 * scale && ++slow >= 3) { done = true; break; }  // not shrinking: diverges
                }
            }
            Dprev = D;
            haveD = true;
        }
        Sprev = S;
        haveS = true;
    }
    if (!done) {  // ran out of terms: accept a loose answer if it has clearly settled
        double tmag = std::abs(isSum ? t : t - 1.0);
        if (haveE && haveE0 && std::abs(E1 - E0) <= 1e-3 * (1 + std::abs(E1))) result = E1;
        else if (haveD && std::abs(Dprev) <= 2e-3 * (1 + std::abs(S)) && tmag <= 1e-2 * (1 + std::abs(S))) result = S;
    }
    c.s[in.slot] = saved;
    return result;
}

cplx Program::ev(int i, Ctx& c) const {
    const Inst& in = code_[i];
    switch (in.op) {
        case Op::Num: case Op::Const: return in.val;
        case Op::Var: return c.s[in.slot];
        case Op::Add: return ev(in.a, c) + ev(in.b, c);
        case Op::Sub: return ev(in.a, c) - ev(in.b, c);
        case Op::Mul: {
            const cplx a = ev(in.a, c);
            if (a.real() == 0 && a.imag() == 0) return cplx(0, 0);  // 0 * anything: skip the work
            return cmul(a, ev(in.b, c));
        }
        case Op::Div: return cdiv(ev(in.a, c), ev(in.b, c));
        case Op::Pow: return cpow(ev(in.a, c), ev(in.b, c), c.real);
        case Op::Neg: return -ev(in.a, c);
        case Op::Func:
            if (in.b >= 0) {  // two-argument built-ins
                double a = std::round(ev(in.a, c).real()), m = std::round(ev(in.b, c).real());
                if (!std::isfinite(a) || !std::isfinite(m) || m == 0 || std::fabs(a) > 9e15 || std::fabs(m) > 4e9) return NaNc;
                long long ai = (long long)a, mi = (long long)std::fabs(m);
                long long r = ((ai % mi) + mi) % mi;
                if (in.fn == Fn::Mod) return (double)r;
                // Legendre symbol (a / p): 0, 1 or -1 (p = 2: parity)
                if (mi == 2) return (double)(r & 1);
                if (r == 0) return 0.0;
                return powmod((uint64_t)r, (uint64_t)(mi - 1) / 2, (uint64_t)mi) == 1 ? 1.0 : -1.0;
            }
            return cfunc(in.fn, ev(in.a, c));
        case Op::Fact: {
            cplx a = ev(in.a, c);
            if (a.imag() == 0 && a.real() >= 0 && a.real() == std::floor(a.real()) && a.real() <= 170) {
                double r = 1;
                for (int k = 2; k <= (int)a.real(); ++k) r *= k;
                return r;
            }
            return gammaC(a + 1.0);
        }
        case Op::Sum: case Op::Prod: {
            const cplx ident = in.op == Op::Sum ? cplx(0) : cplx(1);
            double lo = ev(in.a, c).real(), hi = ev(in.b, c).real();
            if (std::isnan(lo) || std::isnan(hi)) return NaNc;
            Range r{lo, hi, std::isinf(lo), std::isinf(hi)};
            if ((r.loInf && lo > 0) || (r.hiInf && hi < 0)) return ident;
            if (!r.loInf) r.lo = std::ceil(lo - 1e-9);
            if (!r.hiInf) r.hi = std::floor(hi + 1e-9);
            const double lim = in.animated ? std::max(0.0, c.lim) : INFINITY;
            if (!r.loInf && !r.hiInf) {
                double full = r.hi - r.lo + 1;
                if (full <= 0) return ident;
                if (in.primeCount && lim >= full) {  // counting primes: pi(hi) - pi(lo - 1)
                    double a = primePi(r.hi), b = primePi(r.lo - 1);
                    if (std::isfinite(a) && std::isfinite(b)) return a - b;
                }
                double count = std::min(full, 1e7);
                return partialSum(in, c, r, std::min(lim, count), count);
            }
            const double cap = std::max(1, c.animTerms);
            if (lim <= cap) return partialSum(in, c, r, lim, 1e18);
            cplx full = infiniteSum(in, c, r);
            if (lim >= cap + 1) return full;
            cplx part = partialSum(in, c, r, cap, 1e18);
            double f = lim - cap;
            if (!finiteC(full)) return f < 0.5 ? part : NaNc;  // diverges here: drop out halfway
            return part + f * (full - part);
        }
        case Op::Call: return NaNc;
        case Op::NumDeriv: {
            cplx x0 = c.s[in.slot];
            double h = 1e-5 * std::max(1.0, std::abs(x0));
            c.s[in.slot] = x0 + h;
            cplx fp = ev(in.a, c);
            c.s[in.slot] = x0 - h;
            cplx fm = ev(in.a, c);
            c.s[in.slot] = x0;
            return (fp - fm) / (2 * h);
        }
    }
    return NaNc;
}

cplx Program::eval(cplx* slots, double sumLimit, bool realMode, int maxTerms, int animTerms) const {
    if (root_ < 0) return NaNc;
    Ctx c{slots, sumLimit, realMode, maxTerms, animTerms};
    return ev(root_, c);
}

}  // namespace eq

// ---------------------------------------------------------------- definitions & calls

namespace eq {

DefHeader parseHeader(const std::string& src) {
    DefHeader h;
    h.body = src;
    size_t eqPos = std::string::npos;
    int depth = 0;
    for (size_t i = 0; i < src.size(); ++i) {
        char c = src[i];
        if (c == '(' || c == '[' || c == '{') ++depth;
        else if (c == ')' || c == ']' || c == '}') --depth;
        else if (c == '=' && depth == 0) { eqPos = i; break; }
    }
    if (eqPos == std::string::npos) return h;
    std::string lhs = src.substr(0, eqPos);
    size_t i = 0;
    auto ws = [&] { while (i < lhs.size() && std::isspace((unsigned char)lhs[i])) ++i; };
    auto ident = [&]() -> std::string {
        size_t s = i;
        if (i < lhs.size() && std::isalpha((unsigned char)lhs[i])) {
            ++i;
            while (i < lhs.size() && (std::isalnum((unsigned char)lhs[i]) || lhs[i] == '_')) ++i;
        }
        return lhs.substr(s, i - s);
    };
    ws();
    std::string name = ident();
    if (name.empty()) return h;
    ws();
    std::vector<std::string> params;
    if (i < lhs.size() && lhs[i] == '(') {
        ++i;
        for (;;) {
            ws();
            std::string p = ident();
            if (p.empty()) return h;
            params.push_back(p);
            ws();
            if (i < lhs.size() && lhs[i] == ',') { ++i; continue; }
            if (i < lhs.size() && lhs[i] == ')') { ++i; break; }
            return h;
        }
        ws();
    }
    if (i != lhs.size()) return h;  // not a plain header (e.g. "x^2 = y"); let the parser decide
    h.body = src.substr(eqPos + 1);
    if (params.empty() && (name == "y" || name == "z")) return h;
    h.named = true;
    h.name = name;
    h.params = params.empty() ? std::vector<std::string>{"x"} : params;
    return h;
}

static bool containsFree(const NodeP& n, const std::string& v) { return dependsOn(n, v); }

NodeP substitute(const NodeP& n, const std::map<std::string, NodeP>& repl) {
    if (repl.empty()) return n;
    if (n->op == Op::Var) {
        auto it = repl.find(n->name);
        return it != repl.end() ? it->second : n;
    }
    if (n->kids.empty()) return n;
    if (n->op == Op::Sum || n->op == Op::Prod) {
        NodeP lo = substitute(n->kids[0], repl), hi = substitute(n->kids[1], repl);
        std::map<std::string, NodeP> inner = repl;
        inner.erase(n->name);
        std::string idx = n->name;
        NodeP body = n->kids[2];
        bool capture = false;
        for (auto& kv : inner)
            if (containsFree(kv.second, idx) && dependsOn(body, kv.first)) capture = true;
        if (capture) {  // rename the index so substituted expressions keep their meaning
            std::string fresh = idx + "'";
            while (true) {
                bool clash = false;
                for (auto& kv : inner) clash |= containsFree(kv.second, fresh);
                if (!clash) break;
                fresh += "'";
            }
            body = renameVar(body, idx, fresh);
            idx = fresh;
        }
        auto m = std::make_shared<Node>(*n);
        m->name = idx;
        m->kids = {lo, hi, substitute(body, inner)};
        return m;
    }
    std::vector<NodeP> k;
    for (auto& c : n->kids) k.push_back(substitute(c, repl));
    return withKids(n, std::move(k));
}

static NodeP expandImpl(const NodeP& n, const FnTable& t, std::vector<std::string>& stack, std::string& err) {
    if (!err.empty()) return nullptr;
    std::vector<NodeP> k;
    for (auto& c : n->kids) {
        NodeP e = expandImpl(c, t, stack, err);
        if (!e) return nullptr;
        k.push_back(e);
    }
    if (n->op != Op::Call) return n->kids.empty() ? n : withKids(n, std::move(k));
    auto it = t.defs.find(n->name);
    if (it == t.defs.end() || !it->second.body) { err = "Unknown function '" + n->name + "'"; return nullptr; }
    const auto& def = it->second;
    if (def.params.size() != k.size()) {
        err = n->name + " takes " + std::to_string(def.params.size()) + " argument" +
              (def.params.size() == 1 ? "" : "s") + ", got " + std::to_string(k.size());
        return nullptr;
    }
    if (std::find(stack.begin(), stack.end(), n->name) != stack.end()) {
        err = "Circular definition: " + n->name + " uses itself";
        return nullptr;
    }
    stack.push_back(n->name);
    NodeP body = expandImpl(def.body, t, stack, err);
    stack.pop_back();
    if (!body) return nullptr;
    std::map<std::string, NodeP> repl;
    for (size_t i = 0; i < k.size(); ++i) repl[def.params[i]] = k[i];
    return substitute(body, repl);
}

NodeP expandCalls(const NodeP& n, const FnTable& t, std::string& err) {
    err.clear();
    if (!n) return nullptr;
    std::vector<std::string> stack;
    return expandImpl(n, t, stack, err);
}

}  // namespace eq

namespace eq {
double zetaZero(int k) {
    const auto& z = zeroTable();
    return (k >= 1 && k <= (int)z.size()) ? z[k - 1] : NAN;
}
int zetaZeroCount() { return (int)zeroTable().size(); }
void warmZetaZeros() { (void)zeroTable(); }
}  // namespace eq

// ---------------------------------------------------------------- prime counting

#include <mutex>
#include <thread>
#include <unordered_map>

namespace eq {
namespace {

// Odd-only segmented sieve. Segment k covers [k*S, (k+1)*S); bit j <-> k*S + 2j + 1.
constexpr uint64_t kSeg = 1u << 18;
constexpr uint64_t kWords = kSeg / 2 / 64;
constexpr double kPiLimit = 2e9;

const std::vector<uint32_t>& basePrimes() {  // odd primes up to sqrt(kPiLimit)
    static const std::vector<uint32_t> ps = [] {
        const uint32_t lim = 44800;
        std::vector<char> comp(lim + 1, 0);
        std::vector<uint32_t> v;
        for (uint32_t i = 3; i <= lim; i += 2) {
            if (comp[i]) continue;
            v.push_back(i);
            for (uint64_t j = (uint64_t)i * i; j <= lim; j += 2 * i) comp[j] = 1;
        }
        return v;
    }();
    return ps;
}

void sieveSegment(uint64_t k, std::vector<uint64_t>& bits) {
    bits.assign(kWords, ~0ull);
    const uint64_t lo = k * kSeg, hi = lo + kSeg;
    for (uint32_t p : basePrimes()) {
        uint64_t pp = (uint64_t)p * p;
        if (pp >= hi) break;
        uint64_t start = std::max(pp, (lo + p - 1) / p * p);
        if ((start & 1) == 0) start += p;  // odd multiples only
        for (uint64_t m = start; m < hi; m += 2 * p) {
            uint64_t j = (m - lo - 1) / 2;
            bits[j >> 6] &= ~(1ull << (j & 63));
        }
    }
    if (k == 0) bits[0] &= ~1ull;  // 1 is not prime
}

uint64_t popcountPrefix(const std::vector<uint64_t>& bits, uint64_t jmax) {  // bits [0, jmax]
    uint64_t c = 0, w = jmax >> 6;
    for (uint64_t i = 0; i < w; ++i) c += __builtin_popcountll(bits[i]);
    uint64_t rem = (jmax & 63) + 1;
    uint64_t mask = rem == 64 ? ~0ull : ((1ull << rem) - 1);
    return c + __builtin_popcountll(bits[w] & mask);
}

std::mutex g_piMutex;
std::vector<uint64_t> g_prefix = {0};  // g_prefix[k] = odd primes below k * kSeg
std::unordered_map<uint64_t, std::shared_ptr<const std::vector<uint64_t>>> g_segCache;

std::shared_ptr<const std::vector<uint64_t>> segment(uint64_t k) {
    {
        std::lock_guard<std::mutex> lk(g_piMutex);
        auto it = g_segCache.find(k);
        if (it != g_segCache.end()) return it->second;
    }
    auto bits = std::make_shared<std::vector<uint64_t>>();
    sieveSegment(k, *bits);
    std::lock_guard<std::mutex> lk(g_piMutex);
    if (g_segCache.size() > 512) g_segCache.clear();
    g_segCache[k] = bits;
    return bits;
}

void ensurePrefix(uint64_t K) {  // make g_prefix[K] available (counts every segment below K)
    std::lock_guard<std::mutex> lk(g_piMutex);
    uint64_t have = g_prefix.size() - 1;
    if (K <= have) return;
    std::vector<uint64_t> counts(K - have, 0);
    unsigned nt = std::max(1u, std::min(16u, std::thread::hardware_concurrency()));
    std::vector<std::thread> th;
    std::atomic<uint64_t> next{have};
    for (unsigned t = 0; t < nt; ++t)
        th.emplace_back([&] {
            std::vector<uint64_t> bits;
            for (;;) {
                uint64_t k = next++;
                if (k >= K) break;
                sieveSegment(k, bits);
                uint64_t c = 0;
                for (uint64_t w : bits) c += __builtin_popcountll(w);
                counts[k - have] = c;
            }
        });
    for (auto& t : th) t.join();
    for (uint64_t k = have; k < K; ++k) g_prefix.push_back(g_prefix.back() + counts[k - have]);
}

}  // namespace

double primePi(double x) {
    if (!(x >= 2)) return std::isnan(x) ? NAN : 0;
    if (x > kPiLimit) return NAN;
    uint64_t n = (uint64_t)std::floor(x + 1e-9);
    uint64_t k = n / kSeg;
    ensurePrefix(k);
    uint64_t base;
    {
        std::lock_guard<std::mutex> lk(g_piMutex);
        base = g_prefix[k];
    }
    uint64_t total = 1 + base;  // 1 for the prime 2
    uint64_t lo = k * kSeg;
    if (n >= lo + 1) total += popcountPrefix(*segment(k), (n - lo - 1) / 2);
    return (double)total;
}

}  // namespace eq

namespace eq {
const std::vector<double>& zetaBorweinWeights(int n) { return borweinW(n); }
}  // namespace eq

namespace eq {
bool isBuiltinFunction(const std::string& name) {
    Fn f;
    return lookupFn(name, f) || name == "sum" || name == "prod";
}
}  // namespace eq
