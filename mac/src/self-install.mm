#import "self-install.h"

#import <Cocoa/Cocoa.h>

namespace {

NSString* const kBundleId = @"com.goxvi.inputmethod.Goxvi";
NSString* const kKeyboardSettingsUrl =
    @"x-apple.systempreferences:com.apple.Keyboard-Settings.extension";

NSURL* destinationUrl() {
  NSURL* lib = [[NSFileManager defaultManager] URLsForDirectory:NSLibraryDirectory
                                                      inDomains:NSUserDomainMask]
                   .firstObject;
  return [[lib URLByAppendingPathComponent:@"Input Methods" isDirectory:YES]
      URLByAppendingPathComponent:@"Goxvi.app"
                      isDirectory:YES];
}

// An older Goxvi IMK server keeps the old binary mapped; stop it so macOS
// launches the new copy next time the source is selected.
void stopRunningServers() {
  const pid_t self = [NSProcessInfo processInfo].processIdentifier;
  for (NSRunningApplication* app in
       [NSRunningApplication runningApplicationsWithBundleIdentifier:kBundleId]) {
    if (app.processIdentifier != self) [app forceTerminate];
  }
}

// Runs the installed copy with one argument; returns its exit status.
int runInstalled(NSURL* app, NSString* arg) {
  NSURL* exe = [app URLByAppendingPathComponent:@"Contents/MacOS/Goxvi"];
  NSError* error = nil;
  NSTask* task = [NSTask launchedTaskWithExecutableURL:exe
                                             arguments:@[ arg ]
                                                 error:&error
                                    terminationHandler:nil];
  if (!task) return -1;
  [task waitUntilExit];
  return task.terminationStatus;
}

NSString* copyBundle(NSURL* from, NSURL* to) {
  NSFileManager* fm = [NSFileManager defaultManager];
  NSError* error = nil;
  if (![fm createDirectoryAtURL:to.URLByDeletingLastPathComponent
          withIntermediateDirectories:YES
                           attributes:nil
                                error:&error]) {
    return error.localizedDescription;
  }
  if ([fm fileExistsAtPath:to.path] && ![fm removeItemAtURL:to error:&error]) {
    return error.localizedDescription;
  }
  if (![fm copyItemAtURL:from toURL:to error:&error]) return error.localizedDescription;
  return nil;
}

// Returns YES when the user asked to open Keyboard settings.
BOOL showAlert(NSString* title, NSString* text, BOOL offerSettings) {
  [NSApp activateIgnoringOtherApps:YES];
  NSAlert* alert = [[NSAlert alloc] init];
  alert.messageText = title;
  alert.informativeText = text;
  [alert addButtonWithTitle:@"OK"];
  if (offerSettings) [alert addButtonWithTitle:@"Mở cài đặt Bàn phím"];
  return [alert runModal] == NSAlertSecondButtonReturn;
}

}  // namespace

bool shouldSelfInstall(NSURL* bundleUrl) {
  NSString* parent = bundleUrl.URLByDeletingLastPathComponent.lastPathComponent;
  return ![parent isEqualToString:@"Input Methods"];
}

int runSelfInstall(NSURL* bundleUrl) {
  [NSApplication sharedApplication];
  NSURL* dest = destinationUrl();
  stopRunningServers();
  if (NSString* err = copyBundle(bundleUrl, dest)) {
    showAlert(@"Không cài được Goxvi", err, NO);
    return 1;
  }
  if (runInstalled(dest, @"--install") != 0) {
    showAlert(@"Không đăng ký được Goxvi",
              @"macOS từ chối đăng ký bộ gõ. Hãy thử đăng xuất rồi đăng nhập lại, sau đó mở "
              @"Goxvi.app thêm lần nữa.",
              NO);
    return 1;
  }
  // --status checks the persisted list: true only once the user added Goxvi
  // in System Settings (enabling from code is not persisted on macOS 26).
  const bool enabled = runInstalled(dest, @"--status") == 0;
  NSString* text =
      enabled ? @"Chọn Goxvi trong menu bàn phím trên thanh menu để bắt đầu gõ tiếng Việt. "
                @"Bật/tắt tiếng Việt bằng ⌃⇧."
              : @"Bước cuối: bấm \"Mở cài đặt Bàn phím\" → Nguồn nhập → Sửa → bấm + → "
                @"Tiếng Việt → chọn Goxvi → Thêm. Sau đó chọn Goxvi trên thanh menu.\n\n"
                @"Nếu chưa thấy Goxvi trong danh sách, hãy đăng xuất rồi đăng nhập lại.";
  if (showAlert(@"Đã cài Goxvi", text, !enabled)) {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:kKeyboardSettingsUrl]];
  }
  return 0;
}
