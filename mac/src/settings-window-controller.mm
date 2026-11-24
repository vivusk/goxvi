#import "settings-window-controller.h"

#import "settings-about-tab.h"
#import "settings-general-tab.h"
#import "settings-shortcuts-tab.h"

namespace {

NSMenuItem* editItem(NSString* title, SEL action, NSString* key, NSEventModifierFlags mods) {
  NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:key];
  item.keyEquivalentModifierMask = mods;
  return item;  // target nil -> first responder (the focused text field)
}

// An LSUIElement process has no main menu, so ⌘X/⌘C/⌘V/⌘A/⌘Z do nothing in
// text fields. The menu is never shown; it only carries the key equivalents.
// Pasting accented text is the fallback if typing Vietnamese into our own
// window does not work (phase 5 step 6), so this is required.
void installEditMenuOnce() {
  if (NSApp.mainMenu) return;
  NSMenu* edit = [[NSMenu alloc] initWithTitle:@"Edit"];
  const NSEventModifierFlags cmd = NSEventModifierFlagCommand;
  [edit addItem:editItem(@"Undo", @selector(undo:), @"z", cmd)];
  [edit addItem:editItem(@"Redo", @selector(redo:), @"z", cmd | NSEventModifierFlagShift)];
  [edit addItem:[NSMenuItem separatorItem]];
  [edit addItem:editItem(@"Cut", @selector(cut:), @"x", cmd)];
  [edit addItem:editItem(@"Copy", @selector(copy:), @"c", cmd)];
  [edit addItem:editItem(@"Paste", @selector(paste:), @"v", cmd)];
  [edit addItem:editItem(@"Select All", @selector(selectAll:), @"a", cmd)];
  NSMenu* main = [[NSMenu alloc] initWithTitle:@"Goxvi"];
  [main addItem:[[NSMenuItem alloc] initWithTitle:@"Goxvi" action:nil keyEquivalent:@""]];
  NSMenuItem* editHolder = [[NSMenuItem alloc] initWithTitle:@"Edit" action:nil keyEquivalent:@""];
  editHolder.submenu = edit;
  [main addItem:editHolder];
  NSApp.mainMenu = main;
}

NSTabViewItem* tabItem(NSString* label, NSView* view) {
  NSTabViewItem* item = [[NSTabViewItem alloc] initWithIdentifier:label];
  item.label = label;
  item.view = view;
  return item;
}

}  // namespace

@implementation GoxviSettingsWindowController {
  GoxviGeneralTab* _general;
  GoxviShortcutsTab* _shortcuts;
  GoxviAboutTab* _about;
}

+ (void)showSettings {
  static GoxviSettingsWindowController* shared = nil;  // lives for the process
  if (!shared) shared = [[GoxviSettingsWindowController alloc] init];
  installEditMenuOnce();
  // LSUIElement: without activating, the window can open behind the front app.
  [NSApp activateIgnoringOtherApps:YES];
  [shared.window makeKeyAndOrderFront:nil];
}

- (instancetype)init {
  NSWindow* window = [[NSWindow alloc]
      initWithContentRect:NSMakeRect(0, 0, 560, 420)
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskMiniaturizable
                  backing:NSBackingStoreBuffered
                    defer:YES];
  window.title = @"Goxvi — Cài đặt";
  window.releasedWhenClosed = NO;  // reopened by showSettings
  if ((self = [super initWithWindow:window])) {
    _general = [[GoxviGeneralTab alloc] init];
    _shortcuts = [[GoxviShortcutsTab alloc] init];
    _about = [[GoxviAboutTab alloc] init];
    NSTabView* tabs = [[NSTabView alloc] initWithFrame:NSMakeRect(12, 12, 536, 396)];
    [tabs addTabViewItem:tabItem(@"Chung", _general.view)];
    [tabs addTabViewItem:tabItem(@"Gõ tắt", _shortcuts.view)];
    [tabs addTabViewItem:tabItem(@"Giới thiệu", _about.view)];
    [window.contentView addSubview:tabs];
    window.delegate = self;
    [window center];
  }
  return self;
}

// The input menu may have switched Telex/VNI (or a failed save reverted)
// since the window was last shown: always show the live ConfigStore.
- (void)windowDidBecomeKey:(NSNotification*)notification {
  [_general reload];
  [_shortcuts reload];
}

@end
