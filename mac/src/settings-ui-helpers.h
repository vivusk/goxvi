#pragma once

#import <Cocoa/Cocoa.h>

#include "goxvi/engine-types.h"

// Shared building blocks of the Settings window tabs (phase 5): code-built UI,
// no xib. Tabs lay out controls top-down in a flipped view with fixed frames
// (the window is not resizable).
@interface GoxviFlippedView : NSView
@end

NSTextField* goxviLabel(NSString* text, NSRect frame);
NSButton* goxviCheckbox(NSString* title, NSRect frame, id target, SEL action);
NSPopUpButton* goxviPopup(NSArray<NSString*>* titles, NSRect frame, id target, SEL action);
NSButton* goxviButton(NSString* title, NSRect frame, id target, SEL action);

// Saves through ConfigStore::save (atomic write, memory changes only on
// success). On failure shows an NSAlert and returns false; the caller then
// reloads its controls from the unchanged ConfigStore::current().
bool goxviSaveConfig(goxvi::EngineConfig config, NSWindow* window);
