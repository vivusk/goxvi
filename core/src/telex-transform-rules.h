#pragma once

#include <string>
#include <vector>

#include "tone-placement-rules.h"
#include "vietnamese-syllable-parser.h"

namespace goxvi::detail {

// Working word while Composing: transformed letters + pending tone.
// Recomputed/extended one raw key at a time; raw buffer stays source of truth.
struct WordState {
  std::vector<Letter> letters;
  Tone tone = Tone::None;
};

// Render a WordState to display text: place the tone mark on the right vowel
// (per toneStyle) and case each letter. strict must match the strictness used
// while the word was built so the tone-placement parse agrees (relaxed nucleus
// of a free-typed syllable — zaajy → zậy). Shared by the engine and the typo
// auto-corrector so tone placement stays single-sourced (DRY).
std::wstring renderWord(const WordState& w, ToneStyle toneStyle, bool strict);

enum class KeyOutcome {
  Applied,        // key consumed into the word (transform/tone/plain letter)
  UndoToLiteral,  // repeated modifier: transform reverted to its raw keys,
                  // trigger consumed — engine must switch to Literal (H1)
  Invalid,        // word can no longer be Vietnamese — engine → Foreign
};

// Apply one typed key (case preserved) to the word per Telex rules.
// On Invalid the letters may hold a garbage tail; the engine displays the raw
// buffer in Foreign mode and never reads the word again.
// strict=true (default) enforces Vietnamese spell-check; strict=false ("gõ dấu
// tự do") never returns Invalid — tones/diacritics apply to any syllable.
KeyOutcome applyKeyToWord(WordState& word, wchar_t typedKey, bool strict = true);

}  // namespace goxvi::detail
