# goxvi-settings: native (no .NET) settings app -> Goxvi.exe. A thin Win32 GUI
# over goxvi-core's config serializer (schema stays single-sourced in core).
# Replaces the old ~160 MB self-contained WinUI 3 app with a ~1 MB static exe.

# Version numbers from the repo-root VERSION file (shared with installer/csproj).
file(READ "${CMAKE_SOURCE_DIR}/VERSION" _goxvi_version_raw)
string(STRIP "${_goxvi_version_raw}" GOXVI_VERSION_STR)
string(REPLACE "." ";" _goxvi_version_parts "${GOXVI_VERSION_STR}")
list(GET _goxvi_version_parts 0 GOXVI_VER_MAJOR)
list(GET _goxvi_version_parts 1 GOXVI_VER_MINOR)
list(GET _goxvi_version_parts 2 GOXVI_VER_PATCH)
configure_file(settings-version.h.in
               "${CMAKE_CURRENT_BINARY_DIR}/settings-version.h" @ONLY)

add_executable(goxvi-settings WIN32
  src/main.cpp
  src/settings-window.cpp
  src/overview-tab.cpp
  src/shortcuts-tab.cpp
  src/about-tab.cpp
  src/config-store.cpp
  src/app-container-acl.cpp
  src/activate-tip.cpp
  src/activate-tip-log.cpp
  src/default-input-override.cpp
  src/run-hidden-powershell.cpp
  src/language-switch-hotkey.cpp
  # DRY: dùng lại đúng TU initguid của TIP (định nghĩa CLSID/GUID_Profile của
  # Goxvi + toàn bộ GUID msctf) để --activate-tip gọi ActivateProfile, không khai
  # lại GUID. Đây là initguid TU DUY NHẤT trong exe này.
  "${CMAKE_SOURCE_DIR}/tsf/src/goxvi-guids.cpp"
  goxvi-settings.rc
)

set_target_properties(goxvi-settings PROPERTIES OUTPUT_NAME "Goxvi")

# Binary dir: generated settings-version.h. Source dir: so rc.exe resolves
# "assets\goxvi.ico" + "goxvi-settings.manifest" via its include path.
target_include_directories(goxvi-settings PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
  # goxvi-guids.h (CLSID/GUID của TIP) cho activate-tip.cpp + goxvi-guids.cpp.
  "${CMAKE_SOURCE_DIR}/tsf/src"
)

# core: reuse EngineConfig + JSON serializer (DRY schema contract).
# comctl32: tab control/ListView. advapi32: AppContainer DACL. shell32/ole32:
# SHGetKnownFolderPath + CoTaskMemFree.
target_link_libraries(goxvi-settings PRIVATE
  goxvi-core comctl32 advapi32 shell32 ole32 uxtheme)

# The .rc embeds the manifest itself; suppress the linker's default one.
target_link_options(goxvi-settings PRIVATE /MANIFEST:NO)
