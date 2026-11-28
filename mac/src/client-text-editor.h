#pragma once

#import <Foundation/Foundation.h>

#include <string>

#include "composition-session.h"
#include "text-edit.h"

// Stateless IMK client operations of the shim (plan 261115-1819 phase 3), kept
// out of goxvi-input-controller.mm. `client` is the IMKTextInput sender. None
// of these log anything (never typed text).

struct ClientSelection {
  bool readable = false;  // selectedRange.location != NSNotFound
  NSUInteger location = 0;
  NSUInteger length = 0;
};

// The app's caret / selection right now (read BEFORE each key, K12).
ClientSelection readSelection(id client);

// The app still shows Goxvi's marked text (Marked mode only).
bool clientHasMarkedText(id client);

// Direct mode: "replace edit.deleteCount characters before the caret with
// edit.insert". deleteCount == 0 is a plain insert (it replaces any selection
// like normal typing); otherwise the replaced range also covers the selection
// after the caret (inline autocomplete suggestion), as typing would. Caller
// guarantees deleteCount <= selection.location (the tracker desyncs otherwise).
void applyTextEdit(id client, const goxvi_mac::TextEdit& edit, const ClientSelection& selection);

// Marked mode (original D2 path): commitText via insertText, then marked text.
void applyMarkedOutcome(id client, const goxvi_mac::KeyOutcome& outcome);

// `text` is right before the caret (read back from the client).
bool clientTextBeforeCaretIs(id client, NSString* text, const ClientSelection& selection);

// Puts `word` before the caret unless it is already there (the app may have
// kept the dropped marked text); returns the caret after the word.
NSUInteger reinsertWord(id client, NSString* word, const ClientSelection& selection);

// Client bundle identifier; "" when the client does not tell (policy K13).
std::string clientBundleId(id client);
