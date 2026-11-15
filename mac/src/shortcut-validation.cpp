#include "shortcut-validation.h"

namespace goxvi_mac {

ShortcutError validateShortcut(const goxvi::ShortcutEntry& entry) {
  if (entry.trigger.empty()) return ShortcutError::TriggerEmpty;
  for (wchar_t c : entry.trigger) {
    if (c < L'a' || c > L'z') return ShortcutError::TriggerInvalidChar;
  }
  // K1: wstring elements are UTF-16 units, so size() is the unit count.
  if (entry.trigger.size() > static_cast<std::size_t>(goxvi::kMaxRawKeys)) {
    return ShortcutError::TriggerTooLong;
  }
  if (entry.expansion.empty()) return ShortcutError::ExpansionEmpty;
  if (entry.expansion.size() >
      static_cast<std::size_t>(goxvi::kMaxShortcutExpansionChars)) {
    return ShortcutError::ExpansionTooLong;
  }
  return ShortcutError::None;
}

ShortcutError validateShortcutAt(const std::vector<goxvi::ShortcutEntry>& list,
                                 std::size_t index,
                                 const goxvi::ShortcutEntry& candidate) {
  const ShortcutError error = validateShortcut(candidate);
  if (error != ShortcutError::None) return error;
  for (std::size_t i = 0; i < list.size(); ++i) {
    if (i != index && list[i].trigger == candidate.trigger) {
      return ShortcutError::DuplicateTrigger;
    }
  }
  return ShortcutError::None;
}

}  // namespace goxvi_mac
