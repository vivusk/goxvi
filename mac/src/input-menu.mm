#import "input-menu.h"

#import <InputMethodKit/InputMethodKit.h>

namespace {

NSMenuItem* addItem(NSMenu* menu, NSString* title, GoxviMenuTag tag, SEL action, bool on) {
  NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:@""];
  item.tag = tag;
  item.state = on ? NSControlStateValueOn : NSControlStateValueOff;
  [menu addItem:item];
  return item;
}

}  // namespace

NSMenu* buildInputMenu(SEL action, bool vietnameseOn, goxvi::InputMethod method) {
  NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Goxvi"];
  menu.autoenablesItems = NO;  // the Settings item is disabled explicitly
  addItem(menu, @"Tiếng Việt", GoxviMenuTagToggleVietnamese, action, vietnameseOn);
  [menu addItem:[NSMenuItem separatorItem]];
  addItem(menu, @"Telex", GoxviMenuTagTelex, action, method == goxvi::InputMethod::Telex);
  addItem(menu, @"VNI", GoxviMenuTagVni, action, method == goxvi::InputMethod::Vni);
  [menu addItem:[NSMenuItem separatorItem]];
  NSMenuItem* settings = addItem(menu, @"Settings…", GoxviMenuTagSettings, nil, false);
  settings.enabled = NO;  // phase 5
  return menu;
}

NSInteger inputMenuTagFromSender(id sender) {
  id item = sender;
  if ([sender isKindOfClass:[NSDictionary class]]) {
    item = [(NSDictionary*)sender objectForKey:kIMKCommandMenuItemName];
  }
  return [item isKindOfClass:[NSMenuItem class]] ? [(NSMenuItem*)item tag] : 0;
}
