#include <gtest/gtest.h>

#include "key-classifier.h"

namespace goxvi_mac {
namespace {

using goxvi::InputMethod;

constexpr std::uint16_t kKeyCodeA = 0;
constexpr std::uint16_t kKeyCodeSpace = 49;
constexpr std::uint16_t kKeyCodeTop1 = 18;
constexpr std::uint16_t kKeyCodeKeypad1 = 83;

KeyKind plain(wchar_t ch, std::uint16_t keyCode, InputMethod method = InputMethod::Telex) {
  return classifyKey(ch, 1, keyCode, false, false, method);
}

TEST(KeyClassifierTest, LettersBothCases) {
  EXPECT_EQ(plain(L'a', kKeyCodeA), KeyKind::Letter);
  EXPECT_EQ(plain(L'A', kKeyCodeA), KeyKind::Letter);
  EXPECT_EQ(plain(L'z', 6), KeyKind::Letter);
  EXPECT_EQ(plain(L'Z', 6), KeyKind::Letter);
}

TEST(KeyClassifierTest, TopRowDigitsDependOnMethod) {
  EXPECT_EQ(plain(L'1', kKeyCodeTop1, InputMethod::Vni), KeyKind::Letter);
  EXPECT_EQ(plain(L'1', kKeyCodeTop1, InputMethod::Telex), KeyKind::Terminator);
}

TEST(KeyClassifierTest, NumpadDigitsAlwaysTerminate) {
  EXPECT_EQ(classifyKey(L'1', 1, kKeyCodeKeypad1, false, true, InputMethod::Vni),
            KeyKind::Terminator);
  EXPECT_EQ(classifyKey(L'1', 1, kKeyCodeKeypad1, false, true, InputMethod::Telex),
            KeyKind::Terminator);
}

TEST(KeyClassifierTest, ModifierChordsWinOverEverything) {
  EXPECT_EQ(classifyKey(L'c', 1, 8, true, false, InputMethod::Telex), KeyKind::Chord);
  EXPECT_EQ(classifyKey(L'\x7f', 1, kKeyCodeBackspace, true, false, InputMethod::Telex),
            KeyKind::Chord);
}

TEST(KeyClassifierTest, BackspaceAndEscapeByKeyCode) {
  EXPECT_EQ(plain(L'\x7f', kKeyCodeBackspace), KeyKind::Backspace);
  EXPECT_EQ(plain(L'\x1b', kKeyCodeEscape), KeyKind::Escape);
}

TEST(KeyClassifierTest, OtherKeysTerminate) {
  EXPECT_EQ(plain(L' ', kKeyCodeSpace), KeyKind::Terminator);
  EXPECT_EQ(plain(L'.', 47), KeyKind::Terminator);
  EXPECT_EQ(plain(L'\r', 36), KeyKind::Terminator);
  EXPECT_EQ(plain(L'\t', 48), KeyKind::Terminator);
  EXPECT_EQ(plain(L'\xF702', 123), KeyKind::Terminator);  // left arrow
}

TEST(KeyClassifierTest, CharCountOtherThanOneTerminates) {
  EXPECT_EQ(classifyKey(L'a', 0, kKeyCodeA, false, false, InputMethod::Telex),
            KeyKind::Terminator);  // dead key: empty characters
  EXPECT_EQ(classifyKey(L'a', 2, kKeyCodeA, false, false, InputMethod::Telex),
            KeyKind::Terminator);
}

}  // namespace
}  // namespace goxvi_mac
