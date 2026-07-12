#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include "tone-placement-rules.h"
#include "vietnamese-syllable-parser.h"

namespace goxvi::detail {
namespace {

int placementFor(std::wstring_view word, ToneStyle style) {
  std::vector<Letter> v;
  for (wchar_t c : word) v.push_back({c, false});
  const int n = static_cast<int>(v.size());
  return tonePlacementIndex(v.data(), n, parseSyllable(v.data(), n), style);
}

TEST(TonePlacementTest, MarkedVowelWins) {
  EXPECT_EQ(placementFor(L"viêt", ToneStyle::Old), 2);   // ê
  EXPECT_EQ(placementFor(L"thương", ToneStyle::Old), 3); // ươ → ơ (last marked)
  EXPECT_EQ(placementFor(L"mưa", ToneStyle::Old), 1);    // ư
  EXPECT_EQ(placementFor(L"muôn", ToneStyle::Old), 2);   // ô
}

TEST(TonePlacementTest, CodaForcesLastNucleusVowel) {
  EXPECT_EQ(placementFor(L"toan", ToneStyle::Old), 2);  // toán
  EXPECT_EQ(placementFor(L"toan", ToneStyle::New), 2);
}

TEST(TonePlacementTest, TwoVowelsNoCodaByStyle) {
  EXPECT_EQ(placementFor(L"hoa", ToneStyle::Old), 1);  // hòa
  EXPECT_EQ(placementFor(L"hoa", ToneStyle::New), 2);  // hoà
  EXPECT_EQ(placementFor(L"thuy", ToneStyle::Old), 2); // thúy
  EXPECT_EQ(placementFor(L"thuy", ToneStyle::New), 3); // thuý
  EXPECT_EQ(placementFor(L"mia", ToneStyle::Old), 1);  // mía
}

TEST(TonePlacementTest, ThreeVowelsPenultimateBothStyles) {
  EXPECT_EQ(placementFor(L"khuyu", ToneStyle::Old), 3);  // khuỷu
  EXPECT_EQ(placementFor(L"khuyu", ToneStyle::New), 3);
  EXPECT_EQ(placementFor(L"ngoai", ToneStyle::Old), 3);  // ngoài
}

TEST(TonePlacementTest, SingleVowelAndOnsetSpecials) {
  EXPECT_EQ(placementFor(L"gi", ToneStyle::Old), 1);   // gì: i is nucleus (M2)
  EXPECT_EQ(placementFor(L"gia", ToneStyle::Old), 2);  // giá: gi onset
  EXPECT_EQ(placementFor(L"qua", ToneStyle::Old), 2);  // quá: qu onset
  EXPECT_EQ(placementFor(L"la", ToneStyle::Old), 1);
}

TEST(TonePlacementTest, VowelToneCombination) {
  EXPECT_EQ(vowelWithTone(L'a', Tone::Acute, false), L'á');
  EXPECT_EQ(vowelWithTone(L'a', Tone::Acute, true), L'Á');
  EXPECT_EQ(vowelWithTone(kECirc, Tone::Dot, false), L'ệ');
  EXPECT_EQ(vowelWithTone(kECirc, Tone::Dot, true), L'Ệ');
  EXPECT_EQ(vowelWithTone(kUHorn, Tone::Tilde, false), L'ữ');
  EXPECT_EQ(vowelWithTone(kOHorn, Tone::Grave, false), L'ờ');
  EXPECT_EQ(vowelWithTone(L'y', Tone::Hook, false), L'ỷ');
  EXPECT_EQ(vowelWithTone(L'o', Tone::None, false), L'o');
}

TEST(TonePlacementTest, LetterCharCase) {
  EXPECT_EQ(letterChar(kDStroke, true), L'Đ');
  EXPECT_EQ(letterChar(kDStroke, false), kDStroke);
  EXPECT_EQ(letterChar(kACirc, true), L'Â');
  EXPECT_EQ(letterChar(L'b', true), L'B');
  EXPECT_EQ(letterChar(L'b', false), L'b');
}

TEST(TonePlacementTest, ToneKeyMapping) {
  EXPECT_EQ(toneForKey(L's'), Tone::Acute);
  EXPECT_EQ(toneForKey(L'f'), Tone::Grave);
  EXPECT_EQ(toneForKey(L'r'), Tone::Hook);
  EXPECT_EQ(toneForKey(L'x'), Tone::Tilde);
  EXPECT_EQ(toneForKey(L'j'), Tone::Dot);
  EXPECT_EQ(toneForKey(L'k'), Tone::None);
}

}  // namespace
}  // namespace goxvi::detail
