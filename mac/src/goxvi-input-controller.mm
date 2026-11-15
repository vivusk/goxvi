#import "goxvi-input-controller.h"

#include <cstdint>
#include <memory>
#include <utility>

#import "input-menu.h"
#import "settings-window-controller.h"

#include "composition-session.h"
#include "config-store.h"
#include "input-mode-state.h"
#include "key-classifier.h"
#include "wide-nsstring.h"

using goxvi_mac::CompositionSession;
using goxvi_mac::KeyOutcome;

// One CompositionSession per controller, i.e. per IMK client (plan phase 3).
// Việt/Anh state + toggle hotkey are process-wide (InputModeState, phase 4).
// Every callback runs on the main thread. Never logs typed text.
@implementation GoxviInputController {
  std::unique_ptr<CompositionSession> _session;
  std::uint64_t _configRevision;  // ConfigStore revision the session runs on
}

- (instancetype)initWithServer:(IMKServer*)server delegate:(id)delegate client:(id)inputClient {
  self = [super initWithServer:server delegate:delegate client:inputClient];
  if (self) {
    _session = std::make_unique<CompositionSession>(ConfigStore::instance().current());
    _configRevision = ConfigStore::instance().revision();
  }
  return self;
}

// FlagsChanged feeds the modifier-only toggle hotkey (PDR D6).
- (NSUInteger)recognizedEvents:(id)sender {
  (void)sender;
  return NSEventMaskKeyDown | NSEventMaskFlagsChanged;
}

static bool clientHasMarkedText(id client) {
  const NSRange range = [client markedRange];
  return range.location != NSNotFound && range.length > 0;
}

// The app may end the composition on its own (click, ⌘-shortcut it handled,
// field change) without telling us; resync so the next key starts clean.
- (void)dropStaleComposition:(id)client {
  if (_session->composing() && !clientHasMarkedText(client)) _session->abandon();
}

- (void)apply:(const KeyOutcome&)outcome client:(id)client {
  const NSRange noReplace = NSMakeRange(NSNotFound, 0);
  if (outcome.commitText) {
    [client insertText:toNSString(*outcome.commitText) replacementRange:noReplace];
  }
  if (outcome.setsMarkedText()) {
    NSString* marked = toNSString(outcome.markedText);
    [client setMarkedText:marked
           selectionRange:NSMakeRange(marked.length, 0)
         replacementRange:noReplace];
  }
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
    [self dropStaleComposition:sender];
    [self syncConfigIfIdle];
    NSString* chars = event.characters ?: @"";
    const NSEventModifierFlags flags = event.modifierFlags;
    const bool chord = (flags & (NSEventModifierFlagCommand | NSEventModifierFlagControl |
                                 NSEventModifierFlagOption)) != 0;
    const bool numpad = (flags & NSEventModifierFlagNumericPad) != 0;
    const wchar_t ch = chars.length == 1 ? static_cast<wchar_t>([chars characterAtIndex:0]) : 0;
    const goxvi_mac::KeyKind kind = goxvi_mac::classifyKey(
        ch, chars.length, event.keyCode, chord, numpad, _session->inputMethod());
    const KeyOutcome outcome = _session->onKey(kind, ch);
    [self apply:outcome client:sender];
    return outcome.eaten ? YES : NO;
  } catch (...) {
    // Never break typing in the host app: drop the word, pass the key.
    _session->abandon();
    return NO;
  }
}

// Click / focus change / app request (K8): commit exactly what is shown. If the
// app already ended the marked text itself it kept the characters — inserting
// again would duplicate the word, so only resync then.
- (void)finishComposition:(id)client {
  try {
    if (!clientHasMarkedText(client)) {
      _session->abandon();
      return;
    }
    if (auto text = _session->flush()) {
      [client insertText:toNSString(*text) replacementRange:NSMakeRange(NSNotFound, 0)];
    }
  } catch (...) {
    _session->abandon();
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

- (NSMenu*)menu {
  return buildInputMenu(@selector(menuAction:), InputModeState::instance().vietnameseOn,
                        ConfigStore::instance().current().inputMethod);
}

// Telex/VNI from the menu: apply now + persist to config.json (D7, D8).
- (void)selectInputMethod:(goxvi::InputMethod)method {
  ConfigStore& store = ConfigStore::instance();
  if (store.current().inputMethod == method) return;
  [self finishComposition:self.client];
  goxvi::EngineConfig config = store.current();
  config.inputMethod = method;
  store.update(std::move(config));  // failure already logged; memory still updated
  [self syncConfigIfIdle];
}

- (void)menuAction:(id)sender {
  try {
    switch (inputMenuTagFromSender(sender)) {
      case GoxviMenuTagToggleVietnamese:
        [self toggleVietnamese:self.client];
        break;
      case GoxviMenuTagTelex:
        [self selectInputMethod:goxvi::InputMethod::Telex];
        break;
      case GoxviMenuTagVni:
        [self selectInputMethod:goxvi::InputMethod::Vni];
        break;
      case GoxviMenuTagSettings:
        [self finishComposition:self.client];  // the window takes the focus
        [GoxviSettingsWindowController showSettings];
        break;
      default:
        break;
    }
  } catch (...) {
    _session->abandon();
  }
}

@end
