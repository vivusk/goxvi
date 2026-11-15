#pragma once

#import <Cocoa/Cocoa.h>

// "Gõ tắt" tab: shortcutsEnabled + the shortcut list (add / delete / edit in
// place). Every change is validated (shortcut-validation.h) before it is saved;
// a rejected change is explained in a label under the table and not saved.
@interface GoxviShortcutsTab : NSObject
@property(nonatomic, readonly) NSView* view;
- (void)reload;  // from ConfigStore::current()
@end
