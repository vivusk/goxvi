#include "text-edit.h"

#include <algorithm>

namespace goxvi_mac {

TextEdit diffTextEdit(const std::wstring& onScreen, const std::wstring& target) {
  const auto mismatch = std::mismatch(onScreen.begin(), onScreen.end(), target.begin(),
                                      target.end());
  const auto common = static_cast<std::size_t>(mismatch.first - onScreen.begin());
  TextEdit edit;
  edit.deleteCount = onScreen.size() - common;
  edit.insert = target.substr(common);
  return edit;
}

}  // namespace goxvi_mac
