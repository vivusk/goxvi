#include <gtest/gtest.h>

#include <string>

#include "fake-screen.h"
#include "utf8-utf16-conversion.h"

namespace goxvi_mac {
namespace {

using goxvi::EngineConfig;
using testing::FakeScreen;

EngineConfig withShortcuts() {
  EngineConfig cfg;
  cfg.shortcuts = {{L"trc", L"trước"}, {L"vn", L"Việt Nam"}};
  return cfg;
}

// Every command a client receives must insert text (delete-only is ignored).
void expectNoDeleteOnly(const FakeScreen& s) {
  for (const TextEdit& e : s.edits) EXPECT_FALSE(e.insert.empty());
}

TEST(DirectInsertTrackerTest, TypingBuildsWordWithReplaceCommands) {
  FakeScreen s;
  s.text = L"ab ";
  s.caret = 3;
  s.type(L"vieetj");
  EXPECT_EQ(s.text, L"ab việt");
  EXPECT_EQ(s.caret, 7u);
  ASSERT_EQ(s.edits.size(), 5u);           // "v" typed by the app (TB-007)
  EXPECT_EQ(s.edits[0].deleteCount, 0u);  // pure append
  EXPECT_EQ(s.edits[0].insert, L"i");
  EXPECT_EQ(s.edits[2].deleteCount, 1u);  // e -> ê
  EXPECT_EQ(s.edits[2].insert, L"ê");
  EXPECT_TRUE(s.tracker.tracking());
  EXPECT_TRUE(s.lastEaten);
  expectNoDeleteOnly(s);
}

// TB-007: the app types a word's unchanged first letter itself (an IMK
// insertText right after a focus change can be dropped -> "Hello" -> "ello").
TEST(DirectInsertTrackerTest, FirstLetterOfWordPassesThroughToApp) {
  FakeScreen s;
  s.text = L"ab ";
  s.caret = 3;
  s.press(KeyKind::Letter, L'H');
  EXPECT_FALSE(s.lastEaten);
  EXPECT_TRUE(s.edits.empty());
  EXPECT_TRUE(s.tracker.tracking());
  s.type(L"ello");
  EXPECT_EQ(s.text, L"ab Hello");
  s.press(KeyKind::Terminator, L' ');
  s.type(L"ddi");  // "d" passes through, second "d" replaces it
  EXPECT_EQ(s.text, L"ab Hello đi");
  expectNoDeleteOnly(s);
}

TEST(DirectInsertTrackerTest, EnterAfterPlainWordSendsNoCommandAndPassesThrough) {
  FakeScreen s;
  s.type(L"vieetj");
  const std::size_t before = s.edits.size();
  s.press(KeyKind::Terminator, 0);  // Enter: no text, goes to the app (K11)
  EXPECT_EQ(s.edits.size(), before);
  EXPECT_FALSE(s.lastEaten);
  EXPECT_FALSE(s.tracker.tracking());
  EXPECT_EQ(s.text, L"việt");
}

TEST(DirectInsertTrackerTest, SpaceAfterWordIsTypedByTheApp) {
  FakeScreen s;
  s.type(L"laf");
  s.press(KeyKind::Terminator, L' ');
  s.type(L"ddi");
  EXPECT_EQ(s.text, L"là đi");
}

TEST(DirectInsertTrackerTest, ShortcutOnTerminatorReplacesTypedKeys) {
  FakeScreen s(withShortcuts());
  s.type(L"trc");
  s.press(KeyKind::Terminator, L' ');
  const TextEdit& last = s.edits.back();
  EXPECT_EQ(last.deleteCount, 1u);  // common prefix "tr" stays, "c" -> "ước"
  EXPECT_EQ(last.insert, L"ước");
  EXPECT_EQ(s.text, L"trước ");
  EXPECT_FALSE(s.tracker.tracking());
}

TEST(DirectInsertTrackerTest, AutoCorrectOnTerminatorRewritesWord) {
  FakeScreen s;
  s.type(L"twosi");
  s.press(KeyKind::Terminator, L' ');
  EXPECT_EQ(s.text, L"tới ");
  expectNoDeleteOnly(s);
}

TEST(DirectInsertTrackerTest, BackspaceDroppingLastCharPassesThroughToApp) {
  FakeScreen s;
  s.type(L"tieengs");
  s.press(KeyKind::Backspace);  // tiếng -> tiên: replace (tone dropped), eaten
  EXPECT_TRUE(s.lastEaten);
  EXPECT_EQ(s.text, L"tiên");
  const std::size_t before = s.edits.size();
  s.press(KeyKind::Backspace);  // tiên -> tiê: only the last char, the app deletes it
  EXPECT_FALSE(s.lastEaten);
  EXPECT_EQ(s.edits.size(), before);
  EXPECT_EQ(s.text, L"tiê");
  EXPECT_FALSE(s.tracker.isStale(s.caret));
  s.type(L"ng");
  EXPECT_EQ(s.text, L"tiêng");
  expectNoDeleteOnly(s);
}

TEST(DirectInsertTrackerTest, BackspaceMovingToneSendsReplaceAndIsEaten) {
  FakeScreen s;
  s.type(L"hoafn");
  EXPECT_EQ(s.text, L"hoàn");
  s.press(KeyKind::Backspace);  // tone moves back: hoàn -> hòa
  EXPECT_TRUE(s.lastEaten);
  EXPECT_EQ(s.text, L"hòa");
  const TextEdit& last = s.edits.back();
  EXPECT_EQ(last.deleteCount, 3u);
  EXPECT_EQ(last.insert, L"òa");
  expectNoDeleteOnly(s);
}

TEST(DirectInsertTrackerTest, ScreenAlwaysMatchesMarkedModeDisplay) {
  // Same keys through a plain session (marked mode) must show the same word.
  const std::wstring_view keys = L"thuowngf";
  FakeScreen s;
  CompositionSession reference;
  KeyOutcome shown;
  auto check = [&](KeyKind kind, wchar_t ch) {
    s.press(kind, ch);
    shown = reference.onKey(kind, ch);
    EXPECT_EQ(s.text, shown.markedText);
  };
  for (wchar_t c : keys) check(KeyKind::Letter, c);
  for (int i = 0; i < 3; ++i) check(KeyKind::Backspace, 0);
  for (wchar_t c : std::wstring_view(L"owngf")) check(KeyKind::Letter, c);
  EXPECT_EQ(s.text, L"thường");
  expectNoDeleteOnly(s);
}

TEST(DirectInsertTrackerTest, BackspaceToEmptyEndsTracking) {
  FakeScreen s;
  s.text = L"x ";
  s.caret = 2;
  s.type(L"ab");
  s.press(KeyKind::Backspace);
  s.press(KeyKind::Backspace);
  EXPECT_EQ(s.text, L"x ");
  EXPECT_FALSE(s.tracker.tracking());
  s.press(KeyKind::Backspace);  // idle: the app deletes its own text
  EXPECT_FALSE(s.lastEaten);
  EXPECT_EQ(s.text, L"x");
  expectNoDeleteOnly(s);
}

TEST(DirectInsertTrackerTest, EscapeRestoresRawKeys) {
  FakeScreen s;
  s.type(L"vieetj");
  s.press(KeyKind::Escape);
  EXPECT_TRUE(s.lastEaten);
  EXPECT_EQ(s.text, L"vieetj");
  EXPECT_FALSE(s.tracker.tracking());
}

TEST(DirectInsertTrackerTest, OverflowCommitsAndTracksNewWord) {
  FakeScreen s;
  const std::wstring full(goxvi::kMaxRawKeys, L'b');
  s.type(full);
  s.type(L"b");
  EXPECT_EQ(s.text, full + L"b");
  EXPECT_TRUE(s.tracker.tracking());
  EXPECT_FALSE(s.tracker.isStale(s.caret));
  s.press(KeyKind::Backspace);  // only the new word "b" is ours
  EXPECT_EQ(s.text, full);
  EXPECT_FALSE(s.tracker.tracking());
}

TEST(DirectInsertTrackerTest, VniDigitsCompose) {
  EngineConfig cfg;
  cfg.inputMethod = goxvi::InputMethod::Vni;
  FakeScreen s(cfg);
  s.type(L"a61");
  EXPECT_EQ(s.text, L"ấ");
  expectNoDeleteOnly(s);
}

TEST(DirectInsertTrackerTest, NonBmpShortcutExpansionReplacesWholeWord) {
  EngineConfig cfg;
  // K1: non-BMP only via utf8ToWide (surrogate pair), never in an L"" literal.
  const std::wstring smile = goxvi::detail::utf8ToWide("\xF0\x9F\x98\x80");
  ASSERT_EQ(smile.size(), 2u);
  cfg.shortcuts = {{L"cuoi", smile + L"!"}};
  FakeScreen s(cfg);
  s.type(L"cuoi");
  s.press(KeyKind::Terminator, L' ');
  EXPECT_EQ(s.text, smile + L"! ");
  EXPECT_EQ(s.caret, 4u);
}

TEST(DirectInsertTrackerTest, IdleKeysTouchNothing) {
  DirectInsertTracker t;
  const DirectStep step = t.onOutcome(KeyKind::Terminator, KeyOutcome{}, 5);
  EXPECT_FALSE(step.edit.has_value());
  EXPECT_FALSE(step.eaten);
  EXPECT_FALSE(t.tracking());
}

}  // namespace
}  // namespace goxvi_mac
