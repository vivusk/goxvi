#import "goxvi-input-controller.h"

#include <memory>

#include "composition-session.h"
#include "config-store.h"
#include "key-classifier.h"
#include "wide-nsstring.h"

using goxvi_mac::CompositionSession;
using goxvi_mac::KeyOutcome;

// One CompositionSession per controller, i.e. per IMK client (plan phase 3).
// Every callback runs on the main thread. Never logs typed text.
@implementation GoxviInputController {
  std::unique_ptr<CompositionSession> _session;
}

- (instancetype)initWithServer:(IMKServer*)server delegate:(id)delegate client:(id)inputClient {
  self = [super initWithServer:server delegate:delegate client:inputClient];
  if (self) {
    _session = std::make_unique<CompositionSession>(ConfigStore::instance().current());
  }
  return self;
}

// FlagsChanged is requested now so the modifier-only toggle hotkey (PDR D6,
// phase 4) needs no plist/registration change later.
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

- (BOOL)handleEvent:(NSEvent*)event client:(id)sender {
  if (event.type != NSEventTypeKeyDown) return NO;  // flagsChanged: phase 4
  try {
    [self dropStaleComposition:sender];
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

- (void)deactivateServer:(id)sender {
  [self finishComposition:sender];
  [super deactivateServer:sender];
}

@end
