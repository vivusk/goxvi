#pragma once

#include <memory>
#include <string>

#include "goxvi/engine-types.h"

namespace goxvi {

// Telex state machine: Empty → Composing → Foreign | Literal.
// While Composing the raw key buffer is the source of truth and the display
// text is recomputed from it after every key. Foreign shows the raw buffer
// verbatim; Literal freezes the display text as the buffer (R2-L1).
class TelexEngine {
 public:
  explicit TelexEngine(EngineConfig cfg = {});
  ~TelexEngine();
  TelexEngine(const TelexEngine&) = delete;
  TelexEngine& operator=(const TelexEngine&) = delete;

  // Letter keys only (A-Z, a-z as typed). Anything else returns
  // consumed=false — the shim handles word-break/commit itself.
  // overflow=true: key NOT applied; commit current text, reset(), re-feed.
  KeyResult processKey(wchar_t ch);

  // false = buffer empty, shim should pass the backspace through.
  // Composing: pops raw keys until the display shrinks by exactly one char.
  // Foreign: pops one raw key; if the remaining buffer is fully valid
  // Vietnamese again, resumes Composing with the transformed word.
  // Literal: pops one display char, never resurrects a transform (M3).
  bool processBackspace(std::wstring& newDisplay);

  std::wstring currentDisplay() const;

  // Raw keys typed for the current word, verbatim with case — used by the
  // shim's Esc handling: cancel the Vietnamese transform and restore what
  // the user physically typed (UniKey convention). Empty when idle.
  std::wstring rawTypedKeys() const;

  // True while rawTypedKeys() faithfully mirrors what was typed. Goes false
  // after a backspace in Literal state (display pops 1:1 but raw tracking is
  // only best-effort there, so raw can drift from display); reset() restores
  // it. The shim only looks up gõ tắt while this is true (M1 / Decision 9) —
  // otherwise a drifted raw buffer could expand the wrong word.
  bool rawKeysExact() const;

  // Word break / focus change / caret move.
  void reset();

  void setConfig(EngineConfig cfg);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace goxvi
