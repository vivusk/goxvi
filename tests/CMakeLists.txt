include(FetchContent)

# Pinned googletest release; H2: gtest must follow the global static CRT.
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(goxvi-core-tests
  acronym-no-restore-tests.cpp
  config-serializer-tests.cpp
  config-toggle-hotkey-tests.cpp
  foreign-word-tests.cpp
  shortcut-expander-tests.cpp
  syllable-parser-tests.cpp
  telex-engine-tests.cpp
  test-typo-autocorrect.cpp
  tone-placement-tests.cpp
  utf8-utf16-conversion-tests.cpp
  vni-engine-tests.cpp
)

target_link_libraries(goxvi-core-tests PRIVATE goxvi-core gtest_main)
# Parser/placement tests exercise core internals directly.
target_include_directories(goxvi-core-tests PRIVATE ${CMAKE_SOURCE_DIR}/core/src)

if(APPLE)
  # macOS shim logic (mac/src, plan K2) is pure C++ -> tested in the same binary.
  target_sources(goxvi-core-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/mac/tests/composition-session-config-tests.cpp
    ${CMAKE_SOURCE_DIR}/mac/tests/composition-session-tests.cpp
    ${CMAKE_SOURCE_DIR}/mac/tests/direct-insert-tracker-stale-tests.cpp
    ${CMAKE_SOURCE_DIR}/mac/tests/direct-insert-tracker-tests.cpp
    ${CMAKE_SOURCE_DIR}/mac/tests/insert-mode-policy-tests.cpp
    ${CMAKE_SOURCE_DIR}/mac/tests/letter-handoff-tests.cpp
    ${CMAKE_SOURCE_DIR}/mac/tests/key-classifier-tests.cpp
    ${CMAKE_SOURCE_DIR}/mac/tests/shortcut-validation-tests.cpp
    ${CMAKE_SOURCE_DIR}/mac/tests/text-edit-tests.cpp
    ${CMAKE_SOURCE_DIR}/mac/tests/toggle-hotkey-detector-tests.cpp
  )
  target_link_libraries(goxvi-core-tests PRIVATE goxvi-mac-logic)
endif()

include(GoogleTest)
# PRE_TEST: enumerate tests when ctest runs, not during the build. Avoids the
# build failing when AV scans a freshly-built exe (esp. x86 via WoW64) past the
# default 5s discovery timeout; also bumped to 30s for slow first runs.
gtest_discover_tests(goxvi-core-tests
  DISCOVERY_MODE PRE_TEST
  DISCOVERY_TIMEOUT 30
)
