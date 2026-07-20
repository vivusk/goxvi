#pragma once

#include <string>
#include <vector>

namespace goxvi {

// Buffer cap (R2-M1): longest Vietnamese word is ~12 keys; Foreign mode keeps
// composing raw until word end, so long foreign words/URLs could grow without
// bound. When full, processKey reports overflow so the shim commits + resets.
inline constexpr int kMaxRawKeys = 32;

// Per-expansion cap for gõ tắt (settings UI limits the edit box; the parser
// skips oversized entries). Generous for paragraph templates while keeping
// config.json far below kMaxConfigFileBytes.
inline constexpr int kMaxShortcutExpansionChars = 1000;

// Single shared cap for reading %APPDATA%\Goxvi\config.json. The TIP and the
// settings app MUST agree: a config the settings app can write but the TIP
// refuses to read would silently freeze the TIP on its previous config.
inline constexpr long long kMaxConfigFileBytes = 1024 * 1024;

enum class ToneStyle {
  Old,  // hòa, thúy (default)
  New,  // hoà, thuý
};

enum class InputMethod {
  Telex,  // default: letter-triggered (aa, aw, dd, s/f/r/x/j ...)
  Vni,    // digit-triggered (a6, a8, d9, 1-5 ...)
};

// One gõ tắt entry: typing `trigger` (all-lowercase raw keys) and ending the
// word replaces the whole word with `expansion` (free text, may hold accented
// Vietnamese). Both are UTF-16 (converted from UTF-8 JSON by the serializer).
struct ShortcutEntry {
  std::wstring trigger;
  std::wstring expansion;
};

struct EngineConfig {
  ToneStyle toneStyle = ToneStyle::Old;
  InputMethod inputMethod = InputMethod::Telex;
  bool shortcutsEnabled = true;  // gõ tắt master switch (empty list → no-op)
  // Spell-check auto-restore. When a keystroke turns the syllable invalid the
  // engine (by default) reverts the whole word to the raw keys typed, dropping
  // the diacritics — the UniKey convention. Off: keep the accented word as-is
  // and freeze it (Literal) so later keys append verbatim, i.e. the user is
  // allowed to end up with a "misspelt" but still-accented word.
  bool restoreOnInvalid = true;
  // Typo auto-correction: at word commit, a broken word whose diacritic/tone key
  // landed just before its vowel (fast-typed twosi/twsoi for tới) is rewritten
  // to the intended Vietnamese word. Commit-time only — never disturbs live
  // typing. See core/src/typo-autocorrect.cpp.
  bool autoCorrectEnabled = true;
  std::vector<ShortcutEntry> shortcuts;  // later entry wins on duplicate trigger
};

struct KeyResult {
  // false → shim should pass the key through (only for keys the engine does
  // not handle at all; a transition to Foreign is still consumed=true with
  // display = raw buffer).
  bool consumed = false;
  // Buffer full (kMaxRawKeys): the key was NOT applied. Shim must commit the
  // current composition, reset the engine, then feed this key again (R2-M1).
  bool overflow = false;
  // Current composition text (valid when consumed && !overflow).
  std::wstring display;
};

}  // namespace goxvi
