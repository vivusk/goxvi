#include <gtest/gtest.h>

#include <string>

#include "fake-screen.h"
#include "utf8-utf16-conversion.h"

namespace goxvi_mac {
namespace {

using testing::FakeScreen;

KeyOutcome marked(const wchar_t* text) {
  KeyOutcome out;
  out.eaten = true;
  out.markedText = text;
  return out;
}

TEST(DirectInsertTrackerTest, StaleWhenCaretMovedOrTextSelected) {
  DirectInsertTracker t;
  t.onOutcome(KeyKind::Letter, marked(L"v"), 10);
  EXPECT_FALSE(t.isStale(11, 0));
  EXPECT_TRUE(t.isStale(4, 0));   // click elsewhere / field cleared
  EXPECT_TRUE(t.isStale(11, 3));  // selection: typing would overwrite it
}

// Behaviour table row "Terminator/Chord, no rewrite" with a Chord (Cmd+...).
TEST(DirectInsertTrackerTest, ChordAfterPlainWordSendsNoCommandAndPassesThrough) {
  FakeScreen s;
  s.type(L"vieetj");
  const std::size_t before = s.edits.size();
  s.press(KeyKind::Chord, 0);
  EXPECT_EQ(s.edits.size(), before);
  EXPECT_FALSE(s.lastEaten);
  EXPECT_FALSE(s.tracker.tracking());
  EXPECT_EQ(s.text, L"vi\u1EC7t");
}

TEST(DirectInsertTrackerTest, NeverStaleWhenNotTracking) {
  DirectInsertTracker t;
  EXPECT_FALSE(t.isStale(0, 0));
  EXPECT_FALSE(t.isStale(42, 5));
}

TEST(DirectInsertTrackerTest, ResetForgetsWord) {
  DirectInsertTracker t;
  t.onOutcome(KeyKind::Letter, marked(L"vi"), 0);
  ASSERT_TRUE(t.tracking());
  t.reset();
  EXPECT_FALSE(t.tracking());
  EXPECT_FALSE(t.isStale(99, 0));
  // Next word starts from scratch: pure append, nothing deleted.
  const DirectStep step = t.onOutcome(KeyKind::Letter, marked(L"a"), 99);
  ASSERT_TRUE(step.edit.has_value());
  EXPECT_EQ(step.edit->deleteCount, 0u);
  EXPECT_EQ(step.edit->insert, L"a");
}

TEST(DirectInsertTrackerTest, DeleteBeyondCaretDesyncsWithoutCommand) {
  DirectInsertTracker t;
  t.onOutcome(KeyKind::Letter, marked(L"vie"), 0);    // caret now 3
  const DirectStep step = t.onOutcome(KeyKind::Letter, marked(L"viê"), 0);  // app says 0
  EXPECT_FALSE(step.edit.has_value());
  EXPECT_FALSE(step.eaten);  // word abandoned: the key acts as without Goxvi
  EXPECT_TRUE(t.desynced());
  EXPECT_FALSE(t.tracking());
  t.reset();
  EXPECT_FALSE(t.desynced());
}

TEST(DirectInsertTrackerTest, ClickMidWordAbandonsWordWithoutTouchingText) {
  FakeScreen s;
  s.type(L"thuowngf");
  ASSERT_EQ(s.text, L"thường");
  const std::wstring before = s.text;
  s.moveCaret(3);  // click between "thư" and "ờng"
  const std::size_t edits = s.edits.size();
  s.type(L"a");
  EXPECT_EQ(s.edits.size(), edits + 1);
  EXPECT_EQ(s.edits.back().deleteCount, 0u);  // new word: nothing of ours deleted
  EXPECT_EQ(s.text, L"thưaờng");  // old word untouched, "a" typed at the click
  EXPECT_NE(s.text, before);
}

TEST(DirectInsertTrackerTest, FieldClearedAfterSendStartsFresh) {
  FakeScreen s;
  s.type(L"chafo");
  s.text.clear();  // the chat cleared its field (seen as caret jump to 0)
  s.moveCaret(0);
  s.type(L"ok");
  EXPECT_EQ(s.text, L"ok");
}

TEST(DirectInsertTrackerTest, BackspaceAtDocumentStartDesyncs) {
  DirectInsertTracker t;
  t.onOutcome(KeyKind::Letter, marked(L"a"), 0);  // expects caret 1
  KeyOutcome gone = marked(L"");
  const DirectStep step = t.onOutcome(KeyKind::Backspace, gone, 0);
  EXPECT_TRUE(t.desynced());
  EXPECT_FALSE(step.edit.has_value());
}

TEST(DirectInsertTrackerTest, PureDeleteFromNonBackspaceIsWidenedToReplace) {
  DirectInsertTracker t;
  t.onOutcome(KeyKind::Letter, marked(L"abc"), 0);
  const DirectStep step = t.onOutcome(KeyKind::Letter, marked(L"a"), 3);
  ASSERT_TRUE(step.edit.has_value());
  EXPECT_EQ(step.edit->deleteCount, 3u);  // "abc" -> "a" sent as replace, never ins=0
  EXPECT_EQ(step.edit->insert, L"a");
  EXPECT_TRUE(step.eaten);
  EXPECT_FALSE(t.isStale(1, 0));
}

TEST(DirectInsertTrackerTest, WholeWordGoneWithoutTextStopsTrackingAndPassesBackspace) {
  DirectInsertTracker t;
  t.onOutcome(KeyKind::Letter, marked(L"ab"), 0);
  const DirectStep step = t.onOutcome(KeyKind::Backspace, marked(L""), 2);
  EXPECT_FALSE(step.edit.has_value());  // no delete-only command exists for this
  EXPECT_FALSE(step.eaten);
  EXPECT_FALSE(t.tracking());
  EXPECT_FALSE(t.desynced());
}

TEST(DirectInsertTrackerTest, WideningKeepsSurrogatePairWhole) {
  DirectInsertTracker t;
  const std::wstring smile = goxvi::detail::utf8ToWide("\xF0\x9F\x98\x80");
  t.onOutcome(KeyKind::Letter, marked((smile + L"ab").c_str()), 0);
  const DirectStep step = t.onOutcome(KeyKind::Letter, marked(smile.c_str()), 4);
  ASSERT_TRUE(step.edit.has_value());
  EXPECT_EQ(step.edit->deleteCount, 4u);
  EXPECT_EQ(step.edit->insert, smile);
}

}  // namespace
}  // namespace goxvi_mac
