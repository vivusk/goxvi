#pragma once

#include <optional>
#include <string>

#include "goxvi/engine-types.h"
#include "goxvi/telex-engine.h"

// Pure C++ word-composition logic of the macOS shim (plan K2): no Cocoa, so it
// is unit-tested with GoogleTest. The .mm controller only translates NSEvent ->
// KeyKind and applies KeyOutcome to the IMK client. Mirrors the Windows shim
// (tsf/src/goxvi-text-service.cpp processLetter / commitCurrentWord /
// cancelWordToRaw / processBackspace).
namespace goxvi_mac {

enum class KeyKind {
  Letter,      // a-z A-Z (and 0-9 top row in VNI) -> engine
  Backspace,   // keyCode 51
  Escape,      // keyCode 53
  Terminator,  // space, Enter, punctuation, arrows, Tab, digits in Telex ...
  Chord,       // anything with Cmd/Ctrl/Option held
};

// The shim applies it in this order: commitText (insertText) -> marked text
// (setsMarkedText) -> return eaten to IMK.
struct KeyOutcome {
  bool eaten = false;
  std::optional<std::wstring> commitText;
  std::wstring markedText;  // empty = clear the marked text

  // Marked text is touched only when this key changed the composition. A bare
  // commit already consumed the marked range via insertText; clearing again
  // there (or on idle pass-through keys) could make clients replace the
  // current selection with "".
  bool setsMarkedText() const {
    return eaten && (!markedText.empty() || !commitText.has_value());
  }
};

class CompositionSession {
 public:
  explicit CompositionSession(goxvi::EngineConfig config = {});

  KeyOutcome onKey(KeyKind kind, wchar_t ch);

  // Commit for focus loss / click / app-ended composition (K8): returns exactly
  // the text on screen, never rewritten (no gõ tắt, no auto-correct); resets
  // the engine. nullopt when idle.
  std::optional<std::wstring> flush();

  bool composing() const;

  // The word as shown right now (marked text), without ending it.
  std::wstring display() const { return engine_.currentDisplay(); }

  // Reset without committing (the app already ended the composition itself).
  void abandon();

  // New config from the menu / settings (plan K5). Call only while idle so a
  // word never mixes two configs; the controller applies it at word start.
  void setConfig(goxvi::EngineConfig config);

  // Method the engine is configured with; the key classifier must use this one
  // (not ConfigStore) so a word never mixes Telex/VNI digit handling.
  goxvi::InputMethod inputMethod() const { return config_.inputMethod; }

 private:
  KeyOutcome onLetter(wchar_t ch);
  KeyOutcome onBackspace();
  KeyOutcome onEscape();
  // Ends the word: optional rewrite (Terminator/Chord only, K8), then reset.
  std::wstring finishWord(bool applyRewrites);

  goxvi::EngineConfig config_;
  goxvi::TelexEngine engine_;
};

}  // namespace goxvi_mac
