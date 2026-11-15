#include "composition-session.h"

#include <utility>

#include "goxvi/shortcut-expander.h"
#include "goxvi/typo-autocorrect.h"

namespace goxvi_mac {

CompositionSession::CompositionSession(goxvi::EngineConfig config)
    : config_(std::move(config)), engine_(config_) {}

bool CompositionSession::composing() const {
  return !engine_.currentDisplay().empty();
}

void CompositionSession::abandon() { engine_.reset(); }

void CompositionSession::setConfig(goxvi::EngineConfig config) {
  config_ = std::move(config);
  engine_.setConfig(config_);
  engine_.reset();
}

std::optional<std::wstring> CompositionSession::flush() {
  if (!composing()) {
    engine_.reset();
    return std::nullopt;
  }
  return finishWord(/*applyRewrites=*/false);
}

KeyOutcome CompositionSession::onKey(KeyKind kind, wchar_t ch) {
  switch (kind) {
    case KeyKind::Letter:
      return onLetter(ch);
    case KeyKind::Backspace:
      return onBackspace();
    case KeyKind::Escape:
      return onEscape();
    case KeyKind::Terminator:
    case KeyKind::Chord: {
      // K6: commit, then the key itself goes on to the app (never eaten).
      KeyOutcome out;
      if (composing()) out.commitText = finishWord(/*applyRewrites=*/true);
      return out;
    }
  }
  return {};
}

KeyOutcome CompositionSession::onLetter(wchar_t ch) {
  KeyOutcome out;
  goxvi::KeyResult result = engine_.processKey(ch);
  if (result.overflow) {
    // R2-M1: buffer full, key not applied -> commit the word as the Windows
    // shim does (commitCurrentWord default), restart with this key.
    out.commitText = finishWord(/*applyRewrites=*/true);
    result = engine_.processKey(ch);
  }
  if (!result.consumed) return out;  // eaten=false: let the key through
  out.eaten = true;
  out.markedText = result.display;
  return out;
}

KeyOutcome CompositionSession::onBackspace() {
  KeyOutcome out;
  if (!composing()) return out;  // idle: the app deletes its own text
  std::wstring display;
  if (!engine_.processBackspace(display)) return out;
  if (display.empty()) engine_.reset();  // word gone: start the next one clean
  out.eaten = true;
  out.markedText = display;
  return out;
}

KeyOutcome CompositionSession::onEscape() {
  KeyOutcome out;
  if (!composing()) return out;  // idle Esc belongs to the app
  // Mid-word Esc cancels the transform: the raw keys win, never expanded (C1).
  std::wstring raw = engine_.rawTypedKeys();
  if (raw.empty()) raw = engine_.currentDisplay();
  engine_.reset();
  out.eaten = true;
  out.commitText = std::move(raw);
  return out;
}

std::wstring CompositionSession::finishWord(bool applyRewrites) {
  std::optional<std::wstring> replacement;
  // Same priority as Windows: gõ tắt beats auto-correct; only while the raw
  // buffer mirrors what was typed (M1 / Decision 9).
  if (applyRewrites && engine_.rawKeysExact()) {
    const std::wstring raw = engine_.rawTypedKeys();
    if (config_.shortcutsEnabled) {
      replacement = goxvi::matchShortcut(raw, config_.shortcuts);
    }
    if (!replacement && config_.autoCorrectEnabled) {
      replacement = goxvi::autoCorrectTypo(raw, config_);
    }
  }
  std::wstring text = replacement ? std::move(*replacement) : engine_.currentDisplay();
  engine_.reset();
  return text;
}

}  // namespace goxvi_mac
