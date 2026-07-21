#include <gtest/gtest.h>

#include "test-helpers.h"

namespace goxvi {
namespace {

using testhelpers::typeFresh;
using testhelpers::typeString;

struct Case {
  const wchar_t* input;
  const wchar_t* expected;
};

class TelexTypingTest : public ::testing::TestWithParam<Case> {};

TEST_P(TelexTypingTest, ProducesExpectedDisplay) {
  const Case& c = GetParam();
  EXPECT_EQ(typeFresh(c.input), c.expected) << "input: " << c.input;
}

INSTANTIATE_TEST_SUITE_P(
    TelexTable, TelexTypingTest,
    ::testing::Values(
        // Core transforms & tones
        Case{L"vieejt", L"việt"}, Case{L"ddaay", L"đây"},
        Case{L"hoaf", L"hòa"}, Case{L"thuys", L"thúy"},
        Case{L"casc", L"các"}, Case{L"toans", L"toán"},
        Case{L"laf", L"là"}, Case{L"tooi", L"tôi"},
        Case{L"muaf", L"mùa"}, Case{L"cura", L"của"},
        Case{L"ngheef", L"nghề"}, Case{L"ddoocj", L"độc"},
        Case{L"tieengs", L"tiếng"}, Case{L"nguyeenx", L"nguyễn"},
        Case{L"thuee", L"thuê"}, Case{L"thuees", L"thuế"},
        Case{L"vux", L"vũ"}, Case{L"mowis", L"mới"},
        // ư / ơ / ă (w key). Simple Telex: standalone w stays w (Foreign) —
        // only aw/ow/uw/uow transform.
        Case{L"uw", L"ư"}, Case{L"w", L"w"}, Case{L"wow", L"wow"},
        Case{L"thuowng", L"thương"}, Case{L"huowng", L"hương"},
        Case{L"nguwoif", L"người"}, Case{L"nguowif", L"người"},
        Case{L"nuwos", L"nướ"}, Case{L"nuwosc", L"nước"},
        Case{L"trawng", L"trăng"}, Case{L"trangw", L"trăng"},
        Case{L"cuuws", L"cứu"}, Case{L"cuwus", L"cứu"},
        Case{L"ruouwj", L"rượu"}, Case{L"khuyur", L"khuỷu"},
        // qu / gi specials (M2)
        Case{L"quas", L"quá"}, Case{L"gias", L"giá"},
        Case{L"gif", L"gì"}, Case{L"ginf", L"gìn"}, Case{L"gir", L"gỉ"},
        Case{L"gifa", L"già"},
        // z removes tone
        Case{L"lasz", L"la"}, Case{L"laszs", L"lá"},
        // Repeated modifier → undo + literal (H1): restore raw theo ĐÚNG
        // thứ tự phím đã gõ, nuốt phím undo cuối
        Case{L"aaa", L"aa"}, Case{L"ddd", L"dd"},
        Case{L"cass", L"cas"}, Case{L"cascs", L"casc"},
        Case{L"xooong", L"xoong"}, Case{L"vieee", L"viee"},
        Case{L"ww", L"ww"}, Case{L"thuoww", L"thuow"},
        Case{L"muwaw", L"muwa"},
        // Undo với modifier gõ cách quãng sau coda: KHÔNG khai triển tại chỗ
        // (dât → "daat" sai), phải trả về raw đúng thứ tự gõ
        Case{L"dataa", L"data"}, Case{L"banww", L"banw"},
        // Foreign words revert to raw, keep composing raw (H1)
        Case{L"windows", L"windows"}, Case{L"vieejtk", L"vieejtk"},
        // Case preservation
        Case{L"Vieejt", L"Việt"}, Case{L"VIEEJT", L"VIỆT"},
        Case{L"DDoong", L"Đông"}));

TEST(TelexEngineTest, NewToneStyle) {
  EngineConfig cfg;
  cfg.toneStyle = ToneStyle::New;
  EXPECT_EQ(typeFresh(L"hoaf", cfg), L"hoà");
  EXPECT_EQ(typeFresh(L"thuys", cfg), L"thuý");
  EXPECT_EQ(typeFresh(L"quas", cfg), L"quá");     // single-vowel nucleus: same
  EXPECT_EQ(typeFresh(L"khuyur", cfg), L"khuỷu"); // 3 vowels: penult both styles
  EXPECT_EQ(typeFresh(L"toans", cfg), L"toán");   // coda: same
}

TEST(TelexEngineTest, SetConfigSwitchesStyleAfterReset) {
  TelexEngine engine;
  EXPECT_EQ(typeString(engine, L"hoaf"), L"hòa");
  engine.reset();
  EngineConfig cfg;
  cfg.toneStyle = ToneStyle::New;
  engine.setConfig(cfg);
  EXPECT_EQ(typeString(engine, L"hoaf"), L"hoà");
}

TEST(TelexEngineTest, NonLetterKeysNotConsumed) {
  TelexEngine engine;
  EXPECT_FALSE(engine.processKey(L' ').consumed);
  EXPECT_FALSE(engine.processKey(L'1').consumed);
  EXPECT_FALSE(engine.processKey(L'.').consumed);
  typeString(engine, L"vieejt");
  EXPECT_FALSE(engine.processKey(L' ').consumed);  // word untouched
  EXPECT_EQ(engine.currentDisplay(), L"việt");
}

// Esc cancel-to-raw contract (shim restores this text then commits).
TEST(TelexEngineTest, RawTypedKeysForEscCancel) {
  TelexEngine engine;
  typeString(engine, L"vieejt");  // Composing
  EXPECT_EQ(engine.rawTypedKeys(), L"vieejt");
  engine.reset();

  typeString(engine, L"cass");  // Literal after tone undo
  EXPECT_EQ(engine.rawTypedKeys(), L"cass");
  engine.reset();

  typeString(engine, L"Windows");  // Foreign keeps case verbatim
  EXPECT_EQ(engine.rawTypedKeys(), L"Windows");
  engine.reset();

  EXPECT_EQ(engine.rawTypedKeys(), L"");  // idle
}

TEST(TelexEngineTest, ResetClearsWord) {
  TelexEngine engine;
  typeString(engine, L"vieejt");
  engine.reset();
  EXPECT_EQ(engine.currentDisplay(), L"");
  EXPECT_EQ(typeString(engine, L"laf"), L"là");
}

// rawKeysExact gates gõ tắt lookup (M1 / Decision 9): raw must faithfully
// mirror what was typed, which Literal backspace can break.
TEST(TelexEngineTest, RawKeysExactDefaultsTrue) {
  TelexEngine engine;
  EXPECT_TRUE(engine.rawKeysExact());  // idle
  typeString(engine, L"vieejt");       // Composing
  EXPECT_TRUE(engine.rawKeysExact());
  typeString(engine, L"windows");      // Foreign
  engine.reset();
  typeString(engine, L"windows");
  EXPECT_TRUE(engine.rawKeysExact());
}

TEST(TelexEngineTest, RawKeysExactStaysTrueAfterComposingBackspace) {
  TelexEngine engine;
  typeString(engine, L"vieejt");
  std::wstring display;
  engine.processBackspace(display);  // Composing backspace: raw stays exact
  EXPECT_TRUE(engine.rawKeysExact());
}

TEST(TelexEngineTest, RawKeysExactFalseAfterLiteralBackspace) {
  TelexEngine engine;
  EXPECT_EQ(typeString(engine, L"aaa"), L"aa");  // Literal state
  EXPECT_TRUE(engine.rawKeysExact());            // no backspace yet → still exact
  std::wstring display;
  engine.processBackspace(display);              // Literal backspace → drift
  EXPECT_FALSE(engine.rawKeysExact());
  engine.reset();
  EXPECT_TRUE(engine.rawKeysExact());            // reset restores
}

// restoreOnInvalid (khôi phục phím khi gõ sai). Default ON = Vietnamese
// spell-check: a keystroke that makes the syllable invalid reverts the whole
// word to the raw keys typed (drops dấu), UniKey convention.
TEST(TelexEngineTest, RestoreOnInvalidDefaultRevertsToRaw) {
  EXPECT_EQ(typeFresh(L"asd"), L"asd");    // a, s→á, d invalid → raw
  EXPECT_EQ(typeFresh(L"aad"), L"aad");    // â then invalid d → raw
  EXPECT_EQ(typeFresh(L"was"), L"was");    // lone w = Foreign → raw
  EXPECT_EQ(typeFresh(L"zaajy"), L"zaajy");
}

// OFF = "gõ dấu tự do": tones/diacritics apply to ANY syllable, no Vietnamese
// check. was → wá, zaajy → zậy.
TEST(TelexEngineTest, RestoreOnInvalidOffTypesToneFreely) {
  EngineConfig cfg;
  cfg.restoreOnInvalid = false;
  EXPECT_EQ(typeFresh(L"was", cfg), L"wá");
  EXPECT_EQ(typeFresh(L"zaajy", cfg), L"zậy");
  EXPECT_EQ(typeFresh(L"asd", cfg), L"ád");   // tone kept on invalid coda
  EXPECT_EQ(typeFresh(L"aad", cfg), L"âd");   // diacritic kept
}

// Same option applies to VNI: wa1 → wá, za6y5 → zậy.
TEST(VniEngineTest, RestoreOnInvalidOffTypesToneFreely) {
  EngineConfig cfg;
  cfg.inputMethod = InputMethod::Vni;
  cfg.restoreOnInvalid = false;
  EXPECT_EQ(typeFresh(L"wa1", cfg), L"wá");
  EXPECT_EQ(typeFresh(L"za6y5", cfg), L"zậy");
  // Default (spell-check on) reverts these to raw.
  cfg.restoreOnInvalid = true;
  EXPECT_EQ(typeFresh(L"wa1", cfg), L"wa1");
}

// OFF must not change words that ARE valid Vietnamese (no regression).
TEST(TelexEngineTest, RestoreOnInvalidOffLeavesValidWordsUnchanged) {
  EngineConfig cfg;
  cfg.restoreOnInvalid = false;
  EXPECT_EQ(typeFresh(L"vieejt", cfg), L"việt");
  EXPECT_EQ(typeFresh(L"nguyeenx", cfg), L"nguyễn");
  EXPECT_EQ(typeFresh(L"ddaay", cfg), L"đây");
}

// OFF: Esc still cancels to the raw keystrokes (shim contract unchanged).
TEST(TelexEngineTest, RestoreOnInvalidOffEscCancelsToRaw) {
  EngineConfig cfg;
  cfg.restoreOnInvalid = false;
  TelexEngine engine(cfg);
  typeString(engine, L"was");
  EXPECT_EQ(engine.rawTypedKeys(), L"was");
}

}  // namespace
}  // namespace goxvi
