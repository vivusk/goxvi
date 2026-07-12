#include <gtest/gtest.h>

#include "test-helpers.h"

namespace goxvi {
namespace {

using testhelpers::typeFresh;
using testhelpers::typeString;

EngineConfig vniConfig() {
  EngineConfig cfg;
  cfg.inputMethod = InputMethod::Vni;
  return cfg;
}

struct Case {
  const wchar_t* input;
  const wchar_t* expected;
};

class VniTypingTest : public ::testing::TestWithParam<Case> {};

TEST_P(VniTypingTest, ProducesExpectedDisplay) {
  const Case& c = GetParam();
  EXPECT_EQ(typeFresh(c.input, vniConfig()), c.expected) << "input: " << c.input;
}

INSTANTIATE_TEST_SUITE_P(
    VniTable, VniTypingTest,
    ::testing::Values(
        // Core transforms & tones
        Case{L"vie65t", L"việt"}, Case{L"d9a6y", L"đây"},
        Case{L"hoa2", L"hòa"}, Case{L"thuy1", L"thúy"},
        Case{L"ca1c", L"các"}, Case{L"toan1", L"toán"},
        Case{L"la2", L"là"}, Case{L"to6i", L"tôi"},
        Case{L"mua2", L"mùa"}, Case{L"cu3a", L"của"},
        Case{L"nghe62", L"nghề"}, Case{L"d9o6c5", L"độc"},
        Case{L"tie6ng1", L"tiếng"}, Case{L"nguye6n4", L"nguyễn"},
        Case{L"vu4", L"vũ"}, Case{L"mo7i1", L"mới"},
        // ơ / ư (digit 7) and ă (digit 8)
        Case{L"u7", L"ư"},
        Case{L"thuo7ng", L"thương"}, Case{L"huo7ng", L"hương"},
        Case{L"ngu7oi2", L"người"}, Case{L"nu7oc1", L"nước"},
        Case{L"tra8ng", L"trăng"},
        Case{L"cu7u1", L"cứu"}, Case{L"ruo7u5", L"rượu"},
        // qu / gi specials (M2)
        Case{L"qua1", L"quá"}, Case{L"gia1", L"giá"},
        Case{L"gi2", L"gì"}, Case{L"gin2", L"gìn"}, Case{L"gi3", L"gỉ"},
        // Repeated modifier → undo + literal (H1)
        Case{L"a66", L"a6"}, Case{L"d99", L"d9"},
        Case{L"ca11", L"ca1"}, Case{L"a88", L"a8"}, Case{L"u77", L"u7"},
        // Foreign: bare modifier digits (no vowel/onset to attach to)
        Case{L"6", L"6"}, Case{L"7", L"7"}, Case{L"8", L"8"}, Case{L"9", L"9"},
        // Case preservation
        Case{L"Vie65t", L"Việt"}, Case{L"VIE65T", L"VIỆT"},
        Case{L"D9o6ng", L"Đông"}));

TEST(VniEngineTest, NewToneStyle) {
  EngineConfig cfg = vniConfig();
  cfg.toneStyle = ToneStyle::New;
  EXPECT_EQ(typeFresh(L"hoa2", cfg), L"hoà");
  EXPECT_EQ(typeFresh(L"thuy1", cfg), L"thuý");
}

TEST(VniEngineTest, LettersNeverTriggerTelexStyleTransforms) {
  // 'w' has no meaning in VNI; 'aa'/'dd' stay literal double letters.
  EXPECT_EQ(typeFresh(L"aa", vniConfig()), L"aa");
  EXPECT_EQ(typeFresh(L"dd", vniConfig()), L"dd");
  EXPECT_EQ(typeFresh(L"w", vniConfig()), L"w");
}

TEST(VniEngineTest, SetConfigSwitchesMethodAfterReset) {
  TelexEngine engine;
  EXPECT_EQ(typeString(engine, L"hoaf"), L"hòa");  // Telex default
  engine.reset();
  engine.setConfig(vniConfig());
  EXPECT_EQ(typeString(engine, L"hoa2"), L"hòa");
}

}  // namespace
}  // namespace goxvi
