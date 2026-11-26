// Expression engine: parsing, symbolic differentiation, simplification and
// fast complex-valued evaluation.
#pragma once
#include <complex>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace eq {

using cplx = std::complex<double>;

enum class Op { Num, Const, Var, Add, Sub, Mul, Div, Pow, Neg, Func, Sum, Prod, Fact, NumDeriv, Call };

enum class Fn {
    Sin, Cos, Tan, Sec, Csc, Cot, Asin, Acos, Atan, Sinh, Cosh, Tanh,
    Exp, Ln, Log, Sqrt, Abs, Re, Im, Conj, Arg, Floor, Ceil, Round, Sign, Gamma, Zeta,
    IsPrime, Mobius, ZetaZero, Ei, Li, Erf, Erfc, PrimePi,
    Legendre, Mod  // two arguments
};

struct Node;
using NodeP = std::shared_ptr<const Node>;

struct Node {
    Op op = Op::Num;
    cplx val{};             // Num / Const value
    std::string name;       // Var name, Const symbol, Sum/Prod index, NumDeriv variable, Call target
    Fn fn = Fn::Sin;
    std::vector<NodeP> kids;  // Sum/Prod: {lo, hi, body}; NumDeriv: {body}
    bool animated = false;    // top-level Sum/Prod governed by the summation player
};

// --- construction helpers
NodeP num(cplx v);
NodeP cnst(const std::string& sym);  // "pi", "e", "i", "inf"
NodeP var(const std::string& name);
NodeP bin(Op op, NodeP a, NodeP b);
NodeP neg(NodeP a);
NodeP func(Fn f, NodeP a);

// --- parsing
struct ParseResult {
    NodeP root;
    std::string error;
};
// userFns: names of user-defined functions, parsed as calls name(a, b, ...)
ParseResult parse(const std::string& src, const std::set<std::string>* userFns = nullptr);

// A line in a project: "f(x) = ...", "L(u, v) = ...", "y = ..." or just "...".
struct DefHeader {
    bool named = false;              // false: "y = ...", "z = ..." or a bare expression
    std::string name;
    std::vector<std::string> params; // empty when unnamed
    std::string body;                // text right of '=' (or everything)
};
DefHeader parseHeader(const std::string& src);

// User-defined functions available for inlining: name -> (parameters, body).
struct FnTable {
    struct Def { std::vector<std::string> params; NodeP body; };
    std::map<std::string, Def> defs;
};
// Inline every Call (recursively). Returns nullptr and sets err on unknown
// functions, wrong argument counts or circular definitions.
NodeP expandCalls(const NodeP& n, const FnTable& table, std::string& err);
// Replace free variables (respecting sum-index scope, renaming to avoid capture).
NodeP substitute(const NodeP& n, const std::map<std::string, NodeP>& repl);
// Imaginary parts of the nontrivial zeros of zeta (computed once; 1-based k).
double zetaZero(int k);
int zetaZeroCount();
void warmZetaZeros();  // compute the table now (call from a background thread)

// --- tree utilities
NodeP simplify(NodeP n);
NodeP derivative(const NodeP& n, const std::string& var);
NodeP renameVar(const NodeP& n, const std::string& from, const std::string& to);
NodeP markAnimatedSums(const NodeP& n);
void collectAnimatedSums(const NodeP& n, std::vector<const Node*>& out);
void freeVars(const NodeP& n, std::set<std::string>& out);
void sumBoundNames(const NodeP& n, std::set<std::string>& out);  // free names used in sum bounds
bool dependsOn(const NodeP& n, const std::string& var);
std::string toString(const NodeP& n);
const char* fnDisplayName(Fn f);

// Greek letters, shared by lexer, typesetter and UI.
uint32_t greekCodepoint(const std::string& name);      // 0 when not a Greek name
const char* greekNameFor(uint32_t codepoint);          // nullptr when not Greek
std::string utf8(uint32_t cp);

// --- evaluation
struct Inst {
    Op op = Op::Num;
    Fn fn = Fn::Sin;
    int a = -1, b = -1, c = -1;
    int slot = -1;
    cplx val{};
    bool animated = false;
    bool primeCount = false;  // Sum whose body is isprime(index): evaluated as pi(hi) - pi(lo - 1)
};

// Number of primes <= x (exact, x up to 2e9; NaN beyond). Uses a cached segmented sieve.
double primePi(double x);
inline bool isBinaryFn(Fn f) { return f == Fn::Legendre || f == Fn::Mod; }

class Program {
public:
    Program() = default;
    explicit Program(const NodeP& root);
    bool valid() const { return root_ >= 0; }
    int numSlots() const { return (int)names_.size(); }
    const std::vector<std::string>& slotNames() const { return names_; }
    // slots: one value per slot name.
    // sumLimit: number of terms (fractional ok) included in animated sums. For an
    //   infinite series the first `animTerms` terms are partial sums and the step
    //   from animTerms to animTerms + 1 blends into the converged limit.
    // realMode: real-valued odd roots of negative numbers.
    // maxTerms: cap on terms when converging an infinite series.
    cplx eval(cplx* slots, double sumLimit, bool realMode, int maxTerms, int animTerms = 20) const;

private:
    struct Ctx;
    struct Range;
    int compile(const NodeP& n);
    cplx partialSum(const Inst& in, Ctx& c, const Range& r, double terms, double count) const;
    cplx infiniteSum(const Inst& in, Ctx& c, const Range& r) const;
    int slotFor(const std::string& name);
    cplx ev(int i, Ctx& c) const;
    std::vector<Inst> code_;
    std::vector<std::string> names_;
    int root_ = -1;
};

}  // namespace eq
