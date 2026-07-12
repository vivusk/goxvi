#pragma once

#include <string>
#include <string_view>

#include "goxvi/telex-engine.h"

namespace goxvi::testhelpers {

// Feed a key sequence, return the last composition display.
inline std::wstring typeString(TelexEngine& engine, std::wstring_view keys) {
  std::wstring display;
  for (wchar_t c : keys) {
    KeyResult r = engine.processKey(c);
    if (r.consumed && !r.overflow) display = r.display;
  }
  return display;
}

inline std::wstring typeFresh(std::wstring_view keys, EngineConfig cfg = {}) {
  TelexEngine engine(cfg);
  return typeString(engine, keys);
}

}  // namespace goxvi::testhelpers
