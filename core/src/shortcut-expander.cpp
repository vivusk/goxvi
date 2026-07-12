#include "goxvi/shortcut-expander.h"

namespace goxvi {
namespace {

// A trigger can only match a whole raw word that is entirely lowercase a-z.
// Empty or any character outside a-z (uppercase, digit, mark, space) → can
// never match, so bail early. This is the absolute case rule: TRC/Trc/tRc do
// not expand.
bool isAllLowerAscii(const std::wstring& word) {
  if (word.empty()) return false;
  for (const wchar_t c : word) {
    if (c < L'a' || c > L'z') return false;
  }
  return true;
}

}  // namespace

std::optional<std::wstring> matchShortcut(
    const std::wstring& rawWord, const std::vector<ShortcutEntry>& shortcuts) {
  if (!isAllLowerAscii(rawWord)) return std::nullopt;
  // Reverse scan so a later entry wins when triggers collide (Decision 6).
  for (auto it = shortcuts.rbegin(); it != shortcuts.rend(); ++it) {
    if (it->trigger == rawWord) return it->expansion;
  }
  return std::nullopt;
}

}  // namespace goxvi
