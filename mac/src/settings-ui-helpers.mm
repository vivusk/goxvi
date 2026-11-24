#import "settings-ui-helpers.h"

#include <utility>

#include "config-store.h"

@implementation GoxviFlippedView
- (BOOL)isFlipped {
  return YES;
}
@end

NSTextField* goxviLabel(NSString* text, NSRect frame) {
  NSTextField* label = [NSTextField labelWithString:text];
  label.frame = frame;
  return label;
}

NSButton* goxviCheckbox(NSString* title, NSRect frame, id target, SEL action) {
  NSButton* box = [NSButton checkboxWithTitle:title target:target action:action];
  box.frame = frame;
  return box;
}

NSPopUpButton* goxviPopup(NSArray<NSString*>* titles, NSRect frame, id target, SEL action) {
  NSPopUpButton* popup = [[NSPopUpButton alloc] initWithFrame:frame pullsDown:NO];
  [popup addItemsWithTitles:titles];
  popup.target = target;
  popup.action = action;
  return popup;
}

NSButton* goxviButton(NSString* title, NSRect frame, id target, SEL action) {
  NSButton* button = [NSButton buttonWithTitle:title target:target action:action];
  button.frame = frame;
  return button;
}

bool goxviSaveConfig(goxvi::EngineConfig config, NSWindow* window) {
  ConfigStore& store = ConfigStore::instance();
  if (store.save(std::move(config))) return true;
  NSAlert* alert = [[NSAlert alloc] init];
  alert.alertStyle = NSAlertStyleWarning;
  alert.messageText = @"Không lưu được cấu hình";
  alert.informativeText = [NSString
      stringWithFormat:@"Không ghi được tệp %@ (kiểm tra quyền ghi hoặc dung lượng đĩa). "
                       @"Thay đổi chưa được áp dụng.",
                       @(store.path().c_str())];
  if (window) {
    [alert beginSheetModalForWindow:window completionHandler:nil];
  } else {
    [alert runModal];
  }
  return false;
}
