#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "goxvi/engine-types.h"
#include "telex-transform-rules.h"

// Internal seam for the typo auto-corrector: the individual rewrite rules and
// their shared helpers, WITHOUT the English blocklist filter. The public
// autoCorrectTypo() wraps correctTypoNoBlocklist() and then rejects blocklisted
// words. The blocklist generator (a test) uses the no-blocklist seam directly to
// decide which English words WOULD be rewritten — running them through the
// public function instead would already exclude the current blocklist and shrink
// it on every regeneration.
namespace goxvi::detail {

// ---- shared helpers (defined in typo-autocorrect.cpp) ----------------------

wchar_t toLowerAscii(wchar_t c);
std::wstring toLowerAscii(std::wstring_view s);

// Modifier keys that affect the vowel: Telex horn/breve w + tone s f r x j,
// VNI tone 1-5 + circumflex/horn/breve 6 7 8. NOT 'd'/9 (đ modifies a
// consonant) nor vowel-doubling a/e/o (those need the vowel present).
bool isMisplaceableModifier(wchar_t lower, InputMethod method);

// Replay raw keys through the strict transform. Returns the built word iff every
// key was cleanly Applied AND the final syllable is valid (a real, complete
// Vietnamese word — not a Foreign/Literal outcome).
std::optional<WordState> composeStrict(std::wstring_view raw, InputMethod method);

// ---- rewrite rules ---------------------------------------------------------

// Modifier key typed BEFORE the vowel it modifies (twosi -> tới).
std::optional<std::wstring> correctMisplacedModifierNoBlocklist(
    std::wstring_view rawKeys, const EngineConfig& config);

// Final coda ng/nh typed transposed as gn/hn (cuxgn -> cũng, nhahn -> nhanh).
std::optional<std::wstring> correctTransposedCodaNoBlocklist(
    std::wstring_view rawKeys, const EngineConfig& config);

// All rules, first match wins.
std::optional<std::wstring> correctTypoNoBlocklist(std::wstring_view rawKeys,
                                                   const EngineConfig& config);

}  // namespace goxvi::detail
