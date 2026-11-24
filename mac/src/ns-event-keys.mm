#import "ns-event-keys.h"

goxvi_mac::ModifierMask modifierMask(NSEventModifierFlags flags) {
  goxvi_mac::ModifierMask mask = 0;
  if (flags & NSEventModifierFlagControl) mask |= goxvi_mac::kModControl;
  if (flags & NSEventModifierFlagShift) mask |= goxvi_mac::kModShift;
  if (flags & NSEventModifierFlagOption) mask |= goxvi_mac::kModOption;
  if (flags & NSEventModifierFlagCommand) mask |= goxvi_mac::kModCommand;
  return mask;
}

// NSEvent -> KeyKind (+ the single UTF-16 unit typed, 0 otherwise).
goxvi_mac::KeyKind keyKindOf(NSEvent* event, goxvi::InputMethod method, wchar_t* ch) {
  NSString* chars = event.characters ?: @"";
  const NSEventModifierFlags flags = event.modifierFlags;
  const bool chord = (flags & (NSEventModifierFlagCommand | NSEventModifierFlagControl |
                               NSEventModifierFlagOption)) != 0;
  const bool numpad = (flags & NSEventModifierFlagNumericPad) != 0;
  *ch = chars.length == 1 ? static_cast<wchar_t>([chars characterAtIndex:0]) : 0;
  return goxvi_mac::classifyKey(*ch, chars.length, event.keyCode, chord, numpad, method);
}
