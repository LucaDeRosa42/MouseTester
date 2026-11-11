cmake_minimum_required(VERSION 3.20)
project(MouseTester LANGUAGES CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "MouseTester only supports Windows.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static CRT: the .exe runs on any Windows 10/11 without installing the Visual C++ runtime
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- Dear ImGui (downloaded automatically from GitHub on first configure) ---
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC d3d11 dxgi d3dcompiler)

# --- Applicazione ---
add_executable(MouseTester WIN32
    src/main.cpp
    src/raw_input.cpp
    src/analyzer.cpp
    src/mouse_view.cpp
    src/ui.cpp
    src/export.cpp
    src/i18n.cpp
    res/app.rc
)
target_include_directories(MouseTester PRIVATE res)
target_compile_definitions(MouseTester PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(MouseTester PRIVATE imgui hid dwmapi comdlg32)

if(MSVC)
    # /utf-8: UI strings contain non-ASCII characters such as "µ", "·" and "→"
    target_compile_options(MouseTester PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/utf-8 /W3>)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MouseTester)
endif()
