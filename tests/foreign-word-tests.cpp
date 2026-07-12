#include <gtest/gtest.h>

#include "goxvi/engine-types.h"
#include "test-helpers.h"

namespace goxvi {
namespace {

using testhelpers::typeFresh;
using testhelpers::typeString;

TEST(ForeignModeTest, InvalidWordRevertsToRawAndKeepsComposing) {
  TelexEngine engine;
  // "vieejt" composes to "việt"; the invalid 'k' reverts the whole
  // composition to the raw buffer — no mid-word commit (H1, decision 5).
  typeString(engine, L"vieejt");
  KeyResult r = engine.processKey(L'k');
  EXPECT_TRUE(r.consumed);
  EXPECT_EQ(r.display, L"vieejtk");
  // Subsequent keys keep appending raw.
  r = engine.processKey(L'e');
  EXPECT_EQ(r.display, L"vieejtke");
}

TEST(ForeignModeTest, ImmediatelyForeignFirstKey) {
  EXPECT_EQ(typeFresh(L"fan"), L"fan");    // f is no Vietnamese onset
  EXPECT_EQ(typeFresh(L"just"), L"just");
  EXPECT_EQ(typeFresh(L"zz"), L"zz");
}

TEST(ForeignModeTest, CasePreservedInRaw) {
  EXPECT_EQ(typeFresh(L"Windows"), L"Windows");
  EXPECT_EQ(typeFresh(L"URL"), L"URL");
}

TEST(BackspaceTest, ComposingShrinksDisplayByExactlyOne) {
  TelexEngine engine;
  typeString(engine, L"vieejt");  // việt
  std::wstring d;
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"việ");
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"vi");  // ệ carried the transform; no resurrection
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"v");
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"");
  EXPECT_FALSE(engine.processBackspace(d));  // buffer empty → pass through
}

TEST(BackspaceTest, ToneOnlyKeyPopsWholeToneChar) {
  TelexEngine engine;
  typeString(engine, L"cas");  // cá
  std::wstring d;
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"c");  // á removed as one char, tone key not resurrected
}

TEST(BackspaceTest, LiteralPopsOneForOne) {  // M3, R2-L1
  TelexEngine engine;
  EXPECT_EQ(typeString(engine, L"aaa"), L"aa");
  std::wstring d;
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"a");  // NOT â
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"");
  EXPECT_FALSE(engine.processBackspace(d));
}

TEST(BackspaceTest, ForeignPopsOneForOne) {
  TelexEngine engine;
  typeString(engine, L"windows");
  std::wstring d;
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"window");  // 'w' opener is never valid → stays raw
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"windo");
}

TEST(BackspaceTest, ForeignRecoversToVietnameseWhenTailDeleted) {
  // Typo flow: như + g → Foreign shows raw "nhuwg"; deleting the bad key
  // must resume Vietnamese typing, not stay raw for the whole word.
  TelexEngine engine;
  EXPECT_EQ(typeString(engine, L"nhuwg"), L"nhuwg");
  std::wstring d;
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"như");  // back to Composing with the transform restored
  // Continue typing Vietnamese: như + n + g → nhưng, tone still works.
  EXPECT_EQ(typeString(engine, L"ng"), L"nhưng");
  EXPECT_EQ(typeString(engine, L"x"), L"những");
}

TEST(BackspaceTest, ForeignRecoveryVieejtk) {
  TelexEngine engine;
  typeString(engine, L"vieejtk");  // Foreign, raw
  std::wstring d;
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"việt");  // k removed → valid word again
}

TEST(BackspaceTest, ForeignStaysRawWhileTailStillInvalid) {
  TelexEngine engine;
  typeString(engine, L"vieejtkab");  // extra raw keys after the invalid k
  std::wstring d;
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"vieejtka");  // still contains k → still Foreign/raw
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"vieejtk");
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"việt");
}

TEST(OverflowTest, Key33ReportsOverflowWithoutApplying) {  // R2-M1
  TelexEngine engine;
  std::wstring word(32, L'j');  // 'j' → Foreign on key 1, then raw appends
  EXPECT_EQ(typeString(engine, word).size(), 32u);
  KeyResult r = engine.processKey(L'a');
  EXPECT_TRUE(r.overflow);
  EXPECT_EQ(r.display.size(), 32u);  // text the shim must commit
  EXPECT_EQ(engine.currentDisplay().size(), 32u);  // key 33 was NOT applied
  // Contract: shim commits, resets, re-feeds the key as a fresh word.
  engine.reset();
  r = engine.processKey(L'a');
  EXPECT_TRUE(r.consumed);
  EXPECT_FALSE(r.overflow);
  EXPECT_EQ(r.display, L"a");
}

TEST(OverflowTest, LiteralBufferAlsoCapped) {
  TelexEngine engine;
  typeString(engine, L"aaa");  // Literal: display "aa", raw "aaa" (3 keys)
  // Raw tracking continues in Literal (Esc cancel-to-raw), so the cap trips
  // when raw hits 32: 29 more keys fit, key 33 overall overflows.
  for (int i = 0; i < 29; ++i) {
    EXPECT_FALSE(engine.processKey(L'b').overflow) << "append " << i;
  }
  EXPECT_TRUE(engine.processKey(L'b').overflow);
}

}  // namespace
}  // namespace goxvi
