#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "composition-session.h"
#include "direct-insert-tracker.h"

// Test helper: a fake client document + caret driven the way the Direct shim
// will drive the real client (plan phase 3): read caret -> isStale -> onKey ->
// onOutcome -> apply edit -> if not eaten, the app handles the key itself.
namespace goxvi_mac::testing {

struct FakeScreen {
  explicit FakeScreen(goxvi::EngineConfig config = {}) : session(std::move(config)) {}

  CompositionSession session;
  DirectInsertTracker tracker;
  std::wstring text;        // whole document
  std::size_t caret = 0;    // UTF-16 offset (K1)
  std::vector<TextEdit> edits;  // every command sent to the client
  bool lastEaten = false;

  void press(KeyKind kind, wchar_t ch = 0) {
    if (tracker.isStale(caret, 0)) {
      session.abandon();
      tracker.reset();
    }
    const KeyOutcome outcome = session.onKey(kind, ch);
    const DirectStep step = tracker.onOutcome(kind, outcome, caret);
    if (step.edit) apply(*step.edit);
    if (tracker.desynced()) {
      session.abandon();
      tracker.reset();
    }
    lastEaten = step.eaten;
    if (!step.eaten) appHandles(kind, ch);
  }

  void type(std::wstring_view keys) {
    for (wchar_t c : keys) press(KeyKind::Letter, c);
  }

  void apply(const TextEdit& edit) {
    edits.push_back(edit);
    text.replace(caret - edit.deleteCount, edit.deleteCount, edit.insert);
    caret = caret - edit.deleteCount + edit.insert.size();
  }

  // What a client does with a key Goxvi did not eat.
  void appHandles(KeyKind kind, wchar_t ch) {
    if (kind == KeyKind::Backspace) {
      if (caret > 0) text.erase(--caret, 1);
    } else if (kind == KeyKind::Letter || kind == KeyKind::Terminator) {
      if (ch != 0) text.insert(caret++, 1, ch);
    }
  }

  // User clicks elsewhere / the app moves the caret.
  void moveCaret(std::size_t to) { caret = to; }
};

}  // namespace goxvi_mac::testing
