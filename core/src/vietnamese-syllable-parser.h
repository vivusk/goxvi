#pragma once

// Structural parser for (prefixes of) Vietnamese syllables: onset + nucleus +
// coda. Operates on transformed base letters (a â ă đ ư ...), tone excluded.
// "Prefix-valid" means the letters could still grow into a real syllable —
// the engine drops to Foreign mode the moment this stops holding.

namespace goxvi::detail {

// Marked (diacritic-carrying, toneless) vowel code points, lowercase.
inline constexpr wchar_t kABreve = L'ă';   // ă
inline constexpr wchar_t kACirc = L'â';    // â
inline constexpr wchar_t kECirc = L'ê';    // ê
inline constexpr wchar_t kOCirc = L'ô';    // ô
inline constexpr wchar_t kOHorn = L'ơ';    // ơ
inline constexpr wchar_t kUHorn = L'ư';    // ư
inline constexpr wchar_t kDStroke = L'đ';  // đ

struct Letter {
  wchar_t base = 0;    // lowercase base char, may carry a diacritic, no tone
  bool upper = false;  // render as uppercase
};

struct SyllableParts {
  bool valid = false;  // structural prefix-validity
  int onsetLen = 0;    // letters[0 .. onsetLen)
  int nucleusLen = 0;  // letters[onsetLen .. onsetLen+nucleusLen)
  int codaLen = 0;     // letters[onsetLen+nucleusLen .. )
};

// a ă â e ê i o ô ơ u ư y
bool isVowelBase(wchar_t base);

// ă â ê ô ơ ư — vowels that attract the tone mark.
bool isMarkedVowel(wchar_t base);

// strict=true (default): reject anything that is not a prefix-valid Vietnamese
// syllable (valid=false → engine drops to Foreign / raw keys). strict=false
// ("gõ dấu tự do"): split onset/nucleus/coda structurally but skip the
// membership checks, so tones/diacritics still land on the vowel of a
// non-Vietnamese syllable (was → wá, zaajy → zậy). valid is then always true
// for any letters that have a vowel (or a lone consonant onset).
SyllableParts parseSyllable(const Letter* letters, int count, bool strict = true);

}  // namespace goxvi::detail
