#import "settings-shortcuts-tab.h"

#include <utility>
#include <vector>

#include "config-store.h"
#include "shortcut-validation.h"
#import "settings-shortcuts-table.h"
#import "settings-ui-helpers.h"
#include "wide-nsstring.h"

namespace {

NSString* errorMessage(goxvi_mac::ShortcutError error, NSString* trigger) {
  using goxvi_mac::ShortcutError;
  switch (error) {
    case ShortcutError::None: return @"";
    case ShortcutError::TriggerEmpty: return @"Từ tắt không được để trống.";
    case ShortcutError::TriggerInvalidChar:
      return @"Từ tắt chỉ gồm chữ thường a-z (không chữ hoa, số, dấu cách hay dấu).";
    case ShortcutError::TriggerTooLong:
      return [NSString stringWithFormat:@"Từ tắt dài tối đa %d ký tự.", goxvi::kMaxRawKeys];
    case ShortcutError::ExpansionEmpty: return @"Cụm từ đầy đủ không được để trống.";
    case ShortcutError::ExpansionTooLong:
      return [NSString stringWithFormat:@"Cụm từ đầy đủ dài tối đa %d ký tự.",
                                        goxvi::kMaxShortcutExpansionChars];
    case ShortcutError::DuplicateTrigger:
      return [NSString stringWithFormat:@"Từ tắt \"%@\" đã có trong danh sách.", trigger];
  }
  return @"";
}

}  // namespace

@implementation GoxviShortcutsTab {
  NSButton* _enabled;
  GoxviShortcutsTable* _table;
  NSTextField* _trigger;
  NSTextField* _expansion;
  NSTextField* _error;
}

- (instancetype)init {
  if ((self = [super init])) {
    NSView* view = [[GoxviFlippedView alloc] initWithFrame:NSMakeRect(0, 0, 500, 320)];
    _enabled = goxviCheckbox(@"Bật gõ tắt", NSMakeRect(20, 12, 460, 20), self,
                             @selector(toggleEnabled:));
    _table = [[GoxviShortcutsTable alloc] initWithFrame:NSMakeRect(20, 40, 465, 170)];
    __weak GoxviShortcutsTab* weakSelf = self;
    _table.onEdit = ^BOOL(NSInteger row, goxvi::ShortcutEntry entry) {
      return [weakSelf applyEntry:std::move(entry) atIndex:(size_t)row];
    };
    _trigger = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 222, 110, 22)];
    _trigger.placeholderString = @"Từ tắt (a-z)";
    _expansion = [[NSTextField alloc] initWithFrame:NSMakeRect(138, 222, 252, 22)];
    _expansion.placeholderString = @"Cụm từ đầy đủ";
    NSButton* add = goxviButton(@"Thêm", NSMakeRect(395, 217, 95, 32), self, @selector(add:));
    NSButton* remove = goxviButton(@"Xoá dòng chọn", NSMakeRect(15, 252, 140, 32), self,
                                   @selector(remove:));
    _error = goxviLabel(@"", NSMakeRect(20, 290, 465, 20));
    _error.textColor = NSColor.systemRedColor;
    for (NSView* control in @[ _enabled, _table.scrollView, _trigger, _expansion, add, remove,
                               _error ]) {
      [view addSubview:control];
    }
    _view = view;
    [self reload];
  }
  return self;
}

- (void)reload {
  const goxvi::EngineConfig& config = ConfigStore::instance().current();
  _enabled.state = config.shortcutsEnabled ? NSControlStateValueOn : NSControlStateValueOff;
  [_table setEntries:config.shortcuts];
}

- (void)showError:(goxvi_mac::ShortcutError)error trigger:(const std::wstring&)trigger {
  _error.stringValue = errorMessage(error, toNSString(trigger));
}

// Saves `shortcuts`; on success the table shows them, on failure everything is
// reloaded from the unchanged ConfigStore (alert already shown).
- (BOOL)saveShortcuts:(std::vector<goxvi::ShortcutEntry>)shortcuts {
  goxvi::EngineConfig config = ConfigStore::instance().current();
  config.shortcuts = std::move(shortcuts);
  const bool saved = goxviSaveConfig(std::move(config), _view.window);
  [self reload];
  return saved ? YES : NO;
}

// index >= entries.size() appends a new entry; otherwise replaces that row.
- (BOOL)applyEntry:(goxvi::ShortcutEntry)entry atIndex:(size_t)index {
  std::vector<goxvi::ShortcutEntry> shortcuts = _table.entries;
  const goxvi_mac::ShortcutError error = goxvi_mac::validateShortcutAt(shortcuts, index, entry);
  [self showError:error trigger:entry.trigger];
  if (error != goxvi_mac::ShortcutError::None) return NO;
  if (index >= shortcuts.size()) {
    shortcuts.push_back(std::move(entry));
  } else {
    shortcuts[index] = std::move(entry);
  }
  return [self saveShortcuts:std::move(shortcuts)];
}

- (void)toggleEnabled:(id)sender {
  goxvi::EngineConfig config = ConfigStore::instance().current();
  config.shortcutsEnabled = _enabled.state == NSControlStateValueOn;
  if (!goxviSaveConfig(std::move(config), _view.window)) [self reload];
}

- (void)add:(id)sender {
  goxvi::ShortcutEntry entry{toWide(_trigger.stringValue), toWide(_expansion.stringValue)};
  if (![self applyEntry:std::move(entry) atIndex:_table.entries.size()]) return;
  _trigger.stringValue = @"";
  _expansion.stringValue = @"";
  const NSInteger last = _table.tableView.numberOfRows - 1;
  if (last >= 0) [_table.tableView scrollRowToVisible:last];
  [_view.window makeFirstResponder:_trigger];
}

- (void)remove:(id)sender {
  NSIndexSet* rows = _table.tableView.selectedRowIndexes;
  if (rows.count == 0) {
    _error.stringValue = @"Chọn dòng cần xoá trong bảng trước.";
    return;
  }
  std::vector<goxvi::ShortcutEntry> kept;
  const std::vector<goxvi::ShortcutEntry>& current = _table.entries;
  for (size_t i = 0; i < current.size(); ++i) {
    if (![rows containsIndex:i]) kept.push_back(current[i]);
  }
  _error.stringValue = @"";
  [self saveShortcuts:std::move(kept)];
}

@end
