#import "direct-insert-spike.h"

#import <InputMethodKit/InputMethodKit.h>

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <sys/time.h>

#include "text-edit.h"
#include "wide-nsstring.h"

namespace {

NSString* const kSpikeDefaultsKey = @"GoxviSpikeDirectInsert";

// Missing directory / unwritable file -> silently skip (never break typing).
void appendLogLine(const char* format, ...) {
  NSArray<NSString*>* dirs = NSSearchPathForDirectoriesInDomains(
      NSApplicationSupportDirectory, NSUserDomainMask, YES);
  NSString* dir = [dirs.firstObject stringByAppendingPathComponent:@"Goxvi"];
  if (dir == nil) return;
  [[NSFileManager defaultManager] createDirectoryAtPath:dir
                            withIntermediateDirectories:YES
                                             attributes:nil
                                                  error:nil];
  NSString* file = [dir stringByAppendingPathComponent:@"spike-log.txt"];
  FILE* out = std::fopen(file.fileSystemRepresentation, "a");
  if (out == nullptr) return;
  timeval now{};
  gettimeofday(&now, nullptr);
  std::tm local{};
  localtime_r(&now.tv_sec, &local);
  char stamp[32];
  std::strftime(stamp, sizeof stamp, "%y%m%d-%H%M%S", &local);
  std::fprintf(out, "%s.%03d ", stamp, static_cast<int>(now.tv_usec / 1000));
  va_list args;
  va_start(args, format);
  std::vfprintf(out, format, args);
  va_end(args);
  std::fputc('\n', out);
  std::fclose(out);
}

// Bundle IDs are app identifiers, not typed content.
const char* bundleOf(id client) {
  NSString* bundle = [client respondsToSelector:@selector(bundleIdentifier)]
                         ? [client bundleIdentifier]
                         : nil;
  return bundle.length > 0 ? bundle.UTF8String : "?";
}

// NSNotFound prints as -1 so awk can compare fields as plain strings.
long loc(NSUInteger value) { return value == NSNotFound ? -1 : static_cast<long>(value); }

// Reads back the inserted range; compares in memory, logs only the verdict.
const char* verifyInsert(id client, NSUInteger start, const std::wstring& insert) {
  if (insert.empty() || start == NSNotFound) return "unreadable";  // nothing to read back
  NSString* expected = toNSString(insert);
  NSAttributedString* actual =
      [client attributedSubstringFromRange:NSMakeRange(start, expected.length)];
  if (actual == nil) return "unreadable";
  return [actual.string isEqualToString:expected] ? "ok" : "mismatch";
}

}  // namespace

bool spikeDirectInsertEnabled() {
  return [[NSUserDefaults standardUserDefaults] boolForKey:kSpikeDefaultsKey];
}

void setSpikeDirectInsertEnabled(bool enabled) {
  [[NSUserDefaults standardUserDefaults] setBool:enabled forKey:kSpikeDefaultsKey];
}

void spikeLogProbe(id client) {
  const NSRange sel = [client selectedRange];
  const bool selValid = sel.location != NSNotFound;
  // One character before the caret (empty range at the document start).
  const NSUInteger len = selValid && sel.location > 0 ? 1 : 0;
  NSAttributedString* substr =
      selValid ? [client attributedSubstringFromRange:NSMakeRange(sel.location - len, len)] : nil;
  NSArray* attrs = [client validAttributesForMarkedText];
  appendLogLine("bundle=%s ev=probe selValid=%d substr=%s validAttrs=%lu", bundleOf(client),
                selValid ? 1 : 0, substr != nil ? "ok" : "nil",
                static_cast<unsigned long>(attrs.count));
}

void spikeLogFinish(id client, const char* src, bool composing) {
  const NSRange sel = [client selectedRange];
  appendLogLine("bundle=%s ev=finish src=%s composing=%d sel=%ld,%lu", bundleOf(client), src,
                composing ? 1 : 0, loc(sel.location), static_cast<unsigned long>(sel.length));
}

void DirectInsertSpike::apply(const goxvi_mac::KeyOutcome& outcome, id client) {
  if (!outcome.commitText && !outcome.setsMarkedText()) return;  // idle pass-through
  const std::wstring target = outcome.commitText.value_or(L"") +
                              (outcome.setsMarkedText() ? outcome.markedText : L"");
  const goxvi_mac::TextEdit edit = goxvi_mac::diffTextEdit(onScreen, target);
  const int cont = onScreen.empty() ? 0 : 1;
  onScreen = outcome.setsMarkedText() ? outcome.markedText : L"";
  if (edit.empty()) return;  // K11: nothing to change, the key goes straight on

  const NSRange pre = [client selectedRange];
  const NSUInteger del = edit.deleteCount;
  const NSUInteger ins = toNSString(edit.insert).length;
  if (del > 0 && (pre.location == NSNotFound || pre.location < del)) {
    appendLogLine("bundle=%s ev=edit cont=%d pre=%ld,%lu del=%lu ins=%lu post=-1,0 expect=-1 "
                  "verify=skip",
                  bundleOf(client), cont, loc(pre.location),
                  static_cast<unsigned long>(pre.length), static_cast<unsigned long>(del),
                  static_cast<unsigned long>(ins));
    return;
  }
  const NSRange replace =
      del == 0 ? NSMakeRange(NSNotFound, 0) : NSMakeRange(pre.location - del, del);
  [client insertText:toNSString(edit.insert) replacementRange:replace];

  const NSRange post = [client selectedRange];
  const NSUInteger start = pre.location == NSNotFound ? NSNotFound : pre.location - del;
  const long expect = start == NSNotFound ? -1 : static_cast<long>(start + ins);
  const char* verify = verifyInsert(client, start, edit.insert);
  appendLogLine("bundle=%s ev=edit cont=%d pre=%ld,%lu del=%lu ins=%lu post=%ld,%lu expect=%ld "
                "verify=%s",
                bundleOf(client), cont, loc(pre.location), static_cast<unsigned long>(pre.length),
                static_cast<unsigned long>(del), static_cast<unsigned long>(ins),
                loc(post.location), static_cast<unsigned long>(post.length), expect, verify);
}
