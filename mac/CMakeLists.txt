# goxvi-mac: Goxvi.app, the InputMethodKit shim over goxvi-core (PDR D3, D4, D9).
# Installed into ~/Library/Input Methods/ by mac/scripts/install.sh.
enable_language(OBJCXX)

# VERSION (repo root) is the single source of truth for the version string.
file(READ "${CMAKE_SOURCE_DIR}/VERSION" GOXVI_VERSION)
string(STRIP "${GOXVI_VERSION}" GOXVI_VERSION)

# Menu icon is 16 pt (16x16 + 32x32 @2x in one TIFF). Regenerate with:
#   sips -z 16 16 settings/assets/goxvi-logo.png --out icon.png
#   sips -z 32 32 settings/assets/goxvi-logo.png --out icon@2x.png
#   tiffutil -cathidpicheck icon.png icon@2x.png -out mac/assets/goxvi-menu-icon.tiff
set(GOXVI_MENU_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/goxvi-menu-icon.tiff")
set_source_files_properties(${GOXVI_MENU_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

add_executable(goxvi-mac MACOSX_BUNDLE
  src/main.mm
  src/goxvi-input-controller.mm
  ${GOXVI_MENU_ICON}
)

set_target_properties(goxvi-mac PROPERTIES
  OUTPUT_NAME Goxvi
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
)

# -fobjc-arc: ARC for every .mm (no manual retain/release in the shim).
target_compile_options(goxvi-mac PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)

target_link_libraries(goxvi-mac PRIVATE
  goxvi-core
  "-framework Cocoa"
  "-framework InputMethodKit"
  "-framework Carbon"  # TIS* input source API (--install/--status/--uninstall)
)
