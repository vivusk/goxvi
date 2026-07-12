# goxvi-core: pure Telex engine, no Windows API in logic (unit-testable, fuzzable)
add_library(goxvi-core STATIC
  src/config-json-serializer.cpp
  src/shortcut-expander.cpp
  src/telex-engine.cpp
  src/telex-transform-rules.cpp
  src/tone-placement-rules.cpp
  src/utf8-utf16-conversion.cpp
  src/vietnamese-syllable-parser.cpp
  src/vni-transform-rules.cpp
)

target_include_directories(goxvi-core PUBLIC include)
# Vendored nlohmann/json (M1: root-level third_party, core may include it).
target_include_directories(goxvi-core PRIVATE ${CMAKE_SOURCE_DIR}/third_party)
