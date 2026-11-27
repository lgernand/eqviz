// The calculus course: all parts, in order.
#include "lesson_kit.hpp"

std::vector<LessonDef> calculusCourse() {
    std::vector<LessonDef> v;
    for (auto* part : {&calculusCourse1, &calculusCourse2, &calculusCourse3, &calculusCourse4})
        for (LessonDef& L : part()) v.push_back(std::move(L));
    return v;
}
