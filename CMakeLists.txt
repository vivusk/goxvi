cmake_minimum_required(VERSION 3.26)

# H2: static CRT for every target (core, tsf, gtest). The TSF DLL loads into
# arbitrary processes and must not depend on a redistributable VCRuntime;
# deciding linkage here keeps core/tsf/gtest consistent (avoids LNK2038).
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# macOS 13+ (PDR D3). Must precede project(); APPLE is only defined by
# project(), so test the host here (no cross-compiling to/from macOS).
if(CMAKE_HOST_APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0 CACHE STRING "Minimum macOS version")
endif()

project(goxvi CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_options(/W4 /utf-8 /EHsc)
  add_compile_definitions(UNICODE _UNICODE)
else()
  # clang/gcc: sources are UTF-8 and exceptions are on by default.
  add_compile_options(-Wall -Wextra)
endif()

enable_testing()

add_subdirectory(core)
if(WIN32)
  add_subdirectory(tsf)
endif()
if(APPLE)
  add_subdirectory(mac)  # Goxvi.app (InputMethodKit), needs goxvi-core above
endif()
add_subdirectory(tests)
if(WIN32)
  add_subdirectory(settings)  # native Goxvi.exe settings app (replaces WinUI 3)
endif()
