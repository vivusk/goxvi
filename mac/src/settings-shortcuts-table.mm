#import "settings-shortcuts-table.h"

#include <utility>

#include "wide-nsstring.h"

namespace {

NSString* const kTriggerColumn = @"trigger";
NSString* const kExpansionColumn = @"expansion";

NSTableColumn* makeColumn(NSString* identifier, NSString* title, CGFloat width) {
  NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:identifier];
  column.title = title;
  column.width = width;
  return column;
}

}  // namespace

@implementation GoxviShortcutsTable {
  std::vector<goxvi::ShortcutEntry> _entries;
}

- (instancetype)initWithFrame:(NSRect)frame {
  if ((self = [super init])) {
    _tableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, frame.size.width, frame.size.height)];
    [_tableView addTableColumn:makeColumn(kTriggerColumn, @"Từ tắt", 110)];
    [_tableView addTableColumn:makeColumn(kExpansionColumn, @"Cụm từ đầy đủ",
                                          frame.size.width - 140)];
    _tableView.allowsMultipleSelection = YES;
    _tableView.usesAlternatingRowBackgroundColors = YES;
    _tableView.dataSource = self;
    _tableView.delegate = self;
    _scrollView = [[NSScrollView alloc] initWithFrame:frame];
    _scrollView.documentView = _tableView;
    _scrollView.hasVerticalScroller = YES;
    _scrollView.borderType = NSBezelBorder;
  }
  return self;
}

- (const std::vector<goxvi::ShortcutEntry>&)entries {
  return _entries;
}

- (void)setEntries:(std::vector<goxvi::ShortcutEntry>)entries {
  _entries = std::move(entries);
  [_tableView reloadData];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView {
  return (NSInteger)_entries.size();
}

- (NSView*)tableView:(NSTableView*)tableView
    viewForTableColumn:(NSTableColumn*)column
                   row:(NSInteger)row {
  NSTextField* field = [tableView makeViewWithIdentifier:column.identifier owner:self];
  if (!field) {
    field = [[NSTextField alloc] initWithFrame:NSZeroRect];
    field.identifier = column.identifier;
    field.bordered = NO;
    field.drawsBackground = NO;
    field.editable = YES;
    field.lineBreakMode = NSLineBreakByTruncatingTail;
    field.cell.sendsActionOnEndEditing = YES;  // focus loss also commits the edit
    field.target = self;
    field.action = @selector(cellEdited:);
  }
  if (row < 0 || (size_t)row >= _entries.size()) return field;
  const goxvi::ShortcutEntry& entry = _entries[(size_t)row];
  const bool isTrigger = [column.identifier isEqualToString:kTriggerColumn];
  field.stringValue = toNSString(isTrigger ? entry.trigger : entry.expansion);
  return field;
}

- (void)cellEdited:(NSTextField*)sender {
  const NSInteger row = [_tableView rowForView:sender];
  const NSInteger column = [_tableView columnForView:sender];
  if (row < 0 || column < 0 || (size_t)row >= _entries.size()) return;
  goxvi::ShortcutEntry entry = _entries[(size_t)row];
  std::wstring& target = [_tableView.tableColumns[(NSUInteger)column].identifier
                            isEqualToString:kTriggerColumn] ? entry.trigger : entry.expansion;
  const std::wstring edited = toWide(sender.stringValue);
  if (edited == target) return;  // end-editing without a change
  target = edited;
  // Deferred so validation, saving and the reload never run inside the
  // field's own end-editing callback. Accepted: the tab calls setEntries:
  // with the saved list. Rejected: redraw the stored value.
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self.onEdit && self.onEdit(row, entry)) return;
    if ((size_t)row >= self->_entries.size()) return;
    [self->_tableView reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)row]
                                columnIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)column]];
  });
}

@end
