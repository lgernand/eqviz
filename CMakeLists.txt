cmake_minimum_required(VERSION 3.20)
project(eqviz LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
# Prefer a static GLFW so the app bundle runs without Homebrew installed.
find_library(GLFW_STATIC_LIB NAMES libglfw3.a PATHS /opt/homebrew/lib /usr/local/lib NO_DEFAULT_PATH)
if(APPLE AND GLFW_STATIC_LIB)
  add_library(glfw STATIC IMPORTED)
  set_target_properties(glfw PROPERTIES IMPORTED_LOCATION ${GLFW_STATIC_LIB})
  get_filename_component(_glfw_prefix ${GLFW_STATIC_LIB} DIRECTORY)
  target_include_directories(glfw INTERFACE ${_glfw_prefix}/../include)
  target_link_libraries(glfw INTERFACE "-framework Cocoa" "-framework IOKit" "-framework CoreVideo" "-framework QuartzCore")
  set(glfw3_FOUND TRUE)
else()
  find_package(glfw3 3.3 QUIET)
endif()
if(NOT glfw3_FOUND)
  # Fall back to fetching GLFW from source when it is not installed system-wide.
  include(FetchContent)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
  FetchContent_MakeAvailable(glfw)
endif()

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
add_library(imgui STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
# 32-bit ImWchar so the math font's italic alphabet (U+1D44E...) can be used.
target_compile_definitions(imgui PUBLIC IMGUI_USE_WCHAR32 IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

add_executable(eqviz
  src/main.cpp
  src/app.cpp
  src/panel.cpp
  src/scene2d.cpp
  src/scene3d.cpp
  src/project.cpp
  src/layout.cpp
  src/demos.cpp
  src/expr.cpp
  src/typeset.cpp)
if(APPLE)
  enable_language(OBJCXX)
  target_sources(eqviz PRIVATE src/platform_mac.mm)
  target_link_libraries(eqviz PRIVATE "-framework Cocoa")
else()
  target_sources(eqviz PRIVATE src/platform_stub.cpp)
endif()
target_link_libraries(eqviz PRIVATE imgui Threads::Threads)
if(APPLE)
  target_compile_definitions(eqviz PRIVATE GL_SILENCE_DEPRECATION)
  # macOS app bundle: build/eqviz.app (icon from packaging/eqviz.icns when present)
  set(EQVIZ_ICON ${CMAKE_SOURCE_DIR}/packaging/eqviz.icns)
  if(EXISTS ${EQVIZ_ICON})
    target_sources(eqviz PRIVATE ${EQVIZ_ICON})
    set_source_files_properties(${EQVIZ_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  endif()
  set_target_properties(eqviz PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/Info.plist.in)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(eqviz PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# Expression engine self-check: `cmake --build build --target expr_test && ./build/expr_test`
add_executable(expr_test tests/expr_test.cpp src/expr.cpp)
