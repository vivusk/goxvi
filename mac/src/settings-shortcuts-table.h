#pragma once

#import <Cocoa/Cocoa.h>

#include <vector>

#include "goxvi/engine-types.h"

// View-based 2-column table (trigger | expansion) of the "Gõ tắt" tab, edited
// in place. Holds a copy of the shortcut list being shown; the tab owns
// validation + saving through `onEdit`.
@interface GoxviShortcutsTable : NSObject <NSTableViewDataSource, NSTableViewDelegate>
@property(nonatomic, readonly) NSScrollView* scrollView;
@property(nonatomic, readonly) NSTableView* tableView;
// Called when a cell finished editing with the row's would-be entry. Return
// YES if it was accepted (validated + saved); the row is redrawn either way,
// so a rejected edit snaps back to the stored value.
@property(nonatomic, copy) BOOL (^onEdit)(NSInteger row, goxvi::ShortcutEntry entry);

- (instancetype)initWithFrame:(NSRect)frame;
- (const std::vector<goxvi::ShortcutEntry>&)entries;
- (void)setEntries:(std::vector<goxvi::ShortcutEntry>)entries;  // reloads
@end
