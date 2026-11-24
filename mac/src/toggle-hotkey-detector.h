#pragma once

#include <cstdint>
#include <string>

// Modifier-only Việt/Anh toggle hotkey (PDR D6, plan K4), pure C++ so it is
// unit-tested (K2). The .mm controller maps NSEvent modifier flags to
// ModifierMask and feeds flagsChanged/keyDown here. One process-wide instance
// (InputModeState): modifier state belongs to the whole system, not a client.
namespace goxvi_mac {

using ModifierMask = std::uint8_t;
// Only ⌃ ⇧ ⌥ ⌘ count; CapsLock / Fn are ignored by the caller's mapping.
inline constexpr ModifierMask kModControl = 1 << 0;
inline constexpr ModifierMask kModShift = 1 << 1;
inline constexpr ModifierMask kModOption = 1 << 2;
inline constexpr ModifierMask kModCommand = 1 << 3;
inline constexpr ModifierMask kModAll = kModControl | kModShift | kModOption | kModCommand;

// "ctrl+shift" | "option+shift" | "ctrl+option" | "none" (-> 0 = never fires).
// Unknown strings fall back to ctrl+shift (K4).
ModifierMask parseToggleHotkey(const std::string& value);

class ToggleHotkeyDetector {
 public:
  explicit ToggleHotkeyDetector(ModifierMask target = kModControl | kModShift);

  // Changing the target drops any half-pressed state.
  void setTarget(ModifierMask target);
  ModifierMask target() const { return target_; }

  // Current held modifiers after a flagsChanged event. Returns true exactly
  // once per press-release of the full target combo with nothing else
  // pressed in between (fires on the first release, like Ctrl+Shift on Win).
  bool onFlagsChanged(ModifierMask held);

  // Any non-modifier key while modifiers are held cancels the gesture
  // (⌃⇧+arrow selection shortcuts must not toggle).
  void onKeyDown();

  // Drops all half-pressed state. Called on controller activation: a
  // modifier release seen by another app/context must not leave the
  // detector tainted and swallow the next toggle (code-findings F1).
  void reset();

 private:
  ModifierMask target_;
  ModifierMask held_ = 0;
  bool armed_ = false;
  // Set by a foreign modifier or a key press; cleared once all modifiers are
  // up, so e.g. ⌃⇧⌘ then releasing ⌘ cannot re-arm the same gesture.
  bool tainted_ = false;
};

}  // namespace goxvi_mac
