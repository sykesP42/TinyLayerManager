cmake_minimum_required(VERSION 3.16)

add_executable(tlmapp WIN32
    main.cpp
    UI.cpp
    WindowOperator.cpp
    WindowEnumerator.cpp
    WinEventMonitor.cpp
    Theme.cpp
    PresetManager.cpp
    PerWindowSettings.cpp
    IconTexture.cpp
    resources.rc
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_impl_win32.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_impl_dx9.cpp
)

target_include_directories(tlmapp PRIVATE ${CMAKE_SOURCE_DIR}/third_party/imgui)

if (WIN32)
    target_link_libraries(tlmapp PRIVATE d3d9 imm32 psapi)
    target_link_options(tlmapp PRIVATE -static-libgcc -static-libstdc++ -static -Wl,-s)
    target_compile_options(tlmapp PRIVATE -g -O0 -DIMGUI_DISABLE_DEBUG_TOOLS)
endif()