#include <gtest/gtest.h>

#include "goxvi/engine-types.h"
#include "test-helpers.h"

// Rule: spell-check auto-restore (revert an invalid syllable to the raw keys,
// dropping diacritics) applies only when the word contains a lowercase letter.
// An ALL-UPPERCASE word is an acronym/viết tắt the user typed deliberately —
// keep its diacritics instead of reverting: ĐAL (not DDAL), ĐL (not DDL),
// PLÔ (not PLOO). Mixed/first-letter-capital words (Password, Ddal) still
// revert, so English words keep the UniKey convention.

namespace goxvi {
namespace {

using testhelpers::typeFresh;

EngineConfig vniConfig() {
  EngineConfig c;
  c.inputMethod = InputMethod::Vni;
  return c;
}

// --- Telex: all-uppercase acronyms keep diacritics (no restore) ---

TEST(AcronymNoRestoreTest, TelexKeepsDiacriticNoVowelAfterOnset) {
  EXPECT_EQ(typeFresh(L"DDL"), L"ĐL");   // đ + invalid onset "đl" → keep, not DDL
}

TEST(AcronymNoRestoreTest, TelexKeepsDiacriticInvalidCoda) {
  EXPECT_EQ(typeFresh(L"DDAL"), L"ĐAL");  // coda "l" invalid → keep, not DDAL
}

TEST(AcronymNoRestoreTest, TelexHornStillAppliesAfterRelaxFlip) {
  // "PL" is invalid at the second letter (no "pl" onset); the later oo→ô must
  // still fire once the word flips to relaxed. Freezing at Literal would lose it.
  EXPECT_EQ(typeFresh(L"PLOO"), L"PLÔ");
}

// --- Telex: words with a lowercase letter still restore to raw ---

TEST(AcronymNoRestoreTest, TelexFullyLowercaseStillReverts) {
  EXPECT_EQ(typeFresh(L"ddal"), L"ddal");  // revert to raw (drops đ)
}

TEST(AcronymNoRestoreTest, TelexFirstLetterCapitalStillReverts) {
  EXPECT_EQ(typeFresh(L"Ddal"), L"Ddal");   // mixed case → revert to raw
  EXPECT_EQ(typeFresh(L"Ddl"), L"Ddl");     // one lowercase letter → revert
}

// --- Tone-only / plain English acronyms (a vowel but no diacritic) still
//     revert to raw: R/S are Telex tone keys, so keeping them would mangle. ---

TEST(AcronymNoRestoreTest, TelexToneOnlyAcronymReverts) {
  EXPECT_EQ(typeFresh(L"URL"), L"URL");  // R=hook would give ỦL — must revert
  EXPECT_EQ(typeFresh(L"USB"), L"USB");  // S=acute would give ÚB — must revert
}

// --- Valid all-uppercase words are untouched (never hit the Invalid path) ---

TEST(AcronymNoRestoreTest, TelexValidUppercaseWordUnchanged) {
  EXPECT_EQ(typeFresh(L"VIEEJT"), L"VIỆT");
}

// --- Backspace: relaxed acronym pops one display char at a time ---

TEST(AcronymNoRestoreTest, TelexBackspaceAfterRelaxFlip) {
  TelexEngine engine;
  EXPECT_EQ(testhelpers::typeString(engine, L"PLOO"), L"PLÔ");
  std::wstring d;
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"PL");   // ô removed as one char, no revert to raw
  ASSERT_TRUE(engine.processBackspace(d));
  EXPECT_EQ(d, L"P");
}

// --- VNI parity ---

TEST(AcronymNoRestoreTest, VniKeepsDiacriticNoVowel) {
  EXPECT_EQ(typeFresh(L"D9L", vniConfig()), L"ĐL");
}

TEST(AcronymNoRestoreTest, VniHornStillApplies) {
  EXPECT_EQ(typeFresh(L"PLO6", vniConfig()), L"PLÔ");
}

TEST(AcronymNoRestoreTest, VniLowercaseStillReverts) {
  EXPECT_EQ(typeFresh(L"d9l", vniConfig()), L"d9l");
}

}  // namespace
}  // namespace goxvi
