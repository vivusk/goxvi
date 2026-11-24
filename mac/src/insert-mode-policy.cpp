#include "insert-mode-policy.h"

#include <array>

namespace goxvi_mac {
namespace {

// Apps that ignore replacementRange; exact bundle ID match. Default for every
// other app is Direct (compatibility measurements 2026-11: Telegram native,
// VSCode incl. the Claude extension webview, Chrome all type correctly).
constexpr std::array<std::string_view, 2> kMarkedBundles = {
    // Measured 2026-11: replacementRange ignored, caret never moves -> every
    // replace garbles the word (S1-S3 all wrong).
    "com.googlecode.iterm2",
    // Same terminal text model as iTerm2 (inferred; confirm by hand in phase 3).
    "com.apple.Terminal",
};

}  // namespace

InsertMode chooseInsertMode(std::string_view bundleId, bool selectionReadable) {
  if (!selectionReadable) return InsertMode::Marked;
  for (std::string_view marked : kMarkedBundles) {
    if (bundleId == marked) return InsertMode::Marked;
  }
  return InsertMode::Direct;
}

}  // namespace goxvi_mac
