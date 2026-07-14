# goxvi-tsf: TSF text input processor COM DLL (thin shim over goxvi-core)
add_library(goxvi-tsf SHARED
  goxvi-tsf.def
  src/class-factory.cpp
  src/composition-manager.cpp
  src/config-file-watcher.cpp
  src/direct-text-injector.cpp
  src/display-attribute-provider.cpp
  src/dll-exports.cpp
  src/dll-main.cpp
  src/edit-session-callbacks.cpp
  src/goxvi-guids.cpp
  src/goxvi-text-service.cpp
  src/key-event-handler.cpp
  src/key-event-sink.cpp
  src/mouse-click-commit-hook.cpp
  src/tip-registration.cpp
)

# No msctf.lib: SDK 26100 dropped it; we only need CoCreateInstance + GUIDs
# (IIDs come from uuid.lib / the initguid TU in goxvi-guids.cpp).
# imm32: commit-before-click for IMM/CUAS hosts (mouse-click-commit-hook).
target_link_libraries(goxvi-tsf PRIVATE goxvi-core imm32)
