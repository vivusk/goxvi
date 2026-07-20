#include "telex-transform-rules.h"

namespace goxvi::detail {
namespace {

SyllableParts parse(const WordState& w, bool strict) {
  return parseSyllable(w.letters.data(), static_cast<int>(w.letters.size()),
                       strict);
}

wchar_t circumflexOf(wchar_t k) {
  return k == L'a' ? kACirc : k == L'e' ? kECirc : kOCirc;
}

wchar_t hornOf(wchar_t b) {  // w-transform targets
  return b == L'a' ? kABreve : b == L'o' ? kOHorn : kUHorn;
}

wchar_t hornBase(wchar_t b) {
  return b == kABreve ? L'a' : b == kOHorn ? L'o' : L'u';
}

KeyOutcome appendPlainLetter(WordState& w, wchar_t k, bool upper, bool strict) {
  w.letters.push_back({k, upper});
  return parse(w, strict).valid ? KeyOutcome::Applied : KeyOutcome::Invalid;
}

// aa→â, ee→ê, oo→ô; repeated → undo (â → "aa" + Literal). Scans the nucleus
// from the end; a transform that breaks nucleus validity is skipped.
KeyOutcome applyDoubling(WordState& w, wchar_t k, bool upper,
                         const SyllableParts& p, bool strict) {
  const wchar_t circ = circumflexOf(k);
  const int start = p.onsetLen;
  // ư + o → ươ: typing a plain o right after a horned u auto-completes the
  // diphthong (nư + o → nươ), same result as the uo + w path but one key short.
  if (k == L'o' && p.nucleusLen > 0 &&
      w.letters[start + p.nucleusLen - 1].base == kUHorn) {
    w.letters.push_back({kOHorn, upper});
    if (parse(w, strict).valid) return KeyOutcome::Applied;
    w.letters.pop_back();  // ươ-coda invalid here → fall through to plain o
  }
  for (int i = start + p.nucleusLen - 1; i >= start; --i) {
    Letter& letter = w.letters[i];
    if (letter.base == circ) {  // undo: â → a + restore the swallowed key
      letter.base = k;
      w.letters.insert(w.letters.begin() + i + 1, Letter{k, upper});
      return KeyOutcome::UndoToLiteral;
    }
    if (letter.base == k) {
      letter.base = circ;
      if (parse(w, strict).valid) return KeyOutcome::Applied;
      letter.base = k;  // e.g. "ao" + o → "aô" invalid; keep scanning
    }
  }
  return appendPlainLetter(w, k, upper, strict);
}

// w: uo→ươ, a→ă, o→ơ, u→ư; repeated → undo. Simple Telex: a standalone w
// (empty nucleus) is NOT ư — it stays a plain letter so English words like
// "web"/"wow" never morph mid-typing.
KeyOutcome applyHorn(WordState& w, bool upper, const SyllableParts& p,
                     bool strict) {
  const int start = p.onsetLen;
  const int end = start + p.nucleusLen;
  for (int i = end - 1; i >= start; --i) {  // transform scan
    const wchar_t b = w.letters[i].base;
    if (b == L'o' && i > start && w.letters[i - 1].base == L'u') {
      w.letters[i - 1].base = kUHorn;  // uo → ươ (thuowng → thương)
      w.letters[i].base = kOHorn;
      if (parse(w, strict).valid) return KeyOutcome::Applied;
      w.letters[i - 1].base = L'u';
      w.letters[i].base = L'o';
    }
    if (b == L'a' || b == L'o' || b == L'u') {
      w.letters[i].base = hornOf(b);
      if (parse(w, strict).valid) return KeyOutcome::Applied;
      w.letters[i].base = b;  // e.g. "uu" + w: "uư" invalid → try first u
    }
  }
  for (int i = end - 1; i >= start; --i) {  // undo scan
    const wchar_t b = w.letters[i].base;
    if (b == kOHorn && i > start && w.letters[i - 1].base == kUHorn) {
      w.letters[i - 1].base = L'u';  // ươ → "uo" + w
      w.letters[i].base = L'o';
      w.letters.insert(w.letters.begin() + i + 1, Letter{L'w', upper});
      return KeyOutcome::UndoToLiteral;
    }
    if (b == kABreve || b == kOHorn || b == kUHorn) {
      w.letters[i].base = hornBase(b);
      w.letters.insert(w.letters.begin() + i + 1, Letter{L'w', upper});
      return KeyOutcome::UndoToLiteral;
    }
  }
  // Strict: a lone w has no nucleus to horn → Invalid → Foreign. Relaxed: w
  // stays a plain onset letter so tones still apply later (was → wá).
  return appendPlainLetter(w, L'w', upper, strict);
}

KeyOutcome applyDKey(WordState& w, bool upper, const SyllableParts& p,
                     bool strict) {
  if (p.onsetLen == 1 && w.letters[0].base == L'd') {  // dd → đ (also "did"→đi)
    w.letters[0].base = kDStroke;
    return KeyOutcome::Applied;
  }
  if (p.onsetLen == 1 && w.letters[0].base == kDStroke) {  // ddd → "dd"
    w.letters[0].base = L'd';
    w.letters.insert(w.letters.begin() + 1, Letter{L'd', upper});
    return KeyOutcome::UndoToLiteral;
  }
  return appendPlainLetter(w, L'd', upper, strict);
}

}  // namespace

std::wstring renderWord(const WordState& w, ToneStyle toneStyle, bool strict) {
  const int n = static_cast<int>(w.letters.size());
  std::wstring out;
  out.reserve(n);
  int toneIdx = -1;
  if (w.tone != Tone::None) {
    const SyllableParts parts = parseSyllable(w.letters.data(), n, strict);
    toneIdx = tonePlacementIndex(w.letters.data(), n, parts, toneStyle);
  }
  for (int i = 0; i < n; ++i) {
    const Letter& l = w.letters[i];
    out.push_back(i == toneIdx ? vowelWithTone(l.base, w.tone, l.upper)
                               : letterChar(l.base, l.upper));
  }
  return out;
}

KeyOutcome applyKeyToWord(WordState& w, wchar_t typedKey, bool strict) {
  const bool upper = typedKey >= L'A' && typedKey <= L'Z';
  const wchar_t k = upper ? typedKey - L'A' + L'a' : typedKey;
  const SyllableParts p = parse(w, strict);

  if (k == L'z' && w.tone != Tone::None) {  // z clears the tone (lasz → la)
    w.tone = Tone::None;
    return KeyOutcome::Applied;
  }

  const Tone t = toneForKey(k);
  if (t != Tone::None && p.nucleusLen > 0) {  // s f r x j after a vowel = tone
    if (w.tone == t) {  // repeated tone key: undo + literal (cass → cas)
      w.tone = Tone::None;
      w.letters.push_back({k, upper});
      return KeyOutcome::UndoToLiteral;
    }
    w.tone = t;
    return KeyOutcome::Applied;
  }

  if (k == L'a' || k == L'e' || k == L'o')
    return applyDoubling(w, k, upper, p, strict);
  if (k == L'w') return applyHorn(w, upper, p, strict);
  if (k == L'd') return applyDKey(w, upper, p, strict);

  return appendPlainLetter(w, k, upper, strict);
}

}  // namespace goxvi::detail
