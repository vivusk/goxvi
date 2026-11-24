#include "key-classifier.h"

namespace goxvi_mac {

KeyKind classifyKey(wchar_t ch, std::size_t charCount, std::uint16_t keyCode,
                    bool hasCmdCtrlOpt, bool isNumpad, goxvi::InputMethod method) {
  // App shortcuts (⌘C, ⌃A ...) and Option dead keys: commit, never eat.
  if (hasCmdCtrlOpt) return KeyKind::Chord;
  if (keyCode == kKeyCodeBackspace) return KeyKind::Backspace;
  if (keyCode == kKeyCodeEscape) return KeyKind::Escape;
  if (charCount == 1) {
    if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z')) return KeyKind::Letter;
    if (method == goxvi::InputMethod::Vni && !isNumpad && ch >= L'0' && ch <= L'9') {
      return KeyKind::Letter;
    }
  }
  // Space, Enter, Tab, punctuation, arrows, Telex digits, numpad, dead keys ...
  return KeyKind::Terminator;
}

}  // namespace goxvi_mac
