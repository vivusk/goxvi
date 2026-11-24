#pragma once

#include <cstddef>
#include <vector>

#include "goxvi/engine-types.h"

// Gõ tắt validation of the macOS Settings window (phase 5), pure C++ (K2).
// Same limits as the core parser (config-json-serializer.cpp) but stricter
// than the Windows settings app: nothing is silently fixed up (no lowercasing,
// no stripping, no "later duplicate wins") — a bad entry is rejected with a
// reason and never saved.
namespace goxvi_mac {

enum class ShortcutError {
  None,
  TriggerEmpty,
  TriggerInvalidChar,  // anything outside a-z (uppercase, digits, space, ...)
  TriggerTooLong,      // > goxvi::kMaxRawKeys UTF-16 units
  ExpansionEmpty,
  ExpansionTooLong,    // > goxvi::kMaxShortcutExpansionChars UTF-16 units
  DuplicateTrigger,
};

// Checks one entry on its own (no duplicate check).
ShortcutError validateShortcut(const goxvi::ShortcutEntry& entry);

// Checks `candidate` as the entry at `index` of `list` (index >= list.size()
// = a new entry): validateShortcut() plus "no other entry has this trigger".
ShortcutError validateShortcutAt(const std::vector<goxvi::ShortcutEntry>& list,
                                 std::size_t index,
                                 const goxvi::ShortcutEntry& candidate);

}  // namespace goxvi_mac
