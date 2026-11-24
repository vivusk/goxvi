#include <gtest/gtest.h>

#include <string_view>

#include "composition-session.h"

namespace goxvi_mac {
namespace {

std::wstring typeAndCommit(CompositionSession& s, std::wstring_view keys) {
  for (wchar_t c : keys) s.onKey(KeyKind::Letter, c);
  return s.onKey(KeyKind::Terminator, L' ').commitText.value_or(L"");
}

// Menu Telex -> VNI (phase 4) takes effect for the next word.
TEST(CompositionSessionConfigTest, SetConfigSwitchesInputMethod) {
  CompositionSession s;
  EXPECT_EQ(s.inputMethod(), goxvi::InputMethod::Telex);
  EXPECT_EQ(typeAndCommit(s, L"aa"), L"â");  // â
  goxvi::EngineConfig vni;
  vni.inputMethod = goxvi::InputMethod::Vni;
  s.setConfig(vni);
  EXPECT_EQ(s.inputMethod(), goxvi::InputMethod::Vni);
  EXPECT_EQ(typeAndCommit(s, L"a6"), L"â");
  EXPECT_EQ(typeAndCommit(s, L"aa"), L"aa");
}

}  // namespace
}  // namespace goxvi_mac
