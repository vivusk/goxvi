#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include "vietnamese-syllable-parser.h"

namespace goxvi::detail {
namespace {

std::vector<Letter> lettersOf(std::wstring_view s) {
  std::vector<Letter> v;
  for (wchar_t c : s) v.push_back({c, false});
  return v;
}

SyllableParts parseOf(std::wstring_view s) {
  auto v = lettersOf(s);
  return parseSyllable(v.data(), static_cast<int>(v.size()));
}

TEST(SyllableParserTest, OnsetNucleusCoda) {
  auto p = parseOf(L"toan");
  EXPECT_TRUE(p.valid);
  EXPECT_EQ(p.onsetLen, 1);
  EXPECT_EQ(p.nucleusLen, 2);
  EXPECT_EQ(p.codaLen, 1);

  p = parseOf(L"nghieng");
  EXPECT_TRUE(p.valid);
  EXPECT_EQ(p.onsetLen, 3);
  EXPECT_EQ(p.nucleusLen, 2);
  EXPECT_EQ(p.codaLen, 2);
}

TEST(SyllableParserTest, GiOnsetOnlyWhenVowelFollows) {  // M2
  auto p = parseOf(L"gi");  // gì: i is the nucleus
  EXPECT_TRUE(p.valid);
  EXPECT_EQ(p.onsetLen, 1);
  EXPECT_EQ(p.nucleusLen, 1);

  p = parseOf(L"gin");  // gìn
  EXPECT_TRUE(p.valid);
  EXPECT_EQ(p.onsetLen, 1);
  EXPECT_EQ(p.codaLen, 1);

  p = parseOf(L"gia");  // giá: gi is the onset
  EXPECT_TRUE(p.valid);
  EXPECT_EQ(p.onsetLen, 2);
  EXPECT_EQ(p.nucleusLen, 1);
}

TEST(SyllableParserTest, QuOnset) {
  auto p = parseOf(L"qua");
  EXPECT_TRUE(p.valid);
  EXPECT_EQ(p.onsetLen, 2);
  EXPECT_EQ(p.nucleusLen, 1);

  EXPECT_TRUE(parseOf(L"q").valid);    // prefix of qu, still all-consonant
  EXPECT_FALSE(parseOf(L"qa").valid);  // q is not a complete onset
  EXPECT_FALSE(parseOf(L"qun").valid); // consonant right after onset
}

TEST(SyllableParserTest, PrefixValidityOfRawIntermediates) {
  EXPECT_TRUE(parseOf(L"vie").valid);   // → viê
  EXPECT_TRUE(parseOf(L"thuo").valid);  // → thuô/thươ
  EXPECT_TRUE(parseOf(L"cuu").valid);   // → cưu
  EXPECT_TRUE(parseOf(L"ruou").valid);  // → rươu
  EXPECT_TRUE(parseOf(L"uye").valid);   // → uyê
}

TEST(SyllableParserTest, InvalidStructures) {
  EXPECT_FALSE(parseOf(L"st").valid);    // bad onset cluster
  EXPECT_FALSE(parseOf(L"caoo").valid);  // bad nucleus
  EXPECT_FALSE(parseOf(L"antk").valid);  // bad coda
  EXPECT_FALSE(parseOf(L"anb").valid);   // b is not a coda
  EXPECT_FALSE(parseOf(L"oana").valid);  // vowel after coda
  EXPECT_FALSE(parseOf(L"caz").valid);   // z is never a coda
}

TEST(SyllableParserTest, MarkedVowelNuclei) {
  EXPECT_TRUE(parseOf(L"viê").valid);
  EXPECT_TRUE(parseOf(L"thươ").valid);
  EXPECT_TRUE(parseOf(L"ngươ").valid);  // nucleus ươ
  EXPECT_TRUE(parseOf(L"đây").valid);
  EXPECT_FALSE(parseOf(L"caô").valid);
}

TEST(SyllableParserTest, VowelHelpers) {
  EXPECT_TRUE(isVowelBase(L'a'));
  EXPECT_TRUE(isVowelBase(kUHorn));
  EXPECT_FALSE(isVowelBase(L'w'));
  EXPECT_FALSE(isVowelBase(kDStroke));
  EXPECT_TRUE(isMarkedVowel(kOCirc));
  EXPECT_FALSE(isMarkedVowel(L'o'));
}

}  // namespace
}  // namespace goxvi::detail
