#define _CRT_SECURE_NO_WARNINGS  // std::getenv in the manual generator below

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <set>
#include <string>

#include "goxvi/typo-autocorrect.h"
#include "typo-autocorrect-internal.h"

namespace goxvi {
namespace {

EngineConfig telex() {
  EngineConfig c;
  c.inputMethod = InputMethod::Telex;
  return c;
}

EngineConfig vni() {
  EngineConfig c;
  c.inputMethod = InputMethod::Vni;
  return c;
}

std::optional<std::wstring> fix(std::wstring_view raw, EngineConfig c = telex()) {
  return autoCorrectTypo(raw, c);
}

// ---- corrects modifier-before-vowel typos (Telex) --------------------------

TEST(TypoAutocorrect, HornThenToneBeforeVowel) {
  // towsi -> tới; fast-typed as twosi (w before o) / twsoi (w and s before o).
  EXPECT_EQ(fix(L"twosi").value_or(L""), L"tới");
  EXPECT_EQ(fix(L"twsoi").value_or(L""), L"tới");
}

TEST(TypoAutocorrect, WithoutTrailingVowel) {
  EXPECT_EQ(fix(L"twos").value_or(L""), L"tớ");  // tows -> tớ
}

TEST(TypoAutocorrect, PreservesCase) {
  EXPECT_EQ(fix(L"Twosi").value_or(L""), L"Tới");
}

TEST(TypoAutocorrect, HornWithTrailingToneRunPeeled) {
  // twsoi: both w (horn) and s (tone) wedged before the vowel; the whole run is
  // peeled because it is anchored by a 'w'. (Same target tới as twosi.)
  EXPECT_EQ(fix(L"twsoi").value_or(L""), L"tới");
}

// English consonant clusters (Cr, Cl, sw...) must NOT be mistaken for a
// misplaced tone: Telex requires a 'w' in the peeled run, so a lone r/s/f/x/j is
// never pulled. Without this, bra/cram/pray/draw/throw would all be rewritten.
TEST(TypoAutocorrect, EnglishConsonantClustersLeftAlone) {
  for (const wchar_t* w :
       {L"bra", L"cram", L"pray", L"gray", L"draw", L"throw", L"crab", L"free"}) {
    EXPECT_FALSE(fix(w).has_value()) << w;
  }
}

// ---- leaves valid / non-typo words alone -----------------------------------

TEST(TypoAutocorrect, AlreadyValidUntouched) {
  EXPECT_FALSE(fix(L"towsi").has_value());  // correctly typed
  EXPECT_FALSE(fix(L"sao").has_value());
  EXPECT_FALSE(fix(L"la").has_value());
}

TEST(TypoAutocorrect, OnsetConsonantNeverAltered) {
  // "she": the key adjacent to the vowel 'e' is 'h' (a real consonant, not a
  // modifier) — pulling 's' would change the onset, which we forbid.
  EXPECT_FALSE(fix(L"she").has_value());
}

TEST(TypoAutocorrect, NoVowelOrNoOnsetGap) {
  EXPECT_FALSE(fix(L"bcd").has_value());   // no vowel at all
  EXPECT_FALSE(fix(L"").has_value());
}

TEST(TypoAutocorrect, EmptyOnsetNeverCorrected) {
  // No real onset before the misplaced modifier → never touched. Kills the
  // English false-positive class (was, wax, swap, swat) without a blocklist,
  // and garbage like xyz (leading tone key 'x') never becomes "y".
  for (const wchar_t* w : {L"was", L"wax", L"swap", L"swat", L"xyz"}) {
    EXPECT_FALSE(fix(w).has_value()) << w;
    EXPECT_FALSE(detail::correctTypoNoBlocklist(w, telex()).has_value())
        << w;
  }
}

// ---- English blocklist (only real-onset collisions need it) -----------------

TEST(TypoAutocorrect, BlocklistedEnglishLeftAlone) {
  for (const wchar_t* w : {L"two", L"twas"}) {
    EXPECT_FALSE(fix(w).has_value()) << w;
    // ...but the raw peel logic WOULD have rewritten them (why they're listed).
    EXPECT_TRUE(detail::correctTypoNoBlocklist(w, telex()).has_value())
        << w;
  }
}

TEST(TypoAutocorrect, BlocklistCaseInsensitive) {
  EXPECT_FALSE(fix(L"Two").has_value());
  EXPECT_FALSE(fix(L"TWO").has_value());
}

TEST(TypoAutocorrect, TwosIsCorrectedNotBlocked) {
  // "twos" collides with English (plural of two) but is the user's wanted fix
  // target (tớ); the rarer English sense is sacrificed, so it must NOT be listed.
  EXPECT_EQ(fix(L"twos").value_or(L""), L"tớ");
}

// ---- transposed final coda (ng -> gn, nh -> hn) ----------------------------

TEST(TypoAutocorrect, TransposedNgCoda) {
  EXPECT_EQ(fix(L"cuxgn").value_or(L""), L"cũng");  // cuxng -> cũng
  EXPECT_EQ(fix(L"cogn").value_or(L""), L"cong");
  EXPECT_EQ(fix(L"khoogn").value_or(L""), L"không");  // khoong -> không
  EXPECT_EQ(fix(L"khogn").value_or(L""), L"khong");
  EXPECT_EQ(fix(L"tieegns").value_or(L""), L"tiếng");  // tieengs -> tiếng
}

TEST(TypoAutocorrect, TransposedNhCoda) {
  EXPECT_EQ(fix(L"nhahn").value_or(L""), L"nhanh");
  EXPECT_EQ(fix(L"mihn").value_or(L""), L"minh");
}

TEST(TypoAutocorrect, TransposedChCoda) {
  EXPECT_EQ(fix(L"sahcs").value_or(L""), L"sách");  // sachs -> sách
  EXPECT_EQ(fix(L"kihc").value_or(L""), L"kich");
}

TEST(TypoAutocorrect, TransposedCodaToneAfterPair) {
  // tone key typed after the (swapped) coda still rides along: cugnx -> cũng.
  EXPECT_EQ(fix(L"cugnx").value_or(L""), L"cũng");
}

TEST(TypoAutocorrect, TransposedCodaPreservesCase) {
  EXPECT_EQ(fix(L"Cuxgn").value_or(L""), L"Cũng");
}

TEST(TypoAutocorrect, TransposedCodaVni) {
  EXPECT_EQ(fix(L"cu4gn", vni()).value_or(L""), L"cũng");
}

TEST(TypoAutocorrect, TransposedCodaValidWordsUntouched) {
  // Correctly typed words with a real gn/hn-free coda are never rewritten...
  for (const wchar_t* w : {L"cung", L"nhanh", L"cuxng"}) {
    EXPECT_FALSE(fix(w).has_value()) << w;
  }
  // ...and a word-initial "gn" has no nucleus in front, so it is out of scope.
  EXPECT_FALSE(fix(L"gnaw").has_value());
}

// ---- VNI -------------------------------------------------------------------

TEST(TypoAutocorrect, VniModifiersBeforeVowel) {
  // to71i -> tới; mistyped t71oi (7 horn + 1 sắc before o).
  EXPECT_EQ(fix(L"t71oi", vni()).value_or(L""), L"tới");
}

TEST(TypoAutocorrect, VniDStrokeDigitNotPeeled) {
  // 9 (đ) is a consonant modifier, never treated as a vowel modifier.
  EXPECT_FALSE(fix(L"9ao", vni()).has_value());
}

// ---- blocklist generator (manual) ------------------------------------------
//
// Regenerate core/src/english-blocklist.generated.inc from an English wordlist.
// Not part of the normal suite (DISABLED_). Run with:
//   GOXVI_WORDLIST=words.txt GOXVI_BLOCKLIST_OUT=core/src/english-blocklist.generated.inc \
//     goxvi-core-tests --gtest_also_run_disabled_tests \
//       --gtest_filter=*GenerateBlocklist*
// words.txt: one word per line, or "word count" (frequency) — the first token is
// used. Prefer a FREQUENCY list of real usage (used: hermitdave FrequencyWords
// en_50k, from subtitles) over a full dictionary: a full dictionary drags in
// archaic/non-words (cwo, lwo, mwa) that no one types but ARE valid Vietnamese
// typos, so blocklisting them would suppress real corrections. Note the kept
// entries include romanized names common in Vietnamese usage (Hwang, Kwang) —
// several collide with real Vietnamese words (twang→tăng, hwang→hăng), so
// blocking them is intended, not noise.
TEST(TypoAutocorrect, DISABLED_GenerateBlocklist) {
  const char* listPath = std::getenv("GOXVI_WORDLIST");
  ASSERT_NE(listPath, nullptr) << "set GOXVI_WORDLIST to an English wordlist";
  const char* outPath = std::getenv("GOXVI_BLOCKLIST_OUT");
  ASSERT_NE(outPath, nullptr) << "set GOXVI_BLOCKLIST_OUT to the .inc path";

  std::ifstream in(listPath);
  ASSERT_TRUE(in.is_open()) << listPath;

  // Words that ARE English but the user wants corrected anyway (the Vietnamese
  // sense wins over a rare English one): never blocklist these.
  const std::set<std::wstring> wanted = {L"twos"};  // twos → tớ

  std::set<std::wstring> blocked;  // sorted + unique
  std::string line;
  while (std::getline(in, line)) {
    // Accept both plain wordlists and frequency lists ("word count" per line):
    // take the first whitespace-delimited token. A frequency-ranked list is the
    // RIGHT input — a full dictionary drags in archaic/non-words (cwo, lwo, mwa)
    // that no one types yet ARE valid Vietnamese typos, so blocklisting them
    // would suppress the very corrections this feature exists to make.
    const std::string token = line.substr(0, line.find_first_of(" \t\r\n"));
    if (token.empty()) continue;
    std::wstring w;
    bool pureAlpha = true;
    for (char ch : token) {
      if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
      if (ch < 'a' || ch > 'z') { pureAlpha = false; break; }
      w.push_back(static_cast<wchar_t>(ch));
    }
    if (!pureAlpha || w.empty() || wanted.count(w)) continue;
    // Generated with Telex only. The modifier rule is letter-triggered in Telex
    // and digit-triggered in VNI, so VNI never rewrites an English word; the
    // coda rule (gn/hn) is letter-based in BOTH, but its Telex form accepts a
    // wider tail (tone letters after the pair: "signs"), so the Telex pass is a
    // superset. Ignore the current blocklist to avoid shrinking on rerun.
    if (detail::correctTypoNoBlocklist(w, telex()).has_value()) {
      blocked.insert(w);
    }
  }

  std::ofstream out(outPath, std::ios::binary);
  ASSERT_TRUE(out.is_open()) << outPath;
  out << "// english-blocklist.generated.inc - GENERATED, do not edit by hand.\n"
      << "// Sorted lowercase English words the corrector would otherwise rewrite\n"
      << "// to a valid Vietnamese syllable. Regenerate: see\n"
      << "// tests/test-typo-autocorrect.cpp (DISABLED_GenerateBlocklist).\n";
  for (const std::wstring& w : blocked) {
    out << "L\"";
    for (wchar_t c : w) out << static_cast<char>(c);
    out << "\",\n";
  }
  std::cerr << "wrote " << blocked.size() << " entries to " << outPath << "\n";
}

}  // namespace
}  // namespace goxvi
