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
  GoxviMenuTagSpikeDirectInsert = 5,  // SPIKE (plan 261115-1819 phase 1), removed in phase 3
};

// Every item sends `action` (to the IMK controller), which dispatches on tag.
// spikeDirect ticks the temporary "Spike: chèn trực tiếp" item (phase 1 spike).
NSMenu* buildInputMenu(SEL action, bool vietnameseOn, goxvi::InputMethod method,
                       bool spikeDirect);

// IMK passes menu actions an NSDictionary holding the item under
// kIMKCommandMenuItemName (not the NSMenuItem itself); accepts both shapes.
// Returns 0 when no item can be found.
NSInteger inputMenuTagFromSender(id sender);
