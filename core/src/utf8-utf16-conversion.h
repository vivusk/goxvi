#pragma once

#include <string>
#include <string_view>

// Pure UTF-8 <-> UTF-16 conversion, no Windows API and no deprecated
// std::codecvt — core stays "no OS dependency". Used only by the config
// serializer (JSON is UTF-8, ShortcutEntry is std::wstring / UTF-16 since
// Windows wchar_t is 16-bit). Invalid input is replaced with U+FFFD; never
// throws. Non-BMP code points round-trip through UTF-16 surrogate pairs.
namespace goxvi::detail {

std::wstring utf8ToWide(std::string_view utf8);
std::string wideToUtf8(std::wstring_view wide);

}  // namespace goxvi::detail
