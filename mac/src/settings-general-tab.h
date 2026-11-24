#pragma once

#import <Cocoa/Cocoa.h>

// "Chung" tab: inputMethod, toneStyle, toggleHotkey, restoreOnInvalid,
// autoCorrectEnabled (shortcutsEnabled lives on the "Gõ tắt" tab).
@interface GoxviGeneralTab : NSObject
@property(nonatomic, readonly) NSView* view;
// Re-reads ConfigStore::current() (the input menu may have switched Telex/VNI).
- (void)reload;
@end
