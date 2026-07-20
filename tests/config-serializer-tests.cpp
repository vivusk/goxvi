#include <gtest/gtest.h>

#include "goxvi/config-json-serializer.h"

namespace goxvi {
namespace {

TEST(ConfigSerializerTest, ParsesValidConfig) {
  const EngineConfig c = parseConfigJson(
      R"({"version":1,"toneStyle":"new","inputMethod":"vni"})");
  EXPECT_EQ(c.toneStyle, ToneStyle::New);
  EXPECT_EQ(c.inputMethod, InputMethod::Vni);
}

TEST(ConfigSerializerTest, DefaultsOnEmptyOrBrokenJson) {
  for (const char* bad : {"", "not json {", "42", "[1,2]", "null"}) {
    const EngineConfig c = parseConfigJson(bad);
    EXPECT_EQ(c.toneStyle, ToneStyle::Old) << bad;
    EXPECT_EQ(c.inputMethod, InputMethod::Telex) << bad;
  }
}

TEST(ConfigSerializerTest, MissingFieldsFallBackPerField) {
  const EngineConfig c = parseConfigJson(R"({"version":1,"toneStyle":"new"})");
  EXPECT_EQ(c.toneStyle, ToneStyle::New);
  EXPECT_EQ(c.inputMethod, InputMethod::Telex);  // default
}

TEST(ConfigSerializerTest, IgnoresUnknownFieldsAndVersions) {
  const EngineConfig c = parseConfigJson(
      R"({"version":99,"toneStyle":"old","future":{"x":1}})");
  EXPECT_EQ(c.toneStyle, ToneStyle::Old);
}

TEST(ConfigSerializerTest, WrongTypesFallBack) {
  const EngineConfig c = parseConfigJson(R"({"toneStyle":123})");
  EXPECT_EQ(c.toneStyle, ToneStyle::Old);
}

TEST(ConfigSerializerTest, UnknownToneStyleStringKeepsDefault) {
  const EngineConfig c = parseConfigJson(R"({"toneStyle":"fancy"})");
  EXPECT_EQ(c.toneStyle, ToneStyle::Old);
}

TEST(ConfigSerializerTest, RoundTrip) {
  EngineConfig in;
  in.toneStyle = ToneStyle::New;
  in.inputMethod = InputMethod::Vni;
  const EngineConfig out = parseConfigJson(serializeConfigJson(in));
  EXPECT_EQ(out.toneStyle, in.toneStyle);
  EXPECT_EQ(out.inputMethod, in.inputMethod);
}

// ---- restoreOnInvalid (khôi phục phím khi gõ sai) ---------------------------

TEST(ConfigSerializerTest, RestoreOnInvalidDefaultsTrueWhenMissing) {
  const EngineConfig c = parseConfigJson(R"({"version":1})");
  EXPECT_TRUE(c.restoreOnInvalid);  // default: revert-to-raw like UniKey
}

TEST(ConfigSerializerTest, ParsesRestoreOnInvalidFalse) {
  const EngineConfig c = parseConfigJson(R"({"restoreOnInvalid":false})");
  EXPECT_FALSE(c.restoreOnInvalid);
}

TEST(ConfigSerializerTest, RestoreOnInvalidWrongTypeFallsBack) {
  const EngineConfig c = parseConfigJson(R"({"restoreOnInvalid":"no"})");
  EXPECT_TRUE(c.restoreOnInvalid);  // wrong type → default
}

TEST(ConfigSerializerTest, RestoreOnInvalidRoundTrip) {
  EngineConfig in;
  in.restoreOnInvalid = false;
  const EngineConfig out = parseConfigJson(serializeConfigJson(in));
  EXPECT_FALSE(out.restoreOnInvalid);
}

TEST(ConfigSerializerTest, SerializedShapeHasRestoreOnInvalid) {
  const std::string json = serializeConfigJson(EngineConfig{});
  EXPECT_NE(json.find("\"restoreOnInvalid\": true"), std::string::npos);
}

TEST(ConfigSerializerTest, SerializedShapeMatchesSchemaV1) {
  const std::string json = serializeConfigJson(EngineConfig{});
  EXPECT_NE(json.find("\"version\": 1"), std::string::npos);
  EXPECT_NE(json.find("\"toneStyle\": \"old\""), std::string::npos);
  EXPECT_NE(json.find("\"inputMethod\": \"telex\""), std::string::npos);
}

TEST(ConfigSerializerTest, ParsesVniInputMethod) {
  const EngineConfig c = parseConfigJson(R"({"inputMethod":"vni"})");
  EXPECT_EQ(c.inputMethod, InputMethod::Vni);
}

TEST(ConfigSerializerTest, UnknownInputMethodStringKeepsDefault) {
  const EngineConfig c = parseConfigJson(R"({"inputMethod":"fancy"})");
  EXPECT_EQ(c.inputMethod, InputMethod::Telex);
}

// ---- autoCorrectEnabled (tự sửa lỗi gõ dấu trước nguyên âm) -----------------

TEST(ConfigSerializerTest, AutoCorrectDefaultsTrueWhenMissing) {
  const EngineConfig c = parseConfigJson(R"({"version":1})");
  EXPECT_TRUE(c.autoCorrectEnabled);  // default on
}

TEST(ConfigSerializerTest, ParsesAutoCorrectFalse) {
  const EngineConfig c = parseConfigJson(R"({"autoCorrectEnabled":false})");
  EXPECT_FALSE(c.autoCorrectEnabled);
}

TEST(ConfigSerializerTest, AutoCorrectWrongTypeFallsBack) {
  const EngineConfig c = parseConfigJson(R"({"autoCorrectEnabled":"no"})");
  EXPECT_TRUE(c.autoCorrectEnabled);  // wrong type → default
}

TEST(ConfigSerializerTest, AutoCorrectRoundTrip) {
  EngineConfig in;
  in.autoCorrectEnabled = false;
  const EngineConfig out = parseConfigJson(serializeConfigJson(in));
  EXPECT_FALSE(out.autoCorrectEnabled);
}

TEST(ConfigSerializerTest, SerializedShapeHasAutoCorrect) {
  const std::string json = serializeConfigJson(EngineConfig{});
  EXPECT_NE(json.find("\"autoCorrectEnabled\": true"), std::string::npos);
}

// ---- gõ tắt (shortcuts) -----------------------------------------------------

TEST(ConfigSerializerTest, ParsesShortcutsWithAccentedExpansion) {
  // Accented expansion in UTF-8 source (/utf-8): verifies UTF-8 → UTF-16 parse.
  const EngineConfig c = parseConfigJson(
      R"({"shortcutsEnabled":true,
          "shortcuts":[{"trigger":"trc","expansion":"trước"},
                       {"trigger":"ko","expansion":"không"}]})");
  EXPECT_TRUE(c.shortcutsEnabled);
  ASSERT_EQ(c.shortcuts.size(), 2u);
  EXPECT_EQ(c.shortcuts[0].trigger, L"trc");
  EXPECT_EQ(c.shortcuts[0].expansion, L"trước");
  EXPECT_EQ(c.shortcuts[1].trigger, L"ko");
  EXPECT_EQ(c.shortcuts[1].expansion, L"không");
}

TEST(ConfigSerializerTest, ShortcutsDefaultWhenMissing) {
  const EngineConfig c = parseConfigJson(R"({"version":1})");
  EXPECT_TRUE(c.shortcutsEnabled);  // default true
  EXPECT_TRUE(c.shortcuts.empty());
}

TEST(ConfigSerializerTest, ShortcutsSkipBadEntriesNotWholeArray) {
  const EngineConfig c = parseConfigJson(
      R"({"shortcuts":[
            {"trigger":"ok","expansion":"okay"},
            {"trigger":"nope"},
            {"expansion":"orphan"},
            {"trigger":"","expansion":"empty"},
            {"trigger":123,"expansion":"badtype"},
            "not an object",
            {"trigger":"go","expansion":"golang"}
         ]})");
  ASSERT_EQ(c.shortcuts.size(), 2u);
  EXPECT_EQ(c.shortcuts[0].trigger, L"ok");
  EXPECT_EQ(c.shortcuts[1].trigger, L"go");
}

