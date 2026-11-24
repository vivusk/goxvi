#pragma once

#include <cstddef>
#include <string>

// Pure C++ (plan K10): one direct-insert edit = "replace deleteCount
// characters right before the caret with insert". Mirrors the Windows direct
// mode (tsf/src/direct-text-injector.h sendTextReplacement). Kept for phase 2.
namespace goxvi_mac {

struct TextEdit {
  std::size_t deleteCount = 0;  // UTF-16 code units (K1)
  std::wstring insert;
  bool empty() const { return deleteCount == 0 && insert.empty(); }
};

// Keeps the longest common prefix of onScreen and target; deletes the rest of
// onScreen and inserts the rest of target.
TextEdit diffTextEdit(const std::wstring& onScreen, const std::wstring& target);

}  // namespace goxvi_mac
