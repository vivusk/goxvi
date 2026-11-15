#import "goxvi-input-controller-private.h"

#include <cstdint>
#include <memory>

#include "composition-session.h"
#include "config-store.h"
#include "client-text-editor.h"
#include "direct-insert-tracker.h"
#include "input-mode-state.h"
#include "insert-mode-policy.h"
#include "key-classifier.h"
#include "wide-nsstring.h"

using goxvi_mac::CompositionSession;
using goxvi_mac::InsertMode;
using goxvi_mac::KeyOutcome;

// One CompositionSession per controller, i.e. per IMK client (plan phase 3).
// Việt/Anh state + toggle hotkey are process-wide (InputModeState, phase 4).
// Insert mode (plan 261115-1819 K10-K13): Direct = text goes straight into the
// document via insertText:replacementRange: (hidden composition, Enter after a
// word is never swallowed — K11); Marked = original setMarkedText path, kept
// for clients that ignore replacementRange (terminals). Chosen per word.
// Every callback runs on the main thread. Never logs typed text.
@implementation GoxviInputController {
  std::unique_ptr<CompositionSession> _session;
  std::uint64_t _configRevision;  // ConfigStore revision the session runs on
  goxvi_mac::DirectInsertTracker _tracker;  // Direct mode only
  // Marked until the first key so a finishComposition: before it takes the
  // original path (plan phase 3 Design).
  InsertMode _mode;
}

- (instancetype)initWithServer:(IMKServer*)server delegate:(id)delegate client:(id)inputClient {
  self = [super initWithServer:server delegate:delegate client:inputClient];
  if (self) {
    _session = std::make_unique<CompositionSession>(ConfigStore::instance().current());
    _configRevision = ConfigStore::instance().revision();
    _mode = InsertMode::Marked;
  }
  return self;
}

// FlagsChanged feeds the modifier-only toggle hotkey (PDR D6).
- (NSUInteger)recognizedEvents:(id)sender {
  (void)sender;
  return NSEventMaskKeyDown | NSEventMaskFlagsChanged;
}

// The app may end the composition on its own (click, ⌘-shortcut it handled,
// field change) without telling us; resync so the next key starts clean.
- (void)dropStaleComposition:(id)client {
  if (_session->composing() && !clientHasMarkedText(client)) _session->abandon();
}

// Menu / settings changed the config: adopt it at a word boundary only (K5).
- (void)syncConfigIfIdle {
  ConfigStore& store = ConfigStore::instance();
  if (_configRevision == store.revision() || _session->composing()) return;
  _session->setConfig(store.current());
  _configRevision = store.revision();
}

// Only ⌃ ⇧ ⌥ ⌘ matter; CapsLock / Fn / numpad bits are dropped.
static goxvi_mac::ModifierMask modifierMask(NSEventModifierFlags flags) {
  goxvi_mac::ModifierMask mask = 0;
  if (flags & NSEventModifierFlagControl) mask |= goxvi_mac::kModControl;
  if (flags & NSEventModifierFlagShift) mask |= goxvi_mac::kModShift;
  if (flags & NSEventModifierFlagOption) mask |= goxvi_mac::kModOption;
  if (flags & NSEventModifierFlagCommand) mask |= goxvi_mac::kModCommand;
  return mask;
}

// NSEvent -> KeyKind (+ the single UTF-16 unit typed, 0 otherwise).
static goxvi_mac::KeyKind keyKindOf(NSEvent* event, goxvi::InputMethod method, wchar_t* ch) {
  NSString* chars = event.characters ?: @"";
  const NSEventModifierFlags flags = event.modifierFlags;
  const bool chord = (flags & (NSEventModifierFlagCommand | NSEventModifierFlagControl |
                               NSEventModifierFlagOption)) != 0;
  const bool numpad = (flags & NSEventModifierFlagNumericPad) != 0;
  *ch = chars.length == 1 ? static_cast<wchar_t>([chars characterAtIndex:0]) : 0;
  return goxvi_mac::classifyKey(*ch, chars.length, event.keyCode, chord, numpad, method);
}