TEST(ConfigSerializerTest, ShortcutsSkipOversizedEntries) {
  // Trigger longer than kMaxRawKeys can never match; expansion longer than
  // kMaxShortcutExpansionChars is capped so hand-edited configs can't balloon
  // commit text. Both are skipped like any other bad entry.
  const std::string longTrigger(kMaxRawKeys + 1, 'a');
  const std::string longExpansion(kMaxShortcutExpansionChars + 1, 'x');
  const std::string maxExpansion(kMaxShortcutExpansionChars, 'x');
  const EngineConfig c = parseConfigJson(
      R"({"shortcuts":[
            {"trigger":")" + longTrigger + R"(","expansion":"ok"},
            {"trigger":"big","expansion":")" + longExpansion + R"("},
            {"trigger":"max","expansion":")" + maxExpansion + R"("},
            {"trigger":"go","expansion":"golang"}
         ]})");
  ASSERT_EQ(c.shortcuts.size(), 2u);
  EXPECT_EQ(c.shortcuts[0].trigger, L"max");  // exactly at the cap → kept
  EXPECT_EQ(c.shortcuts[1].trigger, L"go");
}

TEST(ConfigSerializerTest, ShortcutsWrongTypeFallsBack) {
  const EngineConfig c = parseConfigJson(
      R"({"shortcutsEnabled":"yes","shortcuts":"nope"})");
  EXPECT_TRUE(c.shortcutsEnabled);  // wrong type → default
  EXPECT_TRUE(c.shortcuts.empty());
}

TEST(ConfigSerializerTest, ShortcutsRoundTripPreservesOrderAndContent) {
  EngineConfig in;
  in.shortcutsEnabled = false;
  in.shortcuts = {{L"trc", L"trước"}, {L"dd", L"Đây"}, {L"vn", L"Việt Nam 👍"}};
  const EngineConfig out = parseConfigJson(serializeConfigJson(in));
  EXPECT_FALSE(out.shortcutsEnabled);
  ASSERT_EQ(out.shortcuts.size(), in.shortcuts.size());
  for (size_t i = 0; i < in.shortcuts.size(); ++i) {
    EXPECT_EQ(out.shortcuts[i].trigger, in.shortcuts[i].trigger) << i;
    EXPECT_EQ(out.shortcuts[i].expansion, in.shortcuts[i].expansion) << i;
  }
}

TEST(ConfigSerializerTest, SerializedShapeHasShortcutFields) {
  const std::string json = serializeConfigJson(EngineConfig{});
  EXPECT_NE(json.find("\"shortcutsEnabled\": true"), std::string::npos);
  EXPECT_NE(json.find("\"shortcuts\": []"), std::string::npos);
}

}  // namespace
}  // namespace goxvi
