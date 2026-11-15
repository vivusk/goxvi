#import "client-text-editor.h"

#import <InputMethodKit/InputMethodKit.h>

#include "wide-nsstring.h"

namespace {

NSRange noReplacement() { return NSMakeRange(NSNotFound, 0); }

}  // namespace

ClientSelection readSelection(id client) {
  const NSRange range = [client selectedRange];
  ClientSelection selection;
  selection.readable = range.location != NSNotFound;
  if (selection.readable) {
    selection.location = range.location;
    selection.length = range.length;
  }
  return selection;
}

bool clientHasMarkedText(id client) {
  const NSRange range = [client markedRange];
  return range.location != NSNotFound && range.length > 0;
}

// No read-back after the insert: Chromium / VSCode return the old text right
// after insertText (measured 2026-11, plan 261115-1819), so it is never trusted.
void applyTextEdit(id client, const goxvi_mac::TextEdit& edit, NSUInteger caretLocation) {
  const NSRange range = edit.deleteCount == 0
                            ? noReplacement()
                            : NSMakeRange(caretLocation - edit.deleteCount, edit.deleteCount);
  [client insertText:toNSString(edit.insert) replacementRange:range];
}

void applyMarkedOutcome(id client, const goxvi_mac::KeyOutcome& outcome) {
  if (outcome.commitText) {
    [client insertText:toNSString(*outcome.commitText) replacementRange:noReplacement()];
  }
  if (outcome.setsMarkedText()) {
    NSString* marked = toNSString(outcome.markedText);
    [client setMarkedText:marked
           selectionRange:NSMakeRange(marked.length, 0)
         replacementRange:noReplacement()];
  }
}

std::string clientBundleId(id client) {
  if (![client respondsToSelector:@selector(bundleIdentifier)]) return "";
  NSString* bundle = [client bundleIdentifier];
  return bundle.length > 0 ? std::string(bundle.UTF8String) : std::string();
}
