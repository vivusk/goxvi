// Rule: a modifier key (Telex w s f r x j / VNI 1-8) typed BEFORE the vowel it
// should modify — the fast-typing transposition twosi/twsoi for towsi ("tới").
#include <optional>
#include <string>
#include <string_view>

#include "goxvi/engine-types.h"
#include "telex-transform-rules.h"
#include "typo-autocorrect-internal.h"
#include "vietnamese-syllable-parser.h"

namespace goxvi::detail {

std::optional<std::wstring> correctMisplacedModifierNoBlocklist(
    std::wstring_view rawKeys, const EngineConfig& config) {
  const InputMethod method = config.inputMethod;

  // Only broken words are reconsidered: a raw string that already composes to a
  // valid Vietnamese syllable is left exactly as the user typed it.
  if (composeStrict(rawKeys, method)) return std::nullopt;

  const int count = static_cast<int>(rawKeys.size());

  // First vowel: the nucleus start. Needs at least one consonant before it,
  // otherwise there is no onset gap for a modifier to be wedged into.
  int firstV = -1;
  for (int i = 0; i < count; ++i) {
    if (isVowelBase(toLowerAscii(rawKeys[i]))) {
      firstV = i;
      break;
    }
  }
  if (firstV <= 0) return std::nullopt;

  // Peel modifier keys off the RIGHT end of the pre-vowel run (adjacent to the
  // vowel). "she": the char before 'e' is 'h' (not a modifier) → nothing peeled.
  int cut = firstV;
  while (cut > 0 && isMisplaceableModifier(toLowerAscii(rawKeys[cut - 1]), method)) {
    --cut;
  }
  if (cut == firstV) return std::nullopt;  // no misplaced modifier found

  // Telex only: the peeled run MUST contain a 'w' (the horn/breve diacritic).
  // Telex's other tone keys — s f r x j — double as English cluster consonants
  // (br, cr, dr, pr, gr, tr, thr, ps ...), so peeling a lone 'r'/'s' would
  // rewrite bra→bả, cram→cẩm, pray→..., throw→... — an unbounded false-positive
  // class no blocklist can cover. 'w' is never a Vietnamese consonant, so a 'w'
  // wedged before the vowel is almost always a misplaced horn; adjacent tone
  // keys ride along (twsoi → tới). Every real user example carries a 'w'.
  // VNI needs no such gate: its modifiers are DIGITS, absent from English words.
  if (method == InputMethod::Telex) {
    bool hasHorn = false;
    for (int i = cut; i < firstV; ++i) {
      if (toLowerAscii(rawKeys[i]) == L'w') { hasHorn = true; break; }
    }
    if (!hasHorn) return std::nullopt;
  }

  // What stays in front must be a NON-EMPTY complete onset. Non-empty: only fix
  // when a real onset consonant precedes the misplaced modifier — this forbids
  // altering the initial consonant(s) ("she" would need 's' pulled from before
  // 'h') and drops the empty-onset false-positive class (was→ắ, swap→ắp, and
  // garbage like a leading modifier being spuriously peeled).
  if (cut == 0 || !isCompleteOnset(toLowerAscii(rawKeys.substr(0, cut)))) {
    return std::nullopt;
  }

  // Rebuild: onset + vowel run + peeled modifiers (original order/case) + rest.
  int vowelEnd = firstV;
  while (vowelEnd < count && isVowelBase(toLowerAscii(rawKeys[vowelEnd]))) {
    ++vowelEnd;
  }
  std::wstring corrected;
  corrected.reserve(rawKeys.size());
  corrected += rawKeys.substr(0, cut);                     // onset
  corrected += rawKeys.substr(firstV, vowelEnd - firstV);  // vowel run
  corrected += rawKeys.substr(cut, firstV - cut);          // peeled modifiers
  corrected += rawKeys.substr(vowelEnd);                   // coda / remainder

  const std::optional<WordState> fixed = composeStrict(corrected, method);
  if (!fixed) return std::nullopt;

  return renderWord(*fixed, config.toneStyle, /*strict=*/true);
}

}  // namespace goxvi::detail
