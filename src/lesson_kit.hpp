// Helpers for writing lessons: setup, panes, overlays, test questions, and the standard
// sections of a course unit (overview, story, readings, proofs, written test).
#pragma once
#include <string>
#include <vector>

#include "app.hpp"
#include "theme.hpp"

namespace kit {

using Set = std::function<void(StepCtx&)>;
constexpr double PI = 3.14159265358979323846;

// A lesson project: these rows and sliders, pane 0 for the text and `graphs` panes beside it.
inline void setup(App& a, const char* name, std::vector<DemoRow> rows, std::vector<DemoParam> params, int graphs, bool stacked,
                  float noteFrac = 0.37f) {
    std::vector<DemoPane> panes(graphs + 1);
    for (auto& p : panes) p.show = {-1};
    a.loadDemo(name, "", rows, params, panes, 1, 1);
    a.lessonLayout(graphs, stacked, noteFrac);
}

inline TestQuestion typed(std::string text, std::string answer, int review, double tol = 0.01, Set set = {}) {
    TestQuestion q;
    q.text = std::move(text);
    q.answer = std::move(answer);
    q.review = review;
    q.tol = tol;
    q.set = std::move(set);
    return q;
}

inline TestQuestion choice(std::string text, std::vector<std::string> choices, int correct, int review, Set set = {}) {
    TestQuestion q;
    q.text = std::move(text);
    q.choices = std::move(choices);
    q.correct = correct;
    q.review = review;
    q.set = std::move(set);
    return q;
}

// Turn off every overlay and mode on a pane (a clean real-axes graph).
inline void clean(StepCtx& s, int pane) {
    KeyPane& p = s.pane(pane);
    p.complex = p.threeD = p.deriv = p.domainColor = p.fractal = false;
    p.unitCircle = p.matrix = false;
    p.trace.clear();
    p.shadeRow = -1;
    p.curveX.clear();
    p.curveDot.clear();
    p.fieldP.clear();
}

// A 2D graph of these rows (the first is the pane's main row).
inline void graph(StepCtx& s, int pane, std::vector<int> rows, double x0, double x1, double y0, double y1) {
    clean(s, pane);
    s.show(pane, rows);
    s.active(pane, rows.empty() ? 0 : rows[0]);
    s.fit(pane, x0, x1, y0, y1);
}

// A 3D surface of one row, over [-r, r]^2 around (cx, cy).
inline void surface(StepCtx& s, int pane, int row, double r, float zScale, bool complex = false, double cx = 0, double cy = 0) {
    clean(s, pane);
    KeyPane& p = s.pane(pane);
    p.threeD = true;
    p.complex = complex;
    s.show(pane, {row});
    s.active(pane, row);
    s.fit(pane, cx - r, cx + r, cy - r, cy + r);
    p.zScale = zScale;
}

inline void shade(StepCtx& s, int pane, int row, std::string lo, std::string hi, bool quiet = false) {
    KeyPane& p = s.pane(pane);
    p.shadeQuiet = quiet;
    p.shadeRow = row;
    p.shadeLo = std::move(lo);
    p.shadeHi = std::move(hi);
}

// A parametric curve (variable t); the pane is refit with equal axes so shapes are true.
inline void curve(StepCtx& s, int pane, std::string x, std::string y, std::string t0, std::string t1, std::string dot = "", bool equalAxes = true) {
    KeyPane& p = s.pane(pane);
    p.curveX = std::move(x);
    p.curveY = std::move(y);
    p.curveT0 = std::move(t0);
    p.curveT1 = std::move(t1);
    p.curveDot = std::move(dot);
    if (equalAxes) s.refit(pane, true);
}

// A vector field (or slope field when P = "1"); equal axes keep directions true.
inline void field(StepCtx& s, int pane, std::string P, std::string Q, int opts, double sx = 0, double sy = 1, bool equalAxes = true) {
    KeyPane& p = s.pane(pane);
    p.fieldP = std::move(P);
    p.fieldQ = std::move(Q);
    p.fieldOpts = opts;
    p.fieldSX = sx;
    p.fieldSY = sy;
    if (equalAxes) s.refit(pane, true);
}

// A step that only changes the text (the picture stays).
inline LessonStep text(std::string title, std::string body) { return {std::move(title), std::move(body), nullptr}; }

// ------------------------------------------------------------------ course units

// The plan's standard sections for a unit.
struct UnitText {
    std::string bigIdea, topics, weeks, people;
    std::string story;
    std::string read;      // bullet lines
    std::string prove;     // bullet lines
    int testMinutes = 120;
    std::string testName;  // "Unit 3 test"
    std::string test;      // numbered problems, one per paragraph
};

inline LessonStep overview(const UnitText& u, Set set) {
    return {"Overview",
            "**Big idea.** " + u.bigIdea + "\n\n**Topics.** " + u.topics + "\n\n**Time:** " + u.weeks + ". **People:** " + u.people + ".\n\n"
            "Work through the steps in order: the story, the ideas with live graphs, the readings, the proofs, then the written test. "
            "The quick check at the end tests the skills with fresh problems.",
            std::move(set)};
}

inline LessonStep story(const UnitText& u, Set set = nullptr) { return {"The story", u.story, std::move(set)}; }

inline LessonStep reading(const UnitText& u) {
    return text("Read",
                "Read the story first (30\xE2\x80\x93" "60 minutes), then the math with a pencil, reworking each example before its solution. "
                "Then work 30\xE2\x80\x93" "60 problems from the spine text and OpenStax: routine ones until they are automatic, then the hardest few.\n\n" +
                    u.read);
}

inline LessonStep proofs(const UnitText& u) {
    return text("Prove it yourself", "Write each of these closed-book until you can reproduce it cold.\n\n" + u.prove);
}

inline LessonStep writtenTest(const UnitText& u) {
    const std::string h = std::to_string(u.testMinutes / 60) + (u.testMinutes % 60 ? " h " + std::to_string(u.testMinutes % 60) + " min" : " h");
    return text(u.testName + " (written)",
                "**Closed book, no calculator unless a problem says so, one timed sitting of " + h + ".** "
                "Read it only when you are ready to sit it. Send your work, with every step shown, to Claude in your Calculus mastery chat. "
                "Each problem scores 0\xE2\x80\x93" "4; 85% moves you on, 75\xE2\x80\x93" "84% means a short retest.\n\n" + u.test);
}

constexpr const char* kTrack = "calculus-course";
constexpr const char* kStart = "Calculus course: start here";
constexpr const char* kPart1 = "Calculus course, Part I: Differential calculus";
constexpr const char* kPart2 = "Calculus course, Part II: Integral calculus";
constexpr const char* kPart3 = "Calculus course, Part III: Infinite processes";
constexpr const char* kPart4 = "Calculus course, Part IV: Multivariable and vector calculus";
constexpr const char* kPart5 = "Calculus course, Part V: Differential equations";

inline LessonDef unit(std::string id, const char* subject, std::string title, std::string blurb) {
    LessonDef L;
    L.id = std::move(id);
    L.subject = subject;
    L.title = std::move(title);
    L.blurb = std::move(blurb);
    L.track = kTrack;
    L.passPct = 85;
    return L;
}

}  // namespace kit

std::vector<LessonDef> calculusCourse();  // lessons_course*.cpp
std::vector<LessonDef> calculusCourse1();
std::vector<LessonDef> calculusCourse2();
std::vector<LessonDef> calculusCourse3();
std::vector<LessonDef> calculusCourse4();
