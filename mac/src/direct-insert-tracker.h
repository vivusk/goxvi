#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "composition-session.h"
#include "text-edit.h"

// Pure C++ (plan K2/K10): turns a CompositionSession KeyOutcome into one
// direct-insert command (no marked text, composition hidden like UniKey / the
// Windows direct mode) and tracks where the caret should be, so the shim can
// spot an orphaned word (click, app cleared the field — K12).
//
// Compatibility measurements 2026-11 (plan 261115-1819 phase 1 decisions):
// - Clients ignore insertText:@"" with a replacementRange (delete-only), so a
//   command NEVER has an empty insert. ⌫ that only drops the last character
//   passes through to the app instead; any other pure delete is widened by one
//   kept character so it becomes a replace (del>0, ins>0 works everywhere).
// - Read-back of the inserted text is stale in Chromium/VSCode, so staleness
//   is judged only from selectedRange read BEFORE the next key (isStale).
namespace goxvi_mac {

// What the shim does with one key in Direct mode.
struct DirectStep {
  std::optional<TextEdit> edit;  // nullopt = do not touch the client
  bool eaten = false;            // value returned to IMK (NO = key goes to the app)
};

class DirectInsertTracker {
 public:
  // Characters of the current word are on screen (a word is being typed).
  bool tracking() const { return !onScreen_.empty(); }

  // Before each key: true when the app's caret is no longer at the end of the
  // word being typed (or text is selected) -> shim abandons the word (K12).
  bool isStale(std::size_t caretLocation, std::size_t selectionLength) const;

  // After session.onKey(kind, ...). caretLocation = selectedRange.location
  // read before the key. kind tells a ⌫ (may pass through) from other keys.
  DirectStep onOutcome(KeyKind kind, const KeyOutcome& outcome, std::size_t caretLocation);

  // flush / abandon / mode switch: forget the word and the desync flag.
  void reset();

  // True after onOutcome needed to delete more characters than exist before
  // the caret; the shim must abandon() the session, then reset().
  bool desynced() const { return desynced_; }

 private:
  std::wstring onScreen_;  // UTF-16 units of the word as the app shows it (K1)
  std::size_t expectedCaret_ = 0;
  bool desynced_ = false;
};

}  // namespace goxvi_mac
