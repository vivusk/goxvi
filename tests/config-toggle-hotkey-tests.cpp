// toggleHotkey field (macOS plan K4): stored verbatim by core, interpreted by
// the mac shim. Split from config-serializer-tests.cpp to keep files < ~200 lines.
#include <gtest/gtest.h>

#include "goxvi/config-json-serializer.h"

namespace goxvi {
namespace {

TEST(ConfigToggleHotkeyTest, DefaultsToCtrlShift) {
  EXPECT_EQ(EngineConfig{}.toggleHotkey, "ctrl+shift");
}

TEST(ConfigToggleHotkeyTest, OldJsonWithoutFieldKeepsDefault) {
  const EngineConfig c = parseConfigJson(
      R"({"version":1,"toneStyle":"new","inputMethod":"vni"})");
  EXPECT_EQ(c.toggleHotkey, "ctrl+shift");
  EXPECT_EQ(c.inputMethod, InputMethod::Vni);  // other fields unaffected
}

TEST(ConfigToggleHotkeyTest, WrongTypeOrEmptyKeepsDefault) {
  for (const char* json : {R"({"toggleHotkey":123})", R"({"toggleHotkey":true})",
                           R"({"toggleHotkey":null})", R"({"toggleHotkey":""})",
                           R"({"toggleHotkey":["ctrl"]})"}) {
    EXPECT_EQ(parseConfigJson(json).toggleHotkey, "ctrl+shift") << json;
  }
}

TEST(ConfigToggleHotkeyTest, StoresStringVerbatim) {
  // Unknown values are kept as-is: only the shim decides what is valid.
  EXPECT_EQ(parseConfigJson(R"({"toggleHotkey":"none"})").toggleHotkey, "none");
  EXPECT_EQ(parseConfigJson(R"({"toggleHotkey":"weird"})").toggleHotkey, "weird");
}

TEST(ConfigToggleHotkeyTest, SerializerAlwaysWritesField) {
  const std::string json = serializeConfigJson(EngineConfig{});
  EXPECT_NE(json.find("\"toggleHotkey\": \"ctrl+shift\""), std::string::npos);
}

TEST(ConfigToggleHotkeyTest, RoundTripKeepsHotkeyAndOtherFields) {
  for (const char* hotkey : {"ctrl+shift", "option+shift", "ctrl+option", "none"}) {
    EngineConfig in;
    in.toggleHotkey = hotkey;
    in.inputMethod = InputMethod::Vni;
    in.toneStyle = ToneStyle::New;
    in.shortcuts = {{L"vn", L"Việt Nam"}};
    const EngineConfig out = parseConfigJson(serializeConfigJson(in));
    EXPECT_EQ(out.toggleHotkey, in.toggleHotkey);
    EXPECT_EQ(out.inputMethod, in.inputMethod);
    EXPECT_EQ(out.toneStyle, in.toneStyle);
    ASSERT_EQ(out.shortcuts.size(), 1u);
    EXPECT_EQ(out.shortcuts[0].expansion, L"Việt Nam");
  }
}

}  // namespace
}  // namespace goxvi
