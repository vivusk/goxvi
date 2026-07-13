#pragma once

#include "telex-transform-rules.h"

namespace goxvi::detail {

// Apply one typed key (case preserved for letters; digits carry no case) to
// the word per VNI rules: letters are always plain (no letter-triggered
// transforms), digits 1-9 apply tones/diacritics —
//   1 sắc, 2 huyền, 3 hỏi, 4 ngã, 5 nặng, 6 â/ê/ô, 7 ơ/ư (+uo→ươ), 8 ă, 9 đ.
// Same WordState/KeyOutcome contract as the Telex rules (telex-transform-rules.h).
// strict=false ("gõ dấu tự do") never returns Invalid — tones/diacritics apply
// to any syllable (zaajy → zậy).
KeyOutcome applyKeyToWordVni(WordState& word, wchar_t typedKey,
                             bool strict = true);

}  // namespace goxvi::detail
