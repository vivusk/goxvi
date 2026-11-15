#pragma once

#import <Foundation/Foundation.h>

#include <string>
#include <vector>

// K1: a core std::wstring holds UTF-16 code units even with 32-bit wchar_t, so
// each element maps 1:1 to a unichar (never NSUTF32StringEncoding).
inline NSString* toNSString(const std::wstring& wide) {
  std::vector<unichar> units(wide.begin(), wide.end());
  return [NSString stringWithCharacters:units.data() length:units.size()];
}

inline std::wstring toWide(NSString* string) {
  std::vector<unichar> units(string.length);
  [string getCharacters:units.data() range:NSMakeRange(0, units.size())];
  return std::wstring(units.begin(), units.end());
}
