#pragma once

#include <cstddef>
#include <cstdint>

#include "composition-session.h"
#include "goxvi/engine-types.h"

namespace goxvi_mac {

// macOS virtual key codes (Carbon kVK_Delete / kVK_Escape), duplicated here so
// this file stays Cocoa-free and testable.
inline constexpr std::uint16_t kKeyCodeBackspace = 51;
inline constexpr std::uint16_t kKeyCodeEscape = 53;

// Pure mapping of a keyDown to a KeyKind. `ch`/`charCount` come from
// NSEvent.characters (Shift/CapsLock already applied by the layout);
// `hasCmdCtrlOpt` = any of Command/Control/Option held; `isNumpad` =
// NSEventModifierFlagNumericPad. Numpad digits always terminate, even in VNI —
// the Windows shim only takes top-row digits (tsf/src/key-event-handler.cpp).
KeyKind classifyKey(wchar_t ch, std::size_t charCount, std::uint16_t keyCode,
                    bool hasCmdCtrlOpt, bool isNumpad, goxvi::InputMethod method);

}  // namespace goxvi_mac
