// Double-click install for the .dmg build: when Goxvi.app is opened from
// anywhere except an "Input Methods" folder (DMG, Downloads...), it copies
// itself to ~/Library/Input Methods/, registers the input source via the copied
// binary's --install, then guides the user to add it in System Settings (the
// only enable macOS persists for third-party input methods).
#pragma once

#import <Foundation/Foundation.h>

// True when the bundle does not live in an "Input Methods" folder, i.e. the
// user launched it from Finder rather than macOS launching the IMK server.
bool shouldSelfInstall(NSURL* bundleUrl);

// Returns the process exit code (0 = installed and registered).
int runSelfInstall(NSURL* bundleUrl);
