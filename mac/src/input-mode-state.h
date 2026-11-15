#pragma once

#include "toggle-hotkey-detector.h"

// Process-wide Việt/Anh state of Goxvi.app (PDR D6). Starts in Vietnamese at
// every launch and is never persisted (plan: out of scope). Holds the single
// toggle-hotkey detector next to it: every client has its own controller, but
// modifier state is global. Main thread only (IMK callbacks).
struct InputModeState {
  bool vietnameseOn = true;
  goxvi_mac::ToggleHotkeyDetector hotkey;

  static InputModeState& instance() {
    static InputModeState state;
    return state;
  }
};
