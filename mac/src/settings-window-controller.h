#pragma once

#import <Cocoa/Cocoa.h>

// The single Settings window of Goxvi.app (PDR D7, plan K5): same process as
// the IMK server, so a saved change updates ConfigStore in memory and applies
// from the next word, no restart. Tabs: Chung / Gõ tắt / Giới thiệu.
@interface GoxviSettingsWindowController : NSWindowController <NSWindowDelegate>
// Creates the window on first use; later calls bring the same window to front.
+ (void)showSettings;
@end
