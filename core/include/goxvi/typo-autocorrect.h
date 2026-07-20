#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "goxvi/engine-types.h"

// Typo auto-correction for fast-typing transpositions (two keys swapped). Pure,
// no Windows API, unit-testable like the rest of goxvi-core. Applied ONCE at
// word commit (not live) by the tsf shim; the engine keeps showing the raw keys
// while composing. Two rules, first match wins:
//   1. modifier before the vowel it modifies — twosi/twsoi for towsi ("tới"),
//   2. final coda ng/nh typed as gn/hn — cuxgn ("cũng"), nhahn ("nhanh").
namespace goxvi {

// rawKeys: the raw keys typed for the word, verbatim case (letters, plus VNI
// digits). Returns the corrected display text iff ALL of:
//   * rawKeys does NOT already form a valid Vietnamese syllable (only broken
//     words are reconsidered — a valid word is never touched),
//   * one of the two rules applies:
//     - modifier keys (Telex w s f r x j / VNI 1-8) sit directly before the
//       first vowel, wedged between the onset and the nucleus. (Telex) that
//       wedged run must contain a 'w' — the horn/breve diacritic — because
//       Telex's tone keys s f r x j double as English cluster consonants (br,
//       cr, pr ...), so a horn anchor avoids rewriting bra→bả, pray→..., etc.
//       VNI needs no anchor: its modifiers are digits, never in English words.
//       Peeling them must leave a NON-EMPTY complete onset (so "she" — h before
//       e — and any real-consonant change is rejected),
//     - a "gn"/"hn" pair sits at the end of the word (only modifier keys may
//       trail it) and is not word-initial,
//   * the rewritten key sequence composes to a valid Vietnamese syllable,
//   * rawKeys (lowercased) is not in the hard-coded English blocklist.
// Otherwise std::nullopt (leave the raw word as-is).
std::optional<std::wstring> autoCorrectTypo(std::wstring_view rawKeys,
                                            const EngineConfig& config);

}  // namespace goxvi
