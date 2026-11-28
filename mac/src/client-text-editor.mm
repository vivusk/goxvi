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
void applyTextEdit(id client, const goxvi_mac::TextEdit& edit, const ClientSelection& selection) {
  const NSRange range =
      edit.deleteCount == 0
          ? noReplacement()
          : NSMakeRange(selection.location - edit.deleteCount,
                        edit.deleteCount + selection.length);
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

bool clientTextBeforeCaretIs(id client, NSString* text, const ClientSelection& selection) {
  const NSUInteger length = text.length;
  if (!selection.readable || length == 0 || selection.location < length) return false;
  NSAttributedString* before =
      [client attributedSubstringFromRange:NSMakeRange(selection.location - length, length)];
  return [before.string isEqualToString:text];
}

NSUInteger reinsertWord(id client, NSString* word, const ClientSelection& selection) {
  if (clientTextBeforeCaretIs(client, word, selection)) return selection.location;
  [client insertText:word replacementRange:noReplacement()];
  return selection.location + word.length;
}

std::string clientBundleId(id client) {
  if (![client respondsToSelector:@selector(bundleIdentifier)]) return "";
  NSString* bundle = [client bundleIdentifier];
  return bundle.length > 0 ? std::string(bundle.UTF8String) : std::string();
}
