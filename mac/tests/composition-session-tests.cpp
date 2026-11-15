#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "composition-session.h"

namespace goxvi_mac {
namespace {

using goxvi::EngineConfig;

// Feeds letters; returns the outcome of the last key.
KeyOutcome typeLetters(CompositionSession& s, std::wstring_view keys) {
  KeyOutcome last;
  for (wchar_t c : keys) last = s.onKey(KeyKind::Letter, c);
  return last;
}

EngineConfig withShortcuts() {
  EngineConfig cfg;
  cfg.shortcuts = {{L"trc", L"trước"}, {L"vn", L"Việt Nam"}};
  return cfg;
}

TEST(CompositionSessionTest, LettersShowMarkedTextThenTerminatorCommits) {
  CompositionSession s;
  const KeyOutcome typed = typeLetters(s, L"vieetj");
  EXPECT_TRUE(typed.eaten);
  EXPECT_FALSE(typed.commitText.has_value());
  EXPECT_EQ(typed.markedText, L"việt");
  EXPECT_TRUE(typed.setsMarkedText());
  EXPECT_TRUE(s.composing());

  const KeyOutcome end = s.onKey(KeyKind::Terminator, L' ');
  EXPECT_FALSE(end.eaten);  // K6: the space goes on to the app
  ASSERT_TRUE(end.commitText.has_value());
  EXPECT_EQ(*end.commitText, L"việt");
  EXPECT_FALSE(end.setsMarkedText());  // insertText already consumed it
  EXPECT_FALSE(s.composing());
}

TEST(CompositionSessionTest, BackspaceStepsBackThenClearsMarkedText) {
  CompositionSession s;
  typeLetters(s, L"vieetj");
  KeyOutcome out = s.onKey(KeyKind::Backspace, 0);
  EXPECT_TRUE(out.eaten);
  EXPECT_EQ(out.markedText, L"viê");  // display recomputed from raw keys
  for (int i = 0; i < 2; ++i) out = s.onKey(KeyKind::Backspace, 0);
  EXPECT_EQ(out.markedText, L"v");
  out = s.onKey(KeyKind::Backspace, 0);
  EXPECT_TRUE(out.eaten);
  EXPECT_EQ(out.markedText, L"");
  EXPECT_TRUE(out.setsMarkedText());  // empty marked text = clear it
  EXPECT_FALSE(s.composing());
  // Next backspace is idle and belongs to the app.
  EXPECT_FALSE(s.onKey(KeyKind::Backspace, 0).eaten);
}

TEST(CompositionSessionTest, EscapeCommitsRawKeysAndIsEaten) {
  CompositionSession s(withShortcuts());
  typeLetters(s, L"vieetj");
  const KeyOutcome out = s.onKey(KeyKind::Escape, 0);
  EXPECT_TRUE(out.eaten);
  ASSERT_TRUE(out.commitText.has_value());
  EXPECT_EQ(*out.commitText, L"vieetj");
  EXPECT_FALSE(out.setsMarkedText());
  EXPECT_FALSE(s.composing());
}

TEST(CompositionSessionTest, EscapeNeverExpandsShortcut) {
  CompositionSession s(withShortcuts());
  typeLetters(s, L"trc");
  EXPECT_EQ(*s.onKey(KeyKind::Escape, 0).commitText, L"trc");
}

TEST(CompositionSessionTest, ChordCommitsWithoutEating) {
  CompositionSession s;
  typeLetters(s, L"laf");
  const KeyOutcome out = s.onKey(KeyKind::Chord, L'c');
  EXPECT_FALSE(out.eaten);
  EXPECT_EQ(*out.commitText, L"là");
}

TEST(CompositionSessionTest, ShortcutExpandsOnTerminatorAndChord) {
  CompositionSession s(withShortcuts());
  typeLetters(s, L"trc");
  EXPECT_EQ(*s.onKey(KeyKind::Terminator, L' ').commitText, L"trước");
  typeLetters(s, L"vn");
  EXPECT_EQ(*s.onKey(KeyKind::Chord, L'a').commitText, L"Việt Nam");
}

TEST(CompositionSessionTest, NonMatchingWordIsNotExpanded) {
  CompositionSession s(withShortcuts());
  typeLetters(s, L"trca");
  EXPECT_EQ(*s.onKey(KeyKind::Terminator, L' ').commitText, L"trca");
}

TEST(CompositionSessionTest, UppercaseRawNeverExpands) {
  CompositionSession s(withShortcuts());
  typeLetters(s, L"Trc");
  EXPECT_EQ(*s.onKey(KeyKind::Terminator, L' ').commitText, L"Trc");
}

TEST(CompositionSessionTest, ShortcutsDisabledNeverExpand) {
  EngineConfig cfg = withShortcuts();
  cfg.shortcutsEnabled = false;
  CompositionSession s(cfg);
  typeLetters(s, L"trc");
  EXPECT_EQ(*s.onKey(KeyKind::Terminator, L' ').commitText, L"trc");
}

TEST(CompositionSessionTest, AutoCorrectAppliesOnTerminator) {
  CompositionSession s;
  typeLetters(s, L"twosi");
  EXPECT_EQ(*s.onKey(KeyKind::Terminator, L' ').commitText, L"tới");
}

TEST(CompositionSessionTest, AutoCorrectDisabledKeepsTypedText) {
  EngineConfig cfg;
  cfg.autoCorrectEnabled = false;
  CompositionSession s(cfg);
  const std::wstring shown = typeLetters(s, L"twosi").markedText;
  EXPECT_EQ(*s.onKey(KeyKind::Terminator, L' ').commitText, shown);
  EXPECT_NE(shown, L"tới");
}

TEST(CompositionSessionTest, RawDriftAfterLiteralBackspaceSkipsShortcut) {
  EngineConfig cfg;
  cfg.shortcuts = {{L"a", L"X"}, {L"aa", L"X"}, {L"aaa", L"X"}};
  CompositionSession s(cfg);
  EXPECT_EQ(typeLetters(s, L"aaa").markedText, L"aa");  // Literal
  EXPECT_EQ(s.onKey(KeyKind::Backspace, 0).markedText, L"a");  // raw drifts
  EXPECT_EQ(*s.onKey(KeyKind::Terminator, L' ').commitText, L"a");
}

TEST(CompositionSessionTest, OverflowCommitsAndRefeedsKey) {
  CompositionSession s;
  const std::wstring full(goxvi::kMaxRawKeys, L'b');  // Foreign: raw verbatim
  EXPECT_EQ(typeLetters(s, full).markedText, full);
  const KeyOutcome out = s.onKey(KeyKind::Letter, L'b');
  EXPECT_TRUE(out.eaten);
  ASSERT_TRUE(out.commitText.has_value());
  EXPECT_EQ(*out.commitText, full);
  EXPECT_EQ(out.markedText, L"b");
  EXPECT_TRUE(out.setsMarkedText());
}

TEST(CompositionSessionTest, FlushReturnsDisplayedTextUnchanged) {
  CompositionSession s(withShortcuts());
  typeLetters(s, L"vieetj");
  EXPECT_EQ(*s.flush(), L"việt");
  EXPECT_FALSE(s.composing());

  typeLetters(s, L"trc");  // K8: flush never expands gõ tắt
  EXPECT_EQ(*s.flush(), L"trc");

  const std::wstring shown = typeLetters(s, L"twosi").markedText;
  EXPECT_EQ(*s.flush(), shown);  // K8: flush never auto-corrects
}

TEST(CompositionSessionTest, FlushWhenIdleReturnsNullopt) {
  CompositionSession s;
  EXPECT_FALSE(s.flush().has_value());
}

TEST(CompositionSessionTest, IdleKeysPassThroughWithoutCommit) {
  CompositionSession s;
  for (KeyKind kind : {KeyKind::Backspace, KeyKind::Escape, KeyKind::Terminator,
                       KeyKind::Chord}) {
    const KeyOutcome out = s.onKey(kind, L' ');
    EXPECT_FALSE(out.eaten);
    EXPECT_FALSE(out.commitText.has_value());
    EXPECT_FALSE(out.setsMarkedText());
  }
}

TEST(CompositionSessionTest, AbandonStartsNextWordClean) {
  CompositionSession s;
  typeLetters(s, L"vieet");
  s.abandon();
  EXPECT_FALSE(s.composing());
  EXPECT_EQ(typeLetters(s, L"laf").markedText, L"là");
}

TEST(CompositionSessionTest, VniDigitsCompose) {
  EngineConfig cfg;
  cfg.inputMethod = goxvi::InputMethod::Vni;
  CompositionSession s(cfg);
  EXPECT_EQ(s.inputMethod(), goxvi::InputMethod::Vni);
  EXPECT_EQ(typeLetters(s, L"a61").markedText, L"ấ");
}

}  // namespace
}  // namespace goxvi_mac
