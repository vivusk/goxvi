#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "shortcut-validation.h"

namespace goxvi_mac {
namespace {

using goxvi::ShortcutEntry;

TEST(ShortcutValidationTest, AcceptsLowercaseTriggerAndAccentedExpansion) {
  EXPECT_EQ(validateShortcut({L"vn", L"Việt Nam"}), ShortcutError::None);
}

TEST(ShortcutValidationTest, RejectsEmptyTrigger) {
  EXPECT_EQ(validateShortcut({L"", L"x"}), ShortcutError::TriggerEmpty);
}

TEST(ShortcutValidationTest, RejectsUppercaseDigitSpaceAndAccentInTrigger) {
  EXPECT_EQ(validateShortcut({L"Vn", L"x"}), ShortcutError::TriggerInvalidChar);
  EXPECT_EQ(validateShortcut({L"vn1", L"x"}), ShortcutError::TriggerInvalidChar);
  EXPECT_EQ(validateShortcut({L"v n", L"x"}), ShortcutError::TriggerInvalidChar);
  EXPECT_EQ(validateShortcut({L"â", L"x"}), ShortcutError::TriggerInvalidChar);
}

TEST(ShortcutValidationTest, TriggerLengthLimitIsMaxRawKeys) {
  const std::wstring atLimit(goxvi::kMaxRawKeys, L'a');
  EXPECT_EQ(validateShortcut({atLimit, L"x"}), ShortcutError::None);
  EXPECT_EQ(validateShortcut({atLimit + L"a", L"x"}), ShortcutError::TriggerTooLong);
}

TEST(ShortcutValidationTest, RejectsEmptyExpansion) {
  EXPECT_EQ(validateShortcut({L"vn", L""}), ShortcutError::ExpansionEmpty);
}

TEST(ShortcutValidationTest, ExpansionLimitCountsUtf16Units) {
  const std::wstring atLimit(goxvi::kMaxShortcutExpansionChars, L'ệ');
  EXPECT_EQ(validateShortcut({L"vn", atLimit}), ShortcutError::None);
  EXPECT_EQ(validateShortcut({L"vn", atLimit + L"x"}), ShortcutError::ExpansionTooLong);
  // K1: a non-BMP char is a surrogate pair = 2 units, so 1 emoji at the limit
  // pushes it over.
  std::wstring withPair(goxvi::kMaxShortcutExpansionChars - 1, L'x');
  withPair += static_cast<wchar_t>(0xD83D);
  withPair += static_cast<wchar_t>(0xDC4D);
  EXPECT_EQ(validateShortcut({L"vn", withPair}), ShortcutError::ExpansionTooLong);
}

TEST(ShortcutValidationTest, RejectsDuplicateTriggerOfAnotherRow) {
  const std::vector<ShortcutEntry> list{{L"vn", L"Việt Nam"}, {L"ko", L"không"}};
  EXPECT_EQ(validateShortcutAt(list, list.size(), {L"ko", L"x"}),
            ShortcutError::DuplicateTrigger);  // new row
  EXPECT_EQ(validateShortcutAt(list, 0, {L"ko", L"x"}),
            ShortcutError::DuplicateTrigger);  // edit row 0 into row 1's trigger
}

TEST(ShortcutValidationTest, EditingARowKeepingItsOwnTriggerIsAllowed) {
  const std::vector<ShortcutEntry> list{{L"vn", L"Việt Nam"}, {L"ko", L"không"}};
  EXPECT_EQ(validateShortcutAt(list, 1, {L"ko", L"không có"}), ShortcutError::None);
  EXPECT_EQ(validateShortcutAt(list, list.size(), {L"dc", L"được"}),
            ShortcutError::None);
}

TEST(ShortcutValidationTest, EntryErrorReportedBeforeDuplicate) {
  const std::vector<ShortcutEntry> list{{L"vn", L"x"}};
  EXPECT_EQ(validateShortcutAt(list, 1, {L"vn", L""}), ShortcutError::ExpansionEmpty);
}

}  // namespace
}  // namespace goxvi_mac
