#pragma once

#include <optional>
#include <string>
#include <vector>

#include "goxvi/engine-types.h"

// Gõ tắt (shortcut/macro expansion) match rule — pure, no Telex/VNI knowledge,
// unit-testable like the rest of goxvi-core. Public header (core/include/): the
// tsf layer includes it; core/src/ is internal-only.
namespace goxvi {

// rawWord: raw keys typed for the word, verbatim case. Returns the expansion
// iff rawWord is entirely lowercase a-z AND matches exactly one trigger
// (whole word, last matching entry wins). Returns std::nullopt otherwise —
// including any uppercase character (absolute case rule, no normalization).
std::optional<std::wstring> matchShortcut(
    const std::wstring& rawWord, const std::vector<ShortcutEntry>& shortcuts);

}  // namespace goxvi
