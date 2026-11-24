#pragma once

#import <Cocoa/Cocoa.h>

#include "goxvi/engine-types.h"

// Input menu in the menu bar (PDR D7): "Tiếng Việt" / Telex / VNI / Settings….
// Rebuilt on every -menu call so the ticks match the live state (the icon
// itself stays static, plan: out of scope).
typedef NS_ENUM(NSInteger, GoxviMenuTag) {
  GoxviMenuTagToggleVietnamese = 1,
  GoxviMenuTagTelex = 2,
  GoxviMenuTagVni = 3,
  GoxviMenuTagSettings = 4,
};

// Every item sends `action` (to the IMK controller), which dispatches on tag.
NSMenu* buildInputMenu(SEL action, bool vietnameseOn, goxvi::InputMethod method);

// IMK passes menu actions an NSDictionary holding the item under
// kIMKCommandMenuItemName (not the NSMenuItem itself); accepts both shapes.
// Returns 0 when no item can be found.
NSInteger inputMenuTagFromSender(id sender);
