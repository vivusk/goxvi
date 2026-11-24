#include "direct-insert-tracker.h"

#include <utility>

namespace goxvi_mac {
namespace {

bool isLowSurrogate(wchar_t unit) { return unit >= 0xDC00 && unit <= 0xDFFF; }

// Units of the last code point of a non-empty text (2 for a surrogate pair,
// possible only in a gõ tắt expansion — K1), so widening never splits a pair.
std::size_t lastCodePointSize(const std::wstring& text) {
  return text.size() >= 2 && isLowSurrogate(text.back()) ? 2 : 1;
}

}  // namespace

// A selection starting at the expected caret is an inline autocomplete
// suggestion (Chrome omnibox): the word is intact before it, so keep typing —
// applyTextEdit replaces the suggestion together with the word tail.
bool DirectInsertTracker::isStale(std::size_t caretLocation) const {
  return tracking() && caretLocation != expectedCaret_;
}

void DirectInsertTracker::reset() {
  onScreen_.clear();
  expectedCaret_ = 0;
  desynced_ = false;
}

void DirectInsertTracker::adopt(std::wstring onScreen, std::size_t caretLocation) {
  reset();
  onScreen_ = std::move(onScreen);
  expectedCaret_ = caretLocation;
}

DirectStep DirectInsertTracker::onOutcome(KeyKind kind, const KeyOutcome& outcome,
                                          std::size_t caretLocation) {
  DirectStep step;
  step.eaten = outcome.eaten;
  // Idle pass-through key: nothing changed, the client is not touched.
  if (!outcome.commitText && !outcome.setsMarkedText()) return step;

  const std::wstring target = outcome.commitText.value_or(L"") +
                              (outcome.setsMarkedText() ? outcome.markedText : L"");
  std::wstring nextOnScreen = outcome.setsMarkedText() ? outcome.markedText : L"";
  TextEdit edit = diffTextEdit(onScreen_, target);

  // Delete-only commands are ignored by clients (measured 2026-11).
  bool passBackspace = false;
  if (edit.insert.empty() && edit.deleteCount > 0) {
    if (kind == KeyKind::Backspace && edit.deleteCount == 1) {
      passBackspace = true;  // the app deletes that one character itself
    } else if (!target.empty()) {
      // target is a prefix of onScreen_: also replace its last kept character.
      const std::size_t keep = lastCodePointSize(target);
      edit.deleteCount += keep;
      edit.insert = target.substr(target.size() - keep);
    } else {
      // Not expressible (whole multi-character word gone, no replacement
      // text): stop tracking, leave the screen as is; ⌫ still reaches the app.
      onScreen_.clear();
      if (kind == KeyKind::Backspace) step.eaten = false;
      return step;
    }
  }

  // Checked before any subtraction: never a negative range / size_t wrap.
  if (edit.deleteCount > caretLocation) {
    onScreen_.clear();
    desynced_ = true;
    step.eaten = false;  // the word is abandoned: the key acts as without Goxvi
    return step;
  }

  onScreen_ = std::move(nextOnScreen);
  if (passBackspace) {
    expectedCaret_ = caretLocation - 1;
    step.eaten = false;
    return step;
  }
  expectedCaret_ = caretLocation - edit.deleteCount + edit.insert.size();
  // K11: empty edit (Enter / space after a plain word) -> the key goes straight on.
  if (!edit.empty()) step.edit = std::move(edit);
  return step;
}

}  // namespace goxvi_mac
