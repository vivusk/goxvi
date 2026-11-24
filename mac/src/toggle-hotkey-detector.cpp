#include "toggle-hotkey-detector.h"

namespace goxvi_mac {

ModifierMask parseToggleHotkey(const std::string& value) {
  if (value == "none") return 0;
  if (value == "option+shift") return kModOption | kModShift;
  if (value == "ctrl+option") return kModControl | kModOption;
  return kModControl | kModShift;  // "ctrl+shift" and anything unknown (K4)
}

ToggleHotkeyDetector::ToggleHotkeyDetector(ModifierMask target)
    : target_(target & kModAll) {}

void ToggleHotkeyDetector::setTarget(ModifierMask target) {
  target &= kModAll;
  if (target == target_) return;
  target_ = target;
  armed_ = false;
  tainted_ = held_ != 0;
}

bool ToggleHotkeyDetector::onFlagsChanged(ModifierMask held) {
  held &= kModAll;
  held_ = held;
  if (held == 0) tainted_ = false;
  if (target_ == 0) return false;  // "none"
  if ((held & ~target_) != 0) {     // a modifier outside the combo
    armed_ = false;
    tainted_ = true;
    return false;
  }
  if (held == target_) {
    if (!tainted_) armed_ = true;
    return false;
  }
  // held is now a proper subset of the target: first release of the combo.
  if (armed_) {
    armed_ = false;
    tainted_ = held != 0;  // the remaining release(s) must not re-arm
    return true;
  }
  return false;
}

void ToggleHotkeyDetector::onKeyDown() {
  armed_ = false;
  if (held_ != 0) tainted_ = true;
}

void ToggleHotkeyDetector::reset() {
  held_ = 0;
  armed_ = false;
  tainted_ = false;
}

}  // namespace goxvi_mac
