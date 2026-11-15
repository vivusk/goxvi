#include <gtest/gtest.h>

#include <initializer_list>

#include "toggle-hotkey-detector.h"

namespace goxvi_mac {
namespace {

constexpr ModifierMask kCtrl = kModControl;
constexpr ModifierMask kShift = kModShift;
constexpr ModifierMask kCtrlShift = kModControl | kModShift;

// Feeds a flagsChanged sequence; returns how many times it fired.
int feed(ToggleHotkeyDetector& d, std::initializer_list<ModifierMask> seq) {
  int fired = 0;
  for (ModifierMask m : seq) fired += d.onFlagsChanged(m) ? 1 : 0;
  return fired;
}

TEST(ToggleHotkeyDetectorTest, CtrlDownShiftDownShiftUpFiresOnce) {
  ToggleHotkeyDetector d;
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift, kCtrl}), 1);
  EXPECT_EQ(feed(d, {0}), 0);  // releasing the rest does not fire again
}

TEST(ToggleHotkeyDetectorTest, ReleasingBothFiresExactlyOnce) {
  ToggleHotkeyDetector d;
  EXPECT_EQ(feed(d, {kShift, kCtrlShift, kShift, 0}), 1);
  ToggleHotkeyDetector both;  // both released in the same event
  EXPECT_EQ(feed(both, {kCtrl, kCtrlShift, 0}), 1);
}

TEST(ToggleHotkeyDetectorTest, RepeatedGesturesEachFire) {
  ToggleHotkeyDetector d;
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift, 0}), 1);
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift, kCtrl, 0}), 1);
}

// F1: a foreign modifier whose release was never seen (app switch) must not
// swallow the next gesture once the controller is re-activated.
TEST(ToggleHotkeyDetectorTest, ResetClearsTaintFromLostRelease) {
  ToggleHotkeyDetector d;
  EXPECT_EQ(feed(d, {kModCommand}), 0);  // ⌘ down, release lost
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift, kCtrl}), 0);  // tainted without reset
  d.reset();
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift, kCtrl, 0}), 1);
}

TEST(ToggleHotkeyDetectorTest, KeyDownInBetweenCancels) {
  ToggleHotkeyDetector d;
  feed(d, {kCtrl, kCtrlShift});
  d.onKeyDown();  // ⌃⇧A
  EXPECT_EQ(feed(d, {kCtrl, 0}), 0);
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift, 0}), 1);  // next clean gesture works
}

TEST(ToggleHotkeyDetectorTest, KeyDownWhileTargetHeldThenRepressDoesNotFire) {
  ToggleHotkeyDetector d;
  feed(d, {kCtrl, kCtrlShift});
  d.onKeyDown();
  // Shift released and pressed again without letting go of Ctrl: same gesture.
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift, kCtrl, 0}), 0);
}

TEST(ToggleHotkeyDetectorTest, KeyDownWhileIdleDoesNotBlockNextGesture) {
  ToggleHotkeyDetector d;
  d.onKeyDown();
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift, 0}), 1);
}

TEST(ToggleHotkeyDetectorTest, ExtraModifierCancels) {
  ToggleHotkeyDetector d;
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift, kCtrlShift | kModCommand, kCtrlShift, kCtrl, 0}), 0);
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift | kModOption, kCtrlShift, 0}), 0);
}

TEST(ToggleHotkeyDetectorTest, SingleModifierNeverFires) {
  ToggleHotkeyDetector d;
  EXPECT_EQ(feed(d, {kShift, 0, kCtrl, 0}), 0);
}

TEST(ToggleHotkeyDetectorTest, NoneNeverFires) {
  ToggleHotkeyDetector d(parseToggleHotkey("none"));
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift, kCtrl, 0}), 0);
}

TEST(ToggleHotkeyDetectorTest, OtherTargetsWork) {
  ToggleHotkeyDetector d(parseToggleHotkey("option+shift"));
  EXPECT_EQ(feed(d, {kCtrl, kCtrlShift, 0}), 0);
  EXPECT_EQ(feed(d, {kModOption, kModOption | kShift, kModOption, 0}), 1);
}

TEST(ToggleHotkeyDetectorTest, SetTargetMidGestureDropsState) {
  ToggleHotkeyDetector d;
  feed(d, {kCtrl, kCtrlShift});
  d.setTarget(kModControl | kModOption);
  EXPECT_EQ(feed(d, {kCtrl, 0}), 0);
}

TEST(ToggleHotkeyDetectorTest, ParseMapsKnownValuesAndFallsBack) {
  EXPECT_EQ(parseToggleHotkey("ctrl+shift"), kCtrlShift);
  EXPECT_EQ(parseToggleHotkey("option+shift"), kModOption | kModShift);
  EXPECT_EQ(parseToggleHotkey("ctrl+option"), kModControl | kModOption);
  EXPECT_EQ(parseToggleHotkey("none"), 0);
  EXPECT_EQ(parseToggleHotkey("weird"), kCtrlShift);
  EXPECT_EQ(parseToggleHotkey(""), kCtrlShift);
}

}  // namespace
}  // namespace goxvi_mac
