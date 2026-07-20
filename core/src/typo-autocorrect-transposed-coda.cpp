// Rule: a final two-letter coda ("ng", "nh", "ch") typed with its letters
// swapped — the other half of the fast-typing transposition class (the first
// half is the modifier-before-vowel rule). "cuxgn" → cũng, "nhahn" → nhanh,
// "sahcs" → sách. Only the LAST such pair is considered, and only when
// everything after it is modifier keys (tone typed after the coda: "cugnx").
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "goxvi/engine-types.h"
#include "telex-transform-rules.h"
#include "typo-autocorrect-internal.h"

namespace goxvi::detail {
namespace {

// The typed pair is a swapped coda: "gn"/"hn" (→ ng/nh) or "hc" (→ ch).
bool isSwappedCodaPair(wchar_t a, wchar_t b) {
  if (b == L'n') return a == L'g' || a == L'h';
  return b == L'c' && a == L'h';
}

}  // namespace

std::optional<std::wstring> correctTransposedCodaNoBlocklist(
    std::wstring_view rawKeys, const EngineConfig& config) {
  const InputMethod method = config.inputMethod;

  // Only broken words are reconsidered (same contract as the other rule): a raw
  // string that already composes to a valid syllable is never touched.
  if (composeStrict(rawKeys, method)) return std::nullopt;

  const int count = static_cast<int>(rawKeys.size());
  // Right-to-left: the coda is at the end. i >= 1 — a coda always has a nucleus
  // in front of it, so the pair can never start the word ("gnaw" is safe).
  for (int i = count - 2; i >= 1; --i) {
    if (!isSwappedCodaPair(toLowerAscii(rawKeys[i]), toLowerAscii(rawKeys[i + 1]))) {
      continue;
    }

    // Whatever trails the pair must be modifier keys only — the coda is the last
    // thing in a syllable, a tone key after it is the only legal remainder.
    bool tailIsModifiers = true;
    for (int j = i + 2; j < count && tailIsModifiers; ++j) {
      tailIsModifiers = isMisplaceableModifier(toLowerAscii(rawKeys[j]), method);
    }
    if (!tailIsModifiers) continue;

    // Swap in place: each letter keeps its own case ("CUXGN" → "CUXNG").
    std::wstring corrected(rawKeys);
    std::swap(corrected[i], corrected[i + 1]);

    const std::optional<WordState> fixed = composeStrict(corrected, method);
    if (fixed) return renderWord(*fixed, config.toneStyle, /*strict=*/true);
  }
  return std::nullopt;
}

}  // namespace goxvi::detail
