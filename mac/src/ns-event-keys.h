#pragma once

#import <AppKit/AppKit.h>

#include "goxvi/engine-types.h"
#include "key-classifier.h"
#include "toggle-hotkey-detector.h"

// NSEvent -> pure C++ key types for the controller (split out of
// goxvi-input-controller.mm to keep it < 200 lines). Never logs typed text.

// Only ⌃ ⇧ ⌥ ⌘ matter; CapsLock / Fn / numpad bits are dropped.
goxvi_mac::ModifierMask modifierMask(NSEventModifierFlags flags);

// NSEvent -> KeyKind (+ the single UTF-16 unit typed, 0 otherwise).
goxvi_mac::KeyKind keyKindOf(NSEvent* event, goxvi::InputMethod method, wchar_t* ch);
