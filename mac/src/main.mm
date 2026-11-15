// Goxvi.app entry point. Without arguments it runs as the IMK server that macOS
// launches when the "Goxvi" input source is selected. The --install/--status/
// --uninstall commands are used by mac/scripts/*.sh (PDR D9, phase 2).
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <InputMethodKit/InputMethodKit.h>

#include "config-store.h"
#include "self-install.h"

#include <cstdio>
#include <cstring>

namespace {

// Must match TISInputSourceID / InputMethodConnectionName in Info.plist.in.
NSString* const kSourceId = @"com.goxvi.inputmethod.Goxvi";
NSString* const kConnectionName = @"com.goxvi.inputmethod.Goxvi_Connection";

// IMKServer must outlive [NSApp run]; a global strong ref keeps ARC from freeing it.
IMKServer* gServer = nil;

// Finds our source including not-yet-enabled ones (includeAllInstalled=true),
// otherwise --status could not tell "registered but disabled" from "missing".
TISInputSourceRef copySource() {
  NSDictionary* filter = @{(__bridge NSString*)kTISPropertyInputSourceID : kSourceId};
  CFArrayRef list = TISCreateInputSourceList((__bridge CFDictionaryRef)filter, true);
  if (!list) return nullptr;
  TISInputSourceRef source = nullptr;
  if (CFArrayGetCount(list) > 0) {
    source = (TISInputSourceRef)CFRetain(CFArrayGetValueAtIndex(list, 0));
  }
  CFRelease(list);
  return source;
}

bool isEnabled(TISInputSourceRef source) {
  CFBooleanRef value =
      (CFBooleanRef)TISGetInputSourceProperty(source, kTISPropertyInputSourceIsEnabled);
  return value && CFBooleanGetValue(value);
}

// Exit != 0 only when registration itself fails; enabling may legitimately need
// System Settings or a log out/in on first install (plan.md "Kiem thu tay").
int runInstall() {
  NSURL* bundleUrl = [[NSBundle mainBundle] bundleURL];
  OSStatus err = TISRegisterInputSource((__bridge CFURLRef)bundleUrl);
  std::printf("register %s: %d\n", bundleUrl.path.UTF8String, (int)err);
  if (err != noErr) {
    std::fprintf(stderr, "error: TISRegisterInputSource failed (%d)\n", (int)err);
    return 1;
  }
  TISInputSourceRef source = copySource();
  if (!source) {
    std::printf("warning: source not visible yet; enable Goxvi in System Settings "
                "or log out/in\n");
    return 0;
  }
  OSStatus enableErr = TISEnableInputSource(source);
  std::printf("enable: %d\n", (int)enableErr);
  if (enableErr != noErr || !isEnabled(source)) {
    std::printf("warning: source not enabled yet; enable Goxvi in System Settings "
                "or log out/in\n");
  }
  CFRelease(source);
  return 0;
}

// Informational only: install.sh prints this line but never uses its exit code.
int runStatus() {
  TISInputSourceRef source = copySource();
  const bool registered = source != nullptr;
  const bool enabled = registered && isEnabled(source);
  if (source) CFRelease(source);
  std::printf("registered=%d enabled=%d\n", registered ? 1 : 0, enabled ? 1 : 0);
  return (registered && enabled) ? 0 : 1;
}

// Missing or already-disabled source is not an error (idempotent uninstall).
int runUninstall() {
  TISInputSourceRef source = copySource();
  if (!source) {
    std::printf("disable: source not registered, nothing to do\n");
    return 0;
  }
  if (isEnabled(source)) {
    OSStatus err = TISDisableInputSource(source);
    std::printf("disable: %d\n", (int)err);
  } else {
    std::printf("disable: source already disabled\n");
  }
  CFRelease(source);
  return 0;
}

int runServer() {
  [NSApplication sharedApplication];
  ConfigStore::instance().load();  // once at startup (plan K5)
  gServer = [[IMKServer alloc] initWithName:kConnectionName
                           bundleIdentifier:[[NSBundle mainBundle] bundleIdentifier]];
  if (!gServer) {
    NSLog(@"Goxvi: failed to create IMKServer");
    return 1;
  }
  [NSApp run];
  return 0;
}

}  // namespace

int main(int argc, const char* argv[]) {
  @autoreleasepool {
    if (argc > 1) {
      if (std::strcmp(argv[1], "--install") == 0) return runInstall();
      if (std::strcmp(argv[1], "--status") == 0) return runStatus();
      if (std::strcmp(argv[1], "--uninstall") == 0) return runUninstall();
    }
    // Opened from Finder (e.g. the .dmg), not launched by IMK: install itself.
    NSURL* bundleUrl = [[NSBundle mainBundle] bundleURL];
    if (shouldSelfInstall(bundleUrl)) return runSelfInstall(bundleUrl);
    return runServer();
  }
}
