#import "goxvi-input-controller.h"

@implementation GoxviInputController

// FlagsChanged is requested now so the modifier-only toggle hotkey (PDR D6,
// phase 4) needs no plist/registration change later.
- (NSUInteger)recognizedEvents:(id)sender {
  (void)sender;
  return NSEventMaskKeyDown | NSEventMaskFlagsChanged;
}

// Skeleton (phase 2): every key passes through untouched; composition comes in phase 3.
- (BOOL)handleEvent:(NSEvent*)event client:(id)sender {
  (void)event;
  (void)sender;
  return NO;
}

@end
