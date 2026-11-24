#pragma once

#include <string_view>

// Pure C++ (plan K13): which insertion mode the shim uses for a client app.
// Hard-coded on purpose — no config.json field, no Settings UI; the exception
// list changes only by editing insert-mode-policy.cpp.
namespace goxvi_mac {

enum class InsertMode {
  Direct,  // insertText:replacementRange:, no marked text (hidden composition)
  Marked,  // setMarkedText fallback (original D2 behaviour)
};

// bundleId = client bundle identifier (may be empty when unknown).
// selectionReadable = selectedRange.location != NSNotFound when the word starts;
// Direct needs the caret position, so an unreadable caret always means Marked.
InsertMode chooseInsertMode(std::string_view bundleId, bool selectionReadable);

}  // namespace goxvi_mac
