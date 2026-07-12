#pragma once

#include "goxvi/engine-types.h"
#include "vietnamese-syllable-parser.h"

namespace goxvi::detail {

enum class Tone { None, Acute, Grave, Hook, Tilde, Dot };  // sắc huyền hỏi ngã nặng

// s f r x j → tone; anything else → Tone::None.
Tone toneForKey(wchar_t lowerKey);

// Index (into letters) of the vowel that carries the tone mark.
// Rules: last marked vowel (â ê ô ơ ă ư) wins (ươ → ơ); else with coda → last
// nucleus vowel; else 1 vowel → it; 2 vowels → old: first, new: last;
// 3 vowels → penultimate (both styles). Invalid parse → last vowel overall
// (tolerant fallback, only reachable on undo edge cases).
int tonePlacementIndex(const Letter* letters, int count, const SyllableParts& parts,
                       ToneStyle style);

// Precomposed vowel for base+tone+case, e.g. (ê, Dot, false) → ệ.
// Tone::None returns the plain (cased) vowel.
wchar_t vowelWithTone(wchar_t base, Tone tone, bool upper);

// Cased letter without tone; handles đ→Đ and marked vowels.
wchar_t letterChar(wchar_t base, bool upper);

}  // namespace goxvi::detail
