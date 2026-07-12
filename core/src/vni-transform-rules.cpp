#include "vni-transform-rules.h"

namespace goxvi::detail {
namespace {

SyllableParts parse(const WordState& w) {
  return parseSyllable(w.letters.data(), static_cast<int>(w.letters.size()));
}

Tone toneForDigit(wchar_t k) {
  switch (k) {
    case L'1': return Tone::Acute;
    case L'2': return Tone::Grave;
    case L'3': return Tone::Hook;
    case L'4': return Tone::Tilde;
    case L'5': return Tone::Dot;
    default: return Tone::None;
  }
}

KeyOutcome appendPlainLetter(WordState& w, wchar_t k, bool upper) {
  w.letters.push_back({k, upper});
  return parse(w).valid ? KeyOutcome::Applied : KeyOutcome::Invalid;
}

wchar_t circumflexOf(wchar_t base) {
  return base == L'a' ? kACirc : base == L'e' ? kECirc : base == L'o' ? kOCirc : 0;
}

wchar_t circumflexBase(wchar_t marked) {
  return marked == kACirc ? L'a' : marked == kECirc ? L'e' : L'o';
}

// a6/e6/o6 → â/ê/ô; repeated 6 → undo (â → a + literal "6").
KeyOutcome applyCircumflex(WordState& w, wchar_t k, bool upper, const SyllableParts& p) {
  const int start = p.onsetLen;
  const int end = start + p.nucleusLen;
  for (int i = end - 1; i >= start; --i) {
    const wchar_t b = w.letters[i].base;
    if (b == kACirc || b == kECirc || b == kOCirc) {
      w.letters[i].base = circumflexBase(b);
      w.letters.insert(w.letters.begin() + i + 1, Letter{k, upper});
      return KeyOutcome::UndoToLiteral;
    }
  }
  for (int i = end - 1; i >= start; --i) {
    const wchar_t b = w.letters[i].base;
    const wchar_t circ = circumflexOf(b);
    if (circ != 0) {
      w.letters[i].base = circ;
      if (parse(w).valid) return KeyOutcome::Applied;
      w.letters[i].base = b;
    }
  }
  return appendPlainLetter(w, k, upper);
}

wchar_t hornOf(wchar_t base) { return base == L'o' ? kOHorn : kUHorn; }
wchar_t hornBase(wchar_t marked) { return marked == kOHorn ? L'o' : L'u'; }

// o7/u7 → ơ/ư, uo7 → ươ; repeated 7 → undo.
KeyOutcome applyHorn(WordState& w, wchar_t k, bool upper, const SyllableParts& p) {
  const int start = p.onsetLen;
  const int end = start + p.nucleusLen;
  for (int i = end - 1; i >= start; --i) {
    const wchar_t b = w.letters[i].base;
    if (b == L'o' && i > start && w.letters[i - 1].base == L'u') {
      w.letters[i - 1].base = kUHorn;
      w.letters[i].base = kOHorn;
      if (parse(w).valid) return KeyOutcome::Applied;
      w.letters[i - 1].base = L'u';
      w.letters[i].base = L'o';
    }
    if (b == L'o' || b == L'u') {
      w.letters[i].base = hornOf(b);
      if (parse(w).valid) return KeyOutcome::Applied;
      w.letters[i].base = b;
    }
  }
  for (int i = end - 1; i >= start; --i) {
    const wchar_t b = w.letters[i].base;
    if (b == kOHorn && i > start && w.letters[i - 1].base == kUHorn) {
      w.letters[i - 1].base = L'u';
      w.letters[i].base = L'o';
      w.letters.insert(w.letters.begin() + i + 1, Letter{k, upper});
      return KeyOutcome::UndoToLiteral;
    }
    if (b == kOHorn || b == kUHorn) {
      w.letters[i].base = hornBase(b);
      w.letters.insert(w.letters.begin() + i + 1, Letter{k, upper});
      return KeyOutcome::UndoToLiteral;
    }
  }
  return appendPlainLetter(w, k, upper);
}

// a8 → ă; repeated 8 → undo.
KeyOutcome applyBreve(WordState& w, wchar_t k, bool upper, const SyllableParts& p) {
  const int start = p.onsetLen;
  const int end = start + p.nucleusLen;
  for (int i = end - 1; i >= start; --i) {
    if (w.letters[i].base == kABreve) {
      w.letters[i].base = L'a';
      w.letters.insert(w.letters.begin() + i + 1, Letter{k, upper});
      return KeyOutcome::UndoToLiteral;
    }
  }
  for (int i = end - 1; i >= start; --i) {
    if (w.letters[i].base == L'a') {
      w.letters[i].base = kABreve;
      if (parse(w).valid) return KeyOutcome::Applied;
      w.letters[i].base = L'a';
    }
  }
  return appendPlainLetter(w, k, upper);
}

// d9 → đ; repeated 9 → undo.
KeyOutcome applyDStroke(WordState& w, wchar_t k, bool upper, const SyllableParts& p) {
  if (p.onsetLen == 1 && w.letters[0].base == L'd') {
    w.letters[0].base = kDStroke;
    return KeyOutcome::Applied;
  }
  if (p.onsetLen == 1 && w.letters[0].base == kDStroke) {
    w.letters[0].base = L'd';
    w.letters.insert(w.letters.begin() + 1, Letter{k, upper});
    return KeyOutcome::UndoToLiteral;
  }
  return appendPlainLetter(w, k, upper);
}

}  // namespace

KeyOutcome applyKeyToWordVni(WordState& w, wchar_t typedKey) {
  const bool upper = typedKey >= L'A' && typedKey <= L'Z';
  const wchar_t k = upper ? typedKey - L'A' + L'a' : typedKey;
  const SyllableParts p = parse(w);

  if (k < L'0' || k > L'9') {  // letters: always plain, except ư + o auto-horn
    // ư + o → ươ: typing o right after a horned u (u7 → ư) auto-completes the
    // diphthong, mirroring the Telex path (nư + o → nươ).
    if (k == L'o' && p.nucleusLen > 0 &&
        w.letters[p.onsetLen + p.nucleusLen - 1].base == kUHorn) {
      w.letters.push_back({kOHorn, upper});
      if (parse(w).valid) return KeyOutcome::Applied;
      w.letters.pop_back();
    }
    return appendPlainLetter(w, k, upper);
  }

  const Tone t = toneForDigit(k);
  if (t != Tone::None && p.nucleusLen > 0) {  // 1 2 3 4 5 after a vowel = tone
    if (w.tone == t) {  // repeated tone digit: undo + literal (la1 1 → la1)
      w.tone = Tone::None;
      w.letters.push_back({k, upper});
      return KeyOutcome::UndoToLiteral;
    }
    w.tone = t;
    return KeyOutcome::Applied;
  }

  switch (k) {
    case L'6': return applyCircumflex(w, k, upper, p);
    case L'7': return applyHorn(w, k, upper, p);
    case L'8': return applyBreve(w, k, upper, p);
    case L'9': return applyDStroke(w, k, upper, p);
    default: return appendPlainLetter(w, k, upper);  // '0' or tone digit, no nucleus
  }
}

}  // namespace goxvi::detail