// Mid-word toggle commits exactly what is shown first (flush, K8).
- (void)toggleVietnamese:(id)client {
  [self finishComposition:client];
  InputModeState& mode = InputModeState::instance();
  mode.vietnameseOn = !mode.vietnameseOn;
}

- (void)onFlagsChanged:(NSEvent*)event client:(id)sender {
  goxvi_mac::ToggleHotkeyDetector& hotkey = InputModeState::instance().hotkey;
  hotkey.setTarget(goxvi_mac::parseToggleHotkey(ConfigStore::instance().current().toggleHotkey));
  if (hotkey.onFlagsChanged(modifierMask(event.modifierFlags))) [self toggleVietnamese:sender];
}

- (BOOL)handleEvent:(NSEvent*)event client:(id)sender {
  try {
    if (event.type == NSEventTypeFlagsChanged) {
      [self onFlagsChanged:event client:sender];
      return NO;  // modifiers always reach the app
    }
    if (event.type != NSEventTypeKeyDown) return NO;
    InputModeState::instance().hotkey.onKeyDown();
    if (!InputModeState::instance().vietnameseOn) {
      // English: every key goes straight through, no composition.
      if (_session->composing()) [self finishComposition:sender];
      return NO;
    }
    return [self handleVietnameseKey:event client:sender] ? YES : NO;
  } catch (...) {
    // Never break typing in the host app: drop the word, pass the key.
    [self forgetWord];
    return NO;
  }
}

- (void)forgetWord {
  _session->abandon();
  _tracker.reset();
}

// Word start: pick the insert mode for this client, kept to the word's end.
- (void)beginWord:(id)client selection:(const ClientSelection&)selection {
  _tracker.reset();
  _mode = goxvi_mac::chooseInsertMode(clientBundleId(client), selection.readable);
  [self syncConfigIfIdle];
}

- (bool)handleVietnameseKey:(NSEvent*)event client:(id)sender {
  const ClientSelection sel = readSelection(sender);
  if (_session->composing()) {
    if (_mode == InsertMode::Marked) {
      [self dropStaleComposition:sender];
    } else if (!sel.readable || _tracker.isStale(sel.location, sel.length)) {
      // Click / caret moved / app cleared the field (K12): the word is
      // already in the document, keep it and start a new one.
      [self forgetWord];
    }
  }
  if (!_session->composing()) [self beginWord:sender selection:sel];
  wchar_t ch = 0;
  const goxvi_mac::KeyKind kind = keyKindOf(event, _session->inputMethod(), &ch);
  const KeyOutcome outcome = _session->onKey(kind, ch);
  if (_mode == InsertMode::Marked) {
    applyMarkedOutcome(sender, outcome);
    return outcome.eaten;
  }
  // step.eaten, not outcome.eaten: a ⌫ dropping the last character must
  // reach the app (clients ignore delete-only inserts — measured 2026-11).
  const goxvi_mac::DirectStep step = _tracker.onOutcome(kind, outcome, sel.location);
  if (step.edit) applyTextEdit(sender, *step.edit, sel.location);
  if (_tracker.desynced()) [self forgetWord];
  return step.eaten;
}

// Focus change / app request / toggle / menu (K8). Direct: the word is
// already in the document, nothing to commit — never touches the client.
// Marked: commit exactly what is shown; if the app already ended the marked
// text itself it kept the characters — inserting again would duplicate the
// word, so only resync then.
- (void)finishComposition:(id)client {
  try {
    if (_mode == InsertMode::Direct) {
      [self forgetWord];
      return;
    }
    if (!clientHasMarkedText(client)) {
      _session->abandon();
      return;
    }
    if (auto text = _session->flush()) {
      [client insertText:toNSString(*text) replacementRange:NSMakeRange(NSNotFound, 0)];
    }
  } catch (...) {
    [self forgetWord];
  }
}

- (void)commitComposition:(id)sender {
  [self finishComposition:sender];
}

// Focus came back: modifier releases may have happened outside Goxvi, so the
// process-wide detector starts clean (code-findings F1).
- (void)activateServer:(id)sender {
  [super activateServer:sender];
  InputModeState::instance().hotkey.reset();
}

- (void)deactivateServer:(id)sender {
  [self finishComposition:sender];
  [super deactivateServer:sender];
}

@end
