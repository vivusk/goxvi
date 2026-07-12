#include <gtest/gtest.h>

#include <string>

#include "utf8-utf16-conversion.h"  // core/src on the tests include path

namespace goxvi::detail {
namespace {

TEST(Utf8Utf16Test, RoundTripAscii) {
  const std::string s = "trc hello 123";
  EXPECT_EQ(wideToUtf8(utf8ToWide(s)), s);
}

TEST(Utf8Utf16Test, RoundTripAccentedVietnamese) {
  for (const std::string s : {std::string("trước"), std::string("Đông"),
                              std::string("ượ"), std::string("Việt Nam")}) {
    EXPECT_EQ(wideToUtf8(utf8ToWide(s)), s) << s;
  }
}

TEST(Utf8Utf16Test, DecodesKnownCodePoint) {
  const std::wstring w = utf8ToWide("ư");  // U+01B0
  ASSERT_EQ(w.size(), 1u);
  EXPECT_EQ(w[0], static_cast<wchar_t>(0x01B0));
}

TEST(Utf8Utf16Test, EmptyString) {
  EXPECT_TRUE(utf8ToWide("").empty());
  EXPECT_TRUE(wideToUtf8(L"").empty());
}

TEST(Utf8Utf16Test, InvalidUtf8BecomesReplacementNoCrash) {
  const std::string bad = "\xFF\xFE"           // invalid lead bytes
                          "a\xC3"              // truncated 2-byte at end
                          "\x80";              // stray continuation
  const std::wstring w = utf8ToWide(bad);
  // 'a' survives; every bad byte maps to U+FFFD; no crash / throw.
  bool hasA = false, hasReplacement = false;
  for (const wchar_t c : w) {
    if (c == L'a') hasA = true;
    if (c == static_cast<wchar_t>(0xFFFD)) hasReplacement = true;
  }
  EXPECT_TRUE(hasA);
  EXPECT_TRUE(hasReplacement);
}

TEST(Utf8Utf16Test, OverlongEncodingRejected) {
  // Overlong "/" (0x2F) encoded as 2 bytes → U+FFFD, not '/'.
  const std::wstring w = utf8ToWide("\xC0\xAF");
  for (const wchar_t c : w) EXPECT_NE(c, L'/');
}

TEST(Utf8Utf16Test, NonBmpRoundTripThumbsUp) {
  const std::string s = "\xF0\x9F\x91\x8D";  // 👍 U+1F44D, 4-byte UTF-8
  const std::wstring w = utf8ToWide(s);
  ASSERT_EQ(w.size(), 2u);  // surrogate pair in UTF-16
  EXPECT_GE(w[0], static_cast<wchar_t>(0xD800));
  EXPECT_LE(w[0], static_cast<wchar_t>(0xDBFF));
  EXPECT_GE(w[1], static_cast<wchar_t>(0xDC00));
  EXPECT_LE(w[1], static_cast<wchar_t>(0xDFFF));
  EXPECT_EQ(wideToUtf8(w), s);
}

TEST(Utf8Utf16Test, NonBmpRoundTripMathU) {
  const std::string s = "\xF0\x9D\x94\x98";  // 𝔘 U+1D518
  EXPECT_EQ(wideToUtf8(utf8ToWide(s)), s);
}

TEST(Utf8Utf16Test, UnpairedSurrogateBecomesReplacement) {
  const std::wstring loneHigh(1, static_cast<wchar_t>(0xD800));
  const std::string s = wideToUtf8(loneHigh);
  EXPECT_EQ(s, "\xEF\xBF\xBD");  // U+FFFD in UTF-8
}

}  // namespace
}  // namespace goxvi::detail
