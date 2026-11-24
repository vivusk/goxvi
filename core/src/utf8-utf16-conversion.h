#pragma once

#include <string>
#include <string_view>

// Pure UTF-8 <-> UTF-16 conversion, no Windows API and no deprecated
// std::codecvt — core stays "no OS dependency". Used only by the config
// serializer (JSON is UTF-8, ShortcutEntry is std::wstring). Invalid input is
// replaced with U+FFFD; never throws. Non-BMP code points round-trip through
// UTF-16 surrogate pairs.
// Convention K1 (macOS plan): a core std::wstring ALWAYS holds UTF-16 code
// units, even where wchar_t is 32-bit (macOS/Linux) — each element is <= 0xFFFF
// and non-BMP chars are surrogate pairs. Shims map element -> UTF-16 unit
// (e.g. unichar), never UTF-32. Don't write non-BMP chars in L"" literals.
namespace goxvi::detail {

std::wstring utf8ToWide(std::string_view utf8);
std::string wideToUtf8(std::wstring_view wide);

}  // namespace goxvi::detail
