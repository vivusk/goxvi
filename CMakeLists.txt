cmake_minimum_required(VERSION 3.26)

# H2: static CRT for every target (core, tsf, gtest). The TSF DLL loads into
# arbitrary processes and must not depend on a redistributable VCRuntime;
# deciding linkage here keeps core/tsf/gtest consistent (avoids LNK2038).
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(goxvi CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(/W4 /utf-8 /EHsc)
add_compile_definitions(UNICODE _UNICODE)

enable_testing()

add_subdirectory(core)
add_subdirectory(tsf)
add_subdirectory(tests)
add_subdirectory(settings)  # native Goxvi.exe settings app (replaces WinUI 3)
