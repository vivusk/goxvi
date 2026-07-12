#include <gtest/gtest.h>

#include "goxvi/shortcut-expander.h"

namespace goxvi {
namespace {

std::vector<ShortcutEntry> sampleShortcuts() {
  return {{L"trc", L"trước"}, {L"ko", L"không"}, {L"vn", L"Việt Nam"}};
}

TEST(ShortcutExpanderTest, BasicMatch) {
  const auto r = matchShortcut(L"trc", sampleShortcuts());
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(*r, L"trước");
}

TEST(ShortcutExpanderTest, AnyUppercaseNeverMatches) {
  for (const wchar_t* w : {L"TRC", L"Trc", L"tRc", L"trC"}) {
    EXPECT_FALSE(matchShortcut(w, sampleShortcuts()).has_value()) << w;
  }
}

TEST(ShortcutExpanderTest, WholeWordOnly) {
  // Substring / superset of a trigger must not match (whole word only).
  EXPECT_FALSE(matchShortcut(L"tr", sampleShortcuts()).has_value());
  EXPECT_FALSE(matchShortcut(L"trcx", sampleShortcuts()).has_value());
}

TEST(ShortcutExpanderTest, EmptyWordNeverMatches) {
  EXPECT_FALSE(matchShortcut(L"", sampleShortcuts()).has_value());
}

TEST(ShortcutExpanderTest, UnknownWordNoMatch) {
  EXPECT_FALSE(matchShortcut(L"xyz", sampleShortcuts()).has_value());
}

TEST(ShortcutExpanderTest, EmptyListNeverMatches) {
  EXPECT_FALSE(matchShortcut(L"trc", {}).has_value());
}

TEST(ShortcutExpanderTest, NonLowercaseTriggerInConfigNeverMatches) {
  // A trigger that slipped in with non-a-z chars can never match, because a
  // valid rawWord is always pure lowercase a-z.
  const std::vector<ShortcutEntry> bad = {{L"TR", L"x"}, {L"a1", L"y"}, {L"", L"z"}};
  EXPECT_FALSE(matchShortcut(L"tr", bad).has_value());
  EXPECT_FALSE(matchShortcut(L"a1", bad).has_value());
}

TEST(ShortcutExpanderTest, DuplicateTriggerLastEntryWins) {
  const std::vector<ShortcutEntry> dup = {{L"a", L"first"}, {L"a", L"second"}};
  const auto r = matchShortcut(L"a", dup);
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(*r, L"second");
}

}  // namespace
}  // namespace goxvi
